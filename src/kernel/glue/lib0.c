/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <user_runtime.h>
#include <image.h>
#include <syscalls.h>


extern void sys_exit(int retcode);

extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);

int _start(image_id image_id, struct uspace_program_args *);
static void _call_ctors(void);


int
_start(image_id imageID, struct uspace_program_args *args)
{
//	int i;
//	int retcode;
	void *initBefore = NULL;
	void *initAfter = NULL;

	args->rld_export->get_image_symbol(imageID, B_INIT_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT, (void **)&initBefore);
	args->rld_export->get_image_symbol(imageID, B_INIT_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT, (void **)&initAfter);

	if (initBefore)
		((libinit_f *)(initBefore))(imageID, args);

	_call_ctors();

	if (initAfter)
		((libinit_f *)(initAfter))(imageID, args);

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
