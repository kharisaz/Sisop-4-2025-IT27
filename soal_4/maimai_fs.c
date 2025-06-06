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

void rot13(char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if ((data[i] >= 'a' && data[i] <= 'z'))
            data[i] = 'a' + (data[i] - 'a' + 13) % 26;
        else if ((data[i] >= 'A' && data[i] <= 'Z'))
            data[i] = 'A' + (data[i] - 'A' + 13) % 26;
    }
}

void shift_content(const char *in_buf, char *out_buf, size_t size, int direction) {
    for (size_t i = 0; i < size; i++) {
        unsigned char val = (unsigned char)in_buf[i];
        val = (unsigned char)(val + (direction * ((i + 1) % 256)));
        out_buf[i] = (char)val;
    }
}

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
        EVP_CIPHER_CTX_free(ctx);
        return -4;
    }

    EVP_CIPHER_CTX_free(ctx);
    return out_len1 + out_len2;
}

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

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[512];
    compute_chiho_path(fpath, sizeof(fpath), path);
    int fd = open(fpath, fi->flags);
    if (fd == -1) return -errno;
    fi->fh = fd;
    return 0;
}

static ssize_t write_file_at_path(const char *fullpath, const char *buf, size_t size, off_t offset) {
    int fd = open(fullpath, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) return -errno;

    ssize_t written = pwrite(fd, buf, size, offset);
    close(fd);
    if (written == -1) return -errno;
    return written;
}

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

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
};

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
