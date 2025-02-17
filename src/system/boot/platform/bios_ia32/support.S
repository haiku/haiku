/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#define FUNCTION(x) .global x; .type x,@function; x


null_idt_descr:
	.word	0
	.word	0,0
		
FUNCTION(reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done

