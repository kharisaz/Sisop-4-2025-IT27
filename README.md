# Sisop-4-2025-IT27

===================[Kelompok IT27]======================

Khumaidi Kharis Az-zacky - 5027241049

Mohamad Arkan Zahir Asyafiq - 5027241120

Abiyyu Raihan Putra Wikanto - 5027241042

============[Laporan Resmi Penjelasan Soal]=============

## soal_1(Revisi)
Program ini adalah FUSE filesystem yang mengkonversi file hexadecimal menjadi image ketika file tersebut dibaca melalui mount point.

📋 Struktur Code
1. Headers dan Definitions

        #define FUSE_USE_VERSION 26  // Menggunakan FUSE API versi 2.6
        #include <fuse.h>     // Library FUSE utama
        #include <stdio.h>    // Standard I/O
        #include <string.h>   // String operations
        #include <errno.h>    // Error codes
        #include <fcntl.h>    // File control
        #include <stdlib.h>   // Standard library
        #include <unistd.h>   // UNIX standard
        #include <sys/stat.h> // File status
        #include <sys/types.h>// System types
        #include <dirent.h>   // Directory operations
        #include <time.h>     // Time functions
        #include <ctype.h>    // Character types

   2. Global Variables

                // Cache untuk mencegah konversi duplikat
                static char converted_files[100][256];  // Array nama file yang sudah dikonversi
                static int converted_count = 0;         // Jumlah file yang sudah dikonversi
                
                // Path directories
                static const char *anomali_dir = "anomali";           // Source directory
                static const char *image_dir = "anomali/image";       // Output directory
                static const char *log_file = "anomali/conversion.log"; // Log file
      

3. Cache Management Functions

        // Cek apakah file sudah pernah dikonversi
        int is_already_converted(const char* filename) {
            for (int i = 0; i < converted_count; i++) {
                if (strcmp(converted_files[i], filename) == 0) {
                    return 1; // Sudah dikonversi
                }
            }
            return 0; // Belum dikonversi
        }
        
        // Tandai file sebagai sudah dikonversi
        void mark_as_converted(const char* filename) {
            if (converted_count < 100) {
                strncpy(converted_files[converted_count], filename, 255);
                converted_files[converted_count][255] = '\0';
                converted_count++;
            }
        }

   
        Fungsi: Mencegah file dikonversi berulang kali

4. Directory Setup Functions

        // Buat directory jika belum ada
        int create_dir(const char* path) {
            if (mkdir(path, 0755) == 0) {
                printf("✓ Created: %s/\n", path);
                return 0;
            } else if (errno == EEXIST) {
                printf("✓ Exists: %s/\n", path);
                return 0;
            }
            printf("❌ Failed: %s/\n", path);
            return -1;
        }
        
        // Setup semua directory yang diperlukan
        int setup_dirs() {
            printf("=== Auto Setup ===\n");
            if (create_dir("hexed") != 0) return -1;       // Code directory
            if (create_dir("anomali") != 0) return -1;     // Source directory
            if (create_dir("anomali/image") != 0) return -1; // Output directory
            if (create_dir("mnt") != 0) return -1;          // Mount point
            printf("✅ Setup complete!\n\n");
            return 0;
        }
   
Fungsi: Membuat struktur folder otomatis

5. Download & Extract Functions


        // Download file dari Google Drive
        int download() {
            printf("📥 Downloading...\n");
            int result = system("curl -L -o anomali_samples.zip 'https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download'");
            if (result != 0) {
                // Fallback ke wget jika curl gagal
                result = system("wget -O anomali_samples.zip 'https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download'");
            }
            return result;
        }
        
        // Extract zip dan pindahkan file ke lokasi yang benar
        int extract() {
            printf("📦 Extracting...\n");
            
            // Extract ke current directory
            int result = system("unzip -o anomali_samples.zip");
            
            if (result == 0) {
                printf("✓ Extraction successful\n");
                
                // Handle folder double (anomali/anomali/)
                if (access("anomali/1.txt", F_OK) == 0) {
                    printf("✓ Files already in correct location\n");
                } else if (access("anomali/anomali/1.txt", F_OK) == 0) {
                    printf("🔄 Moving files from double folder...\n");
                    system("mv anomali/anomali/*.txt anomali/ 2>/dev/null");
                    system("rmdir anomali/anomali/ 2>/dev/null");
                    printf("✓ Files moved to correct location\n");
                }
                
                // Clean up
                system("rm -f anomali_samples.zip");
                printf("✓ Extraction and cleanup complete\n");
            }
            
            return result;
        }
Fungsi: Download file anomali dan extract ke struktur yang benar

6. Hexadecimal Conversion Functions


        // Convert karakter hex ke integer
        int hex_to_int(char c) {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        }
        
        // Convert string hex ke binary data
        int hex_to_bin(const char* hex, unsigned char** bin, size_t* size) {
            size_t len = strlen(hex);
            size_t count = 0;
            
            // Hitung karakter hex yang valid
            for (size_t i = 0; i < len; i++) {
                if (isxdigit(hex[i])) count++;
            }
            
            if (count == 0) return -1;
            if (count % 2 != 0) count++; // Padding jika jumlah ganjil
            
            *size = count / 2;
            *bin = malloc(*size);
            if (*bin == NULL) return -1;
            
            // Konversi hex ke binary
            size_t hex_pos = 0, bin_pos = 0;
            for (size_t i = 0; i < len && bin_pos < *size; i++) {
                if (isxdigit(hex[i])) {
                    if (hex_pos % 2 == 0) {
                        (*bin)[bin_pos] = hex_to_int(hex[i]) << 4;
                    } else {
                        (*bin)[bin_pos] |= hex_to_int(hex[i]);
                        bin_pos++;
                    }
                    hex_pos++;
                }
            }
            return 0;
        }
Fungsi: Konversi string hexadecimal ke data binary

7. Timestamp & Logging Functions


        // Generate timestamp untuk nama file
        void get_time(char* buf, size_t size) {
            time_t t = time(NULL);
            struct tm* tm = localtime(&t);
            strftime(buf, size, "%Y-%m-%d_%H:%M:%S", tm);
        }
        
        // Generate timestamp untuk log
        void get_log_time(char* buf, size_t size) {
            time_t t = time(NULL);
            struct tm* tm = localtime(&t);
            strftime(buf, size, "[%Y-%m-%d][%H:%M:%S]", tm);
        }
        
        // Tulis entry ke log file
        void write_log_entry(const char* src, const char* dst) {
            FILE* f = fopen(log_file, "a");
            if (f) {
                char timestamp[64];
                get_log_time(timestamp, sizeof(timestamp));
                fprintf(f, "%s: Successfully converted hexadecimal text %s to %s.\n", 
                        timestamp, src, dst);
                fclose(f);
                printf("📝 Logged: %s -> %s\n", src, dst);
            }
        }
Fungsi: Generate timestamp dan logging aktivitas

8. Core Conversion Function


        // Fungsi utama konversi file hex ke image
        int convert_file(const char* filename) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", anomali_dir, filename);
            
            // Baca file hex
            FILE* f = fopen(path, "r");
            if (!f) return -1;
            
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            char* hex = malloc(size + 1);
            fread(hex, 1, size, f);
            hex[size] = '\0';
            fclose(f);
            
            // Konversi hex ke binary
            unsigned char* bin;
            size_t bin_size;
            if (hex_to_bin(hex, &bin, &bin_size) != 0) {
                free(hex);
                return -1;
            }
            
            // Generate nama file output dengan timestamp
            char timestamp[32];
            get_time(timestamp, sizeof(timestamp));
            
            char basename[256];
            strncpy(basename, filename, sizeof(basename) - 1);
            char* dot = strrchr(basename, '.');
            if (dot) *dot = '\0';
            
            char outname[1024];
            snprintf(outname, sizeof(outname), "%s_image_%s.png", basename, timestamp);
            
            char outpath[1024];
            snprintf(outpath, sizeof(outpath), "%s/%s", image_dir, outname);
            
            // Tulis binary data ke file image
            FILE* out = fopen(outpath, "wb");
            if (!out) {
                free(hex);
                free(bin);
                return -1;
            }
            
            fwrite(bin, 1, bin_size, out);
            fclose(out);
            
            // Log dan cleanup
            write_log_entry(filename, outname);
            printf("✅ Converted: %s -> %s (%zu bytes)\n", filename, outname, bin_size);
            
            free(hex);
            free(bin);
            sleep(1); // Delay untuk timestamp unik
            return 0;
        }
Fungsi: Core function untuk konversi hex ke image

9. FUSE Operations

        // FUSE getattr - mendapatkan atribut file
        static int hello_getattr(const char *path, struct stat *stbuf) {
            memset(stbuf, 0, sizeof(struct stat));
            
            if (strcmp(path, "/") == 0) {
                // Root directory
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                return 0;
            }
            
            // Map path dari mount ke anomali directory
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s%s", anomali_dir, path);
            
            if (stat(full_path, stbuf) == 0) {
                return 0;
            }
            
            return -ENOENT;
        }
        
        // FUSE readdir - list isi directory
        static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                                 off_t offset, struct fuse_file_info *fi) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s%s", anomali_dir, path);
            
            DIR *dp = opendir(full_path);
            if (!dp) return -ENOENT;
            
            filler(buf, ".", NULL, 0);
            filler(buf, "..", NULL, 0);
            
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
                    filler(buf, de->d_name, NULL, 0);
                }
            }
            
            closedir(dp);
            return 0;
        }
        
        // FUSE open - buka file untuk reading
        static int hello_open(const char *path, struct fuse_file_info *fi) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s%s", anomali_dir, path);
            
            if (access(full_path, F_OK) != 0) return -ENOENT;
            if ((fi->flags & 3) != O_RDONLY) return -EACCES;
            
            return 0;
        }
        
        // FUSE read - TRIGGER untuk konversi hex ke image
        static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                              struct fuse_file_info *fi) {
            // TRIGGER KONVERSI untuk file .txt (hanya sekali per file)
            const char* filename = path + 1;
            if (strstr(filename, ".txt") && strlen(filename) > 4) {
                if (!is_already_converted(filename)) {
                    printf("🚀 Converting: %s\n", filename);
                    if (convert_file(filename) == 0) {
                        mark_as_converted(filename);
                    }
                }
            }
            
            // Baca file asli dan return ke user
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s%s", anomali_dir, path);
            
            FILE *f = fopen(full_path, "r");
            if (!f) return -errno;
            
            fseek(f, 0, SEEK_END);
            long file_size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            if (offset >= file_size) {
                fclose(f);
                return 0;
            }
            
            if (offset + (long)size > file_size) {
                size = file_size - offset;
            }
            
            fseek(f, offset, SEEK_SET);
            size_t bytes = fread(buf, 1, size, f);
            fclose(f);
            
            return bytes;
        }
Fungsi: FUSE operations untuk virtual filesystem

10. Main Function


        int main(int argc, char *argv[]) {
            if (argc < 2) {
                printf("Usage: %s <mountpoint>\n", argv[0]);
                return 1;
            }
            
            printf("=== FUSE Hex to Image Converter ===\n");
            printf("🧙 Shorekeeper's Assistant\n\n");
            
            // Setup environment
            if (setup_dirs() != 0) return 1;
            if (download() != 0) return 1;
            if (extract() != 0) return 1;
            
            // Initialize log
            FILE* log = fopen(log_file, "w");
            if (log) fclose(log);
            
            printf("\n🚀 Starting FUSE...\n");
            printf("Mount point: %s\n", argv[argc-1]);
            
            // Start FUSE main loop
            return fuse_main(argc, argv, &hello_oper, NULL);
        }
    
🚀 Cara Menjalankan Program
1. Persiapan Environment
bash# Install dependencies
sudo apt-get update
sudo apt-get install gcc libfuse-dev pkg-config curl unzip

Pastikan di directory yang tepat
cd ~/modul_4/soal_1/

2. Compile Program
bash# Compile dengan flag yang benar
gcc -D_FILE_OFFSET_BITS=64 -o hexed hexed.c -lfuse -lpthread

3. Jalankan Program
Mode 1: Foreground (Recommended untuk Development)
bash# Terminal 1 - Run FUSE program
./hexed -f mnt

Output yang diharapkan:
=== FUSE Hex to Image Converter ===
🧙 Shorekeeper's Assistant
=== Auto Setup ===
✓ Created: hexed/
✓ Created: anomali/
✓ Created: anomali/image/
✓ Created: mnt/
✅ Setup complete!
📥 Downloading...
📦 Extracting...
✓ Extraction and cleanup complete
🚀 Starting FUSE...
Mount point: mnt

Mode 2: Debug Mode (untuk Troubleshooting)
bash# Terminal 1 - Run dengan debug output
./hexed -d -f mnt
Akan menampilkan semua FUSE operations

Mode 3: Background/Daemon Mode
bash# Run di background
./hexed mnt

Cek apakah running
ps aux | grep hexed
mount | grep mnt

4. Test Program (Terminal Baru)
bash# Terminal 2 - Test functionality

1. List files di mount point
ls mnt/
Expected: 1.txt 2.txt 3.txt 4.txt 5.txt 6.txt 7.txt conversion.log image

2. Trigger konversi dengan membaca file
cat mnt/1.txt
Expected: Hex content displayed + konversi triggered

3. Cek hasil konversi
ls anomali/image/
Expected: 1_image_2025-05-23_23:20:10.png

4. Cek log
cat anomali/conversion.log
Expected: [2025-05-23][23:20:10]: Successfully converted hexadecimal text 1.txt to 1_image_2025-05-23_23:20:10.png.
5. Test file lain
cat mnt/2.txt
cat mnt/3.txt
head mnt/4.txt

6. Cek semua hasil
ls anomali/image/
cat anomali/conversion.log

5. Stop Program
bash# Unmount FUSE filesystem
fusermount -u mnt/


## soal_2(No Revisi)

Program ini adalah FUSE filesystem yang menggabungkan 14 fragmen file Baymax.jpeg (.000 hingga .013) menjadi satu file utuh yang dapat dibaca, disalin, dan dimanipulasi seolah-olah tidak pernah terpecah.

📋 Struktur Code
1. Headers dan Konstanta

        #define FUSE_USE_VERSION 26
        #include <fuse.h>
        // ... other includes
        
        #define MAX_FRAGMENT_SIZE 1024  // Ukuran maksimal per fragmen: 1 KB
        #define MAX_PATH_LEN 1024       // Panjang maksimal path
        #define LOG_FILE "activity.log"  // File log aktivitas
Fungsi: Menggunakan FUSE API versi 2.6 dan mendefinisikan konstanta penting.

2. Variabel Global

        static const char *mount_path = "./mount_dir";   // Mount point
        static const char *relics_path = "./relics";    // Directory fragmen asli
        static const char *download_url = "https://..."; // URL download Baymax fragments
        
        // Tracking operasi copy
        static int is_copying = 0;
        static char last_read_path[MAX_PATH_LEN] = {0};
        static time_t last_read_time = 0;
Fungsi: Path directory dan variabel untuk tracking operasi copy.

3. Download & Setup Functions

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
            
            // Cek apakah fragmen sudah ada
            char first_fragment[MAX_PATH_LEN];
            sprintf(first_fragment, "%s/Baymax.jpeg.000", relics_path);
            
            if (access(first_fragment, F_OK) != 0) {
                printf("Downloading Baymax fragments...\n");
                
                // Download ZIP file
                char download_cmd[MAX_PATH_LEN];
                sprintf(download_cmd, "wget -q \"%s\" -O temp_baymax.zip", download_url);
                system(download_cmd);
                
                // Extract ke directory relics
                printf("Extracting files to relics directory...\n");
                char extract_cmd[MAX_PATH_LEN];
                sprintf(extract_cmd, "unzip -q temp_baymax.zip -d %s", relics_path);
                system(extract_cmd);
                
                // Hapus file ZIP
                system("rm -f temp_baymax.zip");
            } else {
                printf("Baymax fragments already exist in relics directory.\n");
            }
            
            // Initialize log file
            FILE *log_file = fopen(LOG_FILE, "w");
            if (log_file) {
                fclose(log_file);
            }
            
            printf("Environment ready!\n");
        }
Fungsi:

Membuat directory relics/ dan mount_dir/
Download fragmen Baymax jika belum ada
Extract ZIP ke directory relics/
Initialize log file

4. Logging Functions
   
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
Fungsi: Mencatat aktivitas ke activity.log dengan timestamp.

6. Copy Detection Function
   
        void check_copy_operation(const char *path) {
            time_t now = time(NULL);
            if (strcmp(path, last_read_path) == 0 && difftime(now, last_read_time) < 2) {
                // Deteksi destination copy dari command line
                char dest[MAX_PATH_LEN] = "/tmp";  // default destination
                char cmd[MAX_PATH_LEN * 2];
                char cmdout[MAX_PATH_LEN * 4] = {0};
                
                // Cari proses cp yang sedang running
                sprintf(cmd, "ps axww | grep -E 'cp [^ ]+ [^ ]+' | grep -v grep");
                FILE *fp = popen(cmd, "r");
                if (fp) {
                    if (fgets(cmdout, sizeof(cmdout) - 1, fp)) {
                        // Parse output ps untuk mendapatkan destination copy
                        char *p = strstr(cmdout, path + 1);
                        if (p) {
                            p += strlen(path + 1);
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
Fungsi: Mendeteksi operasi copy dengan monitoring proses cp dan mencatat ke log.

8. Utility Functions
   
        // Generate path lengkap untuk fragmen
        void get_relics_path(char *relics_full_path, const char *path, int fragment_index) {
            if (fragment_index >= 0) {
                sprintf(relics_full_path, "%s%s.%03d", relics_path, path, fragment_index);
            } else {
                sprintf(relics_full_path, "%s%s", relics_path, path);
            }
        }
        
        // Hitung jumlah fragmen file
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
        
        // Hitung ukuran total dari semua fragmen
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
        
        // Cek apakah semua fragmen Baymax ada
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
Fungsi: Helper functions untuk path handling, counting fragments, dan size calculation.

🔧 FUSE Operations
7. getattr - File Attributes
cstatic int baymax_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    // Khusus untuk Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
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
        
        if (access(relics_full_path, F_OK) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;
            stbuf->st_size = get_file_size(path);
            return 0;
        }
    }
    
    return -ENOENT;
}
Fungsi: Menampilkan Baymax.jpeg sebagai file utuh dengan ukuran gabungan semua fragmen.
8. readdir - Directory Listing
cstatic int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }
    
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // Tambahkan Baymax.jpeg jika fragmen ada
    if (check_baymax_exists()) {
        filler(buf, "Baymax.jpeg", NULL, 0);
    }
    
    // Tampilkan file lain dari relics (tanpa ekstensi fragmen)
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
                
                // Skip Baymax.jpeg fragments
                if (strncmp(name, "Baymax.jpeg.", 12) == 0) {
                    continue;
                }
                
                if (dot && (dot != name) && (isdigit(*(dot+1)))) {
                    // Ini adalah fragmen file - hilangkan ekstensi
                    *dot = '\0';
                    
                    // Cek apakah sudah ditampilkan
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
Fungsi: Menampilkan file sebagai satu kesatuan (tanpa ekstensi .000, .001, dll).
9. read - File Reading dengan Fragment Assembly
cstatic int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    int fragment_count = 0;
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        fragment_count = 14; // Baymax.jpeg = 14 fragmen
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
    
    // Cari fragmen mana yang berisi offset yang diminta
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
    
    // Baca data dari fragmen secara berurutan
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
        fragment_offset = 0;  // Reset untuk fragmen berikutnya
    }
    
    // Deteksi operasi copy
    if (is_copying && offset == 0 && size >= 4096) {
        check_copy_operation(path);
    }
    
    return total_read;
}
Fungsi: CORE FEATURE - Menggabungkan fragmen secara seamless saat file dibaca.
10. write - File Writing dengan Fragment Split
cstatic int baymax_write(const char *path, const char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    // Tolak penulisan ke Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        return -EACCES;
    }
    
    // Hapus fragmen yang ada
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
    
    // Tulis data ke fragmen baru (1KB per fragmen)
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
    
    // Log aktivitas WRITE
    char log_msg[MAX_PATH_LEN * 2];
    strcpy(log_msg, path + 1);
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
Fungsi: Memecah file baru menjadi fragmen 1KB dan menyimpan di relics/.
11. unlink - File Deletion
cstatic int baymax_unlink(const char *path) {
    // Hapus Baymax.jpeg
    if (strcmp(path, "/Baymax.jpeg") == 0) {
        char log_msg[MAX_PATH_LEN * 2];
        sprintf(log_msg, "Baymax.jpeg.000 - Baymax.jpeg.013");
        
        // Hapus semua 14 fragmen Baymax
        for (int i = 0; i < 14; i++) {
            char fragment_path[MAX_PATH_LEN];
            sprintf(fragment_path, "%s/Baymax.jpeg.%03d", relics_path, i);
            unlink(fragment_path);
        }
        
        log_activity("DELETE", log_msg);
        return 0;
    }
    
    // Hapus file lain
    char fragment_path[MAX_PATH_LEN];
    int fragment_index = 0;
    int total_fragments = 0;
    
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
    
    // Log aktivitas DELETE
    char log_msg[MAX_PATH_LEN * 2];
    sprintf(log_msg, "%s.000 - %s.%03d", path + 1, path + 1, total_fragments - 1);
    log_activity("DELETE", log_msg);
    
    return 0;
}
Fungsi: Menghapus semua fragmen file dan mencatat ke log.
🚀 Cara Menjalankan Program
1. Persiapan & Compile
bash# Install dependencies
sudo apt-get install gcc libfuse-dev pkg-config wget unzip

# Compile program
gcc -D_FILE_OFFSET_BITS=64 -o baymax baymax.c -lfuse -lpthread
2. Jalankan Program
bash# Run program (otomatis download & mount)
./baymax

# Atau dengan debug mode
./baymax -d
3. Expected Output
bash$ ./baymax
Preparing environment...
Creating relics directory...
Creating mount directory...
Downloading Baymax fragments...
Extracting files to relics directory...
Environment ready!
Mounting FUSE filesystem at ./mount_dir...
Press Ctrl+C to unmount and exit.
4. Test Functionality (Terminal Baru)
bash# Test 1: List files
ls mount_dir/
# Expected: Baymax.jpeg (dan file lain jika ada)

# Test 2: View file info
ls -la mount_dir/Baymax.jpeg
stat mount_dir/Baymax.jpeg

# Test 3: Read file (gabungan seamless)
file mount_dir/Baymax.jpeg
# Expected: JPEG image data

# Test 4: Copy file
cp mount_dir/Baymax.jpeg /tmp/

# Test 5: Create new file
echo "test data" > mount_dir/test.txt

# Test 6: Delete file
rm mount_dir/test.txt

# Test 7: Check logs
cat activity.log
5. Expected Log Output
bash$ cat activity.log
[2025-05-23 23:30:01] READ: Baymax.jpeg
[2025-05-23 23:30:05] COPY: Baymax.jpeg -> /tmp/Baymax.jpeg
[2025-05-23 23:30:10] WRITE: test.txt -> test.txt.000
[2025-05-23 23:30:15] DELETE: test.txt.000 - test.txt.000



## soal_3

Soal ini meminta untuk membuat sistem AntiNK menggunakan Docker yang menjalankan FUSE dalam container terisolasi. Sistem harus mendeteksi file dengan kata kunci "nafis" atau "kimcun" dan membalikkan nama file tersebut saat ditampilkan. Saat file berbahaya (kimcun atau nafis) terdeteksi, sistem akan mencatat peringatan ke dalam log. Isi dari file teks normal akan di enkripsi menggunakan ROT13 saat dibaca, sedangkan file teks berbahaya tidak di enkripsi. Semua aktivitas dicatat dengan ke dalam log file /var/log/it24.log yang dimonitor secara real-time oleh container logger.

### antink.c

        #define FUSE_USE_VERSION 31
        #include <fuse3/fuse.h>
        #include <limits.h>
        #include <dirent.h>
        #include <string.h>
        #include <errno.h>
        #include <fcntl.h>
        #include <unistd.h>
        #include <stdlib.h>
        #include <stdio.h>
        #include <time.h>

Header FUSE dan Header C

        static char *ROOT;               // host directory path
        static const char *LOG = "/var/log/it24.log";
        const char *keys[] = {"nafis", "kimcun"};

ROOT akan menyimpan path absolut dari direktori sumber (/it24_host).

LOG lokasi file log di dalam container (antink_logs).

keys[] daftar kata kunci yang dianggap “berbahaya”.

        void log_msg(const char *t, const char *f) {
            FILE *l = fopen(LOG, "a");
            if (!l) return;
            time_t u = time(NULL);
            struct tm x = *localtime(&u);
            fprintf(l, "%04d-%02d-%02d %02d:%02d:%02d [%s] %s\n",
                    x.tm_year+1900, x.tm_mon+1, x.tm_mday,
                    x.tm_hour, x.tm_min, x.tm_sec,
                    t, f);
            fclose(l);
        }

log_msg membuka file log (fopen mode append), menulis timestamp + tipe (t: "ALERT", "ENCRYPT", atau "REVERSE") + nama file (f), lalu menutup file.

Format timestamp: YYYY-MM-DD hh:mm:ss.

        int has_key(const char *n, char buf[]) {
            for (int i = 0; i < 2; i++) {
                if (strcasestr(n, keys[i])) {
                    strcpy(buf, keys[i]);
                    return 1;
                }
            }
            return 0;
        }

has_key memeriksa apakah string n mengandung salah satu keys.

Jika ya, meng-copy kata kunci ke buf dan kembalikan 1; jika tidak, kembalikan 0.

        void rev_str(char *s) {
            int i = 0, j = strlen(s) - 1;
            while (i < j) {
                char c = s[i]; s[i] = s[j]; s[j] = c;
                i++; j--;
            }
        }

rev_str membalik urutan karakter dalam string (in-place).

        void rot13(char *s) {
            for (; *s; s++) {
                if (*s >= 'a' && *s <= 'z') *s = ((*s - 'a' + 13) % 26) + 'a';
                else if (*s >= 'A' && *s <= 'Z') *s = ((*s - 'A' + 13) % 26) + 'A';
            }
        }

Setiap huruf digeser 13 posisi dalam alfabet, menjaga kapitalisasi. (ROT13)

        static int x_getattr(const char *p, struct stat *st) {
            char f[PATH_MAX];
            snprintf(f, PATH_MAX, "%s%s", ROOT, p);
            if (lstat(f, st) < 0) return -errno;
            return 0;
        }

Mendapatkan atribut file :

Panggil lstat() untuk mengisi st (size, mode, mtime, dll).

Jika lstat gagal, return kode error (-errno), kalau berhasil return 0.

        static int x_readdir(const char *p, void *b, fuse_fill_dir_t fn,
                             off_t o, struct fuse_file_info *i, unsigned f) {
            DIR *d; struct dirent *e;
            char buf[64], dpath[PATH_MAX], name[NAME_MAX+1];
            snprintf(dpath, PATH_MAX, "%s%s", ROOT, p);
            d = opendir(dpath);
            if (!d) return -errno;
            while ((e = readdir(d))) {
                strcpy(name, e->d_name);
                if (has_key(e->d_name, buf)) {
                    rev_str(name);
                    log_msg("ALERT", e->d_name);
                }
                fn(b, name, NULL, 0, 0);
            }
            closedir(d);
            return 0;
        }

Membaca isi direktori :

- Buka direktori ROOT + p dengan opendir().

- Loop pakai readdir() untuk setiap entri (struct dirent *e).

- Salin nama asli ke variabel name.

- Deteksi key: jika e->d_name mengandung “nafis” atau “kimcun”,

- balik name (pakai rev_str),

- tulis log [ALERT].

- fn(buf, name, NULL, 0, 0) akan “menambahkan” entry ke list yang dikembalikan oleh ls.

- Setelah selesai, closedir(d), return 0.

        static int x_open(const char *p, struct fuse_file_info *i) {
            char f[PATH_MAX];
            snprintf(f, PATH_MAX, "%s%s", ROOT, p);
            int fd = open(f, i->flags);
            if (fd < 0) return -errno;
            close(fd);
            return 0;
        }

- Cek bahwa file ada dan bisa dibuka dengan open(ROOT+p, fi->flags).

- Kalau gagal, return -errno. Kalau berhasil, langsung close() dan return 0.

- Memastikan FUSE memvalidasi permission dan keberadaan file sebelum read.

        static int x_read(const char *p, char *b, size_t s, off_t o,
                          struct fuse_file_info *i) {
            (void)i;
            char f[PATH_MAX], k[64];
            snprintf(f, PATH_MAX, "%s%s", ROOT, p);
            int fd = open(f, O_RDONLY);
            if (fd < 0) return -errno;
            char *t = malloc(s);
            int r = pread(fd, t, s, o);
            if (r > 0) {
                memcpy(b, t, r);
                if (has_key(p, k)) log_msg("REVERSE", p);
                else { rot13(b); log_msg("ENCRYPT", p); }
            }
            free(t); close(fd);
            return r;
        }

- Buka file asal (open(ROOT+p, O_RDONLY)), baca size byte mulai di offset offset via pread().

- Simpan hasil baca ke buffer sementara t.

- Copy ke buffer FUSE (memcpy(buf, t, r)).

- Jika path mengandung key, cukup tulis log [REVERSE].

- Jika bukan anomali, panggil rot13(buf) untuk enkripsi, lalu log [ENCRYPT].

- Tutup file dan return jumlah byte yang dibaca (atau error code).

        static struct fuse_operations ops = {
            .getattr = x_getattr,
            .readdir = x_readdir,
            .open    = x_open,
            .read    = x_read,
        };

Struct ops berisi daftar fungsi yang menangani operasi dasar filesystem seperti melihat atribut, membaca direktori, membuka, dan membaca file menggunakan FUSE.

        int main(int argc, char *argv[]) {
            if (argc < 3) {
                fprintf(stderr, "Usage: %s <root_dir> <mount_point>\n", argv[0]);
                return 1;
            }
            ROOT = realpath(argv[1], NULL);
            if (!ROOT) {
                perror("realpath");
                return 1;
            }
        
            // jalankan FUSE di foreground + debug mode
            int fuse_argc = 4;
            char *fuse_argv[] = { argv[0], "-f", "-d", argv[2] };
        
            umask(0);
            return fuse_main(fuse_argc, fuse_argv, &ops, NULL);
        }

- Cek argumen: harus ada dua path: root dan mount.

- realpath: mengubah path sumber jadi absolut, simpan di ROOT.

- Buat array fuse_argv 

- umask(0): atur permission mask agar FUSE bisa mengatur mode file sesuai host.

- fuse_main(...): jalankan loop utama FUSE—akan mem-block (tidak exit) sampai unmount.

### docker-compose.yml

        services:
          antink-server:
            build: .
            container_name: antink-server-1
            privileged: true           # perlu untuk mount propagation & fuse
            cap_add:
              - SYS_ADMIN
            devices:
              - /dev/fuse:/dev/fuse
            security_opt:
              - apparmor:unconfined
            volumes:
              - ./it24_host:/it24_host:rw
              - ./antink_mount:/antink_mount:rw,shared    # <== enable shared propagation
              - ./antink_logs:/var/log:rw
            command: ["/src/antink", "/it24_host", "/antink_mount"]
        
          antink-logger:
            image: busybox
            container_name: antink-logger-1
            volumes:
              - ./antink_logs:/var/log:rw
            command: ["sh","-c","tail -F /var/log/it24.log"]

antink-server

- privileged & cap_add: beri izin ekstra untuk mount FUSE.

- devices: map /dev/fuse dari host.

- volumes:

    host → /it24_host

    local dir → /antink_mount (shared propagation agar FUSE mount terlihat di host)

    host → /var/log

- command: override CMD di Dockerfile, jalankan FUSE dengan argumen yang sama.

antink-logger

- Image sederhana BusyBox.

- Mount log volume, lalu tail -F file log agar log baru muncul realtime.

#### Dockerfile

        FROM ubuntu:22.04
        
        RUN apt-get update && \
            apt-get install -y build-essential pkg-config libfuse3-dev fuse3

- Di sini kita menggunakan Ubuntu 22.04 sebagai sistem operasi di dalam container.

- RUN menjalankan perintah di dalam layer image.

- apt-get update memperbarui daftar paket.

- apt-get install -y ... memasang: build-essential, pkg-config, libfuse3-dev, fuse3

        RUN mkdir -p /it24_host /antink_mount /antink_logs

mkdir -p membuat tiga direktori kosong di dalam container

        COPY antink.c /src/antink.c
        WORKDIR /src

COPY menyalin file antink.c dari folder proyek ke dalam direktori /src di container.

WORKDIR berguna untuk membuat /src sebagai direktori kerja default; setiap perintah berikutnya dijalankan dari sana.

        RUN gcc -g antink.c -o antink `pkg-config fuse3 --cflags --libs`

Memanggil gcc (GNU C Compiler)

        CMD ["/src/antink", "/it24_host", "/antink_mount"]

CMD menetapkan perintah default yang dijalankan saat container dijalankan.

Disini kita menjalankan binary antink dengan dua argumen:

/it24_host → direktori sumber file

/antink_mount → direktori mount point di mana FUSE akan “memunculkan” filesystem

## soal_4

Pada soal ini kita ditugaskan untuk membuat program fuse yang berupa game, dimana perlu membuat dua directori (fuse_dir & chiho) dan mount. Terdapat 6 area yang dapat diakses dalam game, yakni starter, metro, dragon, blackrose, heaven, dan youth. Adapan directory fuse_dir menjadi tujuan user input, dan directory chiho dapat menyimpan input dengan merubah format data sesuai dengan area.

- Define dan import library

        #define FUSE_USE_VERSION 31
        
        #include <fuse3/fuse.h>
        #include <stdio.h>
        #include <string.h>
        #include <errno.h>
        #include <fcntl.h>
        #include <stdlib.h>
        #include <unistd.h>
        #include <sys/types.h>
        #include <sys/stat.h>
        #include <dirent.h>
        #include <openssl/evp.h>
        #include <openssl/rand.h>
        #include <zlib.h>
        
        #define BASE_DIR "chiho"
        #define SEVEN_REF_DIR "fuse_dir/7sref"
        #define IV_SIZE 16
        
        #define AES_KEY "thisisaverysecurekeyforaes256!!!"

- Mendeklarasikan struct dan array yang menyimpan area yang tesedia di dalam game serta ketentuan perubahan format data input berdasarkan area yang akan disimpan di directry chiho.

        typedef struct {
            const char *name;
            const char *extension;
        } AreaInfo;
        
        static const AreaInfo areas[] = {
            {"starter", ".mai"},
            {"metro", ".ccc"},
            {"dragon", ".rot"},
            {"blackrose", ".bin"},
            {"heaven", ".enc"},
            {"youth", ".gz"},
            {"7sref", ""}
        };

- Selanjutnya, mendeklarasikan function untuk menemukan dan check area. Mencari area berdasarkan nama area dari input, dan mengakses atau reach area berdasarkan path..

        static int is_area_root(const char *path) {
            for (int i = 0; i < (int)(sizeof(areas) / sizeof(areas[0])); i++) {
                char arena_path[64];
                snprintf(arena_path, sizeof(arena_path), "/%s", areas[i].name);
                if (strcmp(path, arena_path) == 0) return 1;
            }
            return 0;
        }
        
        static const AreaInfo *find_area_by_name(const char *name) {
            for (int i = 0; i < (int)(sizeof(areas) / sizeof(areas[0])); i++) {
                if (strcmp(name, areas[i].name) == 0)
                    return &areas[i];
            }
            return NULL;
        }
        static const AreaInfo *get_area_from_path(const char *path, char *area_name, size_t area_name_size) {
            for (int i = 0; i < (int)(sizeof(areas) / sizeof(areas[0])-1); i++) {
                size_t len = strlen(areas[i].name);
                if (strncmp(path, "/", 1) == 0 && strncmp(path+1, areas[i].name, len) == 0 && (path[1+len] == '/' || path[1+len] == '\0')) {
                    if (area_name)
                        snprintf(area_name, area_name_size, "%s", areas[i].name);
                    return &areas[i];
                }
            }
            return NULL;
        }

- Membuat function untuk menyimpan input user dari area fuse_dir ke directory chiho yang nantinya format file dirubah berdasarkan ketentuan area dan directory 7sref sebagai gateaway. 

        static void compute_chiho_path(char *fpath, size_t size, const char *path) {
            const AreaInfo *area = get_area_from_path(path, NULL, 0);
            if (!area) {
                snprintf(fpath, size, "%s%s", BASE_DIR, path);
                return;
            }
            const char *filename = path + 1 + strlen(area->name);
            if (*filename == '/')
                filename++;
        
            snprintf(fpath, size, "%s/%s/%s%s", BASE_DIR, area->name, filename, area->extension);
        }
        
        static void compute_7sref_path(char *fpath, size_t size, const char *path) {
            const AreaInfo *area = get_area_from_path(path, NULL, 0);
            if (!area) {
                snprintf(fpath, size, "%s%s", SEVEN_REF_DIR, path); // fallback
                return;
            }
            const char *filename = path + 1 + strlen(area->name);
            if (*filename == '/')
                filename++;
        
            size_t flen = strlen(filename);
            char filename_no_ext[512];
            snprintf(filename_no_ext, sizeof(filename_no_ext), "%s", filename);
        
            size_t ext_len = strlen(area->extension);
            if (ext_len > 0 && flen > ext_len) {
                if (strcmp(filename + flen - ext_len, area->extension) == 0) {
                    filename_no_ext[flen - ext_len] = '\0';
                }
            }
        
            snprintf(fpath, size, "%s/%s/%s", SEVEN_REF_DIR, area->name, filename_no_ext);
        }

  - Membuat function untuk enkripsi menggunakan ROT13 sesuai dengan ketentuan area ketiga, dragon.
 
          void rot13(char *data, size_t len) {
            for (size_t i = 0; i < len; i++) {
                if ((data[i] >= 'a' && data[i] <= 'z'))
                    data[i] = 'a' + (data[i] - 'a' + 13) % 26;
                else if ((data[i] >= 'A' && data[i] <= 'Z'))
                    data[i] = 'A' + (data[i] - 'A' + 13) % 26;
            }
        }

- Untuk ketentuan area kedua, metro, perlu membuat function untuk shift file sesuai lokasinya, dimana terdapat ketentuan dalam shift :

E -> E

n -> o (+(1 % 256))

e -> g (+(2 % 256))

r -> u (+(3 % 256))


        void shift_content(const char *in_buf, char *out_buf, size_t size, int direction) {
            for (size_t i = 0; i < size; i++) {
                unsigned char val = (unsigned char)in_buf[i];
                val = (unsigned char)(val + (direction * ((i + 1) % 256)));
                out_buf[i] = (char)val;
            }
        }

- Selanjutnya membuat function untuk enkripsi data ke AES-256-CBC menggunakan openssl, yang sesuai dengan ketentuan pada area Heaven.

                int aes_encrypt(const char *in, char *out, int len, unsigned char *iv) {
                    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
                    if (!ctx) return -1;
                
                    int ret = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)AES_KEY, iv);
                    if (ret != 1) {
                        EVP_CIPHER_CTX_free(ctx);
                        return -2;
                    }
                
                    int out_len1 = 0, out_len2 = 0;
                    ret = EVP_EncryptUpdate(ctx, (unsigned char *)out, &out_len1, (unsigned char *)in, len);
                    if (ret != 1) {
                        EVP_CIPHER_CTX_free(ctx);
                        return -3;
                    }
                
                    ret = EVP_EncryptFinal_ex(ctx, (unsigned char *)out + out_len1, &out_len2);
                    if (ret != 1) {
                        EVP_CIPHER_CTX_free(ctx);
                        return -4;
                    }
                
                    EVP_CIPHER_CTX_free(ctx);
                    return out_len1 + out_len2;
                }
                
                int aes_decrypt(const char *in, char *out, int len, unsigned char *iv) {
                    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
                    if (!ctx) return -1;
                
                    int ret = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)AES_KEY, iv);
                    if (ret != 1) {
                        EVP_CIPHER_CTX_free(ctx);
                        return -2;
                    }
                
                    int out_len1 = 0, out_len2 = 0;
                    ret = EVP_DecryptUpdate(ctx, (unsigned char *)out, &out_len1, (unsigned char *)in, len);
                    if (ret != 1) {
                        EVP_CIPHER_CTX_free(ctx);
                        return -3;
                    }
                
                    ret = EVP_DecryptFinal_ex(ctx, (unsigned char *)out + out_len1, &out_len2);
                    if (ret != 1) {
                        EVP_CIPHER_C
          TX_free(ctx);
                        return -4;
                    }
                
                    EVP_CIPHER_CTX_free(ctx);
                    return out_len1 + out_len2;
                }

- Selanjutnya, perlu membuat function untuk mengompresi file ke zib untuk menghemat storage. Fitur ini digunakan ketika menyimpan file ke dalam area youth.

        int gzip_compress(const char *in, size_t in_size, char *out, size_t *out_size) {
            z_stream zs = {0};
            int ret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
            if (ret != Z_OK) return -1;
        
            zs.next_in = (Bytef *)in;
            zs.avail_in = (uInt)in_size;
            zs.next_out = (Bytef *)out;
            zs.avail_out = (uInt)(*out_size);
        
            ret = deflate(&zs, Z_FINISH);
            if (ret != Z_STREAM_END) {
                deflateEnd(&zs);
                return -2;
            }
        
            *out_size = zs.total_out;
            deflateEnd(&zs);
            return 0;
        }
        
        int gzip_decompress(const char *in, size_t in_size, char *out, size_t *out_size) {
            z_stream zs = {0};
            int ret = inflateInit(&zs);
            if (ret != Z_OK) return -1;
        
            zs.next_in = (Bytef *)in;
            zs.avail_in = (uInt)in_size;
            zs.next_out = (Bytef *)out;
            zs.avail_out = (uInt)(*out_size);
        
            ret = inflate(&zs, Z_FINISH);
            if (ret != Z_STREAM_END) {
                inflateEnd(&zs);
                return -2;
            }
        
            *out_size = zs.total_out;
            inflateEnd(&zs);
            return 0;
        }

- Mendeklarasikan functionn xmp_getattr untuk mengambil atribut file/direktori.

        static int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
            (void)fi;
            memset(stbuf, 0, sizeof(struct stat));
        
            if (strcmp(path, "/") == 0 || is_area_root(path)) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                return 0;
            }
        
            char fpath[512];
            compute_chiho_path(fpath, sizeof(fpath), path);
            int res = lstat(fpath, stbuf);
            if (res == -1) return -errno;
            return 0;
        }

- Mendeklarasikan functionn xmp_readdir untuk membaca isi sebuah direktori.

        static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
            off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
            (void)offset;
            (void)fi;
            (void)flags;
        
            filler(buf, ".", NULL, 0, 0);
            filler(buf, "..", NULL, 0, 0);
        
            if (strcmp(path, "/") == 0) {
                for (int i = 0; i < (int)(sizeof(areas) / sizeof(areas[0])); i++) {
                    filler(buf, areas[i].name, NULL, 0, 0);
                }
                return 0;
            }
        
            char fpath[512];
            compute_chiho_path(fpath, sizeof(fpath), path);
        
            DIR *dp = opendir(fpath);
            if (dp == NULL) return -errno;
        
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                struct stat st = {0};
                st.st_ino = de->d_ino;
                st.st_mode = de->d_type << 12;
                filler(buf, de->d_name, &st, 0, 0);
            }
            closedir(dp);
            return 0;
        }

- Mendeklarasikan functionn xmp_open untuk membuka atau menjalankan file.

        static int xmp_open(const char *path, struct fuse_file_info *fi) {
            char fpath[512];
            compute_chiho_path(fpath, sizeof(fpath), path);
            int fd = open(fpath, fi->flags);
            if (fd == -1) return -errno;
            fi->fh = fd;
            return 0;
        }

- Membuat function untuk menulis data ke sebuah file pada path tertentu, dimulai dari offset tertentu dalam file tersebut.

        static ssize_t write_file_at_path(const char *fullpath, const char *buf, size_t size, off_t offset) {
            int fd = open(fullpath, O_WRONLY | O_CREAT, 0644);
            if (fd == -1) return -errno;
        
            ssize_t written = pwrite(fd, buf, size, offset);
            close(fd);
            if (written == -1) return -errno;
            return written;
        }

- Selanjutnya, membuat function untuk melakukan transformasi khusus pada data yang akan ditulis ke file, tergantung pada path-nya. Dimana function ini berperan dalam menentukan directory tujuan untuk merubah format file berdasarkan area tujuan.

        static int area_transform_before_write(const char *path, const char *in_buf, size_t size, char **out_buf, size_t *out_size) {
        
            if (strncmp(path, "/metro/", 7) == 0) {
                *out_buf = (char *)malloc(size);
                if (!*out_buf) return -ENOMEM;
                shift_content(in_buf, *out_buf, size, 1);
                *out_size = size;
                return 0;
            } else if (strncmp(path, "/dragon/", 8) == 0) {
                *out_buf = strdup(in_buf);
                if (!*out_buf) return -ENOMEM;
                rot13(*out_buf, size);
                *out_size = size;
                return 0;
            } else if (strncmp(path, "/heaven/", 8) == 0) {
                unsigned char iv[IV_SIZE];
                if (RAND_bytes(iv, IV_SIZE) != 1) return -EIO;
                *out_buf = (char *)malloc(size + IV_SIZE);
                if (!*out_buf) return -ENOMEM;
                memcpy(*out_buf, iv, IV_SIZE);
                int enc_size = aes_encrypt(in_buf, *out_buf + IV_SIZE, (int)size, iv);
                if (enc_size < 0) {
                    free(*out_buf);
                    *out_buf = NULL;
                    return -EIO;
                }
                *out_size = enc_size + IV_SIZE;
                return 0;
            } else if (strncmp(path, "/youth/", 7) == 0) {
                size_t comp_buf_size = size * 2;
                *out_buf = (char *)malloc(comp_buf_size);
                if (!*out_buf) return -ENOMEM;
                int ret = gzip_compress(in_buf, size, *out_buf, &comp_buf_size);
                if (ret != 0) {
                    free(*out_buf);
                    *out_buf = NULL;
                    return -EIO;
                }
                *out_size = comp_buf_size;
                return 0;
            } else {
                *out_buf = (char *)malloc(size);
                if (!*out_buf) return -ENOMEM;
                memcpy(*out_buf, in_buf, size);
                *out_size = size;
                return 0;
            }
        }

- Mendeklarasikan functionn xmp_write untuk menulis ke file.

        static int xmp_write(const char *path, const char *buf, size_t size,
            off_t offset, struct fuse_file_info *fi) {
        
            char chiho_path[512];
            char sevenref_path[512];
            compute_chiho_path(chiho_path, sizeof(chiho_path), path);
            compute_7sref_path(sevenref_path, sizeof(sevenref_path), path);
        
            char *transformed_buf = NULL;
            size_t transformed_size = 0;
            int ret = area_transform_before_write(path, buf, size, &transformed_buf, &transformed_size);
            if (ret != 0) return ret;
        
            ssize_t written_chiho = write_file_at_path(chiho_path, transformed_buf, transformed_size, offset);
            if (written_chiho < 0) {
                free(transformed_buf);
                return (int)written_chiho;
            }
        
            ssize_t written_7sref = write_file_at_path(sevenref_path, buf, size, offset);
            if (written_7sref < 0) {
                fprintf(stderr, "Warning: failed to write to 7sref path %s: %s\n", sevenref_path, strerror(-written_7sref));
            }
        
            free(transformed_buf);
        
            return written_chiho;
        }

- Mendeklarasikan xmp_read untuk membaca isi file

        static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi) {
            int fd = (fi == NULL) ? -1 : fi->fh;
            if (fd == -1) {
                char fpath[512];
                compute_chiho_path(fpath, sizeof(fpath), path);
                fd = open(fpath, O_RDONLY);
                if (fd == -1) return -errno;
            }
        
            ssize_t read_size = pread(fd, buf, size, offset);
            if (read_size == -1) {
                int err = -errno;
                if (fi == NULL) close(fd);
                return err;
            }
        
            if (strncmp(path, "/metro/", 7) == 0) {
                char *temp_buf = (char *)malloc(read_size);
                if (temp_buf == NULL) {
                    if (fi == NULL) close(fd);
                    return -ENOMEM;
                }
                shift_content(buf, temp_buf, (size_t)read_size, -1);
                memcpy(buf, temp_buf, (size_t)read_size);
                free(temp_buf);
            } else if (strncmp(path, "/dragon/", 8) == 0) {
                rot13(buf, (size_t)read_size);
            } else if (strncmp(path, "/heaven/", 8) == 0) {
                unsigned char iv[IV_SIZE];
                if (pread(fd, iv, IV_SIZE, 0) != IV_SIZE) {
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                ssize_t encrypted_len = read_size - IV_SIZE;
                if (encrypted_len <= 0) {
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                char *encrypted_data = (char *)malloc(encrypted_len);
                if (encrypted_data == NULL) {
                    if (fi == NULL) close(fd);
                    return -ENOMEM;
                }
                if (pread(fd, encrypted_data, encrypted_len, offset + IV_SIZE) != encrypted_len) {
                    free(encrypted_data);
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                int decrypted_size = aes_decrypt(encrypted_data, buf, encrypted_len, iv);
                free(encrypted_data);
                if (decrypted_size < 0) {
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                read_size = decrypted_size;
            } else if (strncmp(path, "/youth/", 7) == 0) {
                char *compressed_data = (char *)malloc(read_size);
                if (compressed_data == NULL) {
                    if (fi == NULL) close(fd);
                    return -ENOMEM;
                }
                if (pread(fd, compressed_data, read_size, offset) != read_size) {
                    free(compressed_data);
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                size_t decompressed_size = size * 2;
                char *decompressed_buf = (char *)malloc(decompressed_size);
                if (decompressed_buf == NULL) {
                    free(compressed_data);
                    if (fi == NULL) close(fd);
                    return -ENOMEM;
                }
                int ret = gzip_decompress(compressed_data, (size_t)read_size, decompressed_buf, &decompressed_size);
                free(compressed_data);
                if (ret != 0) {
                    free(decompressed_buf);
                    if (fi == NULL) close(fd);
                    return -EIO;
                }
                memcpy(buf, decompressed_buf, decompressed_size);
                read_size = (ssize_t)decompressed_size;
                free(decompressed_buf);
            }
        
            if (fi == NULL) close(fd);
            return (int)read_size;
        }

- Mendeklarasikan xmp_create yang digunakan untuk membuat file baru.

        static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
            char chiho_path[512];
            compute_chiho_path(chiho_path, sizeof(chiho_path), path);
            int fd = creat(chiho_path, mode);
            if (fd == -1) return -errno;
            fi->fh = fd;
        
            char sevenref_path[512];
            compute_7sref_path(sevenref_path, sizeof(sevenref_path), path);
            int fd7 = creat(sevenref_path, mode);
            if (fd7 == -1) {
                fprintf(stderr, "Warning: failed to create 7sref file %s: %s\n", sevenref_path, strerror(errno));
            } else {
                close(fd7);
            }
            return 0;
        }

- Mendeklarasikan xmp_unlink yang digunakan untuk menghapus file.

        static int xmp_unlink(const char *path) {
            char chiho_path[512];
            compute_chiho_path(chiho_path, sizeof(chiho_path), path);
            int res = unlink(chiho_path);
            if (res == -1) return -errno;
        
            char sevenref_path[512];
            compute_7sref_path(sevenref_path, sizeof(sevenref_path), path);
            int res7 = unlink(sevenref_path);
            if (res7 == -1 && errno != ENOENT) {
                fprintf(stderr, "Warning: failed to unlink 7sref file %s: %s\n", sevenref_path, strerror(errno));
            }
            return 0;
        }

- Membuat function make_directory untuk membuat directory mount (fuse_dir dan chiho) secara otomatis ketika file dijalankan.

        static int make_directory(const char *path, mode_t mode) {
            if (mkdir(path, mode) != 0) {
                if (errno == EEXIST) {
                    return 0;
                } else {
                    perror(path);
                    return -1;
                }
            }
            return 0;
        }

- Mendeklarasikan struct yang berisi fuse operation.

        static struct fuse_operations xmp_oper = {
            .getattr = xmp_getattr,
            .readdir = xmp_readdir,
            .open = xmp_open,
            .read = xmp_read,
            .write = xmp_write,
            .create = xmp_create,
            .unlink = xmp_unlink,
        };

- Terakhir, membuat int main() sebagai fungsi pertama atau titik awal eksekusi program.

        int main(int argc, char *argv[]) {
            if (make_directory(BASE_DIR, 0755) != 0) {
                fprintf(stderr, "Failed to create %s\n", BASE_DIR);
                exit(1);
            }
            const char *sub_dirs[] = {
                "starter", "metro", "dragon", "blackrose", "heaven", "youth"
            };
            for (int i = 0; i < 6; i++) {
                char path[128];
                snprintf(path, sizeof(path), "%s/%s", BASE_DIR, sub_dirs[i]);
                if (make_directory(path, 0755) != 0) {
                    fprintf(stderr, "Failed to create %s\n", path);
                    exit(1);
                }
            }
        
            if (make_directory("fuse_dir", 0755) != 0) {
                fprintf(stderr, "Failed to create %s\n", "fuse_dir");
                exit(1);
            }
            for (int i = 0; i < (int)(sizeof(areas) / sizeof(areas[0])); i++) {
                char path[128];
                snprintf(path, sizeof(path), "fuse_dir/%s", areas[i].name);
                if (make_directory(path, 0755) != 0) {
                    fprintf(stderr, "Failed to create %s\n", path);
                    exit(1);
                }
            }
            printf("Directories created successfully!\n");
            return fuse_main(argc, argv, &xmp_oper, NULL);
        }
