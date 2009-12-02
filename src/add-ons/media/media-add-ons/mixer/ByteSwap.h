/*
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BYTE_SWAP_H
#define _BYTE_SWAP_H


#include <SupportDefs.h>


class ByteSwap {
public:
								ByteSwap(uint32 format);
								~ByteSwap();

			void				Swap(void *buffer, size_t bytecount);

private:
			void 				(*fFunc)(void *, size_t);
};


inline void
ByteSwap::Swap(void *buffer, size_t bytecount)
{
	(*fFunc)(buffer, bytecount);
}

#endif	// _BYTE_SWAP_H
