/*******************************************************************************
/
/	File:			modules.h
/
/	Description:	Public module-related API
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MODULE_H
#define _MODULE_H

#include <BeBuild.h>
#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

/* module flags */

#define	B_KEEP_LOADED		0x00000001


/* module info structure */

typedef struct module_info module_info;

struct module_info {
	const char	*name;
	uint32		flags;
	status_t	(*std_ops)(int32, ...);
};

#define	B_MODULE_INIT	1
#define	B_MODULE_UNINIT	2

_IMPEXP_KERNEL status_t	get_module(const char *path, module_info **vec);
_IMPEXP_KERNEL status_t	put_module(const char *path);

_IMPEXP_KERNEL status_t get_next_loaded_module_name(uint32 *cookie, char *buf, size_t *bufsize);
_IMPEXP_KERNEL void *	open_module_list(const char *prefix);
_IMPEXP_KERNEL status_t	read_next_module_name(void *cookie, char *buf, size_t *bufsize);
_IMPEXP_KERNEL status_t	close_module_list(void *cookie);

#ifdef __cplusplus
}
#endif

#endif
