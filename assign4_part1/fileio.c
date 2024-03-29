#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "restart.h"
#include "fileio.h"
#include <string.h>

#if 1
#define VERBOSE(p) (p)
#else
#define VERBOSE(p) (0)
#endif

int DEBUG = 0;

int file_read(char *path, int offset, void *buffer, size_t bufbytes)
{
    if(path == NULL || buffer == NULL || bufbytes <= 0 || offset < 0)
      return IOERR_INVALID_ARGS;
    FILE* file_ptr;
    file_ptr = fopen(path, "r");
    if(!file_ptr){
      return IOERR_INVALID_PATH;
    }
    fseek(file_ptr, offset, SEEK_SET);
    size_t read = fread(buffer, 1, bufbytes, file_ptr);
    fclose(file_ptr);
    return read;
}

int file_info(char *path, void *buffer, size_t bufbytes)
{
    if (!path || !buffer || bufbytes < 1)
	return IOERR_INVALID_ARGS;
    FILE* file_ptr;
    file_ptr = fopen(path, "a");
    if(!file_ptr)
      return IOERR_INVALID_PATH;
    struct stat st;
    stat(path, &st);
    long long size = st.st_size;
    time_t accessed = st.st_atime;
    time_t modified = st.st_mtime;
    char *type = (S_ISDIR(st.st_mode)) ? "d" : "f";
    sprintf((char*)buffer, "Size:%d Accessed:%d Modified:%d Type %s", (int)size, (int)accessed, (int)modified, type);
    // debugging :  printf("Size:%d Accessed:%d Modified:%d Type:%s", (int)size, (int)accessed, (int)modified, type);
    fclose(file_ptr);
    return 0;
}

int file_write(char *path, int offset, void *buffer, size_t bufbytes)
{
    if(path == NULL || offset < 0 || buffer == NULL || bufbytes <=0)
      return IOERR_INVALID_ARGS;
    FILE* file_ptr;
    file_ptr = fopen(path, "w");
    if(!file_ptr)
      return IOERR_INVALID_PATH;
    fseek(file_ptr, offset, SEEK_SET);
    int wrote = fwrite(buffer, 1, bufbytes, file_ptr);
    fclose(file_ptr);
    if(DEBUG)
      printf("wrote: %d\n", wrote);
    return wrote;
}

int file_create(char *path, char *pattern, int repeatcount)
{
    if(path == NULL || pattern == NULL || repeatcount < 0)
      return IOERR_INVALID_ARGS;
    FILE* file_ptr;
    file_ptr = fopen(path, "a");
    if(!file_ptr)
      return IOERR_INVALID_PATH;
    for(int i=0;i<repeatcount;i++){
      fputs(pattern, file_ptr);
    }
    fclose(file_ptr);
    return 0;
}

int file_remove(char *path)
{
    if(path == NULL)
      return IOERR_INVALID_ARGS;
    int res = remove(path);
    if(res){
      return IOERR_INVALID_PATH;
    }
    return res;
}

int dir_create(char *path)
{
    errno = 0;
    if(path == NULL)
      return IOERR_INVALID_ARGS;
    int res = mkdir(path,0777);
    if(errno == EEXIST)
      return IOERR_INVALID_PATH;
    if(res){
      return IOERR_POSIX;
    }
    return res;
}

int dir_list(char *path, void *buffer, size_t bufbytes)
{
    if(path == NULL || buffer == NULL || bufbytes <= 0)
      return IOERR_INVALID_ARGS;
    DIR* directory = opendir(path);
    if(!directory){
      return IOERR_INVALID_PATH;
    }
    long int bytesleft = bufbytes;
    long int offset = 0;
    struct dirent* entry;
    entry = readdir(directory);
    while(entry){
      bytesleft -= sizeof(entry->d_name);
      if(bytesleft >= 0){
        offset += sprintf((char*)(buffer+offset), "%s\n", entry->d_name);
        entry = readdir(directory);
      } else{
        closedir(directory);
        return IOERR_BUFFER_TOO_SMALL;
      }
    }
    closedir(directory);
    return 0;
}


int file_checksum(char *path)
{
    if(path == NULL)
      return IOERR_INVALID_ARGS;
    char buf[10000];
    long int len = file_read(path,0,buf,10000);
    if(len <= 0)
      return len;
    unsigned short chck = checksum(buf, len, 0);
    return chck;
}

int dir_checksum(char *path)
{
    int chck = 0;
    if(path == NULL)
      return IOERR_INVALID_ARGS;
    DIR* mainDir;
    mainDir = opendir(path);
    if(mainDir == NULL)
      return IOERR_INVALID_PATH;
    char currPath[1000], nextPath[1000];
    strcpy(currPath, path);
    struct dirent* entry;
    entry = readdir(mainDir);
    while(entry){
      if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
        entry = readdir(mainDir);
        continue;
      }
      struct stat st;
      sprintf(nextPath, "%s/%s", currPath, entry->d_name);
      stat(nextPath, &st);
      if(S_ISDIR(st.st_mode)){
        chck = checksum(entry->d_name, strlen(entry->d_name), chck);
        chck += dir_checksum(nextPath);
      } else {
        chck += file_checksum(nextPath);
      }
      entry = readdir(mainDir);
    }
    closedir(mainDir);
    return chck;
}
