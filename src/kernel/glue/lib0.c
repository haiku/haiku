/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <user_runtime.h>
#include <syscalls.h>

extern void sys_exit(int retcode);

extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);

int _start(unsigned image_id, struct uspace_prog_args_t *);
static void _call_ctors(void);

int
_start(unsigned imid, struct uspace_prog_args_t *uspa)
{
//	int i;
//	int retcode;
	void *ibc;
	void *iac;

	ibc= uspa->rld_export->dl_sym(imid, "INIT_BEFORE_CTORS", 0);
	iac= uspa->rld_export->dl_sym(imid, "INIT_AFTER_CTORS", 0);

	if(ibc) {
		((libinit_f*)(ibc))(imid, uspa);
	}
	_call_ctors();
	if(iac) {
		((libinit_f*)(iac))(imid, uspa);
	}

	return 0;
}

static
void _call_ctors(void)
{
	void (**f)(void);

	for(f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}

int __gxx_personality_v0(void);

// XXX hack to make gcc 3.x happy until we get libstdc++ working
int __gxx_personality_v0(void)
{
	return 0;
}
