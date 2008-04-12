;
; Copyright 2007, Dengg David, david-d@gmx.at. All rights reserved.
; Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
; Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
; Distributed under the terms of the MIT License.

; Steps to create BootLoader.h
; 1. nasm -f bin bootman.S -o bootman.bin
; 2. cc MakeArray.cpp -o make_array -lstdc++
; 3. ./make_array kBootLoader bootman.bin > BootLoader.h
; or
; nasm -f bin bootman.S -o bootman.bin && ./make_array kBootLoader bootman.bin > BootLoader.h

%assign USE_TEST_MENU				0

%assign BOOT_BLOCK_START_ADDRESS	0x7c00

%assign MBR_SIGNATURE				0xAA55

; BIOS calls

%assign BIOS_VIDEO_SERVICES			0x10
%assign BIOS_DISK_SERVICES			0x13
%assign BIOS_KEYBOARD_SERVICES		0x16
%assign BIOS_REBOOT					0x19
%assign BIOS_TIME_SERVICES			0x1A

; video services
%assign SET_VIDEO_MODE					0x00	; al - mode

%assign SET_CURSOR						0x02	; dl - column
												; dh - row
												; bh - page


%assign GET_CURSOR						0x03	; bh - page
												; -> dl - column
												;	dh - row
												;	Cursor shape:
												;	ch - starting scan line
												;	cl - ending scan line

%assign	SCROLL_UP						0x06	; al - lines (0: clear screen)
												; bh - attribute
												; ch - upper line
												; cl - left column
												; dh - lower line
												; dl - right column

%assign WRITE_CHAR						0x09	; al - char
												; bh - page
												; bl - attribute
												; cx - count

;%assign WRITE_CHAR						0x0e	; al - char
												; bh - page
												; bl - foreground color (graphics mode only)

; disk services
%assign READ_DISK_SECTORS				0x02	; dl	- drive
												; es:bx - buffer
												; dh	- head (0 - 15)
												; ch	- track 7:0 (0 - 1023)
												; cl	- track 9:8,
												;		 sector (1 - 17)
												; al	- sector count
												; -> al - sectors read
%assign READ_DRIVE_PARAMETERS			0x08	; dl - drive
												; -> cl - max cylinder 9:8
												;	   - sectors per track
												;	ch - max cylinder 7:0
												;	dh - max head
												;	dl - number of drives (?)
%assign CHECK_DISK_EXTENSIONS_PRESENT	0x41	; bx - 0x55aa
												; dl - drive
												; -> success: carry clear
												;	ah - extension version
												;	bx - 0xaa55
												;	cx - support bit mask
												; -> error: carry set
%assign EXTENDED_READ					0x42	; dl - drive
												; ds:si - address packet
												; -> success: carry clear
												; -> error: carry set

%assign FIXED_DISK_SUPPORT				0x1	 	; flag indicating fixed disk
												; extension command subset

; keyboard services
%assign READ_CHAR						0x00	; -> al - ASCII char
												;	ah - scan code

%assign	PROBE_CHAR						0x01	; -> zf = 0
												;	al - ASCII char
												;	ah - scan code
; timer services
%assign READ_CLOCK						0x00	; -> cx	- high word
												;	dx - low word
												;	one tick = 1/18.2s

%assign	TICKS_PER_SECOND				19

; video modes
%assign GRAPHIC_MODE_80x25				0x12	; 640 x 480 graphic mode

%assign TEXT_COLUMNS					80		; Number of columns
%assign TEXT_ROWS						25		; Number of rows

; Colors
%assign	BLACK							0
%assign	BLUE							1
%assign	GREEN							2
%assign CYAN							3
%assign RED								4
%assign MAGENTA							5
%assign BROWN							6
%assign LIGHT_GRAY						7
%assign	DARK_GRAY						8
%assign	LIGHT_BLUE						9
%assign	LIGHT_GREEN						10
%assign	LIGHT_CYAN						11
%assign	LIGHT_RED						12
%assign	LIGHT_MAGENTA					13
%assign	YELLOW							14
%assign	WHITE							15

%assign BRIGHT_COLOR_MASK				8


; Key codes
%assign KEY_DOWN						0x50
%assign KEY_UP							0x48
%assign KEY_RETURN						0x1C

; String constants with their length
%define TITLE							'Haiku Boot Manager'
%strlen TITLE_LENGTH					TITLE
%define SELECT_OS_MESSAGE				'Select an OS from the menu'
%strlen SELECT_OS_MESSAGE_LENGTH		SELECT_OS_MESSAGE 

; 16 bit code
SECTION
BITS 16


; nicer way to get the size of a structure
%define sizeof(s)	s %+ _size

; using a structure in a another structure definition
%macro  nstruc  1-2	 1
					resb	sizeof(%1) * %2
%endmacro

%macro DEBUG_PAUSE 0
	push	ax
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES
	pop		ax
%endmacro

%macro clearScreen 0
	mov		ah, SCROLL_UP
	xor		al, al
	mov		bh, WHITE
	xor		cx, cx
	mov		dx, (TEXT_ROWS-1) * 0x100 + (TEXT_COLUMNS-1)
	int		BIOS_VIDEO_SERVICES
%endmacro

; Prints a null terminated string
; si ... offset to string
%macro printString 0
	push	cx
	jmp		.loop_condition
.loop
	mov		cx, 1
	mov		ah, WRITE_CHAR
	int		BIOS_VIDEO_SERVICES
	mov		ah, GET_CURSOR
	int		BIOS_VIDEO_SERVICES
	inc		dl
	mov		ah, SET_CURSOR
	int		BIOS_VIDEO_SERVICES	
.loop_condition 
	lodsb
	cmp		al, 0
	jnz		.loop
	pop		cx
	ret
%endmacro

; 64 bit value
struc   quadword
	.lower			resd	1
	.upper			resd	1
endstruc

; address packet as required by the EXTENDED_READ BIOS call
struc   AddressPacket
	.packet_size	resb	1
	.reserved1		resb	1
	.block_count	resb	1
	.reserved2		resb	1
	.buffer			resd	1
	.offset			nstruc	quadword
endstruc

; use code available in stage 1
%define printstr printStringStage1

stage1:
	mov		ax, 0x07C0							; BOOT_BLOCK_START_ADDRESS / 16
	mov		ds, ax								; Setup segment registers
	mov		es, ax
	mov		ss, ax
	
	mov		sp, 0xFFFF							; Make stack empty
	
	cld											; String operations increment index registers
	
	clearScreen

	mov		bh, 0								; Text output on page 0
	
	; Print title centered at row 2 
	mov		ah, SET_CURSOR
	mov		dx, 1 * 0x100 + (40 - TITLE_LENGTH / 2)
	int		BIOS_VIDEO_SERVICES

	mov		si, kTitle
	mov		bl, WHITE
	call	printstr

	; Print message centered at second last row
	mov		ah, SET_CURSOR
	mov		dx, (TEXT_ROWS-2) * 0x100 + (40 - SELECT_OS_MESSAGE_LENGTH / 2)
	mov		bl, LIGHT_GRAY 
	int		BIOS_VIDEO_SERVICES

	mov		si, kSelectOSMessage
	call	printstr

	; Chain load rest of boot loader 
	mov		ah, EXTENDED_READ					; Load 3 more sectors
	mov		dl, 0x80							; First HDD
	mov		si, nextStageDAP
	int		BIOS_DISK_SERVICES
	jc		.error								; I/O error
	jmp		stage2								; Continue in loaded stage 2

.error: 
	mov		si, kError
	mov		bl, RED
	call	printstr
.halt
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES	
	hlt
	jmp		.halt

printStringStage1:
	printString
		
nextStageDAP: 
	istruc AddressPacket
		at AddressPacket.packet_size,	db		0x10
		at AddressPacket.block_count,	db		0x03
		at AddressPacket.buffer,		dw		0x0200, 0x07c0
		at AddressPacket.offset,		dw		1
	iend

kTitle:
	db		TITLE, 0x00
kSelectOSMessage:
	db		SELECT_OS_MESSAGE, 0x00
kError:
	db		'Error loading sectors!', 0x00

	; Fill the missing space to reach 510 byte
	times 510 -($-$$) db 'B'

signature:
	; Magic marker "AA55" (to identify a valid boot record)
	dw		MBR_SIGNATURE

; ======================================================================
; ======================= SECOND SECTOR ================================
; ======================================================================

; Use code available in stage 1
%define printstr printStringStage2

%assign	TIMEOUT_OFF	0xffff


stage2:
	mov		ax, [defaultItem]					; Select default item
	mov		[selection], ax
	
	mov		ax, TICKS_PER_SECOND				; Calculate timeout ticks
	mul		word [timeout]
	mov		bx, dx
	push	ax
	
	mov		ah, READ_CLOCK
	int		BIOS_TIME_SERVICES
	
	pop		ax									; Add current ticks
	add		ax, dx
	adc		bx, cx
	mov		[timeoutTicks], ax
	mov		[timeoutTicks + 2], bx
	
	mov		al, [listItemCount]					; Calculate start row for menu
	shr		al, 1
	mov		bl, TEXT_ROWS / 2
	sub		bl, al								; y = TEXT_ROWS / 2 - number of items / 2
	mov		[firstLine], bl

	call	printMenu

	cmp		word [timeout], TIMEOUT_OFF
	je		inputLoop

timeoutLoop:
	mov		ah, PROBE_CHAR
	int		BIOS_KEYBOARD_SERVICES
	jnz		inputLoop							; cancel timeout if key is pressed
	call	isTimeoutReached
	jnc		timeoutLoop
	jmp		bootSelectedPartition

isTimeoutReached:
	mov		ah, READ_CLOCK
	int		BIOS_TIME_SERVICES
	cmp		cx, [timeoutTicks + 2]
	jb		.returnFalse
	ja		.returnTrue
	cmp		dx, [timeoutTicks]
	ja		.returnTrue
.returnFalse:
	clc
	ret
.returnTrue:
	stc
	ret
	
; ================== Wait for a key and do something with it ==================
mainLoop:
	call	printMenu
	
inputLoop:
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES				; AL = ASCII Code, AH = Scancode

	cmp		ah, KEY_DOWN
	je		selectNextPartition			 
	
	cmp		ah, KEY_UP
	je		selectPreviousPartition
	
	cmp		ah, KEY_RETURN
	je		bootSelectedPartition
	
	jmp		inputLoop

selectNextPartition:
	mov		ax, [selection]
	inc		ax
	cmp		ax, [listItemCount]
	jne		.done								; At end of list?
	xor		ax, ax								; Then jump to first entry
.done:
	mov		[selection], ax
	jmp		mainLoop
	
selectPreviousPartition:
	mov		ax, [selection]
	or		ax, ax
	jnz		.done								; At top of list?
	mov		ax, [listItemCount]					; Then jump to last entry		
.done:
	dec		ax
	mov		[selection], ax
	jmp		mainLoop   


; ======================= Print the OS list ============================
printMenu:
	mov		al, [firstLine]
	mov		[currentLine], al
	
	mov		si, list							; Start at top of list
	xor		cx, cx								; The index of the current item
	
.loop:
	lodsb										; String length incl. 0-terminator
	dec		al									; center menu item
	shr		al, 1								; x = TEXT_COLUMNS / 2 - length / 2
	mov		dl, TEXT_COLUMNS / 2
	sub		dl, al
	
	mov		dh, [currentLine]
	mov		ah, SET_CURSOR
	int		BIOS_VIDEO_SERVICES

	mov		di, cx
	and		di, 3
	mov		bl, [kColorTable + di]				; Text color

	cmp		cx, [selection]
	jne		.print								; Selected item reached?
	xor		bl, BRIGHT_COLOR_MASK				; Highlight it

.print:
	call	printstr
	add		si, sizeof(quadword)				; Skip start sector quad word

	inc		byte [currentLine]
	inc		cx

	cmp		cx, [listItemCount]
	jne		.loop
	
	mov		ah, SET_CURSOR						; Set cursor position
	mov		dx, (TEXT_ROWS-4) * 0x100 + (TEXT_COLUMNS / 3)
	mov		dx, 0x181C
	int		BIOS_VIDEO_SERVICES
	ret	


; ========================== Chainload ==========================

bootSelectedPartition:

	mov		si, list							; Search address of start sector  
												; of the selected item.
	mov		cx, [selection]
	inc		cx									; Number of required iterations
	
	xor		ah, ah								; The high-byte of the string length
												; see loop body
	jmp		.search_loop_entry
	
.search_loop:
	add		si, sizeof(quadword)				; Skip to begin of next menu item

.search_loop_entry:
	lodsb										; Length of menu item name
	add		si, ax								; Skip name to address of start sector
	loop	.search_loop
	
	mov		di, bootSectorDAP+AddressPacket.offset	; Copy start sector
	mov		cx, 4								; It is stored in a quad word
.copy_start_sector
	lodsw
	stosw
	loop .copy_start_sector

	mov		ah, EXTENDED_READ					; Now read start sector from HD
	mov		dl, 0x80							; overwriting our first stage
	mov		si, bootSectorDAP
	int		BIOS_DISK_SERVICES
	mov		si, kReadError
	jc		printAndHalt						; Failed to read sector

	mov		ax, [signature]
	cmp		ax, MBR_SIGNATURE
	mov		si, kNoBootablePartitionError
	jne		printAndHalt						; Missing signature
	
	jmp		$$									; Start loaded boot loader
	

; ====================== THE printMenu function ================

printStringStage2:
	printString

printAndHalt:
	mov		bx, 0x0F							; Page number and foreground color
	call	printstr
.halt
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES	
	hlt
	jmp		.halt
	
; ================================ DATA ===========================

selection: 
	dw		0x0000
firstLine: 
	db		0
currentLine: 
	db		0
timeoutTicks:
	dw		0, 0

bootSectorDAP:
	istruc AddressPacket
		at AddressPacket.packet_size,	db		0x10
		at AddressPacket.block_count,	db		0x01
		at AddressPacket.buffer,		dw		0x0000, 0x07c0
	iend

kColorTable:
	db BLUE, RED, CYAN, GREEN	
kReadError:
	db		'Error loading sectors', 0x00
kNoBootablePartitionError:
	db		'Not a bootable partition', 0x00
kBootMessage:
	db		'Boot!', 0x00


listItemCount:
defaultItem			equ		listItemCount + 2
timeout				equ		defaultItem + 2
list				equ		timeout + 2
	
; dw number of entries
; dw the default entry
; dw the timeout (-1 for none)
; entry:
; db size of partition name 0-terminated string
; db 0-terminated string with partition name
; quadword start sector

%if USE_TEST_MENU
	dw		0x06
	
	dw		2
	
	dw		5

	db		0x06
	db		'HAIKU', 0
	dw		1, 0, 0, 0

	db		0x08
	db		'FreeBSD', 0
	dw		0x003F, 0, 0, 0

	db		0x04
	db		'DOS', 0
	dw		0x003E, 0, 0, 0

	db		0x06
	db		'LINUX', 0
	dw		0x003F, 0, 0, 0

	db		0x08
	db		'BeOS R5', 0
	dw		0x003F, 0, 0, 0

	db		0x07
	db		'OpenBSD', 0
	dw		0xAAAA, 0, 0, 0
%endif

