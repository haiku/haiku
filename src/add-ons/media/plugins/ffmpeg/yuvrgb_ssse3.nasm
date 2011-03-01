;
; Copyright (C) 2009-2010 David McPaul
;
; All rights reserved. Distributed under the terms of the MIT License.
;

; A rather unoptimised set of ssse3 yuv to rgb converters
; does 8 pixels per loop

; inputer:
; reads 128 bits of yuv 8 bit data and puts
; the y values converted to 16 bit in xmm0
; the u values converted to 16 bit and duplicated into xmm1
; the v values converted to 16 bit and duplicated into xmm2

; conversion:
; does the yuv to rgb conversion using 16 bit fixed point and the
; results are placed into the following registers as 8 bit clamped values
; r values in xmm3
; g values in xmm4
; b values in xmm5

; outputer:
; writes out the rgba pixels as 8 bit values with 0 for alpha

; xmm6 used for scratch
; xmm7 used for scratch

%macro  cglobal 1
	global  _%1
	%define %1 _%1
	align 16
%1:
%endmacro

SECTION .data align=16

Const16	dw	16
	dw	16
	dw	16
	dw	16
	dw	16
	dw	16
	dw	16
	dw	16

Const128	dw	128
	dw	128
	dw	128
	dw	128
	dw	128
	dw	128
	dw	128
	dw	128
	
UMask	db	0x01
	db	0x80
	db	0x01
	db	0x80
	db	0x05
	db	0x80
	db	0x05
	db	0x80
	db	0x09
	db	0x80
	db	0x09
	db	0x80
	db	0x0d
	db	0x80
	db	0x0d
	db	0x80

VMask	db	0x03
	db	0x80
	db	0x03
	db	0x80
	db	0x07
	db	0x80
	db	0x07
	db	0x80
	db	0x0b
	db	0x80
	db	0x0b
	db	0x80
	db	0x0f
	db	0x80
	db	0x0f
	db	0x80
	
YMask	db	0x00
	db	0x80
	db	0x02
	db	0x80
	db	0x04
	db	0x80
	db	0x06
	db	0x80
	db	0x08
	db	0x80
	db	0x0a
	db	0x80
	db	0x0c
	db	0x80
	db	0x0e
	db	0x80

UVMask	db	0x01
	db	0x80
	db	0x03
	db	0x80
	db	0x05
	db	0x80
	db	0x07
	db	0x80
	db	0x09
	db	0x80
	db	0x0b
	db	0x80
	db	0x0d
	db	0x80
	db	0x0f
	db	0x80

shuffconst db 0x0
		db 0x01
		db 0x00
		db 0x01
		db 0x04
		db 0x05
		db 0x04
		db 0x05
		db 0x08
		db 0x09
		db 0x08
		db 0x09
		db 0x0c
		db 0x0d
		db 0x0c
		db 0x0d

RConst	dw 0
		dw 5743
		dw 0
		dw 5743
		dw 0
		dw 5743
		dw 0
		dw 5743
		
GConst	dw -1409
		dw -2925
		dw -1409
		dw -2925
		dw -1409
		dw -2925
		dw -1409
		dw -2925
		
BConst	dw 7258
		dw 0
		dw 7258
		dw 0
		dw 7258
		dw 0
		dw 7258
		dw 0

; conversion code 
%macro yuv2rgbssse3 0
; u = u - 128
; v = v - 128
; r = y + 0 * u + 1.403 * v
; g = y + -0.344 * u + -0.714 * v
; b = y + 1.773 * u + 0 * v
; subtract 128 from u and v
	psubsw xmm3, [Const128]			; u = u - 128, v = v -128
	
	pshufd xmm5, xmm3, 0xE4			; duplicate
	movdqa xmm4, xmm3				; duplicate
	
; subtract 16 from y
;	psubsw xmm0, [Const16]			; y = y - 16

	pmaddwd xmm3, [RConst]			; multiply and add
	pmaddwd xmm4, [GConst]			; to get RGB offsets to Y
	pmaddwd xmm5, [BConst]			;

	psrad xmm3, 12					; Scale back to original range
	psrad xmm4, 12					;
	psrad xmm5, 12					;

	pshufb xmm3, [shuffconst]		; duplicate results
	pshufb xmm4, [shuffconst]		; 2 y values per const
	pshufb xmm5, [shuffconst]		;

	paddsw xmm3, xmm0				; and add to y
	paddsw xmm4, xmm0				;
	paddsw xmm5, xmm0				;
%endmacro

; outputer
%macro rgba32ssse3output 0
; clamp values
	pxor xmm7,xmm7
	packuswb xmm3,xmm7				; clamp to 0,255 and pack R to 8 bit per pixel
	packuswb xmm4,xmm7				; clamp to 0,255 and pack G to 8 bit per pixel
	packuswb xmm5,xmm7				; clamp to 0,255 and pack B to 8 bit per pixel
; convert to bgra32 packed
	punpcklbw xmm5,xmm4				; bgbgbgbgbgbgbgbg
	movdqa xmm0, xmm5				; save bg values
	punpcklbw xmm3,xmm7				; r0r0r0r0r0r0r0r0
	punpcklwd xmm5,xmm3				; lower half bgr0bgr0bgr0bgr0
	punpckhwd xmm0,xmm3				; upper half bgr0bgr0bgr0bgr0
; write to output ptr
	movntdq [edi], xmm5				; output first 4 pixels bypassing cache
	movntdq [edi+16], xmm0			; output second 4 pixels bypassing cache
%endmacro


; void Convert_YUV422_RGBA32_SSSE3(void *fromPtr, void *toPtr, int width)
width    equ	ebp+16
toPtr    equ	ebp+12
fromPtr  equ	ebp+8

; void Convert_YUV420P_RGBA32_SSSE3(void *fromYPtr, void *fromUPtr, void *fromVPtr, void *toPtr, int width)
width1    equ	ebp+24
toPtr1    equ	ebp+20
fromVPtr  equ	ebp+16
fromUPtr  equ	ebp+12
fromYPtr  equ	ebp+8

SECTION .text align=16

cglobal Convert_YUV422_RGBA32_SSSE3
; reserve variables
	push ebp
	mov ebp, esp
	push edi
	push esi
	push ecx
	
	mov esi, [fromPtr]
	mov edi, [toPtr]
	mov ecx, [width]
; loop width / 8 times
	shr ecx,3
	test ecx,ecx
	jng ENDLOOP
REPEATLOOP:							; loop over width / 8
	prefetchnta [esi+256]
; YUV422 packed inputer
	movdqa xmm0, [esi]				; should have yuyv yuyv yuyv yuyv
	pshufd xmm3, xmm0, 0xE4			; copy to xmm1
; extract both y giving y0y0
	pshufb xmm0, [YMask]
; extract u and v to have u0v0
	pshufb xmm3, [UVMask]

yuv2rgbssse3
	
rgba32ssse3output

; endloop
	add edi,32
	add esi,16
	sub ecx, 1				; apparently sub is better than dec
	jnz REPEATLOOP
ENDLOOP:
; Cleanup
	pop ecx
	pop esi
	pop edi
	mov esp, ebp
	pop ebp
	ret

cglobal Convert_YUV420P_RGBA32_SSSE3
; reserve variables
	push ebp
	mov ebp, esp
	push edi
	push esi
	push ecx
	push eax
	push ebx
		
	mov esi, [fromYPtr]
	mov eax, [fromUPtr]
	mov ebx, [fromVPtr]
	mov edi, [toPtr1]
	mov ecx, [width1]
; loop width / 8 times
	shr ecx,3
	test ecx,ecx
	jng ENDLOOP1
REPEATLOOP1:						; loop over width / 8
	prefetchnta [esi+256]
	prefetchnta [eax+128]
	prefetchnta [ebx+128]

; YUV420 Planar inputer
	movq xmm0, [esi]				; fetch 8 y values (8 bit) yyyyyyyy00000000
	movd xmm3, [eax]				; fetch 4 u values (8 bit) uuuu000000000000
	movd xmm1, [ebx]				; fetch 4 v values (8 bit) vvvv000000000000
	
; convert y to 16 bit
	pxor xmm7,xmm7					; 00000000000000000000000000000000
	punpcklbw xmm0,xmm7				; interleave xmm7 into xmm0 y0y0y0y0y0y0y0y0
	
; combine u and v
	punpcklbw xmm3,xmm1				; uvuvuvuv00000000
	punpcklbw xmm3,xmm7				; u0v0u0v0u0v0u0v0
	
yuv2rgbssse3
	
rgba32ssse3output

; endloop
	add edi,32
	add esi,8
	add eax,4
	add ebx,4
	sub ecx, 1				; apparently sub is better than dec
	jnz REPEATLOOP1
ENDLOOP1:
; Cleanup
	pop ebx
	pop eax
	pop ecx
	pop esi
	pop edi
	mov esp, ebp
	pop ebp
	ret

SECTION .note.GNU-stack noalloc noexec nowrite progbits
