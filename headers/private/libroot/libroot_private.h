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

extern int __gABIVersion;
extern int __gAPIVersion;

extern char _single_threaded;
	/* This determines if a process runs single threaded or not */

status_t __look_up_in_path(const char *name, char *buffer);
status_t __parse_invoke_line(char *invoker, char ***_newArgs,
			char * const **_oldArgs, int32 *_argCount, const char *arg0);
status_t __get_next_image_dependency(image_id id, uint32 *cookie,
			const char **_name);
status_t __test_executable(const char *path, char *invoker);
status_t __flatten_process_args(const char* const* args, int32 argCount,
			const char* const* env, int32* envCount, const char* executablePath,
			char*** _flatArgs, size_t* _flatSize);
thread_id __load_image_at_path(const char* path, int32 argCount,
			const char **args, const char **environ);
void _call_atexit_hooks_for_range(addr_t start, addr_t size);
void __init_env(const struct user_space_program_args *args);
void __init_env_post_heap(void);
status_t __init_heap(void);
void __heap_terminate_after(void);
void __heap_before_fork(void);
void __heap_after_fork_child(void);
void __heap_after_fork_parent(void);
void __heap_thread_init(void);
void __heap_thread_exit(void);

void __init_time(addr_t commPageTable);
void __arch_init_time(struct real_time_data *data, bool setDefaults);
bigtime_t __arch_get_system_time_offset(struct real_time_data *data);
bigtime_t __get_system_time_offset();
void __init_pwd_backend(void);
void __reinit_pwd_backend_after_fork(void);
int32 __arch_get_stack_trace(addr_t* returnAddresses, int32 maxCount,
	int32 skipFrames, addr_t stackBase, addr_t stackEnd);

void __set_stack_protection(void);


#ifdef __cplusplus
}
#endif

#endif	/* LIBROOT_PRIVATE_H */
