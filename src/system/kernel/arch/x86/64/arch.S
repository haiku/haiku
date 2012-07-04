/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text


/* void x86_fxsave(void* fpuState); */
FUNCTION(x86_fxsave):
	fxsave	(%rdi)
	ret
FUNCTION_END(x86_fxsave)


/* void x86_fxrstor(const void* fpuState); */
FUNCTION(x86_fxrstor):
	fxrstor	(%rdi)
	ret
FUNCTION_END(x86_fxrstor)


/* void x86_fxsave_swap(void* oldFpuState, const void* newFpuState); */
FUNCTION(x86_fxsave_swap):
	fxsave	(%rdi)
	fxrstor	(%rsi)
	ret
FUNCTION_END(x86_fxsave_swap)


/* uint64 x86_read_msr(uint32 register); */
FUNCTION(x86_read_msr):
	mov		%edi, %ecx
	rdmsr
	shl		$32, %rdx
	mov		%eax, %eax
	or		%rdx, %rax
	ret
FUNCTION_END(x86_read_msr)


/* void x86_write_msr(uint32 register, uint64 value); */
FUNCTION(x86_write_msr):
	mov		%rsi, %rdx
	mov		%esi, %eax
	mov		%edi, %ecx
	shr		$32, %rdx
	wrmsr
	ret
FUNCTION_END(x86_write_msr)


null_idt_descr:
	.word	0
	.quad	0

FUNCTION(x86_reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done
FUNCTION_END(x86_reboot)
