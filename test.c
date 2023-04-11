#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

void validate_paths(char *path)
{
    struct stat STATS;
    /* if stat() returns a non-zero value an error occured */
    if (stat(path, &STATS) != 0)
    {
        printf("zla sciezka\n");
        exit(EXIT_FAILURE);
    }

    /* S_ISDIR returns 1 if the file type is a directory */
    if (S_ISDIR(STATS.st_mode))
        printf("dziala\n");
    else
        printf("nie dziala\n");
}

int main(int argc, char* argv[])
{
    /* if there was no arguments provided */
    if ((argc < 2) && (argc > 3)) 
    {
        exit(EXIT_FAILURE);
    }
    validate_paths(argv[1]);
    validate_paths(argv[2]);
}