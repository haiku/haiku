/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include <sys/cdefs.h>


uint16 calculate_crc(uint16 crc, uint8 *data, uint16 length);


uint32 calculate_crc32c(uint32 crc32c, const unsigned char *buffer,
	unsigned int length);


