/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIBROOT_PRIVATE_H
#define LIBROOT_PRIVATE_H


#include <SupportDefs.h>


struct uspace_program_args;
struct real_time_data;

status_t __parse_invoke_line(char *invoker, char ***_newArgs,
			char * const **_oldArgs, int32 *_argCount);
status_t __test_executable(const char *path, char *invoker);
void __init_env(const struct uspace_program_args *args);
void __init_heap(void);

void __init_time(void);
void __arch_init_time(struct real_time_data *data, bool setDefaults);
bigtime_t __arch_get_system_time_offset(struct real_time_data *data);


extern char _single_threaded;
	/* This determines if a process runs single threaded or not */


#endif	/* LIBROOT_PRIVATE_H */
