/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */ 


#include "rld_priv.h"


typedef void (*init_term_function)(image_id);


void
arch_call_init(image_t *image)
{
	((init_term_function *)image->init_routine)(image->id);
}


void
arch_call_term(image_t *image)
{
	((init_term_function *)image->term_routine)(image->id);
}

