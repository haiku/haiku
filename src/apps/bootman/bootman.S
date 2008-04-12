;
; Copyright 2007, Dengg David, david-d@gmx.at. All rights reserved.
; Distributed under the terms of the MIT License.
; 	
%assign	BIOS_VIDEO_SERVICES			0x10
%assign	BIOS_KEYBOARD_SERVICES		0x16
%assign	WRITE_CHAR						0x0e	; al - char
												; bh - page
												; bl - foreground color (graphics mode only)

[bits 16]						; Real Mode yeah
start:
; ======================= SCREEN SETUP =================================
	mov ax, 0x07C0					; Segment Location 0x07C0
	mov ds, ax					; Set Data Segment to 0x07C0
	mov es, ax					; Set Extra Segment to 0x07C0
	mov ss, ax					; Set Stack Segment to 0x07C0
	mov sp, 0xFFFF 

	mov ah, 0x00					; Set Screen Resolution to 640x480
	mov al, 0x12
	mov bx, 0x0
	mov cx, 0x1
	int 0x10

 	mov ah, 0x02					; Set cursor position
 	mov dx, 0x0120
 	int 0x10


 	mov si, signation				; Print "Haiku Bootmanager"
	call printstr


 	mov ah, 0x02					; Set cursor position
 	mov dx, 0x1B1B
 	int 0x10

 	mov si, select_os				; Print "Select Os..."
	call printstr

	mov ah, 0
	int 0x16

; ======================= LOAD 3 MORE SECTORS INTO MEM =================
load_from_disk:						; load 3 more sectors into mem
	mov ah, 0x42
	mov dl, 0x80
	mov si, DAP
	int 0x13
	mov si, error
	jc printstr
	jmp secsec					; Jump into the second sector

ende:
	mov ah, 0x0e				
	mov al, 'F'
	mov bx, 0x2
	mov cx, 0x1
	int 0x10
	jmp $						; Final Loopy. Never reached

%if 0
printstr:						; Poor mans printf
	lodsb
	mov ah, 0x0E
	mov bx, 0x0F
	mov cx, 0x01
	int 0x10
	cmp al, 0x00
	jnz printstr
	ret
%else
printstr:
	; page and foreground color
	jmp .loop_condition
.loop
	mov ah, WRITE_CHAR
	mov bx, 0x0F
	mov cx, 1
	int BIOS_VIDEO_SERVICES
.loop_condition	
	lodsb
	cmp al, 0
	jnz .loop
	ret
%endif
		
DAP:
	db 0x10						; Size of DAP
	db 0x00						; unused (zero)
	db 0x03						; number of sectors to read
	db 0x00						; unused (zero)
	dw 0x0200					; segment:offset pointer to buffer
	dw 0x07C0					; segment:offset pointer to buffer
	dw 0x0001					; LBA address
	dw 0x0000
	dw 0x0000
	dw 0x0000
		
WinDAP:
	db 0x0B						; Size of DAP
	db 0x00						; unused (zero)
	db 0x01						; number of sectors to read
	db 0x00						; unused (zero)
	dw 0x0000					; segment:offset pointer to buffer
	dw 0x07C0					; segment:offset pointer to buffer
	dw 0x003F					; LBA address
	dw 0x0000


signation:
	db 'Haiku Bootmanager', 0x00
select_os:
	db 'Select an OS from the menu', 0x00
	
	times 510 -($-$$) db 'B'			; Fill the missing space to reach 510 byte and set the 
	dw 0xAA55 					; magic marker "AA55" (to identify a valid bootrecord)

; ======================================================================
; ======================= SECOND SECTOR ================================
; ======================================================================

secsec:
; ======================= Center list ==================================
	xor bx,bx					; Get ready for the Division
	xor ax, ax					; Get ready for the Division
	mov al, [list_items]
 	mov bl, 2					; Divide by two
	div bl						; Divide

	mov bl, 12
	sub bl, al					; Substract from 12 (screen is 80x25)
	mov dh, bl
	mov [first_position], dx

; ======================= Print the OS list ============================
print_list:
	mov dx, [first_position]
	mov [position], dx

	mov ah, 0x02					; Set cursor position
	int 0x10
	mov si, list					; Load the List
	xor cx,cx					; Load listitems to CX
	mov byte [colour], 0x0000	; Reset Colour
print_list_loop:				
	lodsb						; Get lenght of string from list into al
							; Get the Text to be in the middle (40 - lenght/2)
	xor bx,bx					; Get ready for the Division
	mov ah, 0x00 
 	mov bl, 2					; Divide by two
	div bl						; Divide

	mov bl, 40
	sub bl, al					; Substract from 40 (screen is 80x25)
	mov al, bl
	mov dx, [position]				; Get last position
	inc dh						; Next line
	mov dl, al					; Move it to the place where the row should be for the interrupt (DL)
	
	mov ah, 0x02					; Set cursor position
	mov [position], dx
	int 0x10
	
	mov dx, [selection]
	mov byte bl, [colour]				; Load Colour
	and bx, 0x03					; Just
	inc bl						; Colours ... 1-4
	cmp cx, [selection]
	jnz no_highlight
	xor bx, 0x08					; Highlight selected item
no_highlight:
	mov byte [colour], bl				; Store Colour
	call print_list_item
	inc cx
	cmp cx, [list_items]
	jne print_list_loop
	
; ================== Wait for a key and do something with it ==================

main_loopy:
	mov ah, 0x02					; Set cursor position
 	mov dx, 0x181C
 	int 0x10
 	
 	mov ah, 0x00   					; Keyboard Read: wait for key
	int 0x16      					; AL = ASCII Code, AH = Scancode
	cmp	ah, 0x50
	jz 	key_down				; Was it key_down, key_up... ?
	cmp	ah, 0x48
	jz 	key_up
	cmp ah, 0x1C
	jz 	key_enter

 	jmp main_loopy

key_down:
	mov ax, [selection]
	mov bx, [list_items]
	dec bx
	cmp ax, bx
	je	key_down_no				; Are we at the bottom of the list?
	inc ax
	mov [selection], ax
key_down_no:
	jmp print_list
key_up:
	mov ax, [selection]
	mov bx, [list_items]
	cmp ax, 0
	je 	key_down_no				; Are we at the top of the list?
	dec ax
	mov [selection], ax
key_up_no:
	jmp print_list	


; ========================== Chainload ==========================

key_enter:
	mov cx, [selection]
	mov si, list
	inc cx
load_LBA:
	lodsb 
	cmp al, 0x80
	jne load_LBA
	loop load_LBA
	
	lodsw
	mov [LBA_selectionA], ax
	lodsw
	mov [LBA_selectionB], ax
	
	mov si, ChainLoad_DAP				; Load the List
	mov ax, [LBA_selectionA]
	mov word [si+8], ax
	mov ax, [LBA_selectionB]
	mov word [si+10], ax

	mov ah, 0x42
	mov dl, 0x80
	mov si, ChainLoad_DAP
	int 0x13
	mov si, error_lfd
	jc print_debug

	mov si, 0x0
	xor ax, ax
	mov si, error_nb
	mov byte al, [0x01FE]
	cmp al, 0x55
	jne print_debug
	
	mov byte al, [0x01FF]
	cmp al, 0xAA
	jne print_debug

	mov si, yeah					; Yeah
	jmp print_debug

	


fin:
	mov ah, 0x0e
	mov al, 'F'
	mov bx, 0x2
	mov cx, 0x1
	int 0x10
	jmp $


; ====================== THE print_list function ================

print_list_item:
print_list_item_loop:
	lodsb
	cmp al,0x80
	jz print_list_item_end
	mov ah, 0x0E
	int 0x10
	jmp print_list_item_loop
print_list_item_end:
	lodsw
	lodsw
	ret

print_debug:						; Poor mans printf again
	lodsb
	mov ah, 0x0E
	mov bx, 0x0F
	mov cx, 0x01
	int 0x10
	cmp al, 0x00
	jnz print_debug
	jmp $
	
; ================================ DATA ===========================

error_lfd:
	db 'Error loading Sectors', 0x00
error_nb:
	db 'Not a bootable partition', 0x00
LBA_selectionA:
	dw 0x0000	
LBA_selectionB:
	dw 0x0000
selection:
	dw 0x0000
position:
	dw 0x091D
lenght:
	dw 0x0000
colour:
	dw 0x0000
first_position:
	dw 0x091F
nobots:
	db 'Boot sector signature not found', 0x00
error:
	db 'Error loading sectors', 0x00
yeah:
	db 'Boot!', 0x00



list_items:
	dw 0x06
list:
	db 0x05
	db 'HAIKU' , 0x80
	dw 0x003E
	dw 0x0000

	db 0x07
	db 'FreeBSD' , 0x80
	dw 0x003F
	dw 0x0000

	db 0x03
	db 'DOS' , 0x80
	dw 0x003E
	dw 0x0000

	db 0x05
	db 'LINUX' , 0x80
	dw 0x003F
	dw 0x0000

	db 0x07
	db 'BeOS R5' , 0x80
	dw 0x003F
	dw 0x0000

	db 0x07
	db 'OpenBSD' , 0x80
	dw 0xAAAA
	dw 0x0000

ChainLoad_DAP:
	db 0x10						; Size of DAP
	db 0x00						; unused (zero)
	db 0x01						; number of sectors to read
	db 0x00						; unused (zero)
	dw 0x0000					; segment:offset pointer to buffer
	dw 0x07C0					; segment:offset pointer to buffer
	dw 0x003F					; LBA address
	dw 0x0000
	dw 0x0000
	dw 0x0000




