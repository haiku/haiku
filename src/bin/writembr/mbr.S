; Copyright 2011, Jean-Loïc Charroud, jcharroud@free.fr
; Distributed under the terms of the MIT License or LGPL v3
;
; Haiku (C) Copyright Haiku 2011
; MBR Boot code
;
; assemble the Master boot record with:
;	yasm -f bin -O5 -o mbr.bin mbr.S
;
; assemble the MBR's code (does not contain the partiton table
; nor the MAGIC code) with:
;	yasm -f bin -O5 -o mbrcode.bin mbr.S -dMBR_CODE_ONLY=1


;%define	DEBUG			1
;%define	MBR_CODE_ONLY	1


;CONST
	%assign	DISKSIG			440				; Disk signature offset
	%assign	PT_OFF			0x1be			; Partition table offset
	%assign	MAGIC_OFF		0x1fe			; Magic offset
	%assign	MAGIC			0xaa55			; Magic bootable signature
	%assign	SECSIZE			0x200			; Size of a single disk sector
	%assign	FLG_ACTIVE		0x80			; active partition flag
	%assign	SECTOR_COUNT	0x01			; Number of record to load from
											; the ctive partition

	%assign	LOAD			0x7c00			; Load address
	%assign	EXEC			0x600			; Execution address
	%assign	HEAP			EXEC+SECSIZE	; Execution address

;BIOS calls
	%assign	BIOS_VIDEO_SERVICES			0x10; ah - function
	%assign	BIOS_DISK_SERVICES			0x13
	%assign	BIOS_KEYBOARD_SERVICES		0X16
	%assign	BIOS_BASIC					0X18
	%assign	BIOS_REBOOT					0X19

	;BIOS calls parameters
		; video services
			%assign	WRITE_CHAR			0x0e; al - char
											; bh - page?

		; disk services
			%assign	READ_DISK_SECTORS	0x02; dl	- drive
											; es:bx	- buffer
											; dh	- head
											; ch7:0 - track7:0
											; cl7:6 - track9:8
											;	5:0	- sector
											; al	- sector count
											; -> al - sectors read
			%assign	READ_DRV_PARAMETERS	0x08; dl - drive
											; -> cl - max cylinder 9:8
											;		- sectors per track
											;    ch - max cylinder 7:0
											;    dh - max head
											;    dl - number of drives (?)
			%assign	CHK_DISK_EXTENTIONS	0x41; bx - 0x55aa
											; dl - drive
											; -> success: carry clear
											;    ah - extension version
											;    bx - 0xaa55
											;    cx - support bit mask
											;		1 - Device Access using the
											;			packet structure
											;		2 - Drive Locking and
											;			Ejecting
											;		4 - Enhanced Disk Drive
											;			Support (EDD)
											; -> error: carry set
			%assign	EXTENDED_READ		0x42; dl - drive
											; ds:si - address packet
											; -> success: carry clear
											; -> error: carry set

			%assign FIXED_DSK_SUPPORT	0x1	; flag indicating fixed disk
											; extension command subset

		; keyboard services
			%assign	READ_CHAR			0x00; -> al - ASCII char

;MACRO
	; nicer way to get the size of a structure
	%define	sizeof(s)	s %+ _size

	; using a structure in a another structure definition
	%macro	nstruc	1-2		1
		resb	sizeof(%1) * %2
	%endmacro

	; nicer way to access GlobalVariables
	%macro	declare_var	1
		%define %1	HEAP + GlobalVariables. %+ %1
	%endmacro

	%macro	puts	1
		mov		si,		%1
		call	_puts
	%endmacro

	%macro	error	1
		mov		si, %1
		jmp		_error
	%endmacro

;TYPEDEFS
	; 64 bit value
	struc	quadword
		.lower			resd	1
		.upper			resd	1
	endstruc

	struc	CHS_addr
		.head			resb	1
		.sector			resb	1
		.cylindre		resb	1
	endstruc

	struc	PartitionEntry
		.status			resb	1
		.CHS_first		nstruc	CHS_addr
		.type			resb	1
		.CHS_last		nstruc	CHS_addr
		.LBA_start		resd	1
		.LBA_size		resd	1
	endstruc
		
	; address packet as required by the EXTENDED_READ BIOS call
	struc	AddressPacket
		.packet_size	resb	1
		.reserved		resb	1
		.block_count	resw	1
		.buffer			resd	1
		.sector			nstruc	quadword
		;.long_buffer	nstruc	quadword
		;	We don't need the 64 bit buffer pointer. The 32 bit .buffer is more
		;	than sufficient.
	endstruc

	; Structure containing the variables that don't need pre-initialization.
	; this structure will be allocated onto our "heap".
	struc	GlobalVariables
		.boot_drive_id				resd	1
		.boot_partition				resd	1
		.address_packet				nstruc	AddressPacket
	endstruc

;alias for easy access to our global variables
	declare_var	boot_drive_id
	declare_var	boot_partition
	declare_var	address_packet

;/////////////////////////////////////////////////////////////////////////
;// A 512 byte MBR boot manager that simply boots the active partition. //
;/////////////////////////////////////////////////////////////////////////
; 16 bit code
SECTION .text
BITS 16
ORG EXEC						; MBR is loaded at 0x7c00 but relocated at 0x600
start:							; we run the LOADed code
		cli						; disable interrupts
		cld						; clear direction flag (for string operations)
	init:
		xor		ax,		ax		; Zero
		mov		es,		ax		; Set up extra segment
		mov		ds,		ax		; Set up data segment
		mov		ss,		ax		; Set up stack segment
		mov		sp,		LOAD	; Set up stack pointer

		;init our heap allocated variables with zeros
		mov		di,		HEAP	;start adress
		mov		cx,		sizeof(GlobalVariables)	;size
		rep						; while(cx--)
			stosb				;	es[di++]:=al;

	; Relocate ourself to a lower address so that we are out of the way
	; when we load in the bootstrap from the partition to boot.
	reloc:
		mov		si,		LOAD	; Source
		; init AddressPacket.buffer now since LOAD is in 'si'
		mov [address_packet+AddressPacket.buffer],si
		mov byte[address_packet+AddressPacket.packet_size],sizeof(AddressPacket)
		mov byte[address_packet+AddressPacket.block_count],SECTOR_COUNT

		mov		di,		EXEC	; Destination
		mov		ch,		1		; count cx:=256 (cl cleared by precedent rep call)
		rep						; while(cx--)
			movsw				;   es[di++]:=ds[si++] 
								; //di and si are incremented by sizeof(word)
		jmp word 0x0000:continue; FAR jump to the relocated "continue" (some
								; BIOSes initialise CS to 0x07c0 so we must set
								; CS correctly)

continue:						; Now we run EXEC_based relocated code
		sti						; enable interrupts
		%ifdef DEBUG
		puts	kMsgStart
		%endif

search_active_partition:
		mov		si,EXEC+PT_OFF				; point to first table entry
		mov		al,04						; there are 4 table entries
	.loop:									; SEARCH FOR AN ACTIVE ENTRY
		cmp		byte[si],FLG_ACTIVE			; is this the active entry?
		je		found_active				; yes
		add		si,	sizeof(PartitionEntry)	; next PartitionEntry
		dec		al							; decrease remaining entry count
		jnz		.loop						; loop if entry count > 0
		jmp		no_bootable_active_partition; last partition reached

found_active:								; active partition (pointed by si)
		mov		[boot_partition],si			; Save active partition pointer

	.get_read_sector:						; initialise address_packet:
		mov eax,[si + PartitionEntry.LBA_start]
		mov [address_packet+AddressPacket.sector],eax

		; if LBA_adress equals 0 then it's not a valid PBR (it is the MBR)
		; this can append when we only have a CHS adress in the partition entry
		test	eax,		eax				;if ( LBA_adress == 0 )
		jz		no_disk_extentions			;then no_disk_extentions()

check_disk_extensions:
		; Note: this test may be unnecessary since EXTENDED_READ also
		; set the carry flag when extended calls are not supported
		%ifdef DEBUG
		puts	kMsgCheckEx
		%endif

		mov		ah, CHK_DISK_EXTENTIONS		; set command
		mov		bx, 0x55aa					; set parameter : hton(MAGIC)
		; dl has not changed yet, still contains the drive ID
		int		BIOS_DISK_SERVICES			; if( do_command() <> OK )
		jc		no_disk_extentions			; then use simple read operation 
											; else use extended read
disk_extentions:
		%ifdef DEBUG
		puts	kMsgRead_Ex
		%endif

		; load first bloc active partition
		; dl has not changed yet, still contains the drive ID
		mov		si,	address_packet			; set command parameters
		mov		ah, EXTENDED_READ			; set command
	.read_PBR:
		int		BIOS_DISK_SERVICES			; if ( do_command() <> OK )
		jc		no_disk_extentions			; then try CHS_read();

check_for_bootable_partition:
		cmp		word[LOAD+MAGIC_OFF],MAGIC	; if ( ! volume.isBootable() )
		jne		no_bootable_active_partition; then error();

jump_PBR:
		%ifdef DEBUG
		puts	kMsgBootPBR
		call	_pause
		%else
		puts	kMsgStart
		%endif

		; jump to 0x7c00 with :
		;	- CS=0
		;	- DL=drive number
		;	- DS:SI=pointer to the selected partition table
		;			entry (required by some PBR)

		; dl has not changed yet, still contains the drive ID
		mov		si,		[boot_partition]
		jmp		LOAD						; jump into partition boot loader

no_disk_extentions:
		%ifdef DEBUG
		puts	kMsgNoExtentions
		%endif

		mov		si,		[boot_partition]	; Restore active partition pointer

		;load CHS PBR sector info
		mov		dh,		[si+1]				; dh 7:0 = head 7:0   (0 - 255)
		mov		cx,		[si+2]				; cl 5:0 = sector 7:0 (1 - 63)
											; cl 7:6 = cylinder 9:8	(0 - 1023)
											; ch 7:0 = cylinder 7:0
	.check_Sector:
		mov		al,		cl
		and		al,		0x3F				; extract sector
		test	al,		al;					; if (sector==0) 
		jz		no_bootable_active_partition; then error("invalid sector");
		cmp		al,		0x3F				; if(  (Sector == 63)
		jne		.CHS_valid					;
	.check_Cylinder:
		mov		ax,		cx
		shr		ah,		6
		cmp		ax,		0x03FF				;	and	 (Cylinder == 1023)
		jne		.CHS_valid
	.check_Head:
		cmp		dh,		0xFF				;	and  ( (Head == 255)
		jne		.CHS_valid
		cmp		dh,		0xFE				;	    or (Head == 254) )  )
		je		no_bootable_active_partition; then error("invalid CHS_adress");

	.CHS_valid:
		; dl has not changed yet, still contains the drive ID
		mov		bx,		LOAD				; set buffer
		mov		al,		SECTOR_COUNT		; set Size
		mov		ah,		READ_DISK_SECTORS	; set read command
		int		BIOS_DISK_SERVICES			; if ( do_command() == OK )
		jnc		check_for_bootable_partition; then resume(normal boot sequence)
											; else continue;

no_bootable_active_partition:
		mov		si, kMsgNoBootable
		;jmp _error							; _error is the next line !

_error:
	; display a non-empty null-terminated string on the screen,
	; wait for a key pressed and go back to the bios
	; IN :
	;	- si = address of string to display
	; OUT :
	; DIRTY :
	;	- ax
	;	- si
		call	_puts
		call	_pause
		puts	kMsgROMBASIC
		int		BIOS_BASIC					; BIOS_BASIC give the control back
											; to the BIOS. Doing so, let some
											; BIOSes try to boot an alternate 
											; device (PXE/CD/HDD : see your 
											; BIOS' boot device order )
_pause:
	; wait for a key pressed
	; IN :
	; OUT :
	; DIRTY :
	;	- ax
		mov		ah,		READ_CHAR
		int		BIOS_KEYBOARD_SERVICES
		ret

_puts:
	; display a non-empty null-terminated string on the screen
	; IN :
	;	- si = address of string to display
	; OUT :
	; DIRTY :
	;	- ax
	;	- bx
	;	- si
		xor		bx,		bx					; bx:=0
	.loop:									; do {
		lodsb								;	al=[si++];
		mov		ah,		WRITE_CHAR			;	
		int		BIOS_VIDEO_SERVICES			;	WRITE_CHAR(al)
		or		al, al						; } while (al<>0);
		jnz		.loop
		ret

data:
		kMsgROMBASIC	db	'ROM BASIC',0
		kMsgNoBootable	db	'No bootable active volume',13,10,0
		kMsgStart		db	'Loading system',10,13,0

	%ifdef DEBUG
		kMsgBootPBR		db	'JMP PBR',13,10,0
		kMsgRead_Ex		db	'Read_ex block',13,10,0
		kMsgCheckEx		db	'CheckEx',13,10,0
		kMsgNoExtentions db	'Read block',13,10,0
		kMsgReloc		db	'reloc MBR',13,10,0
	%endif

; check whether the code is small enough to fit in the boot code area
end:
		;use nasm instead of yasm to check  the code size
		;%if end - start > DISKSIG
		;	%error "Code exceeds master boot code area!"
		;%endif

%ifdef MBR_CODE_ONLY
	;just build the code.
	;Do not generate the datas
%else
		;use nasm instead of yasm => use %rep instead of times
		;%rep start + DISKSIG - end
		;	db	0					;fill the rest of the code area
		;%endrep
		times start + DISKSIG - end db 0
		kMbrDiskID	dd	0			;Disk signature
		dw 0						;reserved
		PartitionTable times PartitionEntry_size * 4 db 0
DiskSignature:
		;use nasm instead of yasm to check  the MAGIC offset
		;%if DiskSignature - start <> MAGIC_OFF
		;	%error "incorrect Disk Signature offset"
		;%endif
		kMbrSignature	db	0x55, 0xAA
%endif
