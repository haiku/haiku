#ifndef INIT_TERM_DYN_H
#define INIT_TERM_DYN_H
/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <image.h>


struct uspace_program_args;

void _init_before(image_id id, struct uspace_program_args *args);
void _init_after(image_id id, struct uspace_program_args *args);
void _term_before(image_id id, struct uspace_program_args *args);
void _term_after(image_id id, struct uspace_program_args *args);

#endif	/* INIT_TERM_DYN_H */
