/*
 * Copyright 2009, Colin Günther. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*-
 * Copyright (c) KATO Takenori, 1999.
 *
 * All rights reserved.  Unpublished rights reserved under the copyright
 * laws of Japan.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*	$NetBSD: bus.h,v 1.12 1997/10/01 08:25:15 fvdl Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FBSD_COMPAT_MACHINE_BUS_H_
#define _FBSD_COMPAT_MACHINE_BUS_H_


#include <machine/_bus.h>
#include <machine/cpufunc.h>


// TODO: x86 specific!
#define I386_BUS_SPACE_IO			0
#define I386_BUS_SPACE_MEM			1

#define BUS_SPACE_MAXADDR_32BIT		0xffffffff
#ifdef __x86_64__
#	define BUS_SPACE_MAXADDR		0xffffffffffffffffull
#else
#	define BUS_SPACE_MAXADDR		0xffffffff
#endif

#define BUS_SPACE_MAXSIZE_32BIT		0xffffffff
#define BUS_SPACE_MAXSIZE			0xffffffff

#define BUS_SPACE_UNRESTRICTED	(~0)

#define BUS_SPACE_BARRIER_READ		1
#define BUS_SPACE_BARRIER_WRITE		2


uint8_t bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint16_t bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint32_t bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
void bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint8_t value);
void bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint16_t value);
void bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint32_t value);


static inline void
bus_space_read_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
			bus_size_t offset, u_int8_t *addr, size_t count)
{
	if (tag == I386_BUS_SPACE_IO) {
		int _port_ = bsh + offset;
		__asm __volatile("							\n\
			cld										\n\
		1:	inb %w2,%%al							\n\
			stosb									\n\
			incl %2									\n\
			loop 1b"								:
		    "=D" (addr), "=c" (count), "=d" (_port_):
		    "0" (addr), "1" (count), "2" (_port_)	:
		    "%eax", "memory", "cc");
	} else {
		void* _port_ = (void*) (bsh + offset);
		memcpy(addr, _port_, count);
	}
}


static inline void
bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, u_int32_t *addr, size_t count)
{
	if (tag == I386_BUS_SPACE_IO) {
		int _port_ = bsh + offset;
		__asm __volatile("							\n\
			cld										\n\
		1:	inl %w2,%%eax							\n\
			stosl									\n\
			addl $4,%2								\n\
			loop 1b"								:
			"=D" (addr), "=c" (count), "=d" (_port_):
			"0" (addr), "1" (count), "2" (_port_)	:
			"%eax", "memory", "cc");
	} else {
		void* _port_ = (void*) (bsh + offset);
		memcpy(addr, _port_, count);
	}
}


static inline void
bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, const u_int8_t *addr, size_t count)
{
	if (tag == I386_BUS_SPACE_IO) {
		int _port_ = bsh + offset;
		__asm __volatile("							\n\
			cld										\n\
		1:	lodsb									\n\
			outb %%al,%w0							\n\
			incl %0									\n\
			loop 1b"								:
			"=d" (_port_), "=S" (addr), "=c" (count):
			"0" (_port_), "1" (addr), "2" (count)	:
			"%eax", "memory", "cc");
	} else {
		void* _port_ = (void*) (bsh + offset);
		memcpy(_port_, addr, count);
	}
}


static inline void
bus_space_barrier(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, bus_size_t len, int flags)
{
	if (flags & BUS_SPACE_BARRIER_READ)
		__asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
	else
		__asm__ __volatile__ ("" : : : "memory");
}


static inline void
bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, const u_int8_t *addr, size_t count)
{

	if (tag == I386_BUS_SPACE_IO)
		outsb(bsh + offset, addr, count);
	else {
		__asm __volatile("								\n\
			cld											\n\
		1:	lodsb										\n\
			movb %%al,(%2)								\n\
			loop 1b"									:
			"=S" (addr), "=c" (count)					:
			"r" (bsh + offset), "0" (addr), "1" (count)	:
			"%eax", "memory", "cc");
	}
}


#include <machine/bus_dma.h>


#define bus_space_write_stream_4(t, h, o, v) \
	bus_space_write_4((t), (h), (o), (v))

#endif /* _FBSD_COMPAT_MACHINE_BUS_H_ */
