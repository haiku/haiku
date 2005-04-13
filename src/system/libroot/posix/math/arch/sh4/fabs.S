/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define FUNCTION(name) .align 2 ; .globl _##name ; .type _##name,@function ; _##name

FUNCTION(fabs):
	fabs	dr4
	rts
	fmov	dr4,dr0
