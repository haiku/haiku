/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef __newos__kernel__user_runtime__hh__
#define __newos__kernel__user_runtime__hh__


#include <defines.h>


struct rld_export_t
{
	int   (*dl_open )(char const *path, unsigned flags);
	int   (*dl_close)(int lib, unsigned flags);
	void *(*dl_sym  )(int lib, char const *sym, unsigned flags);

	int   (*load_addon)(char const *path, unsigned flags);
	int   (*unload_addon)(int add_on, unsigned flags);
	void *(*addon_symbol)(int lib, char const *sym, unsigned flags);
};

struct uspace_prog_args_t
{
	char prog_name[SYS_MAX_OS_NAME_LEN];
	char prog_path[SYS_MAX_PATH_LEN];
	int  argc;
	int  envc;
	char **argv;
	char **envp;

	/*
	 * hooks into rld for POSIX and BeOS library/module loading
	 */
	struct rld_export_t *rld_export;
};

typedef void (libinit_f)(unsigned, struct uspace_prog_args_t const *);

void INIT_BEFORE_CTORS(unsigned, struct uspace_prog_args_t const *);
void INIT_AFTER_CTORS(unsigned, struct uspace_prog_args_t const *);


#endif
