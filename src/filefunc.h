#ifndef FILEFUNC_H
#define FILEFUNC_H

int is_file(const char* path);

int is_dir(const char* path);

void read_write_copy(const char *src_path, const char *dest_path);

void sendfile_copy(const char *src_path, const char *dest_path);

int file_size(const char* path);

void remove_file(const char* file_path);

int checksumck(char *file1, char *file2);

int is_dir_empty(const char* dirpath);

int file_exists(char* filepath);

void copy_file(char* src_path, char* dest_path);

#endif
