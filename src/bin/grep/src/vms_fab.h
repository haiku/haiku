/*
   This file includes the setup for the file access block for VMS.
   Written by Phillip C. Brisco 8/98.
 */

#include <rms.h>
#include <ssdef.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <starlet.h>

#if defined(VMS) && defined(__DECC) /* need function prototype */
# if (__DECC_VER<50790004)           /* have an own one         */
char *alloca(unsigned int);
# else
#  define alloca __ALLOCA
# endif
#endif


struct FAB fab;
struct NAM nam;

int length_of_fna_buffer;
int fab_stat;
char expanded_name[NAM$C_MAXRSS];
char fna_buffer[NAM$C_MAXRSS];
char result_name[NAM$C_MAXRSS];
char final_name[NAM$C_MAXRSS];
int max_file_path_size = NAM$C_MAXRSS;
char *arr_ptr[32767];
