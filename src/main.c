#include "common.h"
#include "skeleton.h"
#include "readdir.h"
#include "filefunc.h"

/* 5 min sleep time is the default value */
int MIMIR_TIME = 5*60;

int COPY_THRESHOLD = 10*100000;

volatile bool FIRST_CHECK = false;
volatile bool FIRST_CHECK_REC = false;

volatile int user_input_check = 0;

void signal_handler(int signal)
{
    if (signal == USER_TRIGGER)
    {
        syslog(LOG_INFO, "File check requested by user.");
        user_input_check = 1;
    }
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
                    read_dir(SOURCE_PATH, DESTINATION_PATH, FIRST_COPY_REC);
                    syslog(LOG_NOTICE, "Daemon has successfully created first directory copy_rec!");
                    FIRST_CHECK_REC = true;
                }
                else if (FIRST_CHECK_REC == true)
                {
                    validate_path(SOURCE_PATH);
                    validate_path(DESTINATION_PATH);
                    read_dir(SOURCE_PATH, DESTINATION_PATH, REGULAR_CHECK_REC);
                    syslog(LOG_NOTICE, "Regular check complete!_rec");
                    read_dir(DESTINATION_PATH, SOURCE_PATH, DEST_CHECK_REC);
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
                read_dir(SOURCE_PATH, DESTINATION_PATH, FIRST_COPY);
                syslog(LOG_NOTICE, "Daemon has successfully created first directory copy!");
                FIRST_CHECK = true;
            }
            else if (FIRST_CHECK == true)
            {
                validate_path(SOURCE_PATH);
                validate_path(DESTINATION_PATH);
                read_dir(SOURCE_PATH, DESTINATION_PATH, REGULAR_CHECK);
                syslog(LOG_NOTICE, "Regular check complete!");
                read_dir(DESTINATION_PATH, SOURCE_PATH, DEST_CHECK);
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