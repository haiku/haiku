/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

.text

.globl get_tc
get_tc:
	.long	0x4e7a0003		// movec %tc,%d0
	rts

.globl set_tc
set_tc:
	.long	0x4e7b0003		// movec %d0,%tc
	rts

.globl get_urp
get_urp:
	.long	0x4e7a0806		// movec %urp,%d0
	rts

.globl set_urp
set_urp:
	.long	0x4e7b0806		// movec %d0,%urp
	rts

.globl get_srp
get_srp:
	.long	0x4e7a0807		// movec %srp,%d0
	rts

.globl set_srp
set_srp:
	.long	0x4e7b0807		// movec %d0,%srp
	rts

