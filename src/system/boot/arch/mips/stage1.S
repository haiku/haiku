/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
.text
.globl _start
_start:
	j 	0x88004074; // start of the stage2 bootloader
	nop

.data
foo:
#include "boot/mips/final.asminclude"

