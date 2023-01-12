#ifndef SFS_API_H
#define SFS_API_H


#include "sfs_base.h"

// You can add more into this file.

void mksfs(int fresh);

int sfs_getnextfilename(char *filename);

int sfs_getfilesize(const char *filename);

int sfs_fopen(const char *filename);

int sfs_fclose(int fd);

int sfs_fwrite(int fd, const char *buffer, int length);

int sfs_fread(int fd, char *buffer, int length);

int sfs_fseek(int fd, int location);

int sfs_remove(const char *filename);


#endif
