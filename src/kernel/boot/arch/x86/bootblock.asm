; **
; ** Copyright 1998 Brian J. Swetland
; ** All rights reserved.
; **
; ** Redistribution and use in source and binary forms, with or without
; ** modification, are permitted provided that the following conditions
; ** are met:
; ** 1. Redistributions of source code must retain the above copyright
; **    notice, this list of conditions, and the following disclaimer.
; ** 2. Redistributions in binary form must reproduce the above copyright
; **    notice, this list of conditions, and the following disclaimer in the
; **    documentation and/or other materials provided with the distribution.
; ** 3. The name of the author may not be used to endorse or promote products
; **    derived from this software without specific prior written permission.
; **
; ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
; ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; ** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; ** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; ** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
; ** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; ** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; /*
; ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
; ** Distributed under the terms of the NewOS License.
; */

; /*
; ** Reviewed, documented and some minor modifications and bug-fixes
; ** applied by Michael Noisternig on 2001-09-02.
; */

%define VESA_X_TARGET 800
%define VESA_Y_TARGET 600
%define VESA_BIT_DEPTH_TARGET 32

SECTION
CODE16
ORG 0x7c00                   ; start code at 0x7c00
	jmp		short start
sectors	dw 800               ; this is interpreted as data (bytes 3 and 4)
	                         ; and patched from outside to the size of the bootcode (in sectors)
start:
	cli                      ; no interrupts please
	cld
	xor		ax,ax
	mov		ss,ax            ; setup stack from 0 - 0x7c00
	mov		sp,0x7c00        ; stack pointer to top of stack
	call	enable_a20       ; enable a20 gate
	lgdt	[ss:gdt]         ; load global descriptor table
	mov		eax,cr0          ; switch to protected mode
	or		al,0x1           ;   by setting 'protected mode enable' bit
	mov		cr0,eax          ;   and writing back
	jmp		dword 0x8:unreal32  ; do the actual switch into protected mode
    ; The switch into protected mode and back is to get the processor into 'unreal' mode
    ; where it is in 16-bit mode but with segments that are larger than 64k.
    ; this way, the floppy code can load the image directly into memory >1Mb.
unreal32:
	mov		bx,0x10          ; load all of the data segments with large 32-bit segment
	mov		ds,bx
	mov		es,bx
	mov		ss,bx
	and		al,0xfe          ; switch back to real mode
	mov		cr0,eax
	jmp		0x7c0:unreal-0x7c00   ; actually go back to 16-bit
unreal:
	xor		ax,ax            ; load NULL-descriptor (base 0) into ds, es, ss
	mov		es,ax            ; the processor doesn't clear the internal high size bits on these descriptors
	mov		ds,ax            ; so the descriptors can now reference 4Gb of memory, with size extensions
	mov		ss,ax

	; read in the second half of this stage of the bootloader
	xor		dx,dx            ; start at head 0
	mov		bx,0x2           ; start at sector 2 for the second half of this loader
	mov     cx,1             ; one sector
	mov		edi,0x7e00       ; right after this one
	sti
	call	load_floppy

	; read in the rest of the disk
	mov		edi,0x100000     ; destination buffer (at 1 MB) for sector reading in load_floppy
	mov		bx,0x3           ;   start at sector 3 (and cylinder 0)
	mov		cx,[sectors]     ;   read that much sectors
	xor		dx,dx            ;   start at head 0
	sti
	mov		si,loadmsg
	call	print
	dec		cx				 ; TK: one sector is already read as second part of boot block
	call	load_floppy      ; read remaining sectors at address edi
	call 	disable_floppy_motor
	mov		si,okmsg
	call	print
	cli

	; call	enable_vesa
	mov		[in_vesa],al

	mov		ebx,[dword 0x100074] ; load dword at rel. address 0x74 from read-destination-buffer
	add		ebx,0x101000         ; for stage2 entry
	mov		al,0xcf
	mov		[ds:gdt+14],al       ; set desc. 1 and 2
	mov		[ds:gdt+22],al       ;   to 32-bit segment
	lgdt	[ds:gdt]             ; load global descriptor table

	mov		eax,1
	mov		cr0,eax              ; enter protected mode
	jmp		dword 0x8:code32     ; flush prefetch queue
code32:
BITS 32
	mov		ax,0x10          ; load descriptor 2 in all segment selectors (except cs)
	mov		ds,ax
	mov		es,ax
	mov		fs,ax
	mov		gs,ax
	mov		ss,ax
	mov		ebp,0x10000
	mov		esp,ebp

	mov		eax,[vesa_info]
	push	eax

	xor		eax,eax
	mov		al,[in_vesa]
	push	eax

	call	find_mem_size
	push	eax

	call	ebx              ; jump to stage2 entry
inf:jmp		short inf

; find memory size by testing
; OUT: eax = memory size
find_mem_size:
	mov		eax,0x31323738   ; test value
	mov		esi,0x100ff0     ; start above conventional mem + HMA = 1 MB + 1024 Byte
_fms_loop:
	mov		edx,[esi]        ; read value
	mov		[esi],eax        ;   write test value
	mov		ecx,[esi]        ;   read it again
	mov		[esi],edx        ; write back old value
	cmp		ecx,eax
	jnz		_fms_loop_out    ; read value != test value -> above mem limit
	add		esi,0x1000       ; test next page (4 K)
	jmp		short _fms_loop
_fms_loop_out:
	mov		eax,esi
	sub		eax,0x1000
	add		eax,byte +0x10
	ret

BITS 16
; read sectors into memory
; IN: bx = sector # to start with: should be 2 as sector 1 (bootsector) was read by BIOS
;     cx = # of sectors to read
;     edi = buffer
load_floppy:
	push	bx
	push	cx
tryagain:
	mov		al,0x13          ; read a maximum of 18 sectors
	sub		al,bl            ;   substract first sector (to prevent wrap-around ???)

	xor		ah,ah            ; TK: don't read more then required, VMWare doesn't like that
	cmp		ax,cx
	jl		shorten
	mov		ax,cx
shorten:

	mov		cx,bx            ;   -> sector/cylinder # to read from
	mov		bx,0x8000        ; buffer address
	mov		ah,0x2           ; command 'read sectors'
	push	ax
	int		0x13             ;   call BIOS
	pop		ax               ; TK: should return number of transferred sectors in al
	                         ; but VMWare 3 clobbers it, so we (re-)load al manually
	jnc		okok             ;   no error -> proceed as usual
	dec		byte [retrycnt]
	jz		fail
	xor		ah,ah			 ; reset disk controller
	int		0x13
	jmp		tryagain		 ; retry
okok:
	mov		byte [retrycnt], 3	; reload retrycnt
	mov		si,dot
	call	print
	mov		esi,0x8000       ; source
	xor		ecx,ecx
	mov		cl,al            ; copy # of read sectors (al)
	shl		cx,0x7           ;   of size 128*4 bytes
	rep	a32 movsd        ;   to destination (edi) setup before func3 was called
	pop		cx
	pop		bx
	xor		dh,0x1           ; read: next head
	jnz		bar6
	inc		bh               ; read: next cylinder
bar6:
	mov		bl,0x1           ; read: sector 1
	xor		ah,ah
	sub		cx,ax            ; substract # of read sectors
	jg		load_floppy      ;   sectors left to read ?
	ret

disable_floppy_motor:
	xor		al,al
	mov		dx,0x3f2         ; disable floppy motor
	out		dx,al
	ret

; prints message in reg. si
print:
	pusha
_n:
	lodsb
	or		al,al
	jz		short _e
	mov		ah,0x0E
	mov		bx,7
	int		0x10
	jmp		_n
_e:
	popa
	ret

; print errormsg, wait for keypress and reboot
fail:
	mov		si,errormsg
	call	print
	xor		ax, ax
	int		0x16
	int		0x19

; enables the a20 gate
;   the usual keyboard-enable-a20-gate-stuff
enable_a20:
	call	_a20_loop
	jnz		_enable_a20_done
	mov		al,0xd1
	out		0x64,al
	call	_a20_loop
	jnz		_enable_a20_done
	mov		al,0xdf
	out		0x60,al
_a20_loop:
	mov		ecx,0x20000
_loop2:
	jmp		short _c
_c:
	in		al,0x64
	test	al,0x2
	loopne	_loop2
_enable_a20_done:
	ret

loadmsg		db	"Loading",0
errormsg	db	0x0a,0x0d,"Error reading disk.",0x0a,0x0d,0
okmsg		db	"OK",0x0a,0x0d,0
dot			db	".",0
gdt:
	; the first entry serves 2 purposes: as the GDT header and as the first descriptor
	; note that the first descriptor (descriptor 0) is always a NULL-descriptor
	db 0xFF        ; full size of GDT used
	db 0xff        ;   which means 8192 descriptors * 8 bytes = 2^16 bytes
	dw gdt         ;   address of GDT (dword)
	dd 0
	; descriptor 1:
	dd 0x0000ffff  ; base - limit: 0 - 0xfffff * 4K
	dd 0x008f9a00  ; type: 16 bit, exec-only conforming, <present>, privilege 0
	; descriptor 2:
	dd 0x0000ffff  ; base - limit: 0 - 0xfffff * 4K
	dd 0x008f9200  ; type: 16 bit, data read/write, <present>, privilege 0

retrycnt	db 3
in_vesa     db 0
vesa_info	dw 0

	times 510-($-$$) db 0  ; filler for boot sector
	dw	0xaa55             ; magic number for boot sector

; Starting here is the second sector of the boot code

; fool around with vesa mode
enable_vesa:
	; put the VBEInfo struct at 0x30000
	mov		eax,0x30000
	mov		[vesa_info],eax
	mov		dx,0x3000
	mov		es,dx
	mov		ax,0x4f00
	mov		di,0
	int		0x10

	; check the return code
	cmp		al,0x4f
	jne		done_vesa_bad
	cmp		ah,0x00
	jne		done_vesa_bad

	; check the signature on the data structure
	mov		eax,[es:00]
	cmp		eax,0x41534556   ; 'VESA'
	je		vesa_sig_ok
	cmp		eax,0x32454256   ; 'VBE2'
	jne		done_vesa_bad

vesa_sig_ok:
	; scan through each mode and grab the info on them
	les		bx,[es:14]	; calculate the pointer to the mode list
	mov		di,0x200	; push the buffer up a little to be past the VBEInfo struct

mode_loop:
	mov		cx,[es:bx]	; grab the next mode in the list
	cmp		cx,0xffff
	je		done_vesa_bad
	and		cx,0x01ff
	mov		ax,0x4f01
	int		0x10

	; if it's 1024x768x32, go for it
	mov		ax,[es:di]
	test	ax,0x1        ; test the supported bit
	jz		next_mode
	test	ax,0x08       ; test the linear frame mode bit
	mov		ax,[es:di+18]
	cmp		ax,VESA_X_TARGET       ; x
	jne		next_mode
	mov		ax,[es:di+20]
	cmp		ax,VESA_Y_TARGET       ; y
	jne		next_mode
	mov		al,[es:di+25]
	cmp		al,VESA_BIT_DEPTH_TARGET         ; bit_depth
	jne		next_mode

	; looks good, switch into it
	mov		ax,0x4f02
	mov		bx,cx
	or		bx,0x4000     ; add the linear mode bit
	int		0x10
	jmp		done_vesa_good

next_mode:
	; get ready to try the next mode
	inc		bx
	inc		bx
	jmp		mode_loop

done_vesa_good:
	mov		ax,0x1
	ret

done_vesa_bad:
	xor		ax,ax
	ret

	times 1024-($-$$) db 0  ; filler for second sector of the loader

