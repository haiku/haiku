/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIBROOT_PRIVATE_H
#define LIBROOT_PRIVATE_H


#include <SupportDefs.h>
#include <image.h>


struct user_space_program_args;
struct real_time_data;


#ifdef __cplusplus
extern "C" {
#endif

extern char _single_threaded;
	/* This determines if a process runs single threaded or not */

status_t __parse_invoke_line(char *invoker, char ***_newArgs,
			char * const **_oldArgs, int32 *_argCount, const char *arg0);
status_t __get_next_image_dependency(image_id id, uint32 *cookie,
			const char **_name);
status_t __test_executable(const char *path, char *invoker);
status_t __flatten_process_args(const char* const* args, int32 argCount,
			const char* const* env, int32 envCount, char*** _flatArgs,
			size_t* _flatSize);
void _call_atexit_hooks_for_range(addr_t start, addr_t size);
void __init_env(const struct user_space_program_args *args);
void __init_heap(void);
void __init_heap_post_env(void);

void __init_time(void);
void __arch_init_time(struct real_time_data *data, bool setDefaults);
bigtime_t __arch_get_system_time_offset(struct real_time_data *data);
bigtime_t __get_system_time_offset();
void __init_pwd_backend(void);
void __reinit_pwd_backend_after_fork(void);
void* __arch_get_caller(void);


#ifdef __cplusplus
}
#endif

#endif	/* LIBROOT_PRIVATE_H */
