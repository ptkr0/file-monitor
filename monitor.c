#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
   
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

    /* to test out if daemon works properly you need a full path to the catalogs */
    /* E.g. /home/student/Desktop/SO_PROJEKT/file-monitor/KATALOG1 */
    validate_path(SOURCE_PATH);
    validate_path(DESTINATION_PATH);
    
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