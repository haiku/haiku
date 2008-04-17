/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LIBROOT_PRIVATE_H
#define LIBROOT_PRIVATE_H


#include <SupportDefs.h>
#include <image.h>


#define MAX_PASSWD_NAME_LEN			(32)
#define MAX_PASSWD_PASSWORD_LEN		(32)
#define MAX_PASSWD_REAL_NAME_LEN	(128)
#define MAX_PASSWD_HOME_DIR_LEN		(B_PATH_NAME_LENGTH)
#define MAX_PASSWD_SHELL_LEN		(B_PATH_NAME_LENGTH)

#define MAX_PASSWD_BUFFER_SIZE	(	\
	MAX_PASSWD_NAME_LEN				\
	+ MAX_PASSWD_PASSWORD_LEN		\
	+ MAX_PASSWD_REAL_NAME_LEN		\
	+ MAX_PASSWD_HOME_DIR_LEN		\
	+ MAX_PASSWD_SHELL_LEN)

#define	MAX_GROUP_NAME_LEN			(32)
#define	MAX_GROUP_PASSWORD_LEN		(32)
#define	MAX_GROUP_MEMBER_COUNT		(32)

#define MAX_GROUP_BUFFER_SIZE	(	\
	MAX_GROUP_NAME_LEN				\
	+ MAX_GROUP_PASSWORD_LEN		\
	+ ((MAX_GROUP_MEMBER_COUNT + 1) * sizeof(char*)))
	// MAX_GROUP_NAME_LEN and MAX_GROUP_PASSWORD_LEN are char* aligned


#define	MAX_SHADOW_PWD_NAME_LEN			(32)
#define	MAX_SHADOW_PWD_PASSWORD_LEN		(128)

#define MAX_SHADOW_PWD_BUFFER_SIZE	(	\
	MAX_SHADOW_PWD_NAME_LEN				\
	+ MAX_SHADOW_PWD_PASSWORD_LEN)


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
void __init_env(const struct user_space_program_args *args);
void __init_heap(void);

void __init_time(void);
void __arch_init_time(struct real_time_data *data, bool setDefaults);
bigtime_t __arch_get_system_time_offset(struct real_time_data *data);
void __init_pwd_backend(void);
void __reinit_pwd_backend_after_fork(void);


#ifdef __cplusplus
}
#endif

#endif	/* LIBROOT_PRIVATE_H */
