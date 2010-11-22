/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <user_runtime.h>
#include <image.h>

#include "init_term_dyn.h"


// include the version glue -- it's separate for the kernel add-ons only
#include "haiku_version_glue.c"


/*!	These functions are called from _init()/_fini() (in crti.S, crtn.S)
	__haiku_{init,term}_before() is called before crtbegin/end code is
	executed, __haiku_{init,term}_after() after this. crtbegin contains
	code to initialize all global constructors and other GCC related things
	(like exception frames).
 */


#define HIDDEN_FUNCTION(function)   asm volatile(".hidden " #function)


void
__haiku_init_before(image_id id)
{
	void (*before)(image_id);

	HIDDEN_FUNCTION(__haiku_init_before);

	if (get_image_symbol(id, B_INIT_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT,
			(void**)&before) == B_OK) {
		before(id);
	}
}


void
__haiku_init_after(image_id id)
{
	void (*after)(image_id);

	HIDDEN_FUNCTION(__haiku_init_after);

	if (get_image_symbol(id, B_INIT_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT,
			(void**)&after) == B_OK) {
		after(id);
	}
}


void
__haiku_term_before(image_id id)
{
	void (*before)(image_id);

	HIDDEN_FUNCTION(__haiku_term_before);

	if (get_image_symbol(id, B_TERM_BEFORE_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT,
			(void**)&before) == B_OK) {
		before(id);
	}
}


void
__haiku_term_after(image_id id)
{
	void (*after)(image_id);

	HIDDEN_FUNCTION(__haiku_term_after);

	if (get_image_symbol(id, B_TERM_AFTER_FUNCTION_NAME, B_SYMBOL_TYPE_TEXT,
			(void**)&after) == B_OK) {
		after(id);
	}
}
