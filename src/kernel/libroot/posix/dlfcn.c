/* 
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <dlfcn.h>
#include <syscalls.h>
#include <user_runtime.h>

void __init__dlfcn(struct uspace_prog_args_t const *);
static struct rld_export_t const *rld;

void *
dlopen(char const *name, unsigned flags)
{
	return (void*)(rld->dl_open(name, flags));
}

void *
dlsym(void *img, char const *name)
{
	return rld->dl_sym((unsigned)img, name, 0);
}

int
dlclose(void *img)
{
	return rld->dl_close((unsigned)img, 0);
}


void
__init__dlfcn(struct uspace_prog_args_t const *uspa)
{
	rld= uspa->rld_export;
}
