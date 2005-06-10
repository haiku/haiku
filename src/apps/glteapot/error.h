/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef error_h
#define error_h

#include <stdio.h>

extern void fatalerror(char *);

#define DEBUGGING 1

#ifdef DEBUGGING

#define assert(a) if (!(a)) { \
  printf("%s:%d: Failed assertion `"#a"'\n",__FILE__,__LINE__); \
  fatalerror("Failed assertion!"); };

#define checkpoint printf("%s:%d: Checkpoint...\n",__FILE__,__LINE__);\
                   fflush(stdout);

#else //DEBUGGING

#define assert(a)  
#define checkpoint  

#endif //DEBUGGING

#endif
