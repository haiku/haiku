;
; Copyright 2009, Christian Packmann.
; All rights reserved.
; Distributed under the terms of the MIT License, see
; http://www.opensource.org/licenses/mit-license.php

; Assembly code for Painter::_DrawBitmapBilinearCopy32() in Painter.cpp
; This code implements only the inner x-loop, all other processing
; is done in the C code.


; ******  GENERAL NOTES  *****

; The implemented algorithm looks like this:
; (pixLT * leftWeight  +  pixRT * rightWeight) * topWeight
;                            +
; (pixLB * leftWeight  +  pixRB * rightWeight) * bottomWeight
;
; with LT = LeftTop, RT = RightTop, LB = LeftBottom, RB = RightBottom
;
; For more detailed information, see the C implementation in
; Painter.cpp
;
; Implementation notes:
; The calculations are performed with 16-bit arithmetic. All values
; are held in vars/registers as 8-bit values high-shifted by 8 bits;
; i.e. 255<<8. This works because PMULHUW is used for MULs, and this
; algorithm limits the variable values appropriately during all steps.
; This will not work for all algorithms, so take note of that if you
; want to recycle some of the code.

; Notes on the code itself:
; I've tried to keep the code small. That's why I'm using memory accesses
; via index registers as much as possible. This costs execution time due
; to the generated µops, but should minimize decode bandwidth pressure
; due to the many MMX instructions.
; Temporary variables are always stored to the stack instead of global
; data space for this reason. So far I haven't exceeded 8-byte offsets,
; so the instructions only need to encode a BYTE-offset instead of a DWORD.

; Notes on code formatting/comments:
; - integer and vector instructions are indented differently. I find this
;   helpful when parsing code, especially when I haven't looked at it for a
;   longer time.
; - I've tried to comment the code so that it will be understandable and
;   maintainable in the future, and also by other persons than myself.
;   The current comments aren't yet fully standardized, I'm still working
;   on a coherent system for indicating the variables held within a register
;   which will help in understanding the data flow. Any suggestions
;   regarding this are welcome.
; - Abbreviations for datatypes:
;   B = Byte		  8 bit
;   W = Word		 16 bit
;   DW = Doubleword	 32 bit
;   QW = Quadword	 64 bit
;	DQ = Doublequad	128 bit
;	A "p" in front of one of the datatypes signifies that the
;	variable/register is encoded in packed form; i.e. pW means
;	"packed Words"; four Words for a MMX register, 8 for a SSE register.
;	This should help in understanding the logical meaning of the data
;	transformations.
;	For better readability, the datatype indicator for a register is
;   breacketed with '#', a MMX register with 2 uint32 of value 255 would be
;   #pD# 255 255



; ******  Global exports  *****

; Do NOT use '_' in front of your defines, this is done
; with YASMs --prefix option at assembly time.
GLOBAL bilinear_scale_xloop_mmxsse


; ********************
; ******  DATA  ******
; ********************
SECTION .data

DATA_SECTION:
ALIGN 16
DATA_SSSE3:
; data which is identical for MMX and SSE code is shared by declaring
; it as DQ but providing two labels. MMX code just accesses the
; first half.
c4x16UW_129_LShift8: 	TIMES 4 dw 129<<8
c4x16UW_255_LShift8:	TIMES 4 dw 255<<8
c2x32UD_ff000000:		TIMES 4 DD 0xff000000

; Argument definitions

; Parameter offsets assume "push ebp"
PAR_srcPtr EQU 	8
PAR_dstPtr EQU 	12
PAR_xWeightPtr EQU 16
PAR_xmin EQU 	20
PAR_xmax EQU 	24
PAR_wTop EQU 	28
PAR_srcBPR EQU 	32

; Stack storage definitions
ST_Q_wTop					EQU 0
ST_Q_wBottom				EQU 8
ST_Q_c4x16UW_129_LShift8	EQU 16
ST_Q_c4x16UW_255_LShift8	EQU 24
ST_Q_lftWeight_A			EQU 32
ST_Q_rgtWeight_A			EQU 40
ST_Q_lftWeight_B			EQU 48
ST_Q_rgtWeight_B			EQU 56


; ********************
; ******  CODE  ******
; ********************
SECTION .code


; void bilinear_scale_xloop_mmxsse(void* src, void* dst, void* xWeights,
;				uint32 xmin, uint32 xmax, uint16 wTop, uint32 srcBPR )
; Loop stats:
;		34 instructions (6 moves, 5 integer, 23 vector)
; 		12 memory accesses
ALIGN 16
bilinear_scale_xloop_mmxsse:
	push	ebp
	mov		ebp, esp
	and		esp, 0xfffffff8	; align stack to 8-byte boundary
	push	ebx
	push	edi
	push	esi
	sub		esp, 4 + 32	; +4 aligns to 8-byte boundary again; add 4 x QW
; xmin > xmax?
	mov		eax, [ebp + PAR_xmin]
	cmp		eax, [ebp + PAR_xmax]
	ja		.exit
; preparations
	; prepare wTop
	mov		eax, [ebp + PAR_wTop]	; #pB#: 0 0 0 top
	shl		eax, 8					; #pB#: 0 0 top 0
	movd		mm0, eax			; #pW# 0 0 0 top
	pshufw		mm0, mm0, 00000000b	; #pW# top top top top
	movq		[esp + ST_Q_wTop], mm0
	; move constants
	movq		mm5, [c4x16UW_255_LShift8]
	movq		[esp + ST_Q_c4x16UW_255_LShift8], mm5
	; prepare wBottom
	movq		mm1, mm5	; #pW# 255 255 255 255
	psubw		mm1, mm0	; 255 - wTop = wBottom
	movq		[esp + ST_Q_wBottom], mm1

; load params; leave ebx, ecx as scratch
	mov		eax, [ebp + PAR_xmin]	; loop counter
	mov		edx, [ebp + PAR_xWeightPtr]	; xWeights array
	mov		esi, [ebp + PAR_srcPtr]		; source bitmap
	mov		edi, [ebp + PAR_dstPtr]		; desination bitmap
	movq	mm6, [c4x16UW_129_LShift8]
	movq	mm7, [c2x32UD_ff000000]

; main loop
ALIGN 16
.loop:
	; load Left/Right weights into mm0/mm1
	movzx	ebx, WORD [edx + eax*4 + 2] ; xWeights + x*4 + 2-> FilterInfo[x].weight
	shl			ebx, 8		; #pB# 0 0 leftW 0
	pxor		mm2, mm2	; clear before use
	movd		mm0, ebx	; #pW# 0 0 0 leftW
	movq		mm1, [esp + ST_Q_c4x16UW_255_LShift8]
	pshufw		mm0, mm0, 00000000b	; #pW# lW lW lW lW
	psubw		mm1, mm0			; #pW# rW rW rW rW
	movzx	ecx, WORD [edx + eax*4] ; xWeights + x*4 -> FilterInfo[x].index
	pxor		mm3, mm3	; clear before use
	mov		ebx, ecx
	; process top and bottom pixels, interleave instructions to avoid latencies
	pxor		mm4, mm4	; clear before use
	; unpack pixel to high byte
	punpcklbw	mm2, [esi + ecx]	; pixLeftTop
	; unpack pixel to high byte
	punpcklbw	mm3, [esi + ecx + 4] ; pixRightTop

	add		ebx, [ebp + PAR_srcBPR]	; address:bottom pixels
	pmulhuw		mm2, mm0	; pixLT * leftWeight
	pmulhuw		mm3, mm1	; pixRT * rightWeight
	; calc address for bottom pix
	pxor		mm5, mm5	; clear before use
	punpcklbw	mm4, [esi + ebx]	; pixLeftBottom
	punpcklbw	mm5, [esi + ebx + 4] ; pixRightBottom
	pmulhuw		mm4, mm0	; pixLB * leftWeight
	pmulhuw		mm5, mm1	; pixRB * rightWeight

	paddw		mm2, mm3	; pixLT + pixRT
	paddw		mm4, mm5	; pixLB + pixRB
	pmulhuw		mm2, [esp + ST_Q_wTop]	; * weightTop
	pmulhuw		mm4, [esp + ST_Q_wBottom]	; * weightBottom

	; add both temp results
	paddw		mm2, mm4
	; divide by 65025 using integer reciprocal: (*129 >> 7)
	pmulhuw		mm2, mm6
	psrlw		mm2, 7
	; pack & store
	packuswb	mm2, mm2
	por			mm2, mm7	; | 0xff000000
	movd		[edi], mm2	; store pixel as DWord
	add		edi, 4
; loopctr <= xmax?
	inc		eax
	cmp		eax, [ebp + PAR_xmax]
	jle		.loop
.exit:
	emms	; Don't EVER forget to call EMMS!
	add		esp, 4 + 32	; restore  stack pointer
	pop		esi
	pop		edi
	pop		ebx
	mov		esp, ebp
	pop		ebp
	ret
