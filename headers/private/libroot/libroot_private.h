#ifndef LIBROOT_PRIVATE_H
#define LIBROOT_PRIVATE_H
/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


struct uspace_program_args;

void __init_image(const struct uspace_program_args *);
void __init_dlfcn(const struct uspace_program_args *);


extern char _single_threaded;
	/* This determines if a process runs single threaded or not */


#endif	/* LIBROOT_PRIVATE_H */
