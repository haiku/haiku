/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIBROOT_PRIVATE_H
#define LIBROOT_PRIVATE_H


struct uspace_program_args;
struct real_time_data;

void __init_image(const struct uspace_program_args *args);
void __init_dlfcn(const struct uspace_program_args *args);
void __init_env(const struct uspace_program_args *args);
void __init_heap(void);

void __init_time(void);
void __arch_init_time(struct real_time_data *data);


extern char _single_threaded;
	/* This determines if a process runs single threaded or not */


#endif	/* LIBROOT_PRIVATE_H */
