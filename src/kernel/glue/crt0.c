/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "init_term_dyn.h"


void _init(int id, void *args) __attribute__((section(".init")));
void _fini(int id, void *args) __attribute__((section(".fini")));

// constructor/destructor lists as defined in the ld-script
extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);
extern void (*__dtor_list)(void);
extern void (*__dtor_end)(void);


static void
_call_ctors(void)
{ 
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++) {
		(**f)();
	}
}


static void
_call_dtors(void)
{ 
	void (**f)(void);

	for (f = &__dtor_list; f < &__dtor_end; f++) {
		(**f)();
	}
}


void
_init(int id, void *args)
{
	_init_before(id, args);

	_call_ctors();

	_init_after(id, args);
}


void
_fini(int id, void *args)
{
	_term_before(id, args);
	
	_call_dtors();

	_term_after(id, args);
}

