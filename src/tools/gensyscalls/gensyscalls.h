// gensyscalls.h

#ifndef _GEN_SYSCALLS_H
#define _GEN_SYSCALLS_H

typedef struct gensyscall_parameter_info {
	const char	*type;
	int			offset;
	int			size;
	int			actual_size;
} gensyscall_parameter_info;

typedef struct gensyscall_syscall_info {
	const char					*name;
	const char					*kernel_name;
	const char					*return_type;
	int							parameter_count;
	gensyscall_parameter_info	*parameters;
} gensyscall_syscall_info;

#endif	// _GEN_SYSCALLS_H
