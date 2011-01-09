;
; Copyright 2007, Dengg David, david-d@gmx.at. All rights reserved.
; Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
; Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
; Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
; Distributed under the terms of the MIT License.


%assign USE_TEST_MENU				0

%assign BOOT_BLOCK_START_ADDRESS	0x7c00

%assign MBR_SIGNATURE				0xAA55

; BIOS calls

%assign BIOS_VIDEO_SERVICES			0x10
%assign BIOS_DISK_SERVICES			0x13
%assign BIOS_KEYBOARD_SERVICES		0x16
%assign BIOS_REBOOT					0x19		; dl - boot drive number
%assign BIOS_TIME_SERVICES			0x1A

; video services
%assign SET_VIDEO_MODE					0x00	; al - mode

%assign	SET_CURSOR_SHAPE				0x01	; ch - starting scan line (5 bits)
												; cl - ending scan line (5 bits)

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

%assign	GET_MODIFIER_KEYS				0x02	;-> al - modifier key bitmask

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

; Characters
%assign	TRIANGLE_TO_RIGHT				16
%assign	TRIANGLE_TO_LEFT				17

; Key codes
%assign KEY_DOWN						0x50
%assign KEY_UP							0x48
%assign KEY_RETURN						0x1C

; Modifier key bitmasks
%assign MODIFIER_RIGHT_SHIFT_KEY		0x01
%assign MODIFIER_LEFT_SHIFT_KEY			0x02
%assign MODIFIER_CONTROL_KEY			0x04
%assign MODIFIER_ALT_KEY				0x08
%assign MODIFIER_SCROLL_LOCK_KEY		0x10
%assign MODIFIER_NUM_LOCK_KEY			0x20
%assign MODIFIER_CAPS_LOCK_KEY			0x40
%assign MODIFIER_INSERT_KEY				0x80

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

; Variables on stack
struc	Locals
	selection		resw	1
	firstLine		resb	2 ; low byte used only
	timeoutTicks	resd	1
	cursorX			resb	1
	cursorY			resb	1
	cursorShape		resw	1
	biosDrive		resb	1
endstruc

cursorPosition		equ cursorX

%macro DEBUG_PAUSE 0
	push	ax
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES
	pop		ax
%endmacro

%macro CLEAR_SCREEN 0
	mov		ah, SCROLL_UP
	xor		al, al
	mov		bh, WHITE
	xor		cx, cx
	mov		dx, (TEXT_ROWS-1) * 0x100 + (TEXT_COLUMNS-1)
	int		BIOS_VIDEO_SERVICES
%endmacro

; Prints a null terminated string
; bl ... color
; si ... offset to string
%macro PRINT_STRING 0
	push	ax
	push	bx
	push	cx
	push	dx
	xor		bh, bh								; write on page 0
	jmp		.loop_condition
.loop
	mov		dx, [bp + cursorPosition]
	mov		ah, SET_CURSOR
	int		BIOS_VIDEO_SERVICES

	inc		byte [bp + cursorX]

	mov		cx, 1
	mov		ah, WRITE_CHAR
	int		BIOS_VIDEO_SERVICES
.loop_condition
	lodsb
	cmp		al, 0
	jnz		.loop
	pop		dx
	pop		cx
	pop		bx
	pop		ax
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

struc	BootLoaderAddress
	.device			resb	1			; hard drive number
	.offset			nstruc	quadword	; LBA of start start sector
endstruc

; use code available in stage 1
%define printstr printStringStage1

stage1:
	mov		ax, 0x07c0						; BOOT_BLOCK_START_ADDRESS / 16
	mov		ds, ax							; Setup segment registers
	mov		es, ax
	mov		ss, ax

	mov		sp, 0xFFFF - sizeof(Locals)		; Make stack empty
	mov		bp, sp

	mov		[bp + biosDrive], dl			; Store boot drive
	cld										; String operations increment index
											; registers
	CLEAR_SCREEN
	call	hideCursor

	mov		bh, 0							; Text output on page 0

	; Print title centered at row 2
	mov		dx, 1 * 0x100 + (40 - TITLE_LENGTH / 2)
	mov		[bp + cursorPosition], dx

	mov		si, kTitle
	mov		bl, WHITE
	call	printstr

	; Print message centered at second last row
	mov		dx, (TEXT_ROWS-2) * 0x100 + (40 - SELECT_OS_MESSAGE_LENGTH / 2)
	mov		[bp + cursorPosition], dx

	mov		bl, LIGHT_GRAY
	mov		si, kSelectOSMessage
	call	printstr

	; Chain load rest of boot loader
	mov		ah, EXTENDED_READ				; Load 3 more sectors
	mov		dl, [bp + biosDrive]
	mov		si, nextStageDAP
	int		BIOS_DISK_SERVICES
	jc		.error							; I/O error
	jmp		stage2							; Continue in loaded stage 2

.error:
	call	showCursor
	mov		si, kError
	mov		bl, RED
	call	printstr

	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES

	mov		dl, [bp + biosDrive]
	int		BIOS_REBOOT

printStringStage1:
	PRINT_STRING

hideCursor:
	mov		ah, GET_CURSOR
	int		BIOS_VIDEO_SERVICES
	mov		[bp + cursorShape], cx

	mov		ah, SET_CURSOR_SHAPE
	mov		cx, 0x2000
	int		BIOS_VIDEO_SERVICES
	ret

showCursor:
	mov		cx, [bp + cursorShape]
	mov		ah, SET_CURSOR_SHAPE
	int		BIOS_VIDEO_SERVICES
	ret

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

kStage1UnusedSpace	equ	440 - ($-$$)
	; Fill the missing space to reach byte 440
	times kStage1UnusedSpace db 'B'

kDiskSignatue:
	dw		0, 0
kReserved
	dw		0
kPartitionTable
	times	64 db 0

kMBRSignature:
	; Magic marker "AA55" (to identify a valid boot record)
	dw		MBR_SIGNATURE

; ======================================================================
; ======================= SECOND SECTOR ================================
; ======================================================================

; Use code available in stage 2
%define printstr printStringStage2

%assign	TIMEOUT_OFF		0xffff


stage2:
	mov		ax, [defaultItem]					; Select default item
	mov		[bp + selection], ax

	mov		ax, TICKS_PER_SECOND				; Calculate timeout ticks
	mul		word [timeout]
	mov		bx, dx
	push	ax

	mov		ah, READ_CLOCK
	int		BIOS_TIME_SERVICES

	pop		ax									; Add current ticks
	add		ax, dx
	adc		bx, cx
	mov		[bp + timeoutTicks], ax
	mov		[bp + timeoutTicks + 2], bx

	mov		al, [listItemCount]					; Calculate start row for menu
	shr		al, 1
	mov		bl, TEXT_ROWS / 2
	sub		bl, al								; y = TEXT_ROWS / 2 - number of items / 2
	mov		[bp + firstLine], bl

	mov		ah, GET_MODIFIER_KEYS				; Disable timeout if ALT key is pressed
	int		BIOS_KEYBOARD_SERVICES
	and		al, MODIFIER_ALT_KEY
	jz		showMenu
	mov		word [timeout], TIMEOUT_OFF

showMenu:
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
	cmp		cx, [bp + timeoutTicks + 2]
	jb		.returnFalse
	ja		.returnTrue
	cmp		dx, [bp + timeoutTicks]
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
	jne		inputLoop
	jmp		bootSelectedPartition

selectNextPartition:
	mov		ax, [bp + selection]
	inc		ax
	cmp		ax, [listItemCount]
	jne		.done								; At end of list?
	xor		ax, ax								; Then jump to first entry
.done:
	mov		[bp + selection], ax
	jmp		mainLoop

selectPreviousPartition:
	mov		ax, [bp + selection]
	or		ax, ax
	jnz		.done								; At top of list?
	mov		ax, [listItemCount]					; Then jump to last entry
.done:
	dec		ax
	mov		[bp + selection], ax
	jmp		mainLoop


; ======================= Print the OS list ============================
printMenu:
	mov		al, [bp + firstLine]
	mov		[bp + cursorY], al

	mov		si, list							; Start at top of list
	xor		cx, cx								; The index of the current item

.loop:
	lodsb										; String length incl. 0-terminator
	add		al, 3								; center menu item
	shr		al, 1								; x = TEXT_COLUMNS / 2 - length / 2
	mov		dl, TEXT_COLUMNS / 2
	sub		dl, al
	mov		[bp + cursorX], dl

	mov		al, TRIANGLE_TO_RIGHT
	call	updateMarker
	inc		byte [bp + cursorX]

	mov		di, cx
	and		di, 3
	mov		bl, [kColorTable + di]				; Text color

	cmp		cx, [bp + selection]
	jne		.print								; Selected item reached?
	xor		bl, BRIGHT_COLOR_MASK				; Highlight it

.print:
	call	printstr
	add		si, sizeof(BootLoaderAddress)

	add		byte [bp + cursorX], 1
	mov		al, TRIANGLE_TO_LEFT
	call	updateMarker

	inc		byte [bp + cursorY]
	inc		cx

	cmp		cx, [listItemCount]
	jne		.loop
	ret

updateMarker:
	cmp		cx, [bp + selection]
	je		.print
	mov		al, ' '								; Clear marker
.print
	mov		bl, WHITE
	jmp		printChar							; return from subroutine


; ========================== Chainload ==========================

bootSelectedPartition:

	call	showCursor

	call	getSelectedBootLoaderAddress
	lodsb										; Set boot drive
	mov		dl, al

	mov		di, bootSectorDAP+AddressPacket.offset	; Copy start sector
	mov		cx, 4								; It is stored in a quad word
.copy_start_sector
	lodsw
	stosw
	loop .copy_start_sector

	mov		ah, EXTENDED_READ					; Now read start sector from HD
	mov		si, bootSectorDAP
	int		BIOS_DISK_SERVICES
	mov		si, kReadError
	jc		printAndHalt						; Failed to read sector

	mov		ax, [kMBRSignature]
	cmp		ax, MBR_SIGNATURE
	mov		si, kNoBootablePartitionError
	jne		printAndHalt						; Missing signature

	CLEAR_SCREEN

	; Print "Loading <name>" at top of screen
	mov		word [bp + cursorPosition], 0
	mov		si, kLoadingMessage
	mov		bl, LIGHT_GRAY
	call	printstr

	inc		byte [bp + cursorX]
	call	getSelectedMenuItem
	inc		si									; Skip string length byte
	call	printstr

	mov		dx, 0x100
	xor		bh, bh
	mov		ah, SET_CURSOR
	int		BIOS_VIDEO_SERVICES

	call	getSelectedBootLoaderAddress
	mov		dl, [si]							; drive number in dl

	jmp		$$									; Start loaded boot loader


printAndHalt:
	mov		dx, (TEXT_ROWS-4) * 0x100 + (TEXT_COLUMNS / 3)
	mov		[bp + cursorPosition], dx

	mov		bx, 0x0F							; Page number and foreground color
	call	printstr
	mov		ah, READ_CHAR
	int		BIOS_KEYBOARD_SERVICES
	mov		dl, [bp + biosDrive]
	int		BIOS_REBOOT

; Output:
;	si	address of selected menu item
; Trashes:
;	ax, cx
getSelectedMenuItem:
	mov		si, list							; Search address of start sector
												; of the selected item.
	mov		cx, [bp + selection]
	inc		cx									; Number of required iterations

	xor		ah, ah								; The high-byte of the string length
												; see loop body
	jmp		.entry

.loop:
	lodsb										; Length of menu item name
	add		si, ax								; Skip name to BootLoaderAddess
	add		si, sizeof(BootLoaderAddress)

.entry:
	loop	.loop
	ret

getSelectedBootLoaderAddress:
	call	getSelectedMenuItem
	lodsb
	xor		ah, ah
	add		si, ax								; Skip name
	mov		dl, [si]
	test	dl, 0								; if drive is 0, use boot drive
	jz		.takeOverBootDrive
	ret
.takeOverBootDrive:
	mov		dl, [bp + biosDrive]
	mov		[si], dl
	ret

printStringStage2:
	PRINT_STRING

; al ... ASCII character
; bl ... color
printChar:
	push	ax
	push	bx
	push	cx
	push	dx

	xor		bh, bh								; Write on page 0

	mov		dx, [bp + cursorPosition]
	mov		ah, SET_CURSOR
	int		BIOS_VIDEO_SERVICES

	inc		byte [bp + cursorX]

	mov		cx, 1
	mov		ah, WRITE_CHAR
	int		BIOS_VIDEO_SERVICES

	pop		dx
	pop		cx
	pop		bx
	pop		ax
	ret

; ================================ DATA ===========================

bootSectorDAP:
	istruc AddressPacket
		at AddressPacket.packet_size,	db		0x10
		at AddressPacket.block_count,	db		0x01
		at AddressPacket.buffer,		dw		0x0000, 0x07c0
	iend

kColorTable:
	db BLUE, RED, GREEN, CYAN
kReadError:
	db		'Error loading sectors', 0x00
kNoBootablePartitionError:
	db		'Not a bootable partition', 0x00
kLoadingMessage:
	db		'Loading', 0x00


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
; db hard drive number
; quadword start sector

%if USE_TEST_MENU
	dw		0x06

	dw		2

	dw		5

	db		0x06
	db		'HAIKU', 0
	db		0x80
	dw		1, 0, 0, 0

	db		0x08
	db		'FreeBSD', 0
	db		0x80
	dw		0x003F, 0, 0, 0

	db		0x04
	db		'DOS', 0
	db		0x80
	dw		0x003E, 0, 0, 0

	db		0x06
	db		'LINUX', 0
	db		0x80
	dw		0x003F, 0, 0, 0

	db		0x08
	db		'BeOS R5', 0
	db		0x80
	dw		0x003F, 0, 0, 0

	db		0x07
	db		'OpenBSD', 0
	db		0x80
	dw		0xAAAA, 0, 0, 0

	dw		kStage1UnusedSpace
%endif

