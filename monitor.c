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
#include <sys/sendfile.h>

#define BUF_SIZE 4096*1000

/* 10 MB is the default value */
int COPY_THRESHOLD = 10*100000;

/* 5 min sleep time is the default value */
int MIMIR_TIME = 5*60;

static void skeleton_daemon()
{
    pid_t pid;
    
    /* Fork off the parent process */
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
     /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    /* Fork off for the second time*/
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(0);
    
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");
    
    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
    
    /* Open the log file */
    openlog ("file-monitor", LOG_PID, LOG_DAEMON);
}

void validate_path(const char* path)
{
    struct stat STATS;

    /* if stat() returns a non-zero value an error occured */
    if (stat(path, &STATS) != 0)
    {
        syslog(LOG_ERR, "The catalog %s does not exist!", path);
        exit(EXIT_FAILURE);
    }

    /* S_ISDIR returns 1 if the file type is a directory */
    if (S_ISDIR(STATS.st_mode))
    {
        syslog(LOG_NOTICE, "Catalog %s was found.", path);
    }
    else
    {
        syslog(LOG_ERR, "%s is not a catalog!", path);
        exit(EXIT_FAILURE);
    }

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

void list_directory(const char* SOURCE_PATH, const char* DESTINATION_PATH) {
    
    DIR* dir;
    int file_s;
    struct dirent* entry;
    char src_path[PATH_MAX + 1];
    char dest_path[PATH_MAX + 1];
    size_t src_path_len, dest_path_len;

    /* Copy the directory path into src_path and dest_path */
    strncpy (src_path, SOURCE_PATH, sizeof (src_path));
    strncpy (dest_path, DESTINATION_PATH, sizeof (dest_path));

    src_path_len = strlen (SOURCE_PATH);
    dest_path_len = strlen (DESTINATION_PATH);

    /* Start the listing operation of the directory specified on the command line. */
    dir = opendir (SOURCE_PATH);

    /* Loop over all directory entries. */
    while ((entry = readdir (dir)) != NULL) {

        /* Build the path to the directory entry by appending the entry name to the path name. */
        strncpy (src_path + src_path_len, entry->d_name, sizeof (src_path) - src_path_len);

        /* In theory, the path to the target file will be the same, so we can save it right away */
        strncpy (dest_path + dest_path_len, entry->d_name, sizeof (dest_path) - dest_path_len);

        /* Check if entry is a regular file - if so copy it */
        if (is_file (src_path))
        {
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
    }

    /* All done. */
    closedir (dir);
}

int main(int argc, char* argv[])
{
    skeleton_daemon();

    /* ./{daemon name} source_path destination_path */
    if (argc == 3) 
    {
        syslog(LOG_NOTICE, "Using default threshold value: %d MB", COPY_THRESHOLD/100000);
    }

    if (argc == 4) 
    {
        COPY_THRESHOLD = atoi(argv[3]) * 100000;
        syslog(LOG_NOTICE, "Using default threshold value: %d MB", COPY_THRESHOLD/100000);
    }

    if (argc != 3 && argc != 4)
    {
        syslog(LOG_ERR, "Invalid number of arguments <source path> <destination path> opt: <threshold size>");
        exit(EXIT_FAILURE);
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

    /* atm list_directory checks SOURCE_PATH directory and copies all regular files to DESTINATION_PATH*/
    list_directory(SOURCE_PATH, DESTINATION_PATH);
    
    while (1)
    {
        syslog(LOG_NOTICE, "working");
        sleep(5);
        break;
    }
   
    syslog (LOG_NOTICE, "File monitor terminated.");
    closelog();
    
    return EXIT_SUCCESS;
}
