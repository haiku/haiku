/*
 * Copyright (C) 2009 David McPaul
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

; A rather unoptimised set of yuv to rgb converters
; does 8 pixels at a time

; inputer:
; reads 128bits of yuv 8 bit data and puts
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

; conversion code 
%macro yuv2rgb 0
; u = u - 128
; v = v - 128
; r = y + v + v >> 2 + v >> 3 + v >> 5 
; g = y - (u >> 2 + u >> 4 + u >> 5) - (v >> 1 + v >> 3 + v >> 4 + v >> 5)
; b = y + u + u >> 1 + u >> 2 + u >> 6
; subtract 16 from y
	movdqa xmm7, [Const16]			; loads a constant using data cache
	psubsw xmm0,xmm7				; y = y - 16
; subtract 128 from u and v
;	mov eax,128*10001H				; load a constant using instruction cache
;	movd xmm7,eax					; but requires eax to be saved
;	pshufd xmm7,xmm7,0				; and uses more instructions
	movdqa xmm7, [Const128]			; loads a constant using data cache (slower on first fetch but then cached)
	psubsw xmm1,xmm7				; u = u - 128
	psubsw xmm2,xmm7				; v = v - 128
; load r,g,b with y 
	movdqa xmm3,xmm0				; r = y 
	pshufd xmm4,xmm0, 0xE4			; g = y 
	movdqa xmm5,xmm0				; b = y 
; r = r + v + v >> 2 + v >> 3 + v >> 5
	paddsw xmm3, xmm2				; add v to r
	movdqa xmm6, xmm2				; move v to scratch
	psraw  xmm6,2					; divide by 4
	paddsw xmm3, xmm6				; and add to r
	psraw  xmm6,1					; divide by 2
	paddsw xmm3, xmm6				; and add to r
	psraw  xmm6,2					; divide by 4
	paddsw xmm3, xmm6				; and add to r
	
; g = y - u >> 2 - u >> 4 - u >> 5 - v >> 1 - v >> 3 - v >> 4 - v >> 5
	movdqa xmm6,xmm1				; move u to scratch
	psraw  xmm6,2					; divide by 4
	psubsw xmm4,xmm6				; subtract from g
	psraw  xmm6,2					; divide by 4
	psubsw xmm4,xmm6				; subtract from g
	psraw  xmm6,1					; divide by 2
	psubsw xmm4,xmm6				; subtract from g

	movdqa xmm6,xmm2				; move v to scratch
	psraw  xmm6,1					; divide by 2
	psubsw xmm4,xmm6				; subtract from g
	psraw  xmm6,2					; divide by 4
	psubsw xmm4,xmm6				; subtract from g
	psraw  xmm6,1					; divide by 2
	psubsw xmm4,xmm6				; subtract from g
	psraw  xmm6,1					; divide by 2
	psubsw xmm4,xmm6				; subtract from g
; b = y + u + u >> 1 + u >> 2 + u >> 6
	paddsw xmm5, xmm1				; add u to b
	movdqa xmm6, xmm1				; move u to scratch
	psraw  xmm6,1					; divide by 2
	paddsw xmm5, xmm6				; and add to b
	psraw  xmm6,1					; divide by 2
	paddsw xmm5, xmm6				; and add to b
	psraw  xmm6,4					; divide by 32
	paddsw xmm5, xmm6				; and add to b
%endmacro

; outputer
%macro rgba32output 0
; clamp values
	pxor xmm7,xmm7
	packuswb xmm3,xmm7				; clamp to 0,255 and pack R to 8 bit per pixel
	packuswb xmm4,xmm7				; clamp to 0,255 and pack G to 8 bit per pixel
	packuswb xmm5,xmm7				; clamp to 0,255 and pack B to 8 bit per pixel
; convert to bgra32 packed
	punpcklbw xmm3,xmm7				; r0r0r0r0r0r0r0r0
	punpcklbw xmm5,xmm4				; bgbgbgbgbgbgbgbg
	movdqa xmm0, xmm5				; save gb values
	punpcklwd xmm5,xmm3				; lower half bgr0bgr0bgr0bgr0
	punpckhwd xmm0,xmm3				; upper half bgr0bgr0bgr0bgr0
; write to output ptr
	movntdq [edi], xmm5				; output first 4 pixels bypassing cache
	movntdq [edi+16], xmm0			; output second 4 pixels bypassing cache
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

; void Convert_YUV422_RGBA32_SSE2(void *fromPtr, void *toPtr, int width)
width    equ	ebp+16
toPtr    equ	ebp+12
fromPtr  equ	ebp+8

; void Convert_YUV420P_RGBA32_SSE2(void *fromYPtr, void *fromUPtr, void *fromVPtr, void *toPtr, int width)
width1    equ	ebp+24
toPtr1    equ	ebp+20
fromVPtr  equ	ebp+16
fromUPtr  equ	ebp+12
fromYPtr  equ	ebp+8

SECTION .text align=16

cglobal Convert_YUV422_RGBA32_SSE2
; reserve variables
	push ebp
	mov ebp, esp
	push edi
	push esi
	push ecx
	
	mov esi, [fromPtr]
	mov edi, [toPtr]
	mov ecx, [width]
	prefetchnta [esi]	; hint that we will be loading our data outside of cache
; loop width / 8 times
	shr ecx,3
	test ecx,ecx
	jng ENDLOOP
REPEATLOOP:							; loop over width / 8
;	push ecx						; preserve loop counter
; YUV422 packed inputer
	movdqa xmm0, [esi]				; should have yuyv yuyv yuyv yuyv
	pshufd xmm1, xmm0, 0xE4			; copy to xmm1
	movdqa xmm2, xmm0				; copy to xmm2
; extract y
	pxor xmm7,xmm7					; 00000000000000000000000000000000
	pcmpeqd xmm6,xmm6				; ffffffffffffffffffffffffffffffff
	punpcklbw xmm6,xmm7				; interleave xmm7 into xmm6 ff00ff00ff00ff00ff00ff00ff00ff00
	pand xmm0, xmm6					; clear all but y values leaving y0y0 etc
; extract u and duplicate so each u in yuyv becomes 0u0u
	psrld xmm6,8					; 00ff0000 00ff0000 00ff0000 00ff0000
	pand xmm1, xmm6					; clear all yv values leaving 0u00 etc
	psrld xmm1,8					; rotate u to get u000
	pshuflw xmm1,xmm1, 0xA0			; copy u values
	pshufhw xmm1,xmm1, 0xA0			; to get u0u0
; extract v
	pslld xmm6,16					; 000000ff000000ff 000000ff000000ff
	pand xmm2, xmm6					; clear all yu values leaving 000v etc
	psrld xmm2,8					; rotate v to get 00v0
	pshuflw xmm2,xmm2, 0xF5			; copy v values
	pshufhw xmm2,xmm2, 0xF5			; to get v0v0

yuv2rgb
	
rgba32output

; endloop
	add edi,32
	add esi,16
;	pop ecx
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

cglobal Convert_YUV420P_RGBA32_SSE2
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
;	push ecx						; preserve loop counter
; YUV420 Planar inputer
	movq mm0, [esi]					; fetch 8 y values (8 bit) (direct unaligned sse2 loads might be better)
	movd mm1, [eax]					; fetch 4 u values
	movd mm2, [ebx]					; fetch 4 v values
	
	movq2dq xmm0, mm0				; copy y to sse register  yyyyyyyy00000000
	movq2dq xmm1, mm1				; copy u to sse register  uuuu000000000000
	movq2dq xmm2, mm2				; copy v to sse register  vvvv000000000000
; extract y
	pxor xmm7,xmm7					; 00000000000000000000000000000000
	punpcklbw xmm0,xmm7				; interleave xmm7 into xmm0 y0y0y0y0y0y0y0y0
; extract u and duplicate so each becomes 0u0u
	punpcklbw xmm1,xmm7				; interleave xmm7 into xmm1 u0u0u0u000000000
	punpcklwd xmm1,xmm7				; interleave again u000u000u000u000
	pshuflw xmm1,xmm1, 0xA0			; copy u values
	pshufhw xmm1,xmm1, 0xA0			; to get u0u0
; extract v
	punpcklbw xmm2,xmm7				; interleave xmm7 into xmm1 v0v0v0v000000000
	punpcklwd xmm2,xmm7				; interleave again v000v000v000v000
	pshuflw xmm2,xmm2, 0xA0			; copy v values
	pshufhw xmm2,xmm2, 0xA0			; to get v0v0

yuv2rgb
	
rgba32output

; endloop
	add edi,32
	add esi,8
	add eax,4
	add ebx,4
;	pop ecx
	sub ecx, 1				; apparently sub is better than dec
	jnz REPEATLOOP1
ENDLOOP1:
; Cleanup
	emms
	pop ebx
	pop eax
	pop ecx
	pop esi
	pop edi
	mov esp, ebp
	pop ebp
	ret

SECTION .note.GNU-stack noalloc noexec nowrite progbits
