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

void validate_path(char* path)
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

int check_if_is_a_file (const char* path)
{
    struct stat st;
    
    lstat (path, &st);
    if (S_ISREG (st.st_mode))
        return 1;
    else
        return 0;
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
            syslog(LOG_ERR, "Error when writing to file %s.", path);
            exit(EXIT_FAILURE);
        }
        n = err;

        if (n == 0) break;

        err = write(dst_fd, buffer, n);
        if (err == -1) {
            syslog(LOG_ERR, "Error when writing to file %s.", path);
            exit(EXIT_FAILURE);
        }
    }

    syslog(LOG_NOTICE, "Successfully copied file %s", path);
    close(src_fd);
    close(dst_fd);
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

        /* Check if entry is a regular file - if so copy it */
        if (check_if_is_a_file (entry_path))
            read_write_copy(entry_path, dest_path);
    }

    /* All done. */
    closedir (dir);
}

int main(int argc, char* argv[])
{
    skeleton_daemon();

    /* ./{daemon name} source_path destination_path */
    if (argc != 3) 
    {
        syslog(LOG_ERR, "You provided an incorrect number of arguments!");
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