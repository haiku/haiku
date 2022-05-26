/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


	.section	.bss.stack
.globl gStack
.type  gStack, @object
gStack:
	.skip 1*1024*1024
.size	gStack, .-gStack
.globl gStackEnd
gStackEnd:

	.section	.text.head
	la sp, gStackEnd
	li fp, 0
	call _start
	j .
