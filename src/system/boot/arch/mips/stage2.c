/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

static int stack[1024];

asm("
.text
.globl _start
_start:
	lw	$sp,_stack_end_addr
	j	start
	nop
_stack_end_addr:
	.word	stack+(1024*4)

");

void start()
{
	int *a = stack;
	int *b = 0;
	*b = 4;
	for(;;);
}


