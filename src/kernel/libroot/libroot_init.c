/* 
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>


extern void __init__dlfcn(struct uspace_prog_args_t const *uspa);


void
INIT_BEFORE_CTORS(unsigned imid, struct uspace_prog_args_t const *uspa)
{
	__init__dlfcn(uspa);
}

