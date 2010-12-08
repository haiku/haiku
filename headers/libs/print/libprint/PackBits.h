/*
 * PackBits.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PACKBITS_H
#define __PACKBITS_H

int	pack_bits_size(const unsigned char* source, int size);
int	pack_bits(unsigned char* destination, const unsigned char* source,
	int size);

#endif	/* __PACKBITS_H */
