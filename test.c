#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <assert.h>
#include <fcntl.h>

int COPY_THRESHOLD = 10*100000;

void validate_path(char *path)
{
    struct stat STATS;
    /* if stat() returns a non-zero value an error occured */
    if (stat(path, &STATS) != 0)
    {
        printf("podales zla nazwe katalogu %s\n", path);
        exit(EXIT_FAILURE);
    }

    /* S_ISDIR returns 1 if the file type is a directory */
    if (S_ISDIR(STATS.st_mode))
        printf("dziala %s\n", path);
    else
        printf("nie dziala %s\n", path);
}

void add_slash(char *path) 
{
    if (path[strlen(path) - 1] != '/')
    {
        strcat(path, "/");
    }
}

int is_file(const char* path)
{
    struct stat st;
    
    lstat (path, &st);
    if (S_ISREG (st.st_mode))
        return 1;
    else
        return 0;
}

int file_size(const char* path)
{
    int size;
    struct stat st;

    lstat (path, &st);
    return size = st.st_size;
}

void read_write_copy(char *path, char *dest_path)
{
    int src_fd, dst_fd, n, err;
    unsigned char buffer[4096];
    char * src_path, dst_path;

    src_fd = open(path, O_RDONLY);
    dst_fd = open(dest_path, O_WRONLY | O_CREAT | O_EXCL, 0666);

    while (1) {
        err = read(src_fd, buffer, 4096);
        if (err == -1) {
            printf("Error reading file.\n");
            exit(1);
        }
        n = err;

        if (n == 0) break;

        err = write(dst_fd, buffer, n);
        if (err == -1) {
            printf("Error writing to file.\n");
            exit(1);
        }
    }

    close(src_fd);
    close(dst_fd);
}

void remove_file(const char* file_path)
{
    int rem = remove(file_path);
    if (rem == 0)
    {
        printf("usunieto plik: %s", file_path);
    }
    else
    {
        printf("blad przy usuwaniu plik");
    }
}

void list_directory(const char* SOURCE_PATH, const char* DESTINATION_PATH) {
    
    DIR* dir;
    struct dirent* entry;
    char entry_path[PATH_MAX + 1];
    char dest_path[PATH_MAX + 1];
    size_t path_len, path2_len;

    /* Copy the directory path into entry_path. */
    strncpy (entry_path, SOURCE_PATH, sizeof (entry_path));
    strncpy (dest_path, DESTINATION_PATH, sizeof (dest_path));

    path_len = strlen (SOURCE_PATH);
    path2_len = strlen (DESTINATION_PATH);

    /* Start the listing operation of the directory specified on the command line. */
    dir = opendir (SOURCE_PATH);

    /* Loop over all directory entries. */
    while ((entry = readdir (dir)) != NULL) {

        /* Build the path to the directory entry by appending the entry name to the path name. */
        strncpy (entry_path + path_len, entry->d_name, sizeof (entry_path) - path_len);
        strncpy (dest_path + path2_len, entry->d_name, sizeof (dest_path) - path2_len);

        int file_s;
        /* Determine the type of the entry. */
        if (is_file (entry_path))
        {
            file_s = file_size(entry_path);
            printf("wielkosc pliku %s: %d\n", entry_path, file_s);
            read_write_copy(entry_path, dest_path);
            sleep(5);
            remove_file(dest_path);
                
        }

        
    }

    /* All done. */
    closedir (dir);
}

int main(int argc, char *argv[])
{
    /* if there was no arguments provided */
    if (argc == 3) 
    {
        printf("prog przy ktorym zmienia sie sposob kopiowania plikow to: %d megabajtow\n", COPY_THRESHOLD/100000);
    }
    if (argc == 4) 
    {
        COPY_THRESHOLD = atoi(argv[3]) * 100000;
        printf("prog przy ktorym zmienia sie sposob kopiowania plikow to: %d megabajtow\n", COPY_THRESHOLD/100000);

    }

    /* at first determinate length of the argument, then use strcpy to copy contents into allocated memory */
    char SOURCE_PATH[strlen(argv[1]) + 1];
    strcpy(SOURCE_PATH, argv[1]);

    char DESTINATION_PATH[strlen(argv[2]) + 1];
    strcpy(DESTINATION_PATH, argv[2]);

    /* we check if '/' is at the end of the argument given by the user */
    add_slash(SOURCE_PATH);
    add_slash(DESTINATION_PATH);
    
    /* to test out if daemon works properly you need a full path to the catalogs */
    /* E.g. /home/student/Desktop/SO_PROJEKT/file-monitor/KATALOG1 */
    validate_path(SOURCE_PATH);
    validate_path(DESTINATION_PATH);

    list_directory(SOURCE_PATH, DESTINATION_PATH);

}