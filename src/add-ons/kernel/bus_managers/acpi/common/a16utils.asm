NAME    BU_ASM
TITLE   BU_ASM.ASM -- ASM support for DOS BU_LIB

;****************************************************************************
;*      Copyright (C) Intel Corporation 1994-1999
;*
;*  All rights reserved.  No part of this program or publication
;*  may be reproduced, transmitted, transcribed, stored in a
;*  retrieval system, or translated into any language or computer
;*  language, in any form or by any means, electronic, mechanical,
;*  magnetic, optical, chemical, manual, or otherwise, without
;*  the prior written permission of Intel Corporation.
;****************************************************************************
;
;  To assemble with MASM61 -- ml /c a16utils.asm
;
;****************************************************************************

.286
.MODEL large, c

IFDEF MODEL_S
.MODEL small, c
ENDIF

IFDEF MODEL_M
.MODEL medium, c
ENDIF

IFDEF MODEL_L
.MODEL large, c
ENDIF

IFDEF MODEL_C
.MODEL compact, c
ENDIF

IFDEF MODEL_H
.MODEL huge, c
ENDIF

;--------------------------------------------------------------------

GOREAL  MACRO
    LOCAL   here

    mov eax, cr0
    and eax, 0fffffffeh
    mov cr0, eax
    jmp here
here:
ENDM

;--------------------------------------------------------------------

GOPROT  MACRO
    LOCAL here

    xor eax, eax
    xor edx, edx

    mov ax, ds
    shl eax, 4
    lea edx, _gdt
    add eax, edx

    mov [_gdt_ptr+2], ax
    shr eax, 16
    mov [_gdt_ptr+4], ax

    lgdt    FWORD PTR _gdt_ptr
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp here
here:
ENDM
;--------------------------------------------------------------------

.STACK
.DATA

PUBLIC  _dAPAwake, _dAPDone, _dAPFuncPtr, _wProcNumber, _bSPValid
PUBLIC  _wCSegDS, _wCSegES, _wCSegSS, _wCSegSP, _wASegDS, _wASegES
PUBLIC  _dAPHltJmp



    emsfilename     DB      'EMMXXXX0',0

    _dAPAwake   DD 0
    _dAPDone    DD 0
    _dAPFuncPtr DD 0
    _wProcNumber    DW 0
    _bSPValid   DB 0

    _wASegDS    DW 0
    _wASegES    DW 0
    _wCSegDS    DW 0
    _wCSegES    DW 0
    _wCSegSS    DW 0
    _wCSegSP    DW 0

    _block      DW 0
    _gdt_ptr    DW 23
            DD 0
    _gdt        DD 0        ; NULL descriptor
            DD 0        ; NULL descriptor

                    ; 64K real mode style data segment
            DD 0ffffh   ; base 0, limit 0xffff
            DD 00009200h    ; data, CPL=0, 16-bit, 1 byte granularity, r/w

                    ; 4G flat data segment
            DD 0ffffh   ; base 0, limit 0xfffff
            DD 008f9200h    ; data, CPL=0, 16-bit, 4K granularity, r/w
    _old_es     DW 0
    _old_ds     DW 0
    _old_fs     DW 0
    _old_gs     DW 0
    _dAPHltJmp  DD 0        ; jump to halt location

.CODE
EXTERNDEF pascal cprint:proc

PUBLIC  ems_present
ems_present PROC C
    mov     ah,3dh                  ; open file
    xor     al,al                   ; read only operation
    mov     dx,offset emsfilename   ; look for EMMXXXX0
    int     21h
    jc      emserror1               ; if carry flag set, then no EMM driver

    mov     di,ax                   ; save file handle
    mov     bx,ax                   ; set up handle
    mov     ax,4400h                ; get device (channel) info
    int     21h

    mov     ah,3eh                  ; close file
    mov     bx,di                   ; close the file handle up
    int     21h
    jc      emserror1               ; if carry flag set, then no driver is present

    test    dx,10000000b            ; if bit 7 not set, then the channel is a file
    jz      emserror1               ; otherwise the channel is a device driver

    mov     ax,1                    ; 1 = EMS present
    ret

emserror1:
    xor ax,ax           ; 0 = EMS not present
    ret
ems_present ENDP

PUBLIC vm86
vm86        PROC C
    smsw    ax
    and ax, 1
    ret
vm86    ENDP


PUBLIC _set_rows
_set_rows   PROC C rows:WORD
    cmp rows, 25
    je  set_25
    cmp rows, 50
    je  set_50
    cmp rows, 132
    je  set_132
    jmp invalid_input
set_25:
    mov ah, 00
    mov al, 03
    int 10h
    mov ax, rows
    jmp end_set_rows
set_50:
    mov ah, 00
    mov al, 03
    int 10h
    xor bx, bx
    mov ah, 11h
    mov al, 12h
    int 10h
    mov ax, rows
    jmp end_set_rows
set_132:
    mov ah, 00
    mov al, 14h
    int 10h
    xor bx, bx
    mov ah, 11h
    mov al, 12h
    int 10h
    mov ax, rows
    jmp end_set_rows
invalid_input:
    xor ax, ax
    jmp end_set_rows
end_set_rows:
    ret
_set_rows   ENDP


PUBLIC  _cpuid_asm
_cpuid_asm  PROC C hv:DWORD, regs:PTR DWORD

check_8086:
.8086
    pushf
    pop ax
    mov cx, ax
    and ax, 0fffh
    push    ax
    popf
    pushf
    pop ax
    and ax, 0f000h
    cmp ax, 0f000h
    mov ax, 10h
    je  end_cpuid

check_80286:
.286
    or  cx, 0f000h
    push    cx
    popf
    pushf
    pop ax
    and ax, 0f000h
    mov ax, 20h
    jz  end_cpuid

check_80386:
.386
    mov bx, sp
    and sp, not 3
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 40000h
    push    eax
    popfd
    pushfd
    pop eax
    cmp ecx, eax
    jne check_80486
    mov ax, 30h
    mov sp, bx
    jmp end_cpuid

check_80486:
.486
    pushfd
    pop ecx
    mov ecx, eax
    xor eax, 200000h
    push    eax
    popfd
    pushfd
    pop eax
    xor eax, ecx
    mov ax, 40h
    je  end_cpuid

check_cpuid:
.586
    mov eax, hv
    cpuid
    push    ebx
    IF  @DataSize       ; segment is far
    les bx, regs
    mov dword ptr es:[bx], eax
    pop eax
    mov dword ptr es:[bx + 4], eax
    mov dword ptr es:[bx + 8], ecx
    mov dword ptr es:[bx + 12], edx
    ELSE                ; segment is near
    mov bx, regs
    mov dword ptr [bx], eax
    pop eax
    mov dword ptr [bx + 4], eax
    mov dword ptr [bx + 8], ecx
    mov dword ptr [bx + 12], edx
    ENDIF
    mov ax, 5
end_cpuid:
    ret
_cpuid_asm  ENDP

.386p
;--------------------------------------------------------------------
; Pentium Pro MSRs - from Pentium Pro Developer's Manual
;   January 1996 - Appendix C
;
; MTRRs: 200h - 20Fh, 250h, 258h - 259h, 268h - 26Fh, 2FFh
;--------------------------------------------------------------------
PUBLIC  _read_msr
_read_msr   PROC    C dmsr:DWORD, pqmsr:PTR DWORD
    pushad
    mov ecx, dmsr
    dw  320fh           ; RDMSR
    les bx, pqmsr
    mov DWORD ptr es:[bx], eax
    mov DWORD ptr es:[bx + 4], edx
    popad
    ret
_read_msr   ENDP

;--------------------------------------------------------------------

PUBLIC  _write_msr
_write_msr  PROC    C dmsr:DWORD, pqmsr:PTR DWORD
    pushad
    les bx, pqmsr
    mov eax, DWORD ptr es:[bx]
    mov edx, DWORD ptr es:[bx + 4]
    mov ecx, dmsr
    dw  300fh           ; WRMSR
    popad
    ret
_write_msr  ENDP

;----------------------------------------------------------------------------
; Procedure:    dReadCR0 - Reads CR0 CPU Register
;
; Output:   dx:ax and eax - Contents of CR0
;
; Registers:    All registers preserved except EAX and EDX
;----------------------------------------------------------------------------
PUBLIC  dReadCR0
dReadCR0    PROC C
    mov eax, cr0
    mov edx, eax
    shr edx, 16     ; upper 16 bits in dx
    ret
dReadCR0    ENDP

;----------------------------------------------------------------------------
; Procedure:    dReadCR2 - Reads CR2 CPU Register
;
; Output:   dx:ax and eax - Contents of CR2
;
; Registers:    All registers preserved except EAX and EDX
;----------------------------------------------------------------------------
PUBLIC  dReadCR2
dReadCR2    PROC C
    mov eax, cr2
    mov edx, eax
    shr edx, 16     ; upper 16 bits in dx
    ret
dReadCR2    ENDP

;----------------------------------------------------------------------------
; Procedure:    dReadCR3 - Reads CR3 CPU Register
;
; Output:   dx:ax and eax - Contents of CR3
;
; Registers:    All registers preserved except EAX and EDX
;----------------------------------------------------------------------------
PUBLIC  dReadCR3
dReadCR3    PROC C
    mov eax, cr3
    mov edx, eax
    shr edx, 16     ; upper 16 bits in dx
    ret
dReadCR3    ENDP


.586
;----------------------------------------------------------------------------
; Procedure:    dGetProcUpdateRev_asm - Reads PPP Update Revision via an MSR
;
; Output:   dx:ax and eax - PPP Update Revision
;
; Registers:    All registers preserved except EAX, ECX, and EDX
;----------------------------------------------------------------------------
PUBLIC _dGetProcUpdateRev_asm
_dGetProcUpdateRev_asm  PROC    C
    mov ecx, 08Bh   ; model specific register to write
    xor eax, eax
    xor edx, edx
    dw  300Fh       ; WRMSR ==> load 0 to MSR at 8Bh
    mov eax, 1
    cpuid
    mov ecx, 08Bh   ; model specific register to read
    dw  320Fh       ; RDMSR ==> EDX = (read) MSR 8Bh
    mov eax, edx    ; return EAX
    shr edx, 16 ; return DX:AX
    ret
_dGetProcUpdateRev_asm  ENDP

;--------------------------------------------------------------------

PUBLIC  iInterruptFlagState
iInterruptFlagState PROC
    pushf
    pop ax
    and ax, 200h
    shr ax, 9
    ret
iInterruptFlagState ENDP

;--------------------------------------------------------------------

.586P
PUBLIC  vRealFsGs
vRealFsGs       PROC C

    ; TBD:  verify this on an AMD K6-2 266 MHz
    ; Assert A20M# via system control port A (This is a
    ; system dependent feature).
    in  al, 92h
    and al, 0fdh
    out 92h, al     ; re-assert A20M#

    GOPROT

    mov ax, 8
    mov gs, ax

    GOREAL

    mov ax, [_old_fs]   ; restore original
    mov fs, ax
    mov ax, [_old_gs]   ; restore original
    mov gs, ax
    sti
    ret
vRealFsGs       ENDP

;--------------------------------------------------------------------
PUBLIC  vFlatFsGs
vFlatFsGs       PROC C
    cli
    mov ax, fs
    mov [_old_fs], ax   ; save  original fs
    mov ax, gs
    mov [_old_gs], ax   ; save  original gs

    ; Deassert A20M# via system control port A (This is a
    ; system dependent feature)
    in  al, 92h
    or  al, 2
    out 92h, al     ; de-assert A20M#

    GOPROT

    mov ax, 10h
    mov fs, ax
    mov gs, ax

    GOREAL

    ret
vFlatFsGs       ENDP


PUBLIC  vRealEsDs
vRealEsDs       PROC C

    ; TBD:  verify this on an AMD K6-2 266 MHz
    ; Assert A20M# via system control port A (This is a
    ; system dependent feature).
    in  al, 92h
    and al, 0fdh
    out 92h, al     ; re-assert A20M#

    GOPROT

    mov ax, 8
    mov gs, ax

    GOREAL

    mov ax, [_old_es]   ; restore original
    mov es, ax
    mov ax, [_old_gs]   ; restore original
    mov gs, ax
    sti
    ret
vRealEsDs       ENDP

;--------------------------------------------------------------------
PUBLIC  vFlatEsDs
vFlatEsDs       PROC C
    cli
    mov ax, es
    mov [_old_es], ax   ; save  original
    mov ax, gs
    mov [_old_gs], ax   ; save  original

    ; Deassert A20M# via system control port A (This is a
    ; system dependent feature)
    in  al, 92h
    or  al, 2
    out 92h, al     ; de-assert A20M#

    GOPROT

    mov ax, 10h
    mov es, ax
    mov gs, ax

    GOREAL

    ret
vFlatEsDs       ENDP

PUBLIC print
print PROC PASCAL Seq:DWORD


    pushad
    mov     eax, Seq
    push    eax
    call    cprint
    popad

    ret
print ENDP

PUBLIC  DoMove
DoMove       PROC    

    push    edx
;    GOPROT
    pop     edx

    mov     ecx, edx
    shr     ecx, 2          ; byte count / 4 = dword count
    cmp     ecx, 0
    je      DoBytes 

NextDword:
    xor     eax, eax
    lock or eax, gs:[esi]
;    mov     eax, gs:[esi]

    mov     gs:[edi], eax

    add     esi, 4
    add     edi, 4
    loop    NextDword

DoBytes:
    mov     ecx, edx
    and     ecx, 3
    cmp     ecx, 0
    je      Exit            ; no extra bytes to move

NextByte:
    mov     al, byte ptr gs:[esi]
    mov     byte ptr gs:[edi], al
    inc     esi
    inc     edi
    loop    NextByte

Exit:
;    GOREAL
    ret       
    
DoMove       ENDP
    .586

 
;--------------------------------------------------------------------

PUBLIC  _WriteApic
_WriteApic  PROC C
    call    vFlatFsGs
    xor ecx, ecx
    xor edx, edx

    mov cx, [esp+6] ; high  word of apic address
    shl ecx, 16
    mov cx, [esp+4] ; low word of apic address

    mov dx, [esp+10]    ; high  word of apic command
    shl edx, 16
    mov dx, [esp+8] ; low word of apic command

    mov gs:[ecx], edx

    call    vRealFsGs
    ret
_WriteApic  ENDP

;--------------------------------------------------------------------

PUBLIC  _ReadApic
_ReadApic   PROC C
    call    vFlatFsGs
    xor ecx, ecx

    mov cx, [esp+6] ; low word of apic address
    shl ecx, 16
    mov cx, [esp+4] ; high  word of apic address

    mov eax, gs:[ecx]

    push    eax     ; save  eax
    call    vRealFsGs
    pop eax     ; restore eax

    mov edx, eax
    shr edx, 16
    ret
_ReadApic   ENDP

;--------------------------------------------------------------------
; Since the AP shares the BSP's stack, coordination is needed to avoid
; simultaneous usage by both processors, which would cause corruption.
;
; While the AP is executing here, the BSP is executing the code
; which follows the call to vAPStartupAll() or vAPStartupOne()
; in bu_apic.c:vExecuteOnAPs(). After disabling interrupts,
; the BSP stores the sp value in _wCSegSP, sets _bSPValid to 1,
; and spins while _dAPDone != _dAPAwake. (Both are initially zero;
; the code below increments _dAPAwake before spinning on _bSPValid.)
;
; XXX - There is a race condition here:  if the BSP makes it all
; XXX - the way to its spin loop on _dAPDone != _dAPAwake before
; XXX - the AP increments _dAPAwake, the BSP will exit the spin
; XXX - loop without actually having waited for the AP to finish.

PUBLIC  _vSetupAP
_vSetupAP   PROC C
    cli
    mov ax, @data
    mov ds, ax
    mov esp, 0
    lock    inc dword ptr [_dAPAwake]
spin1:
    mov al, [_bSPValid]
    cmp al, 1
    jne spin1
; XXX - The following line is supposed to provide some validity checking,
; XXX - but it does not entirely succeed. It is possible for > 1 AP to
; XXX - see _bSPValid == 1, and exit the spin1 loop just above, before
; XXX - the first one out has had time to decrement _bSPValid; this race
; XXX - defeats the purpose of decrementing it. The decrement is likely
; XXX - not needed anyway due to the exclusion provided by the spin2 loop
; XXX - below.
    lock dec byte ptr[_bSPValid]

    mov ax, [_wCSegSS]
    mov ss, ax
    mov sp, [_wCSegSP]

; critical section - determining unique cpu signatures

spin2:
    mov eax, 1
; XXX - Should the following line have a 'lock' prefix?
    xchg    word ptr [_block], ax
    test    ax, ax
    jne spin2

    mov ax, ds          ; Save ASM DS and ES in global vars
    mov [_wASegDS], ax
    mov ax, es
    mov [_wASegES], ax

    mov ax, [_wCSegES]
    mov es, ax
    mov ax, [_wCSegDS]      ; Restore C DS and ES from global vars
    mov ds, ax

; Update Current Processor Number. Mutual exclusion from other AP's is
; guaranteed by the spin2 loop above.
; Note that the "processor number" determined here represents the order
; in which the processors happened to get through the spin2 lock, and
; has nothing to do with APIC numbers. A system having > 1 AP will not
; necessarily number the AP's consistently from one run to the next.
    mov eax, [_dAPDone]
    inc eax
    mov [_wProcNumber], ax

    call    [_dAPFuncPtr]       ; _dAPFuncPtr needs to point to a void function

    mov ax, [_wASegDS]      ; Restore ASM DS and ES from global vars
    mov ds, ax
    mov ax, [_wASegES]
    mov es, ax

    lock    inc dword ptr [_dAPDone]
    lock    inc byte ptr [_bSPValid]

    xor eax, eax
; A simple 'mov' would serve as well as the following 'xchg'.
    xchg    word ptr [_block], ax
    wbinvd
    mov si, offset _dAPHltJmp
    jmp dword ptr [si]      ; Jump to HLT, JMP $-1 in F000 segment
    hlt             ; Should never get here
_vSetupAP   ENDP

;--------------------------------------------------------------------

PUBLIC  bIn8
bIn8    PROC    C wPort:WORD
    xor ax, ax
    mov dx, wPort
    in  al, dx
    ret
bIn8    ENDP

;--------------------------------------------------------------------

PUBLIC  wIn16
wIn16   PROC    C wPort:WORD
    mov dx, wPort
    in  ax, dx
    ret
wIn16   ENDP

;--------------------------------------------------------------------

PUBLIC  dIn32
dIn32   PROC    C wPort:WORD
    mov dx, wPort
    in  eax, dx
    mov edx, eax
    shr edx, 16
    ret
dIn32   ENDP

;--------------------------------------------------------------------

PUBLIC  vOut8
vOut8   PROC    C wPort:WORD, bValue:BYTE
    mov dx, wPort
    mov al, bValue
    out dx, al
    ret
vOut8   ENDP

;--------------------------------------------------------------------

PUBLIC  vOut16
vOut16  PROC    C wPort:WORD, wValue:WORD
    mov dx, wPort
    mov ax, wValue
    out dx, ax
    ret
vOut16  ENDP

;--------------------------------------------------------------------

PUBLIC  vOut32
vOut32  PROC    C wPort:WORD, dValue:DWORD
    mov dx, wPort
    mov eax, dValue
    out dx, eax
    ret
vOut32  ENDP

;--------------------------------------------------------------------

;   int FlatMove (DWORD dPhysicalDestination, DWORD dPhysicalSource, size_t sSize)
PUBLIC  FlatMoveX
FlatMoveX   PROC    C dDest:DWORD, dSrc:DWORD, sSize:WORD
    call    vm86            ; check if VM86 mode
    cmp     ax, 0
    jne     NotRealMode     ; skip memory move if not in real mode

    call    vFlatFsGs       ; set up FS and GS as 4GB limit selectors with
                            ; base 0:0 and de-assert A20M#
    
    movzx   ecx, sSize
    cmp     ecx, 0
    je      SKipMemoryMove  ; skip memory move if 0==sSize

    mov     eax, dDest
    cmp     eax, 400h
    jbe     SkipMemoryMove  ; skip memory move if destination is 40:0
                            ; BIOS Data Area or less (IVT)

    mov     ebx, dSrc
NextByte:
    mov     dl, gs:[ebx][ecx - 1]
    mov     gs:[eax][ecx - 1], dl
    loop    NextByte

    call    vRealFsGs
    xor     ax, ax
NotRealMode:
    ret

SkipMemoryMove:
    mov ax, 1               ; return error code
    ret
FlatMoveX   ENDP


;   int FlatMove32 (DWORD dPhysicalDestination, DWORD dPhysicalSource, size_t sSize)
PUBLIC  FlatMove32
FlatMove32   PROC    C      dDest:DWORD, dSrc:DWORD, sSize:WORD  

    call    vm86            ; check if VM86 mode
    cmp     ax, 0
    jne     NotRealMode     ; skip memory move if not in real mode
     
    call    vFlatFsGs       ; set up FS and GS as 4GB limit selectors with
                            ; base 0:0 and de-assert A20M#
    movzx   edx, sSize 
    mov     esi, dSrc
    mov     edi, dDest

    push    edx
    call    print
    push    esi
    call    print
    push    edi
    call    print
    
    cmp     edx, 0
    je      SkipMemoryMove  ; skip memory move if 0==sSize

    cmp     edi, 400h
    jbe     SkipMemoryMove  ; skip memory move if destination is 40:0
                            ; BIOS Data Area or less (IVT)
    call    DoMove          ; edx=Count, esi=src, edi=dst

    movzx   ecx, sSize
    shr     ecx, 2
    mov     edi, dDest
PrintLoop:
    mov     eax, gs:[edi]
    push    eax
    call    print
    add     edi, 4
    loop    PrintLoop

    call    vRealFsGs
    xor     ax, ax

NotRealMode:
    ret

SkipMemoryMove:
    call    print
    call    vRealEsDs
    mov     ax, 1           ; return error code
    ret         
    
FlatMove32  ENDP


;   int FlatMove32 (DWORD dPhysicalDestination, DWORD dPhysicalSource, size_t sSize)
PUBLIC  FlatMove32x
FlatMove32x   PROC    C dDest:DWORD, dSrc:DWORD, sSize:WORD
    call    vm86            ; check if VM86 mode
    cmp     ax, 0
    jne     NotRealMode     ; skip memory move if not in real mode

    call    vFlatFsGs       ; set up FS and GS as 4GB limit selectors with
                            ; base 0:0 and de-assert A20M#
    movzx   ecx, sSize
    cmp     ecx, 0
    je      SKipMemoryMove  ; skip memory move if 0==sSize

    mov     eax, dDest
    cmp     eax, 400h
    jbe     SkipMemoryMove  ; skip memory move if destination is 40:0
                            ; BIOS Data Area or less (IVT) 
                            
    mov     ebx, dSrc
    mov     esi, 0

    push    ecx
    call    print
    push    ebx
    call    print
    push    eax
    call    print

    shr     ecx, 2          ; byte count / 4 = dword count
    cmp     ecx, 0
    je      DoBytes

NextDword:
    mov     edx, dword ptr gs:[ebx][esi]
    mov     dword ptr gs:[eax][esi], edx
    push    edx
    call    print
    add     esi, 4
    loop    NextDword

DoBytes:
    movzx   ecx, sSize
    and     ecx, 3
    cmp     ecx, 0
    je      Exit            ; no extra bytes to move

NextByte:
    mov     dl, gs:[ebx][esi]
    mov     gs:[eax][esi], dl
    inc     esi
    loop    NextByte

Exit:
    call    vRealFsGs
    xor     ax, ax

NotRealMode:
    ret

SkipMemoryMove:
    mov ax, 1               ; return error code
    ret
FlatMove32x   ENDP
END
