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
#include <openssl/sha.h>
#include <stdbool.h>

#define BUF_SIZE 4096*1000

#define FIRST_COPY 1
#define REGULAR_CHECK 2
#define DEST_CHECK 3

#define USER_TRIGGER SIGUSR1

bool FIRST_CHECK = false;

volatile int user_input_check = 0;

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

void signal_handler(int signal)
{
    if (signal == USER_TRIGGER)
    {
        syslog(LOG_INFO, "File check requested by user.");
        user_input_check = 1;
    }
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

int file_exists(const char* filepath) {

    if (access(filepath, F_OK) != -1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* we go through path_comparing and we compare it to files in path_to_compare dir*/
void list_directory(const char* path_comparing, const char* path_to_compare, int STATUS) {
    
    DIR* dir;
    struct dirent* entry;
    char src_path[PATH_MAX + 1];
    char dest_path[PATH_MAX + 1];
    size_t src_path_len, dest_path_len;

    /* Copy the directory paths into src_path and dest_path */
    strncpy (src_path, path_comparing, sizeof (src_path));
    strncpy (dest_path, path_to_compare, sizeof (dest_path));

    src_path_len = strlen (path_comparing);
    dest_path_len = strlen (path_to_compare);

    /* Start the listing operation of the directory specified on the command line. */
    dir = opendir (path_comparing);

    /* Loop over all directory entries. */
    while ((entry = readdir (dir)) != NULL) {

        /* Build the path to the directory entry by appending the entry name to the path name. */
        strncpy (src_path + src_path_len, entry->d_name, sizeof (src_path) - src_path_len);

        /* In theory, the path to the target file will be the same, so we can save it right away */
        strncpy (dest_path + dest_path_len, entry->d_name, sizeof (dest_path) - dest_path_len);

        /* depending on the status flag this function will work differently*/
        if (STATUS == FIRST_COPY)
        {
            /* Check if entry is a regular file - if so copy it */
            if (is_file (src_path))
            {
                copy_file(src_path, dest_path);
            }
        }

        else if (STATUS == REGULAR_CHECK)
        {
            if (is_file(src_path))
            {
                if (file_exists(dest_path))
                {
                    if (checksumck(src_path, dest_path) ==1 )
                    {
                        /* file is a regular file and has a copy in dest folder and checksums are the same -- file wasn't modified so we don't have to do anything*/
                    }
                    else
                    {
                        /* file is a regular file and has a copy in dest folder, but checksums aren't the same -- file was modified so we delete the old file and copy the one */
                        remove_file(dest_path);
                        copy_file(src_path, dest_path);
                        syslog(LOG_NOTICE, "File %s was modified between checks.", src_path);
                    }
                }
                else
                {
                    /* file is a regular file, but doesn't have a copy in dest folder -- we copy it */
                    copy_file(src_path, dest_path);
                }
            }
        }

        else if (STATUS == DEST_CHECK)
        {
            /* we go through dest dir and look if the same filenames are in the src dir -- if something is not in the src dir anymore we delete it from dest dir*/
            if (file_exists(src_path) && !(file_exists(dest_path)))
            {
                remove_file(src_path);
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

    signal(USER_TRIGGER, signal_handler);

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

    if (!(is_dir_empty(DESTINATION_PATH)))
    {
      syslog (LOG_ERR, "Error: Destination directory has to be empty for daemon to work properly");
      exit(EXIT_FAILURE);  
    }

    while (1)
    {
        int i = 0;
        while ( i < 1 )
        {
            if (FIRST_CHECK == false)
            {
                list_directory(SOURCE_PATH, DESTINATION_PATH, FIRST_COPY);
                syslog(LOG_NOTICE, "Daemon has successfully created first directory copy!");
                FIRST_CHECK = true;
            }
            else if (FIRST_CHECK == true)
            {
                validate_path(SOURCE_PATH);
                validate_path(DESTINATION_PATH);
                list_directory(SOURCE_PATH, DESTINATION_PATH, REGULAR_CHECK);
                syslog(LOG_NOTICE, "Regular check complete!");
                list_directory(DESTINATION_PATH, SOURCE_PATH, DEST_CHECK);
                syslog(LOG_NOTICE, "Destination check complete!");
            }
            sleep(MIMIR_TIME);
            
            /* we check if user send a message, if he does then user_input_check value changes to 1, when that happens we restart the loop */
            if (user_input_check == 1)
            {
                i = 0;
                continue;
            }
        }  
    }
   
    syslog (LOG_NOTICE, "File monitor terminated.");
    closelog();
    
    return EXIT_SUCCESS;
}
