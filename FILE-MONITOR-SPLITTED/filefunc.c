#include "common.h"
#include "filefunc.h"
#include "readdir.h"

extern int COPY_THRESHOLD;

int is_file(const char* path)
{
    struct stat st;

    lstat (path, &st);
    if (S_ISREG (st.st_mode))
        return 1;
    else
        return 0;
}

int is_dir(const char* path)
{
    struct stat st;

    lstat (path, &st);
    if (S_ISDIR (st.st_mode))
        return 1;
    else
        return 0;
}

/* this function copies file from source dir to dest dir*/
void copy_file(char* src_path, char* dest_path)
{
    int file_s;
    file_s = file_size(src_path);
    if(file_s > COPY_THRESHOLD)
    {
        sendfile_copy (src_path, dest_path);
    }
    else
    {
        read_write_copy (src_path, dest_path);
    }
}

void read_write_copy(const char *src_path, const char *dest_path)
{
    int src_file, dest_file, n, err;
    unsigned char buffer[BUF_SIZE];
    struct stat stat_buf;

    src_file = open(src_path, O_RDONLY);

    /* fstat helps to find file permissions */
    fstat(src_file, &stat_buf);
    dest_file = open(dest_path, O_WRONLY | O_CREAT, stat_buf.st_mode);

    while (1) {
        err = read(src_file, buffer, sizeof(buffer));

        if (err == -1) {
            syslog(LOG_ERR, "Error: unable to copy file %s.", src_path);
            break;
        }
        n = err;

        if (n == 0) break;

        err = write(dest_file, buffer, n);
        if (err == -1) {
            syslog(LOG_ERR, "Error: unable to copy file %s.", src_path);
            break;
        }
    }

    syslog(LOG_NOTICE, "Successfully copied file %s", src_path);
    close(src_file);
    close(dest_file);
}

void sendfile_copy(const char *src_path, const char *dest_path)
{
    int src_file, dest_file, n;
    unsigned char buffer[BUF_SIZE];
    struct stat stat_buf;

    src_file = open(src_path, O_RDONLY);

    /* fstat helps to find file permissions */
    fstat(src_file, &stat_buf);
    dest_file = open(dest_path, O_WRONLY | O_CREAT, stat_buf.st_mode);

    n = 1;
    while (n > 0)
    {
        if (n == -1) {
            syslog(LOG_ERR, "Error when copying file %s.", src_path);
            break;
        }
        n = sendfile(dest_file, src_file, 0, BUF_SIZE);
    }

    close(src_file);
    close(dest_file);
}

/* this function returns size of a file*/
int file_size(const char* path)
{
    int size;
    struct stat st;

    lstat (path, &st);
    return size = st.st_size;
}

void remove_file(const char* file_path)
{
    int rem = remove(file_path);
    if (rem == 0)
    {
        syslog(LOG_NOTICE, "Successfully delete file %s", file_path);
    }
    else
    {
        syslog(LOG_ERR, "Error: unable to delete file %s", file_path);
    }
}

int checksumck(char *file1, char *file2) {
    FILE *src;
    /*Definition of SHA256_CTX struct*/
    SHA256_CTX c1,c2;
    char buf[SHA256_DIGEST_LENGTH];
    ssize_t size, out_writer;
    unsigned char out[2][SHA256_DIGEST_LENGTH];

    /*Initialization of SHA256_CTX struct*/
    SHA256_Init(&c1);
    SHA256_Init(&c2);

    /*File open*/
    src = fopen(file1, "r");

    /*Size for Update function */
    while((size = fread(buf, 1, SHA256_DIGEST_LENGTH, src)) != 0) {
    /*Hash update*/
        SHA256_Update(&c1, buf, size);
    }
    /*Returns string and deletes SHA256_CTX*/
    SHA256_Final(out[0], &c1);

    src = fopen(file2, "r");

    while((size = fread(buf, 1, SHA256_DIGEST_LENGTH, src)) != 0) {
        SHA256_Update(&c2, buf, size);
    }
    SHA256_Final(out[1], &c2);
    fclose(src);
    /*Comparing 2 hashes*/
    if(memcmp(out[0],out[1],SHA256_DIGEST_LENGTH)==0){
        syslog(LOG_ERR, "Checksums match!");
        return 1;
    }
    else{
        syslog(LOG_ERR, "Checksums not the same! %s.", file2);
        return 0;
    }
}

int is_dir_empty(const char* dirpath) {

    DIR *dir;
    dir = opendir(dirpath);
    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (++count > 2) {
            break;
        }
    }

    closedir(dir);
    return (count <= 2);
}

int file_exists(char* filepath) {

    if(is_dir(filepath)){
        add_slash(filepath);
    }
    if (access(filepath, F_OK) != -1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

