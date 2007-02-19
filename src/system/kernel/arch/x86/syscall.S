/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#define FUNCTION(x) .global x; .type x,@function; x
#define SYM(x) .global x; x

.text

/* user space half of the syscall mechanism, to be copied into the commpage */
FUNCTION(_user_syscall_int):
	int	$99
	ret
SYM(_user_syscall_int_end):




