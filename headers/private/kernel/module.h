/*
** Copyright 2001-2002, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H

#include <kernel.h>
#include <stage2.h>

typedef struct module_info module_info;

struct module_info {
	const char  *name;
	uint32      flags;
	int32       (*std_ops)(int32, ...);
};

// boot init
// XXX this is very private, so move it away
extern int module_init( kernel_args *ka, module_info **sys_module_headers );

/* Be Module Compatability... */

#define	B_MODULE_INIT	1
#define	B_MODULE_UNINIT	2
#define	B_KEEP_LOADED		0x00000001

int	 get_module(const char *path, module_info **vec);
int	 put_module(const char *path);

int  get_next_loaded_module_name(uint32 *cookie, char *buf, size_t *bufsize);
void *open_module_list(const char *prefix);
int  read_next_module_name(void *cookie, char *buf, size_t *bufsize);
int  close_module_list(void *cookie);

extern module_info *modules[];

#endif /* _|KRENEL_MODULE_H */
