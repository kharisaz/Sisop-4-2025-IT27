# Sisop-4-2025-IT27

===================[Kelompok IT27]======================

Khumaidi Kharis Az-zacky - 5027241049

Mohamad Arkan Zahir Asyafiq - 5027241120

Abiyyu Raihan Putra Wikanto - 5027241042

============[Laporan Resmi Penjelasan Soal]=============

## soal_1


## soal_2


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

