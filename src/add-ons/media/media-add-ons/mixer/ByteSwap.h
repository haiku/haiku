#ifndef _BYTE_SWAP_H
#define _BYTE_SWAP_H

/* Copyright (C) 2003 Marcus Overhagen
 * Released under terms of the MIT license.
 *
 * A simple byte order swapping class for the audio mixer.
 */

class ByteSwap
{
public:
	ByteSwap(uint32 format);
	~ByteSwap();
	
	void Swap(void *buffer, size_t bytecount);

private:
	void (*fFunc)(void *, size_t);
};

inline void
ByteSwap::Swap(void *buffer, size_t bytecount)
{
	(*fFunc)(buffer, bytecount);
}

#endif
