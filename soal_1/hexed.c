#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

// Mengkonversi 2 karakter heksadesimal menjadi 1 byte
unsigned char hex_to_byte(const char *hex) {
    unsigned char byte = 0;
    for (int i = 0; i < 2; i++) {
        byte <<= 4;
        if (hex[i] >= '0' && hex[i] <= '9') {
            byte |= (hex[i] - '0');
        } else if (hex[i] >= 'A' && hex[i] <= 'F') {
            byte |= (hex[i] - 'A' + 10);
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            byte |= (hex[i] - 'a' + 10);
        }
    }
    return byte;
}

// Mengecek apakah direktori ada
int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Membuat direktori jika belum ada
void create_dir_if_not_exists(const char *path) {
    if (!dir_exists(path)) {
        printf("Creating directory: %s\n", path);
        mkdir(path, 0755);
    }
}

// Mengecek apakah string hanya berisi karakter heksadesimal dan whitespace
int is_hex_string(const char *str) {
    while (*str) {
        if (!isxdigit(*str) && !isspace(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

// Membuat struktur direktori yang dibutuhkan
void create_directory_structure() {
    // Buat direktori utama
    create_dir_if_not_exists("anomali");
    create_dir_if_not_exists("anomali/image");
    create_dir_if_not_exists("hexed");
    create_dir_if_not_exists("out");
    create_dir_if_not_exists("out/image");
    
    // Salin diri sendiri ke direktori hexed
    char self_path[1024];
    readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    self_path[sizeof(self_path) - 1] = '\0';
    
    // Cek apakah file ini sudah berada di direktori hexed
    if (strstr(self_path, "/hexed/") == NULL) {
        char command[1024];
        snprintf(command, sizeof(command) - 1, "cp %s hexed/hexed.c", __FILE__);
        command[sizeof(command) - 1] = '\0';
        system(command);
    }
}

// Simpan file gambar dan update log
void save_image_and_log(const char *hex_content, size_t content_length, const char *base_name) {
    // Mendapatkan nama file tanpa ekstensi
    char base_name_without_ext[512];
    strncpy(base_name_without_ext, base_name, sizeof(base_name_without_ext) - 1);
    base_name_without_ext[sizeof(base_name_without_ext) - 1] = '\0';
    char *dot = strchr(base_name_without_ext, '.');
    if (dot) *dot = '\0';  // Hapus ekstensi
    
    // Mendapatkan waktu saat ini untuk nama file
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp) - 1, "%Y-%m-%d_%H:%M:%S", t);
    timestamp[sizeof(timestamp) - 1] = '\0';
    
    // Buat nama file output untuk anomali dan out
    char anomali_output[2048];
    char out_output[2048];
    
    // Pastikan penggabungan string aman (tidak melebihi buffer)
    size_t anomali_prefix_len = strlen("anomali/image/");
    size_t out_prefix_len = strlen("out/image/");
    size_t base_name_len = strlen(base_name_without_ext);
    size_t timestamp_len = strlen(timestamp);
    size_t suffix_len = strlen("_image_.png");
    
    // Periksa apakah panjang total tidak melebihi buffer
    if (anomali_prefix_len + base_name_len + timestamp_len + suffix_len < sizeof(anomali_output) &&
        out_prefix_len + base_name_len + timestamp_len + suffix_len < sizeof(out_output)) {
        
        // Format nama file dengan aman
        snprintf(anomali_output, sizeof(anomali_output) - 1, "anomali/image/%s_image_%s.png", 
                 base_name_without_ext, timestamp);
        anomali_output[sizeof(anomali_output) - 1] = '\0';
        
        snprintf(out_output, sizeof(out_output) - 1, "out/image/%s_image_%s.png", 
                 base_name_without_ext, timestamp);
        out_output[sizeof(out_output) - 1] = '\0';
        
        // Konversi dan tulis ke file di anomali
        FILE *anomali_file = fopen(anomali_output, "wb");
        if (!anomali_file) {
            fprintf(stderr, "Error: Cannot create file %s\n", anomali_output);
            return;
        }
        
        char hex_pair[3] = {0};
        int hex_index = 0;
        
        for (size_t i = 0; i < content_length; i++) {
            if (isxdigit(hex_content[i])) {
                hex_pair[hex_index++] = hex_content[i];
                
                if (hex_index == 2) {
                    unsigned char byte = hex_to_byte(hex_pair);
                    fputc(byte, anomali_file);
                    hex_index = 0;
                }
            }
        }
        fclose(anomali_file);
        
        // Salin file ke direktori out/image
        FILE *in_file = fopen(anomali_output, "rb");
        FILE *out_file = fopen(out_output, "wb");
        
        if (in_file && out_file) {
            int ch;
            while ((ch = fgetc(in_file)) != EOF) {
                fputc(ch, out_file);
            }
        }
        
        if (in_file) fclose(in_file);
        if (out_file) fclose(out_file);
        
        // Log ke anomali/conversion.log
        FILE *anomali_log = fopen("anomali/conversion.log", "a");
        if (anomali_log) {
            fprintf(anomali_log, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s_image_%s.png.\n", 
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec,
                    base_name, base_name_without_ext, timestamp);
            fclose(anomali_log);
        }
        
        // Log ke out/conversion.log
        FILE *out_log = fopen("out/conversion.log", "a");
        if (out_log) {
            fprintf(out_log, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s_image_%s.png.\n", 
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec,
                    base_name, base_name_without_ext, timestamp);
            fclose(out_log);
        }
        
        printf("Successfully converted %s to %s and %s\n", base_name, anomali_output, out_output);
    } else {
        fprintf(stderr, "Error: Filename too long\n");
    }
}

// Download, ekstrak dan proses file
void process_files(const char *url) {
    char command[1024];
    
    // Download file zip
    snprintf(command, sizeof(command) - 1, "wget -q \"%s\" -O temp.zip", url);
    command[sizeof(command) - 1] = '\0';
    system(command);
    
    // Buat direktori temporer untuk ekstraksi
    system("mkdir -p temp_extract");
    
    // Ekstrak file ke direktori temporer
    system("unzip -q temp.zip -d temp_extract");
    
    // Cari file .txt di direktori temporer (mungkin dalam subdir)
    FILE *fp;
    char findcmd[1024];
    char filepath[1024];
    snprintf(findcmd, sizeof(findcmd) - 1, "find temp_extract -name \"*.txt\" -type f");
    findcmd[sizeof(findcmd) - 1] = '\0';
    
    fp = popen(findcmd, "r");
    if (fp == NULL) {
        printf("Failed to run find command\n");
        return;
    }
    
    // Proses setiap file txt yang ditemukan
    while (fgets(filepath, sizeof(filepath) - 1, fp) != NULL) {
        filepath[sizeof(filepath) - 1] = '\0';
        // Hapus newline di akhir path
        filepath[strcspn(filepath, "\n")] = 0;
        
        // Ekstrak nama file
        char *filename = strrchr(filepath, '/');
        if (filename) {
            filename++; // Skip '/'
        } else {
            filename = filepath;
        }
        
        // Baca konten file
        FILE *txt_file = fopen(filepath, "r");
        if (!txt_file) {
            fprintf(stderr, "Error opening file %s\n", filepath);
            continue;
        }
        
        // Baca konten file
        fseek(txt_file, 0, SEEK_END);
        long file_size = ftell(txt_file);
        fseek(txt_file, 0, SEEK_SET);
        
        char *content = (char *)malloc(file_size + 1);
        if (!content) {
            fprintf(stderr, "Memory allocation failed for %s\n", filepath);
            fclose(txt_file);
            continue;
        }
        
        size_t bytes_read = fread(content, 1, file_size, txt_file);
        content[bytes_read] = '\0';
        fclose(txt_file);
        
        // Salin file ke anomali dan out dengan pengecekan panjang nama file
        if (strlen(filename) < 1000) {  // Batasi panjang nama file
            char target_anomali[2048];
            char target_out[2048];
            
            // Lakukan substring secara manual untuk menghindari warning truncation
            strcpy(target_anomali, "anomali/");
            strcat(target_anomali, filename);
            
            strcpy(target_out, "out/");
            strcat(target_out, filename);
            
            FILE *out_anomali = fopen(target_anomali, "w");
            FILE *out_out = fopen(target_out, "w");
            
            if (out_anomali) {
                fwrite(content, 1, bytes_read, out_anomali);
                fclose(out_anomali);
            }
            
            if (out_out) {
                fwrite(content, 1, bytes_read, out_out);
                fclose(out_out);
            }
            
            // Konversi file
            if (is_hex_string(content)) {
                save_image_and_log(content, bytes_read, filename);
            } else {
                fprintf(stderr, "File %s is not a valid hexadecimal file\n", filename);
            }
        } else {
            fprintf(stderr, "Filename %s is too long\n", filename);
        }
        
        free(content);
    }
    
    pclose(fp);
    
    // Bersihkan file dan direktori sementara
    system("rm -rf temp_extract");
    system("rm -f temp.zip");
}

int main(int argc, char *argv[]) {
    // Buat struktur direktori yang diperlukan
    create_directory_structure();
    
    const char *url = "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download";
    
    if (argc > 1) {
        url = argv[1];
    }
    
    process_files(url);
    
    return 0;
}
