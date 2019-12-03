#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "restart.h"
#include "fileio.h"

#if 1
#define VERBOSE(p) (p)
#else
#define VERBOSE(p) (0)
#endif

int file_read(char *path, int offset, void *buffer, size_t bufbytes)
{
    if(path && buffer && bufbytes)
      return IOERR_INVALID_ARGS;
    FILE* file_ptr;
    file_ptr = fopen(path, "r");
    if(!file_ptr)
      return IOERR_INVALID_PATH;
    fseek(file_ptr, offset, SEEK_SET);
    fgets(buffer, bufbytes, file_ptr);
    fclose(file_ptr);
    if(!buffer){
      return IOERR_POSIX;
    }
    return sizeof(buffer);
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
    char type = (S_ISDIR(st.st_mode)) ? 'd' : 'f';
    sprintf((char*)buffer, "Size:%jd Accessed:%jd Modified:%jd Type:%c", (intmax_t)size, (intmax_t)accessed, (intmax_t)modified, type);
    fclose(file_ptr);
    return 0;
}

int file_write(char *path, int offset, void *buffer, size_t bufbytes)
{
    FILE* file_ptr;
    file_ptr = fopen(path, "a");
    if(!file_ptr)
      return IOERR_INVALID_PATH;
    fseek(file_ptr, offset, SEEK_SET);
    fputs((char*)buffer, file_ptr);
    if(ftell(file_ptr) == offset){
      fclose(file_ptr);
      return IOERR_POSIX;
    }
    long int bitsWrote = ftell(file_ptr) - offset;
    fclose(file_ptr);
    return bitsWrote;
}

int file_create(char *path, char *pattern, int repeatcount)
{
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
    int res = remove(path);
    if(res){
      return IOERR_INVALID_PATH;
    }
    return res;
}

int dir_create(char *path)
{
    int res = mkdir(path,0777);
    if(res){
      return IOERR_POSIX;
    }
    return res;
}

int dir_list(char *path, void *buffer, size_t bufbytes)
{
    DIR* directory = opendir(path);
    if(!directory){
      return IOERR_INVALID_PATH;
    }
    struct dirent* entry;
    entry = readdir(directory);
    while(entry){
      bufbytes -= sizeof(entry->d_name);
      if(bufbytes >= 0){
        sprintf((char*)buffer, "%s\n", entry->d_name);
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
    return IOERR_NOT_YET_IMPLEMENTED;
}

int dir_checksum(char *path)
{
    return IOERR_NOT_YET_IMPLEMENTED;
}
