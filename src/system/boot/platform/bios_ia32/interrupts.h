/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H


#ifndef _ASSEMBLER


#include <sys/cdefs.h>

#include <SupportDefs.h>


struct gdt_idt_descr {
	uint16	limit;
	void*	base;
} _PACKED;


__BEGIN_DECLS


void interrupts_init();
void set_debug_idt();
void restore_bios_idt();
void interrupts_init_kernel_idt(void* idt, size_t idtSize);


__END_DECLS


#endif	// _ASSEMBLER

#define INTERRUPT_FUNCTION_ERROR(vector)	INTERRUPT_FUNCTION(vector)

#define INTERRUPT_FUNCTIONS5(vector1, vector2, vector3, vector4, vector5)	\
	INTERRUPT_FUNCTION(vector1)												\
	INTERRUPT_FUNCTION(vector2)												\
	INTERRUPT_FUNCTION(vector3)												\
	INTERRUPT_FUNCTION(vector4)												\
	INTERRUPT_FUNCTION(vector5)

#define INTERRUPT_FUNCTIONS()				\
	INTERRUPT_FUNCTIONS5(0, 1, 2, 3, 4)		\
	INTERRUPT_FUNCTIONS5(5, 6, 7, 8, 9)		\
	INTERRUPT_FUNCTION_ERROR(10)			\
	INTERRUPT_FUNCTION_ERROR(11)			\
	INTERRUPT_FUNCTION_ERROR(12)			\
	INTERRUPT_FUNCTION_ERROR(13)			\
	INTERRUPT_FUNCTION_ERROR(14)			\
	INTERRUPT_FUNCTION(15)					\
	INTERRUPT_FUNCTION(16)					\
	INTERRUPT_FUNCTION_ERROR(17)			\
	INTERRUPT_FUNCTION(18)					\
	INTERRUPT_FUNCTION(19)

#define DEBUG_IDT_SLOT_COUNT	20


#endif	// INTERRUPTS_H
