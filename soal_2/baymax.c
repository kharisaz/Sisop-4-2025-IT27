#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

#define MAX_FRAGMENT_SIZE 1024  // 1 KB
#define MAX_PATH_LEN 1024
#define LOG_FILE "activity.log"

static const char *mount_path = "./mount_dir";
static const char *relics_path = "./relics";
static const char *download_url = "https://drive.usercontent.google.com/u/0/uc?id=1MHVhFT57Wa9Zcx69Bv9j5ImHc9rXMH1c&export=download";

// Informasi tentang operasi copy terakhir
static int is_copying = 0;
static char last_read_path[MAX_PATH_LEN] = {0};
static time_t last_read_time = 0;

// Mengunduh dan mengekstrak file
void download_and_extract_files() {
    printf("Preparing environment...\n");
    
    // Buat direktori jika belum ada
    struct stat st = {0};
    if (stat(relics_path, &st) == -1) {
        printf("Creating relics directory...\n");
        mkdir(relics_path, 0755);
    }
    
    if (stat(mount_path, &st) == -1) {
        printf("Creating mount directory...\n");
        mkdir(mount_path, 0755);
    }
    
    // Periksa apakah file fragmen pertama (.000) ada
    char first_fragment[MAX_PATH_LEN];
    sprintf(first_fragment, "%s/Baymax.jpeg.000", relics_path);
    
    if (access(first_fragment, F_OK) != 0) {
        printf("Downloading Baymax fragments...\n");
        
        // Download file ZIP
        char download_cmd[MAX_PATH_LEN];
        sprintf(download_cmd, "wget -q \"%s\" -O temp_baymax.zip", download_url);
        system(download_cmd);
        
        // Ekstrak file ke direktori relics
        printf("Extracting files to relics directory...\n");
        char extract_cmd[MAX_PATH_LEN];
        sprintf(extract_cmd, "unzip -q temp_baymax.zip -d %s", relics_path);
        system(extract_cmd);
        
        // Hapus file ZIP sementara
        system("rm -f temp_baymax.zip");
    } else {
        printf("Baymax fragments already exist in relics directory.\n");
    }
    
    // Inisialisasi file log
    FILE *log_file = fopen(LOG_FILE, "w");
    if (log_file) {
        fclose(log_file);
    }
    
    printf("Environment ready!\n");
}

// Fungsi untuk mencatat aktivitas ke dalam file log
void log_activity(const char *activity, const char *path) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec,
                activity, path);
        fclose(log_file);
    }
}

// Cek operasi copy
void check_copy_operation(const char *path) {
    // Cek jika file ini baru saja dibaca
    time_t now = time(NULL);
    if (strcmp(path, last_read_path) == 0 && difftime(now, last_read_time) < 2) {
        // Coba mendeteksi tujuan copy dari command line
        char dest[MAX_PATH_LEN] = "/tmp";  // default destination
        char cmd[MAX_PATH_LEN * 2];
        char cmdout[MAX_PATH_LEN * 4] = {0};
        
        // Cari proses cp yang sedang berjalan
        sprintf(cmd, "ps axww | grep -E 'cp [^ ]+ [^ ]+' | grep -v grep");
        FILE *fp = popen(cmd, "r");
        if (fp) {
            if (fgets(cmdout, sizeof(cmdout) - 1, fp)) {
                // Parsing output ps untuk mendapatkan tujuan copy
                char *p = strstr(cmdout, path + 1);  // +1 untuk skip '/'
                if (p) {
                    p += strlen(path + 1);
                    // Skip whitespace
                    while (*p && (*p == ' ' || *p == '\t')) p++;
                    if (*p) {
                        char *end = p;
                        while (*end && *end != ' ' && *end != '\t' && *end != '\n') end++;
                        *end = '\0';
                        strncpy(dest, p, sizeof(dest) - 1);
                    }
                }
            }
            pclose(fp);
            
            // Log operasi COPY
            char log_msg[MAX_PATH_LEN * 2];
            sprintf(log_msg, "%s -> %s", path + 1, dest);
            log_activity("COPY", log_msg);
            is_copying = 0;
        }
    }
}

// Mendapatkan path lengkap dari file di direktori relics
void get_relics_path(char *relics_full_path, const char *path, int fragment_index) {
    if (fragment_index >= 0) {
        sprintf(relics_full_path, "%s%s.%03d", relics_path, path, fragment_index);
    } else {
        sprintf(relics_full_path, "%s%s", relics_path, path);
    }
}

// Mendapatkan jumlah pecahan file
int get_fragment_count(const char *path) {
    int count = 0;
    char fragment_path[MAX_PATH_LEN];
    
    while (1) {
        get_relics_path(fragment_path, path, count);
        if (access(fragment_path, F_OK) != 0) {
            break;
        }
        count++;
    }
    
    return count;
}

// Mendapatkan ukuran total dari seluruh pecahan file
static off_t get_file_size(const char *path) {
    int count = get_fragment_count(path);
    if (count == 0) return 0;
    
    off_t total_size = 0;
    struct stat st;
    char fragment_path[MAX_PATH_LEN];
    
    for (int i = 0; i < count; i++) {
        get_relics_path(fragment_path, path, i);
        if (stat(fragment_path, &st) == 0) {
            total_size += st.st_size;
        }
    }
    
    return total_size;
}

// Memeriksa apakah file Baymax.jpeg ada di direktori relics
int check_baymax_exists() {
    char fragment_path[MAX_PATH_LEN];
    for (int i = 0; i < 14; i++) {
        sprintf(fragment_path, "%s/Baymax.jpeg.%03d", relics_path, i);
        if (access(fragment_path, F_OK) != 0) {
            return 0; // Salah satu fragmen tidak ditemukan
        }
    }
    return 1; // Semua fragmen ditemukan
}

static int baymax_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Khusus untuk Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        // Periksa apakah fragmen-fragmen Baymax ada
        if (check_baymax_exists()) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = get_file_size("/Baymax.jpeg");
            return 0;
        }
    } else {
        // Untuk file lain
        char relics_full_path[MAX_PATH_LEN];
        get_relics_path(relics_full_path, path, 0);
        
        // Periksa apakah file fragmen pertama (.000) ada
        if (access(relics_full_path, F_OK) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = get_file_size(path);
            return 0;
        }
    }
    
    return -ENOENT;
}

static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // Tambahkan Baymax.jpeg secara eksplisit jika fragmen-fragmennya ada
    if (check_baymax_exists()) {
        filler(buf, "Baymax.jpeg", NULL, 0);
    }
    
    // Tampilkan file-file lain dari relics
    DIR *dir = opendir(relics_path);
    if (dir) {
        struct dirent *entry;
        char seen_files[100][MAX_PATH_LEN] = {0};
        int seen_count = 0;
        
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                char name[MAX_PATH_LEN];
                strcpy(name, entry->d_name);
                char *dot = strrchr(name, '.');
                
                // Skip file Baymax.jpeg karena sudah ditambahkan di atas
                if (strncmp(name, "Baymax.jpeg.", 12) == 0) {
                    continue;
                }
                
                if (dot && (dot != name) && (isdigit(*(dot+1)))) {
                    // Ini adalah fragmen file
                    *dot = '\0';  // Potong ekstensi fragmen
                    
                    // Periksa apakah kita sudah memasukkan file ini
                    int already_seen = 0;
                    for (int i = 0; i < seen_count; i++) {
                        if (strcmp(seen_files[i], name) == 0) {
                            already_seen = 1;
                            break;
                        }
                    }
                    
                    if (!already_seen) {
                        filler(buf, name, NULL, 0);
                        strcpy(seen_files[seen_count], name);
                        seen_count++;
                    }
                }
            }
        }
        closedir(dir);
    }
    
    return 0;
}

static int baymax_open(const char *path, struct fuse_file_info *fi) {
    // Khusus Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        if (!check_baymax_exists()) {
            return -ENOENT;
        }
    } else {
        // File lain
        char relics_full_path[MAX_PATH_LEN];
        get_relics_path(relics_full_path, path, 0);
        
        // Periksa apakah file fragmen pertama (.000) ada
        if (access(relics_full_path, F_OK) != 0) {
            return -ENOENT;
        }
    }
    
    // Catat operasi read
    log_activity("READ", path);
    
    // Update informasi untuk deteksi copy
    strncpy(last_read_path, path, sizeof(last_read_path) - 1);
    last_read_path[sizeof(last_read_path) - 1] = '\0';
    last_read_time = time(NULL);
    is_copying = 1;
    
    return 0;
}

static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    
    int fragment_count = 0;
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        fragment_count = 14; // Baymax.jpeg terdiri dari 14 fragmen
    } else {
        fragment_count = get_fragment_count(path);
    }
    
    if (fragment_count == 0) {
        return -ENOENT;
    }
    
    off_t file_size = get_file_size(path);
    if (offset >= file_size) {
        return 0;  // EOF
    }
    
    // Mencari fragmen mana yang berisi offset yang diminta
    int current_fragment = 0;
    off_t current_offset = 0;
    off_t fragment_offset = 0;
    char fragment_path[MAX_PATH_LEN];
    struct stat st;
    
    while (current_fragment < fragment_count) {
        get_relics_path(fragment_path, path, current_fragment);
        
        if (stat(fragment_path, &st) != 0) {
            return -ENOENT;
        }
        
        if (current_offset + st.st_size > offset) {
            fragment_offset = offset - current_offset;
            break;
        }
        
        current_offset += st.st_size;
        current_fragment++;
    }
    
    // Membaca data dari fragmen yang ditemukan
    size_t total_read = 0;
    
    while (current_fragment < fragment_count && total_read < size) {
        get_relics_path(fragment_path, path, current_fragment);
        
        if (stat(fragment_path, &st) != 0) {
            break;
        }
        
        FILE *fragment_file = fopen(fragment_path, "rb");
        if (!fragment_file) {
            break;
        }
        
        fseek(fragment_file, fragment_offset, SEEK_SET);
        
        size_t bytes_to_read = st.st_size - fragment_offset;
        if (bytes_to_read > (size - total_read)) {
            bytes_to_read = size - total_read;
        }
        
        size_t bytes_read = fread(buf + total_read, 1, bytes_to_read, fragment_file);
        fclose(fragment_file);
        
        total_read += bytes_read;
        if (bytes_read < bytes_to_read) {
            break;  // Error reading
        }
        
        current_fragment++;
        fragment_offset = 0;  // Reset offset for next fragments
    }
    
    // Deteksi operasi copy berdasarkan pola akses
    if (is_copying && offset == 0 && size >= 4096) {
        check_copy_operation(path);
    }
    
    return total_read;
}

static int baymax_write(const char *path, const char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    (void) fi;
    (void) offset;  // Untuk kesederhanaan, kita abaikan offset dan selalu menulis dari awal
    
    // Jika mencoba menulis ke Baymax.jpeg, tolak operasi ini
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        return -EACCES;
    }
    
    // Hapus fragmen yang ada jika ada
    char fragment_path[MAX_PATH_LEN];
    int fragment_index = 0;
    
    while (1) {
        get_relics_path(fragment_path, path, fragment_index);
        if (access(fragment_path, F_OK) != 0) {
            break;
        }
        unlink(fragment_path);
        fragment_index++;
    }
    
    // Tulis data ke fragmen baru
    fragment_index = 0;
    size_t bytes_written = 0;
    
    while (bytes_written < size) {
        get_relics_path(fragment_path, path, fragment_index);
        
        FILE *fragment_file = fopen(fragment_path, "wb");
        if (!fragment_file) {
            return -EIO;
        }
        
        size_t bytes_to_write = size - bytes_written;
        if (bytes_to_write > MAX_FRAGMENT_SIZE) {
            bytes_to_write = MAX_FRAGMENT_SIZE;
        }
        
        size_t res = fwrite(buf + bytes_written, 1, bytes_to_write, fragment_file);
        fclose(fragment_file);
        
        if (res != bytes_to_write) {
            return -EIO;
        }
        
        bytes_written += bytes_to_write;
        fragment_index++;
    }
    
    // Log aktivitas
    char log_msg[MAX_PATH_LEN * 2];
    strcpy(log_msg, path + 1);  // Skip leading '/'
    strcat(log_msg, " -> ");
    
    for (int i = 0; i < fragment_index; i++) {
        char fragment_name[MAX_PATH_LEN];
        if (i > 0) strcat(log_msg, ", ");
        sprintf(fragment_name, "%s.%03d", path + 1, i);
        strcat(log_msg, fragment_name);
    }
    
    log_activity("WRITE", log_msg);
    return size;
}

static int baymax_truncate(const char *path, off_t size) {
    // Jika mencoba truncate Baymax.jpeg, tolak operasi ini
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        return -EACCES;
    }
    
    // Truncate operation untuk support penulisan file
    char fragment_path[MAX_PATH_LEN];
    int fragment_index = 0;
    
    // Hapus fragmen yang ada jika ada
    while (1) {
        get_relics_path(fragment_path, path, fragment_index);
        if (access(fragment_path, F_OK) != 0) {
            break;
        }
        unlink(fragment_path);
        fragment_index++;
    }
    
    if (size == 0) {
        // Jika truncate ke 0, kita cukup menghapus semua fragmen
        return 0;
    }
    
    // Untuk kesederhanaan, kita hanya menangani truncate ke 0 disini
    return 0;
}

static int baymax_unlink(const char *path) {
    // Jika mencoba menghapus Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        char log_msg[MAX_PATH_LEN * 2];
        sprintf(log_msg, "Baymax.jpeg.000 - Baymax.jpeg.013");
        
        // Hapus semua fragmen Baymax.jpeg
        for (int i = 0; i < 14; i++) {
            char fragment_path[MAX_PATH_LEN];
            sprintf(fragment_path, "%s/Baymax.jpeg.%03d", relics_path, i);
            unlink(fragment_path);
        }
        
        log_activity("DELETE", log_msg);
        return 0;
    }
    
    // Untuk file lain
    char fragment_path[MAX_PATH_LEN];
    int fragment_index = 0;
    int total_fragments = 0;
    
    // Hapus semua fragmen
    while (1) {
        get_relics_path(fragment_path, path, fragment_index);
        if (access(fragment_path, F_OK) != 0) {
            break;
        }
        
        if (unlink(fragment_path) != 0) {
            return -errno;
        }
        
        fragment_index++;
        total_fragments++;
    }
    
    if (total_fragments == 0) {
        return -ENOENT;
    }
    
    // Log aktivitas
    char log_msg[MAX_PATH_LEN * 2];
    sprintf(log_msg, "%s.000 - %s.%03d", path + 1, path + 1, total_fragments - 1);
    log_activity("DELETE", log_msg);
    
    return 0;
}

static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Jika mencoba membuat Baymax.jpeg, tolak operasi ini
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        return -EEXIST;
    }
    
    // Buat file kosong untuk implementasi create
    char fragment_path[MAX_PATH_LEN];
    get_relics_path(fragment_path, path, 0);
    
    int fd = creat(fragment_path, mode);
    if (fd == -1) {
        return -errno;
    }
    
    close(fd);
    return 0;
}

static int baymax_utimens(const char *path, const struct timespec ts[2]) {
    // Update timestamps untuk semua fragmen
    int fragment_index = 0;
    char fragment_path[MAX_PATH_LEN];
    
    while (1) {
        get_relics_path(fragment_path, path, fragment_index);
        if (access(fragment_path, F_OK) != 0) {
            break;
        }
        
        int res = utimensat(0, fragment_path, ts, AT_SYMLINK_NOFOLLOW);
        if (res == -1) {
            return -errno;
        }
        
        fragment_index++;
    }
    
    return 0;
}

static struct fuse_operations baymax_oper = {
    .getattr    = baymax_getattr,
    .readdir    = baymax_readdir,
    .open       = baymax_open,
    .read       = baymax_read,
    .write      = baymax_write,
    .truncate   = baymax_truncate,
    .unlink     = baymax_unlink,
    .create     = baymax_create,
    .utimens    = baymax_utimens,
};

int main(int argc, char *argv[]) {
    // Persiapan file dan direktori
    download_and_extract_files();
    
    // Siapkan argumen untuk FUSE
    char *fuse_argv[argc + 2];
    int fuse_argc = 0;
    
    fuse_argv[fuse_argc++] = argv[0];
    fuse_argv[fuse_argc++] = (char*)mount_path;
    
    // Tambahkan opsi -f untuk berjalan di foreground
    fuse_argv[fuse_argc++] = "-f";
    
    // Salin argumen tambahan dari baris perintah
    for (int i = 1; i < argc; i++) {
        fuse_argv[fuse_argc++] = argv[i];
    }
    
    printf("Mounting FUSE filesystem at %s...\n", mount_path);
    printf("Press Ctrl+C to unmount and exit.\n");
    
    return fuse_main(fuse_argc, fuse_argv, &baymax_oper, NULL);
}
