/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2002 Thomas Moestl <tmm@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef FAT_ENDIAN_H
#define FAT_ENDIAN_H


// Modified to support the Haiku FAT driver.

#ifndef FS_SHELL
#include <ByteOrder.h>
#endif


static __inline uint16_t
le16dec(const void* pp)
{
	uint16 const* twoBytes = (uint16 const*)pp;
	return B_LENDIAN_TO_HOST_INT16(*twoBytes);
}


static __inline uint32_t
le32dec(const void* pp)
{
	uint32 const* fourBytes = (uint32 const*)pp;
	return B_LENDIAN_TO_HOST_INT32(*fourBytes);
}


static __inline void
le16enc(void* pp, uint16_t u)
{
	uint16* twoBytes = (uint16*)pp;
	*twoBytes = B_HOST_TO_LENDIAN_INT16(u);
}


static __inline void
le32enc(void* pp, uint32_t u)
{
	uint32* fourBytes = (uint32*)pp;
	*fourBytes = B_HOST_TO_LENDIAN_INT32(u);
}


#endif // FAT_ENDIAN_H
