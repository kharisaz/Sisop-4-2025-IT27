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

static char *ROOT;               // host directory path
static const char *LOG = "/var/log/it24.log";
const char *keys[] = {"nafis", "kimcun"};

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

int has_key(const char *n, char buf[]) {
    for (int i = 0; i < 2; i++) {
        if (strcasestr(n, keys[i])) {
            strcpy(buf, keys[i]);
            return 1;
        }
    }
    return 0;
}

void rev_str(char *s) {
    int i = 0, j = strlen(s) - 1;
    while (i < j) {
        char c = s[i]; s[i] = s[j]; s[j] = c;
        i++; j--;
    }
}

void rot13(char *s) {
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z') *s = ((*s - 'a' + 13) % 26) + 'a';
        else if (*s >= 'A' && *s <= 'Z') *s = ((*s - 'A' + 13) % 26) + 'A';
    }
}

static int x_getattr(const char *p, struct stat *st) {
    char f[PATH_MAX];
    snprintf(f, PATH_MAX, "%s%s", ROOT, p);
    if (lstat(f, st) < 0) return -errno;
    return 0;
}

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

static int x_open(const char *p, struct fuse_file_info *i) {
    char f[PATH_MAX];
    snprintf(f, PATH_MAX, "%s%s", ROOT, p);
    int fd = open(f, i->flags);
    if (fd < 0) return -errno;
    close(fd);
    return 0;
}

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

static struct fuse_operations ops = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open    = x_open,
    .read    = x_read,
};

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
