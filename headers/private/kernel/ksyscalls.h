/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KSYSCALLS_H
#define _KERNEL_KSYSCALLS_H


#include <SupportDefs.h>


#define MAX_SYSCALL_PARAMETERS	16


typedef struct syscall_info {
	void	*function;		// pointer to the syscall function
	int		parameter_size;	// summed up parameter size
} syscall_info;

typedef struct syscall_parameter_info {
	int			offset;
	int			size;
	int			used_size;
	type_code	type;
} syscall_parameter_info;

typedef struct syscall_return_type_info {
	int			size;
	int			used_size;
	type_code	type;
} syscall_return_type_info;

typedef struct extended_syscall_info {
	const char*					name;
	int							parameter_count;
	syscall_return_type_info	return_type;
	syscall_parameter_info		parameters[MAX_SYSCALL_PARAMETERS];
} extended_syscall_info;


extern const int kSyscallCount;
extern const syscall_info kSyscallInfos[];
extern const extended_syscall_info kExtendedSyscallInfos[];


#ifdef __cplusplus
extern "C" {
#endif

int32 syscall_dispatcher(uint32 function, void *argBuffer, uint64 *_returnValue);
status_t generic_syscall_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_KSYSCALLS_H */
