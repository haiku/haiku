/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#define FUNCTION(x) .global x; .type x,@function; x


FUNCTION(execute_n_instructions):
	movl    4(%esp), %ecx
	shrl    $4, %ecx
.again:
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	xorl    %eax, %eax
	loop	.again
	ret

