//
//      FILE: Debug.H
//    AUTHOR: bmc
//     DESCR: Debug header for FallOS
//
// $Id: Debug.H 10 2002-07-09 12:24:59Z ejakowatz $
//

#ifdef _ASSERT_H 
#ifndef FALLOS_DEBUG_HEADER

#error Attempted to include <assert.h> before Debug.H

#endif 
#endif 

#ifndef FALLOS_DEBUG_HEADER
#define FALLOS_DEBUG_HEADER
#define _ASSERT_H

#include <stdlib.h>
#include <stdio.h>

#define panic(X) { printf ("Panic at line %d in file %s:\n%s\n", __LINE__, __FILE__, X); exit(1); }

//
// We define assert to print out the assertion message and then spin.
// The reason to spin instead of exiting is to allow you to Ctrl-C in
// the debugger and be able to walk up the stack, etc...
//

#define assert(X) if (!(X)) { \
  printf ("Assertion failure at line %d in file %s:\n%s\n", __LINE__, \
    __FILE__, #X); while(1); }

int dbg_file_active (char *fn);
void dbg_init();

#define DBG_MAX_FILES    100
#define DBG_MAX_FNAMELEN 100
#define DBG_DEFAULT_FILE ".dbg_files"
#define DBG_ENV_VAR "DBG_FILES"

#ifndef NDEBUG
#define dprintf(arg) if (dbg_file_active (__FILE__)) printf arg;
#define dpause(arg) if (dbg_file_active (__FILE__)) printf arg;
#else 
#define dprintf(arg)
#define dpause(arg)
#endif
#endif 
