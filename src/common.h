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