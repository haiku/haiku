/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/* addresses of values stored in the bootdir, which starts on the next
   page after this code. */
#define BASE 0x8c000000
#define BOOTDIR_BASE (BASE + 0x1000)
#define STAGE2_BOOTDIR_PAGE (BOOTDIR_BASE + 0x60)
#define STAGE2_OFFSET (BOOTDIR_BASE + 0x74)
.text

start:
	mov.l	bootdir_base,r0

	/* load and calculate the offset of the stage2 bootloader into the bootdir */
	mov.l	page_addr_addr,r1
	mov.l	@r1,r1
	shll8	r1
	shll2	r1
	shll2	r1		/* multiply the offset address by 4096 */

	/* find the offset into the stage2 bootloader where the entry point is */
	mov.l	offset_addr,r2
	mov.l	@r2,r2

	/* add all of these numbers together and jump to it */
	add	r1,r0
	add	r2,r0
	jmp	@r0
	nop

.align 2
bootdir_base:
	.long	BOOTDIR_BASE
page_addr_addr:
	.long 	STAGE2_BOOTDIR_PAGE
offset_addr:
	.long	STAGE2_OFFSET
