/*
   This file contains a function, build_argv(), which will take an input
   string and chop it into individual words.  The return value is an
   argv style array (i.e. like what main() receives).

  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "argv.h"

#define DOUBLE_QUOTE  '"'
#define SINGLE_QUOTE  '\''
#define BACK_SLASH    '\\'

char **
build_argv(char *str, int *argc)
{
    int table_size = 16, _argc;
    char **argv;

    if (argc == NULL)
        argc = &_argc;

    *argc = 0;
    argv  = (char **)calloc(table_size, sizeof(char *));

    if (argv == NULL)
        return NULL;
    
    while(*str) {
        /* skip intervening white space */
        while(*str != '\0' && (*str == ' ' || *str == '\t' || *str == '\n'))
            str++;
        
        if (*str == '\0')
            break;
        
        if (*str == DOUBLE_QUOTE) {
            argv[*argc] = ++str;
            while(*str && *str != DOUBLE_QUOTE) {
                if (*str == BACK_SLASH)
                    strcpy(str, str+1);  /* copy everything down */
                str++;
            }
        } else if (*str == SINGLE_QUOTE) {
            argv[*argc] = ++str;
            while(*str && *str != SINGLE_QUOTE) {
                if (*str == BACK_SLASH)
                    strcpy(str, str+1);  /* copy everything down */
                str++;
            }
        } else {
            argv[*argc] = str;
            while(*str && *str != ' ' && *str != '\t' && *str != '\n') {
                if (*str == BACK_SLASH)
                    strcpy(str, str+1);  /* copy everything down */
                str++;
            }
        }
        
        if (*str != '\0')
            *str++ = '\0';   /* chop the string */
        
        *argc = *argc + 1;
        if (*argc >= table_size-1) {
            char **nargv;
            
            table_size = table_size * 2;
            nargv = (char **)calloc(table_size, sizeof(char *));
            
            if (nargv == NULL) {   /* drats! failure. */
                free(argv);
                return NULL;
            }
            
            memcpy(nargv, argv, (*argc) * sizeof(char *));
            free(argv);
            argv = nargv;
        }
    }
    
    return argv;
}
