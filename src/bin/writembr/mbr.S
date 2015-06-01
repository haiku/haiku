/*
 * Partly from:
 * $FreeBSD: src/sys/boot/i386/pmbr/pmbr.s,v 1.2 2007/11/26 21:29:59 jhb Exp $
 *
 * Copyright (c) 2007 Yahoo!, Inc.
 * All rights reserved.
 * Written by: John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Partly from:
 * $FreeBSD: src/sys/boot/i386/mbr/mbr.s,v 1.7 2004/08/28 08:39:35 yar Exp $
 *
 * Copyright (c) 1999 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 *
 * "Hybridisation" and modifications for booting Haiku by Andre' Braga
 * (me@andrebraga.com), with valuable input from Jean-Loïc Charroud
 * (jcharroud@free.fr). The modifications contained herein are released into
 * the Public Domain.
 *
 *
 * 		A 432 bytes MBR IPL (Initial Program Loader) that looks for the UUID of
 * a Haiku Boot GUID partition and boots it, falling back to MBR partitions if
 * said UUID isn't found or if the (primary) GPT is corrupted or non-existent.
 * 		Its usefulness is in being a versatile, "universal" IPL that supports
 * both partitioning styles, allowing it to be used transparently and even
 * facilitating the conversion between partitioning styles, should the need
 * for more partitions or volumes over 2TiB arise (for instance when cloning
 * an older disk to a newer, more capacious one).
 * 		It also paves the way for Haiku to create and support booting from
 * multiple volumes larger than 2TiB, which we're in the very privileged
 * position of enjoying efficiently in the near future due to BFS. Another use
 * case is taking a disk from a Intel EFI machine, plugging it on a BIOS
 * machine and boot just fine; and vice-versa.
 * 		As mentioned, if there are valid partitions defined in the MBR, and the
 * primary GPT becomes corrupt, it can fall back to loading the MBR partition
 * with the active flag set, if one is defined.
 * 		Currently there's no provision for falling back to the GPT copy that
 * lives in the end of the disk, due to the 512 bytes constraint; supporting
 * this is unlikely given that the code is packed tight. An alternative would be
 * disabling support for booting from MBR using BIOS calls other than Int13h
 * function 42h, "Extended Read From Disk" (i.e., LBA mode). It's unlikely that
 * any machine that Haiku supports won't have this BIOS function, but having an
 * "universal" IPL should be quite useful to, say, people using Haiku to
 * rewrite a corrupt MBR on another disk using the excellent DiskProbe.
 * 		The number of sectors loaded depends on the boot style. Booting from a
 * MBR partition assumes that the Partition Boot Record is one sector long,
 * whereas booting from a GPT partition assumes a partition exclusive for a
 * system loader and will either copy its entirety into memory starting from
 * address 0x7c00, or will copy up to circa 545KiB, whichever is smaller. Thus,
 * it remains compatible with the FreeBSD gptloader and should work for loading
 * Bootman from an exclusive Haiku boot manager partition as well.
 * 		It should be easy to adjust the UUID signature as needed. It lives at
 * offset 0x1a0 (416), leaving plenty of space before the 32-bit disk signature
 * at offset 0x1b8 (440), so compatibility with Microsoft Windows and other OSs
 * is maintained.
 */

			.set LOAD,0x7c00
			.set EXEC,0x600
			.set MAGIC,0xaa55
			.set HANDSHAKE,0x55aa
			.set SECSIZE,0x200

			/* data offsets */
			.set UUID,0x1a0
			.set DISKSIG,0x1b8
			.set PT_OFF,0x1be

			.set GPTSTACK,EXEC+SECSIZE*4-8	/* Stack address */
			.set LBABUF,GPTSTACK			/* LBA address pointer buffer, */
											/*  8 bytes long, after stack */

			.set GPT_ADDR,LBABUF+8			/* GPT header address */
			.set GPT_SIG,0					/* Signature offset from LBA 1 */
			.set GPT_SIG_0,0x20494645		/* "EFI ", bswapped */
			.set GPT_SIG_1,0x54524150		/* "PART", bswapped */
			.set GPT_MYLBA,24				/* Offs of curr header copy addr */
			.set GPT_PART_LBA,72			/* Offs of partitions start LBA */
			.set GPT_NPART,80				/* Offs to number of partitions */
			.set GPT_PART_SIZE,84			/* Offs to size of partition */
			.set PART_ADDR,GPT_ADDR+SECSIZE	/* GPT partition array addr. */
			.set PART_TYPE,0
			.set PART_START_LBA,32			/* Offs to 1st LBA on part entry */
			.set PART_END_LBA,40			/* Offs to last LBA */

			.set NHRDRV,0x475				/* Number of hard drives */

			.globl start					/* Entry point */
			.code16

/*
 * Setup the segment registers for flat addressing and setup the stack.
 */
start:		cli								/* Clear interrupts before relocation */

			xorw %ax,%ax					/* Zero */
			movw %ax,%es					/* Address */
			movw %ax,%ds					/*  data */

			movw %ax,%ss					/* Set up */
			movw $GPTSTACK,%sp				/*  stack */

			std								/* String ops set to decrement */
			movw $LOAD,%si					/* We'll clear working memory starting */
			leaw -1(%si),%di				/*  from $LOAD-1 and stopping at EXEC. */
			movw $(LOAD-EXEC-1),%cx			/*  In the end we have %si pointing */
			rep stosb						/*  at LOAD and %di at EXEC. */


/*
 * Relocate ourself to a lower address so that we have more room to load
 * other sectors.
 */
reloc:		cld								/* String ops set to increment */
			movw $SECSIZE,%cx				/* Now we walk forward and relocate. */
			rep	movsb						/*  Tricky, but works great! */

/*
 * Jump to the relocated code.
 */
			jmp $0,$main					/* Absolute address (far) jump */

/*
 * Will land here; we're now at %cs = 0x0000 and %ip a little above 0x0600
 */
main:		sti								/* Re-enable interrupts */

#ifdef VALIDATE_DRV
/*
 * Validate drive number in %dl. Certain BIOSes might not like it.
 */
validate_drv:
			cmpb $0x80,%dl					/* Drive valid? (>= 0x80) */
			jb validate_drv.1				/* No */
			movb NHRDRV,%dh					/* Calculate the highest */
			addb $0x80,%dh					/*  drive number available */
			cmpb %dh,%dl					/* Within range? */
			jb test_extensions				/* Yes, proceed */
validate_drv.1:
			movb $0x80,%dl					/* Else, assume drive 0x80 */
#endif

/*
 * Test if BIOS supports Int13h extensions. If so, will try GPT scheme first.
 * Else, sets flag (%dh = 1) and goes straight to MBR code.
 * (%dl still contains the drive number from BIOS bootstrap)
 */
test_extensions:
			movb $0,%dh						/* We'll test for EDD extensions. */
											/*  LBA read (Int13,42) uses only */
											/*  %dl to get drive number and if */
											/*  we must fall back to CHS read */
											/*  (Int13,02), %dh receives head */
											/*  number, so it's clear to use */
											/*  %dh to hold a "use CHS" flag */

			movw $HANDSHAKE,%bx				/* EDD extensions magic number */
			movb $0x41,%ah					/* BIOS:	EDD extensions */
			int $0x13						/*  present? */
			jc set_chs						/* No, fall back to CHS read */
test_magic:
			cmpw $MAGIC,%bx					/* Magic ok? */
			jne set_chs						/* No, fall back to CHS read */
test_packet:
			testb $0x1,%cl					/* Packet mode present? */
			jnz load_gpt_hdr				/* Yes! */
set_chs:
			movb $1,%dh						/* read_chs overwrites this, and */
											/*  Int13,42 only uses %dl, so */
											/*  it's clear to use %dh as flag */
			jmp try_mbr


/*
 * If we reached here, drive is valid, LBA reads are available, will try GPT.
 * Load the primary GPT header from LBA 1 and verify signature.
 */
load_gpt_hdr:
			movw $GPT_ADDR,%bx
			movw $LBABUF,%si				/* Will load LBA sector 1 from disk */
			movb $1,(%si)					/*  (64-bit value! Memory was zeroed so */
											/*  it's OK to write only the LSB) */
			call read
			cmpl $GPT_SIG_0,GPT_ADDR+GPT_SIG
			jnz try_mbr						/* If invalid GPT header */
			cmpl $GPT_SIG_1,GPT_ADDR+GPT_SIG+4
			jnz try_mbr						/* Fluke :( Try MBR now */

/*
 * GPT is valid. Load a partition table sector from disk and look for a
 * partition matching the UUID found in boot_uuid.
 */
load_part:
			movw $GPT_ADDR+GPT_PART_LBA,%si
			movw $PART_ADDR,%bx
			call read
scan:
			movw %bx,%si					/* Compare partition UUID */
			movw $boot_uuid,%di				/*  with Haiku boot UUID */
			movb $0x10,%cl					/*  (16 bytes) */
			repe cmpsb
			jnz next_part					/* Didn't match, next partition */

/*
 * We found a partition. Load it into RAM starting at 0x7c00.
 */
			movw %bx,%di					/* Save partition pointer in %di */
			leaw PART_START_LBA(%di),%si
			movw $LOAD/16,%bx
			movw %bx,%es
			xorw %bx,%bx
load_bootcode:
			push %si						/* Save %si */
			call read
			pop %si							/* Restore */
			movl PART_END_LBA(%di),%eax		/* See if this was the last LBA */
			cmpl (%si),%eax
			jnz next_boot_lba
			movl PART_END_LBA+4(%di),%eax
			cmpl 4(%si),%eax
			jnz next_boot_lba
			jmp start_loader				/* Jump to boot code */

next_boot_lba:
			incl (%si)						/* Next LBA */
			adcl $0,4(%si)
			mov %es,%ax						/* Adjust segment for next */
			addw $SECSIZE/16,%ax			/*  sector */
			cmp $0x9000,%ax					/* Don't load past 0x90000, */
			jae start_loader				/*  545k should be enough for */
			mov %ax,%es						/*  any boot code. :) */
			jmp load_bootcode

/*
 * Move to the next partition. If we walk off the end of the sector, load
 * the next sector.
 */
next_part:
			decl GPT_ADDR+GPT_NPART			/* Was this the last partition? */
			jz try_mbr						/* UUID boot signature not found */
			movw GPT_ADDR+GPT_PART_SIZE,%ax
			addw %ax,%bx					/* Next partition */
			cmpw $PART_ADDR+0x200,%bx		/* Still in sector? */
			jb scan
			incl GPT_ADDR+GPT_PART_LBA		/* Next sector */
			adcl $0,GPT_ADDR+GPT_PART_LBA+4
			jmp load_part

/*
 * If loading boot sectors from a GPT partition fails, try booting a MBR part.
 * Reset stack/segment. Could have been tainted by the GPT loading code.
 */
try_mbr:
			xorw %ax,%ax					/* Zero */
			movw %ax,%es					/*  extra segment */
			movw $LOAD,%sp					/* Reset stack */

			xorw %si,%si					/* Will point to active partition */
			movw $(EXEC+PT_OFF),%bx			/* Point to partition table start */
			movw $0x4,%cx					/* Tested entries counter (4 total) */
read_mbr_entry:
			cmpb %ch,(%bx)					/* Null entry? (%ch just zeroed) */
			je next_mbr_entry				/* Yes */
			jg err_part_table				/* If 0x1..0x7f */
			testw %si,%si					/* Active already found? */
			jnz err_part_table				/* Yes */
			movw %bx,%si					/* Point to active */
next_mbr_entry:
			addb $0x10,%bl					/* Till */
			loop read_mbr_entry				/*  done */
			testw %si,%si					/* Active found? */
			jnz read_bootsect				/* Yes, read OS loader */
try_diskless:
			int $0x18						/* Else, BIOS: Diskless boot */


/*
 * Ok, now that we have a valid drive and partition entry, load either CHS
 * or LBA from the partition entry and read the boot sector from the partition.
 */
read_bootsect:
			movw %sp,%bx					/* Write addr. (%sp points to LOAD) */
			pushw %si						/* Points at active part. entry; */
											/*  save, else 'read' will trash it */
test_flag:
			cmpb $1,%dh						/* Test flag set by set_chs above */
			jz read_chs						/* CHS read if set */
read_lba:
			addw $0x8,%si					/* Start LBA of partition, 32-bit */
			movw $LBABUF,%di				/* So far either QWORD 1 or 0, so */
			movsl							/*  more significant bytes are all 0 */
			xchg %di,%si					/*  Copy to buffer and swap pointers */
			subw $0x4,%si					/* Adjust back to start of buffer */
			call read
			jmp finished_read				/* Skip the CHS setup */
read_chs:
			movb 0x1(%si),%dh				/* Load head */
			movw 0x2(%si),%cx				/* Load cylinder:sector */
			movb $2,%al						/* Read two sectors */
			movb $2,%ah						/* BIOS: Read sectors from disk */
			int $0x13						/* Call the BIOS */
finished_read:
			jc err_reading					/* If error */


/*
 * Now that we've loaded the bootstrap, check for the 0xaa55 signature. If it
 * is present, execute the bootstrap we just loaded.
 */
			popw %si						/* Restore %si (active part entry) */
			movb %dl,(%si)					/* Patch part record with drive num */
			cmpw $MAGIC,0x1fe(%bx)			/* Bootable? */
			jne err_noboot					/* No, error out. */
											/*  Else, start loader */
start_loader:
			xorw %ax,%ax
			movw %ax,%es					/* Reset %es to zero */
			jmp $0,$LOAD					/* Jump to boot code */


/* Auxiliary functions */


/*
 * Load a sector (64-bit LBA at %si) from disk %dl into %es:%bx by creating
 * a EDD packet on the stack and passing it to the BIOS. Trashes %ax and %si.
 */
read:
			pushl 0x4(%si)					/* Set the LBA */
			pushl 0x0(%si)					/*  address */
			pushw %es						/* Set the address of */
			pushw %bx						/*  the transfer buffer */
			pushw $0x1						/* Read 1 sector */
			pushw $0x10						/* Packet length */
			movw %sp,%si					/* Packet pointer */
			movw $0x4200,%ax				/* BIOS:	LBA Read from disk */
			int $0x13						/* Call the BIOS */
			add $0x10,%sp					/* Restore stack */
			jc err_reading					/* If error */
			ret


/*
 * Output an ASCIZ string pointed at by %si to the console via the BIOS.
 */
putstr.0:
			movw $0x7,%bx					/* Page:attribute */
			movb $0xe,%ah					/* BIOS: Display */
			int $0x10						/*  character */
putstr:
			lodsb							/* Get character */
			testb %al,%al					/* End of string? */
			jnz putstr.0					/* No */
			ret


/*
 * Various error message entry points.
 */
err_part_table:
			movw $msg_badtable,%si			/* "Bad Part. Table!" */
			call putstr
			jmp halt


err_reading:
			movw $msg_ioerror,%si			/* "Read Error!" */
			call putstr
			jmp halt


err_noboot:
			movw $msg_noloader,%si			/* "No Sys Loader!" */
			call putstr
			/* fall-through to halt */


halt:
			cli
			hlt
			jmp halt


/* Data section */


#ifdef VALIDATE_DRV
/* Messages must be shortened so the code fits 440 bytes */
msg_badtable:	.asciz "BadPTbl!"
msg_ioerror:	.asciz "IOErr!"
msg_noloader:	.asciz "NoSysLdr!"
#else
msg_badtable:	.asciz "Bad Part. Table!"
msg_ioerror:	.asciz "Read Error!"
msg_noloader:	.asciz "No Sys Loader!"
#endif

/* Boot partition UUID signature */
			.org UUID,0x0					/* Zero-pad up to UUID offset */

boot_uuid:
			.long 0x42465331				/* 'BFS1' (formally, UUID time-low) */
			.word 0x3ba3					/* UUID time-mid */
			.word 0x10f1					/* UUID time-high & version (v1) */
			.byte 0x80						/* UUID DCE 1.1 variant */
			.byte 0x2a						/* '*' (formally, UUID clock-seq-low) */
			.byte 0x48						/* 'H' */
			.byte 0x61						/* 'a' */
			.byte 0x69						/* 'i' */
			.byte 0x6b						/* 'k' */
			.byte 0x75						/* 'u' */
			.byte 0x21						/* '!' */

#ifndef MBR_CODE_ONLY
/* Disk signature */
			.org DISKSIG,0x0				/* Zero-pad up to signature offset */

sig:
			.long 0							/* OS Disk Signature */
			.word 0							/* "Unknown" in PMBR */

/* Partition table */
			.org PT_OFF,0x0					/* Won't pad, just documenting */

partbl:
			.fill 0x10,0x4,0x0				/* Partition table */
			.word MAGIC						/* Magic number */
#endif
