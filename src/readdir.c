#include "common.h"
#include "readdir.h"
#include "filefunc.h"

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

/* we go through path_comparing and we compare it to files in path_to_compare dir*/
void read_dir(const char* path_comparing, const char* path_to_compare, int STATUS) {

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
            /* Check if entry is a directory if it is create it and then go into read_dir again*/
            else if (is_dir(src_path)){
                mkdir(dest_path, 0700);
                syslog(LOG_NOTICE, "%s, %s", src_path,dest_path);
                add_slash(src_path);
                add_slash(dest_path);
                read_dir(src_path,dest_path,FIRST_COPY_REC);
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
                    /* Call on read_dir again*/
                    read_dir(src_path,dest_path,REGULAR_CHECK_REC);
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
                /*If not then call on read_dir function*/
                else{
                read_dir(src_path,dest_path,DEST_CHECK_REC);
                }
            }
            else{
                read_dir(src_path,dest_path,DEST_CHECK_REC);
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