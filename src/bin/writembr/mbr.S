/*
** Copyright (c) 1999 Robert Nordier
** All rights reserved.
**
** Redistribution and use in source and binary forms are freely
** permitted provided that the above copyright notice and this
** paragraph and the following disclaimer are duplicated in all
** such forms.
**
** This software is provided "AS IS" and without any express or
** implied warranties, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose.
/*

/* A 512 byte MBR boot manager that simply boots the active partition. */

		.set LOAD, 0x7c00				// Load address
		.set EXEC, 0x600 				// Execution address
		.set PT_OFF, 0x1be				// Partition table
		.set MAGIC, 0xaa55				// Magic: bootable

		.set NUM_HARD_DRIVES, 0x475		// Number of hard drives

		.globl start					// Entry point
		.code16

/*
 * Setup the segment registers for flat addressing and setup the stack.
 */
start:	cld								// String ops inc
		xorw %ax, %ax					// Zero
		movw %ax, %es					// Address
		movw %ax, %ds					// data
		movw %ax, %ss					// Set up stack
		movw $LOAD, %sp					//
/*
 * Relocate ourself to a lower address so that we are out of the way when
 * we load in the bootstrap from the partition to boot.
 */ 
		movw $main-EXEC+LOAD, %si		// Source
		movw $main, %di					// Destination
		movw $0x200-(main-start), %cx	// Byte count
		rep								// Relocate code
		movsb							//
/*
 * Jump to the relocated code.
 */
		jmp main-LOAD+EXEC				// Jump to relocated code

/*
 * Scan the partition table looking for an active entry.  Note that %ch is
 * zero from the repeated string instruction above.  We save the offset of
 * the active partition in %si and scan the entire table to ensure that only
 * one partition is marked active.
 */

main:	xorw %si, %si					// No active partition
		movw $partition_table, %bx		// Partition table
		movb $0x4, %cl					// Number of entries
main.1: cmpb %ch, (%bx)					// Null entry?
		je main.2						// Yes
		jg err_partition_table			// If 0x1..0x7f
		testw %si, %si	 				// Active already found?
		jnz err_partition_table			// Yes
		movw %bx, %si					// Point to active
main.2: addb $0x10, %bl					// Till
		loop main.1						//  done
		testw %si, %si	 				// Active found?
		jnz main.3						// Yes
		int $0x18						// BIOS: Diskless boot
/*
 * Ok, we've found a possible active partition.  Check to see that the drive
 * is a valid hard drive number.
 */
main.3: cmpb $0x80, %dl					// Drive valid?
		jb main.4						// No
		movb NUM_HARD_DRIVES, %dh		// Calculate the highest
		addb $0x80, %dh					//   drive number available
		cmpb %dh, %dl					// Within range?
		jb main.5						// Yes
main.4: movb (%si), %dl					// Load drive
/*
 * Ok, now that we have a valid drive and partition entry, load the CHS from
 * the partition entry and read the sector from the disk.
 */
main.5:	movw %sp, %di					// Save stack pointer
		movb 0x1(%si), %dh				// Load head
		movw 0x2(%si), %cx				// Load cylinder:sector
		movw $LOAD, %bx					// Transfer buffer
		cmpb $0xff, %dh					// Might we need to use LBA?
		jnz main.7						// No.
		cmpw $0xffff, %cx				// Do we need to use LBA?
		jnz main.7						// No.
		pushw %cx						// Save %cx
		pushw %bx						// Save %bx
		movw $MAGIC, %bx				// Magic
		movb $0x41, %ah					// BIOS:	EDD extensions present?
		int $0x13						//
		jc main.6						// No.
		cmpw $MAGIC, %bx				// Magic ok?
		jne main.6						// No.
		testb $0x1, %cl					// Packet mode present?
		jz main.6						// No.
		popw %bx						// Restore %bx
		pushl $0x0						// Set the LBA
		pushl 0x8(%si)					//  address
		pushw %es						// Set the address of
		pushw %bx						//  the transfer buffer
		pushw $0x1						// Read 1 sector
		pushw $0x10						// Packet length
		movw %sp, %si					// Packer pointer
		movw $0x4200, %ax				// BIOS:	LBA Read from disk
		jmp main.8						// Skip the CHS setup
main.6:	popw %bx						// Restore %bx
		popw %cx						// Restore %cx
main.7:	movw $0x201, %ax				// BIOS: Read from disk
main.8:	int $0x13						// Call the BIOS
		movw %di,%sp					// Restore stack
		jc err_loading_os				// If error
/*
 * Now that we've loaded the bootstrap, check for the magic 0xaa55 signature.  If it
 * is present, execute the bootstrap we just loaded.
 */
		cmpw $MAGIC, 0x1fe(%bx)			// Bootable?
		jne err_missing_os				// No
		jmp *%bx						// Invoke bootstrap
/*
 * Various error message entry points.
 */
err_partition_table:
		movw $msg_partition_table, %si	// "Invalid partition table"
		jmp putString					//

err_loading_os:
		movw $msg_loading_os, %si		// "Error loading operating system"
		jmp putString					//

err_missing_os:
		movw $msg_missing_os, %si		// "Missing operating system"
		jmp putString					//
/*
 * Output an ASCIZ string to the console via the BIOS.
 */ 
putString.0:
		movw $0x7, %bx			 		// Page:attribute
		movb $0xe, %ah					// BIOS: Display character
		int $0x10						//
putString:
		lodsb							// Get character
		testb %al,%al					// End of string?
		jnz putString.0					// No
putString.1:
		jmp putString.1					// Await reset

msg_partition_table: 	.asciz "Invalid partition table"
msg_loading_os: 		.asciz "Error loading operating system"
msg_missing_os: 		.asciz "Missing operating system"

		.org PT_OFF

partition_table:
		.fill 0x10,0x4,0x0				// Partition table
		.word MAGIC						// Magic number
