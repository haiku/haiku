/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__run_time_linker__hh__
#define __newos__run_time_linker__hh__


#include <ktypes.h>
#include <user_runtime.h>


#define NEWOS_MAGIC_APPNAME	"__NEWOS_APP__"

typedef unsigned dynmodule_id;
int RLD_STARTUP(void *);
int rldmain(void *arg);

dynmodule_id load_program(char const *path, void **entry);
dynmodule_id load_library(char const *path);
dynmodule_id load_addon(char const *path);
dynmodule_id unload_program(dynmodule_id imid);
dynmodule_id unload_library(dynmodule_id imid);
dynmodule_id unload_addon(dynmodule_id imid);

void *dynamic_symbol(dynmodule_id imid, char const *symbol);


void  rldelf_init(struct uspace_prog_args_t const *uspa);

void  rldheap_init(void);
void *rldalloc(size_t);
void  rldfree(void *p);

int   export_dl_open(char const *, unsigned);
int   export_dl_close(int, unsigned);
void *export_dl_sym(int, char const *, unsigned);

int   export_load_addon(char const *, unsigned);
int   export_unload_addon(int, unsigned);
void *export_addon_symbol(int, char const *, unsigned);

#endif

