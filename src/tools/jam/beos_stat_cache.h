// beos_stat_cache.h

#ifndef BEOS_STAT_CACHE_H
#define BEOS_STAT_CACHE_H

#include <dirent.h>
#include <sys/stat.h>

int beos_stat_cache_stat(const char *filename, struct stat *st);

DIR* beos_stat_cache_opendir(const char *dirName);
struct dirent *beos_stat_cache_readdir(DIR *dir);
int beos_stat_cache_closedir(DIR *dir);

#endif BEOS_STAT_CACHE_H
