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
#include <fcntl.h>
#include <sys/sendfile.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <getopt.h>

#define BUF_SIZE 4096*1000

#define FIRST_COPY 1
#define REGULAR_CHECK 2
#define DEST_CHECK 3
#define FIRST_COPY_REC 4
#define REGULAR_CHECK_REC 5
#define DEST_CHECK_REC 6

#define USER_TRIGGER SIGUSR1

bool FIRST_CHECK = false;
bool FIRST_CHECK_REC = false;

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

int is_dir(const char* path)
{
    struct stat st;

    lstat (path, &st);
    if (S_ISDIR (st.st_mode))
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

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
        }
        /* depending on the status flag this function will work differently*/
        if (STATUS == FIRST_COPY)
        {
            /* Check if entry is a regular file - if so copy it */
            if (is_file (src_path))
            {
                copy_file(src_path, dest_path);
            }
        }

        else if (STATUS == FIRST_COPY_REC)
        {
            /* Check if entry is a regular file - if so copy it */
            if (is_file (src_path))
            {
                copy_file(src_path, dest_path);
            }
            /* Check if entry is a directory if it is create it and then go into list_directory again*/
            else if (is_dir(src_path)){
                mkdir(dest_path, 0700);
                syslog(LOG_NOTICE, "%s, %s", src_path,dest_path);
                add_slash(src_path);
                add_slash(dest_path);
                list_directory(src_path,dest_path,FIRST_COPY_REC);
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

        else if (STATUS == REGULAR_CHECK_REC)
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
            /* Check if file is directory*/
            if (is_dir(src_path))
            {
                if (file_exists(dest_path))
                {
                    /* Adjust file paths*/
                    add_slash(src_path);
                    add_slash(dest_path);
                    /* Call on list_directory again*/
                    list_directory(src_path,dest_path,REGULAR_CHECK_REC);
                }
                else
                {
                    /* If directory does not exist we create it */
                    mkdir(dest_path, 0700);
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
        else if (STATUS == DEST_CHECK_REC)
        {
        /* Check if directory*/
        if(is_dir(src_path)){
            /* Check if the directory exists exists */
            if (file_exists(src_path) && !(file_exists(dest_path))){
                /* If its empty remove it*/
                if(is_dir_empty(src_path)){
                rmdir(src_path);
                }
                /*If not then call on list_directory function*/
                else{
                list_directory(src_path,dest_path,DEST_CHECK_REC);
                }
            }
            else{
                list_directory(src_path,dest_path,DEST_CHECK_REC);
            }

        }
            /* If not a directory and is missing in source delete it in dest*/
        else{
            if (file_exists(src_path) && !(file_exists(dest_path)))
            {
                    remove_file(src_path);
            }
        }
        }

    }

    /* All done. */
    closedir (dir);
}

int * THR_TM(char* option){
    char * token,s;
    int * wynik;
    int ck;
    /*Check if user inputed only ':'*/
    if(strlen(option)==1&&option[0]==':'){
        ck=0;
    }
    else{
        /* Check for the position of the separator and adjust ck value accordingly*/
        for(int i=0;i<strlen(option);i++)
        {
            
            if(option[i]==':')
            {
                if(i==0)
                {
                    ck=1;
                    break;
                }
                else if(i==strlen(option)-1)
                {
                    ck=2;
                    break;
                }
                else
                {
                    ck=3;
                    break;
                }
            }
            
        }
    }
    /* Create an array in which the first value is the position of the seprator*/
    if (ck==1 || ck==2){
        token=strtok(option,":");
        wynik=malloc(sizeof(int)*2);
        wynik[0]=ck;
        wynik[1]=atoi(token);
    }
    else if (ck==3) {
        token=strtok(option,":");
        wynik=malloc(sizeof(int)*3);
        wynik[0]=ck;
        wynik[1]=atoi(token);
        token=strtok(NULL,":");
        wynik[2]=atoi(token);
    }
    /*In case no option or only a separator*/
    else {
        wynik=malloc(sizeof(int)*1);
        wynik[0]=0;
    }

    return wynik;

}



int main(int argc, char* argv[])
{
    skeleton_daemon();
    /*defining a variable used in handling options*/
    int option=getopt(argc,argv,"r");
    int i = 0;
    if(option=='r')
        i=4;
    else
        i=3;    
    /* Check for user inputted values for threshhold and sleep timer*/
    if (argc == i){
        syslog(LOG_NOTICE, "Using default threshold value: %d MB", COPY_THRESHOLD/100000);
        syslog(LOG_NOTICE, "Using default sleep timer: %d s", MIMIR_TIME);
    }
    else if(argc == i+1)
    {
        /*Call on the option string handler*/
        int* tmp_val=THR_TM(argv[i]);
        if(tmp_val[0]==0){
            syslog(LOG_ERR, "Wrong formatting opt: <threshold size>:<sleep timer>! Using default values");
            syslog(LOG_NOTICE, "Using default threshold value: %d MB", COPY_THRESHOLD/100000);
            syslog(LOG_NOTICE, "Using default sleep timer: %d s", MIMIR_TIME);
        }
        else if(tmp_val[0]==1)
        {
            MIMIR_TIME=tmp_val[1];
            syslog(LOG_NOTICE, "Using default threshold value: %d MB", COPY_THRESHOLD/100000);
            syslog(LOG_NOTICE, "Using custom sleep timer: %d s", MIMIR_TIME);
        }
        else if(tmp_val[0]==2)
        {
            COPY_THRESHOLD=tmp_val[1]*100000;
            syslog(LOG_NOTICE, "Using custom threshold value: %d MB", COPY_THRESHOLD/100000);
            syslog(LOG_NOTICE, "Using default sleep timer: %d s", MIMIR_TIME);
        }
        else if(tmp_val[0]==3)
        {
            COPY_THRESHOLD=tmp_val[1]*100000;
            MIMIR_TIME=tmp_val[2];
            syslog(LOG_NOTICE, "Using custom threshold value: %d MB", COPY_THRESHOLD/100000);
            syslog(LOG_NOTICE, "Using custom sleep timer: %d s", MIMIR_TIME);
        }
        else
        { 
        syslog(LOG_ERR, "Invalid number of arguments or wrong formatting <source path> <destination path> opt: <threshold size>:<sleep timer>");
        exit(EXIT_FAILURE);
        }
        free(tmp_val);
    }
    else if (argc != i && argc != i+1)
    {
        syslog(LOG_ERR, "Invalid number of arguments <source path> <destination path> opt: <threshold size>:<sleep timer>");
        exit(EXIT_FAILURE);
    }
    

    signal(USER_TRIGGER, signal_handler);

    /* at first determinate length of the argument, then use strcpy to copy contents into allocated memory */
    char SOURCE_PATH[strlen(argv[optind]) + 1];
    strcpy(SOURCE_PATH, argv[optind]);

    char DESTINATION_PATH[strlen(argv[optind+1]) + 1];
    strcpy(DESTINATION_PATH, argv[optind+1]);

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
    if(option=='r'){
           while (1)
            {
                user_input_check = 0;
                if (FIRST_CHECK_REC == false)
                {
                    list_directory(SOURCE_PATH, DESTINATION_PATH, FIRST_COPY_REC);
                    syslog(LOG_NOTICE, "Daemon has successfully created first directory copy_rec!");
                    FIRST_CHECK_REC = true;
                }
                else if (FIRST_CHECK_REC == true)
                {
                    validate_path(SOURCE_PATH);
                    validate_path(DESTINATION_PATH);
                    list_directory(SOURCE_PATH, DESTINATION_PATH, REGULAR_CHECK_REC);
                    syslog(LOG_NOTICE, "Regular check complete!_rec");
                    list_directory(DESTINATION_PATH, SOURCE_PATH, DEST_CHECK_REC);
                    syslog(LOG_NOTICE, "Destination check complete!_rec");
                }
                sleep(MIMIR_TIME);

                /* we check if user send a message, if he does then user_input_check value changes to 1, when that happens we restart the loop */
                if (user_input_check == 1)
                {
                    continue;
                }
            }
    }
    else{
        while (1)
        {
            user_input_check = 0;
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
                continue;
            }
        }
    }

    syslog (LOG_NOTICE, "File monitor terminated.");
    closelog();

    return EXIT_SUCCESS;
}