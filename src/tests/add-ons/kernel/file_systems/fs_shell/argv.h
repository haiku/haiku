#ifndef FS_SHELL_ARGV_H
#define FS_SHELL_ARGV_H

#ifdef __cplusplus
extern "C" {
#endif


/* this function takes a string and chops it into individual "words" */
char **build_argv(char *str, int *argc);


#ifdef __cplusplus
}
#endif

#endif	// FS_SHELL_ARGV_H
