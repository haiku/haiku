/*
 * Copyright 2008 Jan Klötzke
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Emulation based on the Linux Real Mode Interface library.
 * Copyright (C) 1998 by Josh Vanderhoof
 */

#include <vm/vm.h>
#include <thread.h>
#include <vm86.h>
#include <arch_cpu.h>
#include <kernel.h>
#include <ksignal.h>

#include <string.h>
#include <stdlib.h>

//#define TRACE_VM86
#ifdef TRACE_VM86
#	define TRACE(x...)    dprintf("[vm86] " x)
#	define TRACE_NP(x...) dprintf(x)
#else
#	define TRACE(x...)
#	define TRACE_NP(x...)
#endif

#define CLI     0xfa
#define INB     0xec
#define INBI    0xe4
#define INSB    0x6c
#define INSW    0x6d
#define INTn    0xcd
#define INW     0xed
#define INWI    0xe5
#define IRET    0xcf
#define OUTB    0xee
#define OUTBI   0xe6
#define OUTSB   0x6e
#define OUTSW   0x6f
#define OUTW    0xef
#define OUTWI   0xe7
#define POPF    0x9d
#define PUSHF   0x9c
#define STI     0xfb

#define I_FLAG          (1u << 9)
#define DIRECTION_FLAG  (1u << 10)

#define CSEG 0x2e
#define SSEG 0x36
#define DSEG 0x3e
#define ESEG 0x26
#define FSEG 0x64
#define GSEG 0x65


static inline uint16
get_int_seg(int i)
{
	return *(uint16 *)(i * 4 + 2);
}


static inline uint16
get_int_off(int i)
{
	return *(uint16 *)(i * 4);
}


static inline void
pushw(struct vm86_iframe *regs, uint16 i)
{
	regs->esp -= 2;
	*(uint16 *)(((uint32)regs->ss << 4) + regs->esp) = i;
}


static inline void
pushl(struct vm86_iframe *regs, uint32 i)
{
	regs->esp -= 4;
	*(uint32 *)(((uint32)regs->ss << 4) + regs->esp) = i;
}


static inline uint16
popw(struct vm86_iframe *regs)
{
	uint16 ret = *(uint16 *)(((uint32)regs->ss << 4) + regs->esp);
	regs->esp += 2;
	return ret;
}


static inline uint32
popl(struct vm86_iframe *regs)
{
	uint32 ret = *(uint32 *)(((uint32)regs->ss << 4) + regs->esp);
	regs->esp += 4;
	return ret;
}


static void
em_ins(struct vm86_iframe *regs, int size)
{
	unsigned int edx, edi;

	edx = regs->edx & 0xffff;
	edi = regs->edi & 0xffff;
	edi += (unsigned int)regs->es << 4;

	if (regs->flags & DIRECTION_FLAG) {
		if (size == 4)
			asm volatile ("std; insl; cld" : "=D" (edi) : "d" (edx), "0" (edi));
		else if (size == 2)
			asm volatile ("std; insw; cld" : "=D" (edi) : "d" (edx), "0" (edi));
		else
			asm volatile ("std; insb; cld" : "=D" (edi) : "d" (edx), "0" (edi));
	} else {
		if (size == 4)
			asm volatile ("cld; insl" : "=D" (edi) : "d" (edx), "0" (edi));
		else if (size == 2)
			asm volatile ("cld; insw" : "=D" (edi) : "d" (edx), "0" (edi));
		else
			asm volatile ("cld; insb" : "=D" (edi) : "d" (edx), "0" (edi));
	}

	edi -= (unsigned int)regs->es << 4;

	regs->edi &= 0xffff0000;
	regs->edi |= edi & 0xffff;
}


static void
em_rep_ins(struct vm86_iframe *regs, int size)
{
	unsigned int cx;

	cx = regs->ecx & 0xffff;

	while (cx--)
		em_ins(regs, size);

	regs->ecx &= 0xffff0000;
}


static void
em_outs(struct vm86_iframe *regs, int size, int seg)
{
	unsigned int edx, esi, base;

	edx = regs->edx & 0xffff;
	esi = regs->esi & 0xffff;

	switch (seg) {
		case CSEG: base = regs->cs; break;
		case SSEG: base = regs->ss; break;
		case ESEG: base = regs->es; break;
		case FSEG: base = regs->fs; break;
		case GSEG: base = regs->gs; break;
		case DSEG:
		default:
			base = regs->ds;
			break;
	}

	esi += base << 4;

	if (regs->flags & DIRECTION_FLAG) {
		if (size == 4) {
			asm volatile ("std; outsl; cld"
				: "=S" (esi) : "d" (edx), "0" (esi));
		} else if (size == 2) {
			asm volatile ("std; outsw; cld"
				: "=S" (esi) : "d" (edx), "0" (esi));
		} else {
			asm volatile ("std; outsb; cld"
				: "=S" (esi) : "d" (edx), "0" (esi));
		}
	} else {
		if (size == 4)
			asm volatile ("cld; outsl" : "=S" (esi) : "d" (edx), "0" (esi));
		else if (size == 2)
			asm volatile ("cld; outsw" : "=S" (esi) : "d" (edx), "0" (esi));
		else
			asm volatile ("cld; outsb" : "=S" (esi) : "d" (edx), "0" (esi));
	}

	esi -= base << 4;

	regs->esi &= 0xffff0000;
	regs->esi |= esi & 0xffff;
}


static void
em_rep_outs(struct vm86_iframe *regs, int size, int seg)
{
	unsigned int cx;

	cx = regs->ecx & 0xffff;

	while (cx--)
		em_outs(regs, size, seg);

	regs->ecx &= 0xffff0000;
}


static inline void
em_inb(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("inb %w1, %b0"
		: "=a" (regs->eax)
		: "d" (port), "0" (regs->eax));
}


static inline void
em_inw(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("inw %w1, %w0"
		: "=a" (regs->eax)
		: "d" (port), "0" (regs->eax));
}


static inline void
em_inl(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("inl %w1, %0" : "=a" (regs->eax) : "d" (port));
}


static inline void
em_outb(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("outb %b0, %w1" : : "a" (regs->eax), "d" (port));
}


static inline void
em_outw(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("outw %w0, %w1" : : "a" (regs->eax), "d" (port));
}


static inline void
em_outl(struct vm86_iframe *regs, uint32 port)
{
	asm volatile ("outl %0, %w1" : : "a" (regs->eax), "d" (port));
}


static int
emulate(struct vm86_state *state)
{
	unsigned char *instruction;
	struct {
		unsigned char seg;
		unsigned int size : 1;
		unsigned int rep : 1;
	} prefix = { DSEG, 0, 0 };
	int ret = 0;

	instruction = (unsigned char *)((unsigned int)state->regs.cs << 4);
	instruction += state->regs.eip;

	TRACE("emulate: ");

	/* handle prefixes */
	do {
		switch (*instruction) {
			case 0x66:
				TRACE_NP("[SIZE] ");
				prefix.size = 1 - prefix.size;
				state->regs.eip++;
				instruction++;
				break;
			case 0xf3:
				TRACE_NP("REP ");
				prefix.rep = 1;
				state->regs.eip++;
				instruction++;
				break;
			case CSEG:
			case SSEG:
			case DSEG:
			case ESEG:
			case FSEG:
			case GSEG:
				TRACE_NP("SEG(0x%02x) ", *instruction);
				prefix.seg = *instruction;
				state->regs.eip++;
				instruction++;
				break;
			case 0xf0:
			case 0xf2:
			case 0x67:
				/* these prefixes are just ignored */
				TRACE_NP("IGN(0x%02x) ", *instruction);
				state->regs.eip++;
				instruction++;
				break;
			default:
				ret = 1;
		}
	} while (!ret);

	/* handle actual instruction */
	ret = 0;
	switch (*instruction) {
		case INSB:
			TRACE_NP("INSB");
			if (prefix.rep)
				em_rep_ins(&state->regs, 1);
			else
				em_ins(&state->regs, 1);
			state->regs.eip++;
			break;
		case INSW:
			TRACE_NP("INSW");
			if (prefix.rep) {
				if (prefix.size)
					em_rep_ins(&state->regs, 4);
				else
					em_rep_ins(&state->regs, 2);
			} else {
				if (prefix.size)
					em_ins(&state->regs, 4);
				else
					em_ins(&state->regs, 2);
			}
			state->regs.eip++;
			break;
		case OUTSB:
			TRACE_NP("OUTSB");
			if (prefix.rep)
				em_rep_outs(&state->regs, 1, prefix.seg);
			else
				em_outs(&state->regs, 1, prefix.seg);
			state->regs.eip++;
			break;
		case OUTSW:
			TRACE_NP("OUTSW");
			if (prefix.rep) {
				if (prefix.size)
					em_rep_outs(&state->regs, 4, prefix.seg);
				else
					em_rep_outs(&state->regs, 2, prefix.seg);
			} else {
				if (prefix.size)
					em_outs(&state->regs, 4, prefix.seg);
				else
					em_outs(&state->regs, 2, prefix.seg);
			}
			state->regs.eip++;
			break;
		case INBI:
			em_inb(&state->regs, instruction[1]);
			TRACE_NP("IN al(=0x%02lx), 0x%02x", state->regs.eax & 0xff,
				instruction[1]);
			state->regs.eip += 2;
			break;
		case INWI:
			if (prefix.size) {
				em_inl(&state->regs, instruction[1]);
				TRACE_NP("IN eax(=0x%08lx), 0x%02x", state->regs.eax,
					instruction[1]);
			} else {
				em_inw(&state->regs, instruction[1]);
				TRACE_NP("IN ax(=0x%04lx), 0x%02x", state->regs.eax & 0xffff,
					instruction[1]);
			}
			state->regs.eip += 2;
			break;
		case INB:
			em_inb(&state->regs, state->regs.edx);
			TRACE_NP("IN al(=0x%02lx), dx(0x%04lx)", state->regs.edx & 0xff,
				state->regs.edx & 0xffff);
			state->regs.eip++;
			break;
		case INW:
			if (prefix.size) {
				em_inl(&state->regs, state->regs.edx);
				TRACE_NP("IN eax(=0x%08lx), dx(0x%04lx)", state->regs.edx,
					state->regs.edx & 0xffff);
			} else {
				em_inw(&state->regs, state->regs.edx);
				TRACE_NP("IN ax(=0x%04lx), dx(0x%04lx)",
					state->regs.edx & 0xffff, state->regs.edx & 0xffff);
			}
			state->regs.eip++;
			break;
		case OUTBI:
			em_outb(&state->regs, instruction[1]);
			TRACE_NP("OUT 0x%02x, al(0x%02lx)", instruction[1],
				state->regs.eax & 0xff);
			state->regs.eip += 2;
			break;
		case OUTWI:
			if (prefix.size) {
				em_outl(&state->regs, instruction[1]);
				TRACE_NP("OUT 0x%02x, eax(0x%08lx)", instruction[1],
					state->regs.eax);
			} else {
				em_outw(&state->regs, instruction[1]);
				TRACE_NP("OUT 0x%02x, ax(0x%04lx)", instruction[1],
					state->regs.eax & 0xffff);
			}
			state->regs.eip += 2;
			break;
		case OUTB:
			em_outb(&state->regs, state->regs.edx);
			TRACE_NP("OUT dx(0x%04lx), al(0x%02lx)", state->regs.edx & 0xffff,
				state->regs.eax & 0xff);
			state->regs.eip++;
			break;
		case OUTW:
			if (prefix.size) {
				em_outl(&state->regs, state->regs.edx);
				TRACE_NP("OUT dx(0x%04lx), eax(0x%08lx)",
					state->regs.edx & 0xffff, state->regs.eax);
			} else {
				em_outw(&state->regs, state->regs.edx);
				TRACE_NP("OUT dx(0x%04lx), ax(0x%04lx)",
					state->regs.edx & 0xffff, state->regs.eax & 0xffff);
			}
			state->regs.eip++;
			break;
		case INTn:
			TRACE_NP("INT 0x%02x", instruction[1]);
			if (instruction[1] == RETURN_TO_32_INT) {
				ret = 1;
			} else {
				uint16 flags = state->regs.flags;

				/* store real IF */
				flags &= ~I_FLAG;
				flags |= (uint16)state->if_flag << 9;

				pushw(&state->regs, flags);
				pushw(&state->regs, state->regs.cs);
				pushw(&state->regs, state->regs.eip + 2);

				state->regs.cs = get_int_seg(instruction[1]);
				state->regs.eip = get_int_off(instruction[1]);
				state->if_flag = 0;
			}
			break;
		case IRET:
			TRACE_NP("IRET");
			state->regs.eip = popw(&state->regs);
			state->regs.cs = popw(&state->regs);
			state->regs.flags = popw(&state->regs);
			/* restore IF */
			state->if_flag = (state->regs.flags >> 9) & 0x01;
			break;
		case CLI:
			TRACE_NP("CLI");
			state->if_flag = 0;
			state->regs.eip++;
			break;
		case STI:
			TRACE_NP("STI");
			state->if_flag = 1;
			state->regs.eip++;
			break;
		case PUSHF:
		{
			TRACE_NP("PUSHF");
			uint32 flags = state->regs.flags;

			/* store real IF */
			flags &= ~I_FLAG;
			flags |= (uint32)state->if_flag << 9;
			if (prefix.size)
				pushl(&state->regs, flags);
			else
				pushw(&state->regs, flags);
			state->regs.eip++;
			break;
		}
		case POPF:
		{
			TRACE_NP("POPF");
			if (prefix.size)
				state->regs.flags = popl(&state->regs);
			else
				state->regs.flags = popw(&state->regs);
			/* restore IF */
			state->if_flag = (state->regs.flags >> 9) & 0x01;
			state->regs.eip++;
			break;
		}
		default:
			TRACE_NP("UNK");
			ret = -1;
	}

	TRACE_NP("\n");
	return ret;
}


static bool
vm86_fault_callback(addr_t address, addr_t faultAddress, bool isWrite)
{
	struct iframe *frame = i386_get_user_iframe();

	TRACE("Unhandled fault at %#" B_PRIxADDR " touching %#" B_PRIxADDR
		"while %s\n", faultAddress, address, isWrite ? "writing" : "reading");

	// we shouldn't have unhandled page faults in vm86 mode
	x86_vm86_return((struct vm86_iframe *)frame, B_BAD_ADDRESS);

	// not reached
	return false;
}


//	#pragma mark - exported interface


/*! Prepare the thread to execute BIOS code in virtual 8086 mode. Initializes
	the given \a state to default values and maps the BIOS into the teams
	address space. The size of the available conventional RAM is given in
	\a ramSize in bytes which should be greater or equal to VM86_MIN_RAM_SIZE.
*/
extern "C" status_t
vm86_prepare(struct vm86_state *state, unsigned int ramSize)
{
	Team *team = thread_get_current_thread()->team;
	status_t ret = B_OK;
	area_id vectors;

	state->bios_area = B_ERROR;

	// create RAM area
	if (ramSize < VM86_MIN_RAM_SIZE)
		ramSize = VM86_MIN_RAM_SIZE;

	void *address;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = NULL;
	virtualRestrictions.address_specification = B_EXACT_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	state->ram_area = create_area_etc(team->id, "dos", ramSize, B_NO_LOCK,
		B_READ_AREA | B_WRITE_AREA, 0, &virtualRestrictions,
		&physicalRestrictions, &address);
	if (state->ram_area < B_OK) {
		ret = state->ram_area;
		TRACE("Could not create RAM area\n");
		goto error;
	}

	// copy int vectors and BIOS data area
	vectors = map_physical_memory("int vectors", 0, 0x502,
		B_ANY_KERNEL_BLOCK_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		&address);
	if (vectors < B_OK) {
		ret = vectors;
		TRACE("Could not copy vectors\n");
		goto error;
	}
	ret = user_memcpy((void *)0, address, 0x502);
	*((uint32 *)0) = 0xdeadbeef;
	delete_area(vectors);
	if (ret != B_OK)
		goto error;

	// map vga/bios area
	address = (void *)0xa0000;
	state->bios_area = vm_map_physical_memory(team->id, "bios",
		&address, B_EXACT_ADDRESS, 0x60000,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA,
		(addr_t)0xa0000, false);
	if (state->bios_area < B_OK) {
		ret = state->bios_area;
		TRACE("Could not map VGA BIOS.\n");
		goto error;
	}

	return B_OK;

error:
	if (state->bios_area > B_OK)
		vm_delete_area(team->id, state->bios_area, true);
	if (state->ram_area > B_OK)
		vm_delete_area(team->id, state->ram_area, true);
	return ret;
}


/*!	Free ressources which were allocated by vm86_prepare().
*/
extern "C" void
vm86_cleanup(struct vm86_state *state)
{
	Team *team = thread_get_current_thread()->team;

	if (state->bios_area > B_OK)
		vm_delete_area(team->id, state->bios_area, true);
	if (state->ram_area > B_OK)
		vm_delete_area(team->id, state->ram_area, true);
}


/*!	Execute a BIOS call of interrupt \a vec. The given \a state must be
	initialized by vm86_prepare() before, any registers needed by the BIOS too.

	The function will return B_OK if the BIOS was called successfully,
	otherwise an apropriate error code. After the call the registers are
	copied back to \a state to reflect the status after the BIOS returned.

	Any buffer which is given to the BIOS function may be allocated starting
	from address 0x1000 up to the allocated RAM size (see vm86_prepare()). The
	area below 0x1000 is not available because it is used for the interrupt
	vector table, BIOS data area and as real mode stack.
*/
extern "C" status_t
vm86_do_int(struct vm86_state *state, uint8 vec)
{
	int8 *ip;
	int emuState = 0;
	Thread *thread = thread_get_current_thread();
	status_t ret;

	// prepare environment
	state->regs.ss  = 0x600 >> 4;
	state->regs.esp = (0x1000 - 0x600);
	state->regs.cs = get_int_seg(vec);
	state->regs.eip = get_int_off(vec);
	state->if_flag = 0;

	pushw(&state->regs, state->regs.flags);
	pushw(&state->regs, 0x0000); /* CS */
	pushw(&state->regs, 0x0600); /* IP */

	ip = (int8 *)0x600;
	*ip++ = INTn;
	*ip++ = RETURN_TO_32_INT;

	// execute interrupt
	thread->fault_callback = &vm86_fault_callback;
	do {
		ret = x86_vm86_enter(&state->regs);
		if (ret != B_OK)
			break;
		emuState = emulate(state);
	} while (emuState == 0);
	thread->fault_callback = NULL;

	// We might have clobbered %fs, so we need to restore the CPU dependent
	// TLS context that we may have overwritten. Note that we can't simply
	// restore %fs to its previous value as we might not run on the same CPU
	// anymore.
	x86_set_tls_context(thread);

	return emuState < 0 ? B_BAD_DATA : ret;
}

