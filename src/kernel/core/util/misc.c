/* This includes some functions that were part of the net piece */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <ktypes.h>
#include <debug.h>

/* XXX - keep the compiler happy. */
uint16 ones_sum16(uint32 sum, const void *_buf, int len);
uint16 cksum16(void *_buf, int len);
uint16 cksum16_2(void *buf1, int len1, void *buf2, int len2);

uint16 ones_sum16(uint32 sum, const void *_buf, int len)
{
	const uint16 *buf = _buf;

	while(len >= 2) {
		sum += *buf++;
		if(sum & 0x80000000)
			sum = (sum & 0xffff) + (sum >> 16);
		len -= 2;
	}

	if (len) {
		uint8 temp[2];
		temp[0] = *(uint8 *) buf;
		temp[1] = 0;
		sum += *(uint16 *) temp;
	}

	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return sum;
}

uint16 cksum16(void *_buf, int len)
{
	return ~ones_sum16(0, _buf, len);
}

uint16 cksum16_2(void *buf1, int len1, void *buf2, int len2)
{
	uint32 sum;

	sum = ones_sum16(0, buf1, len1);
	return ~ones_sum16(sum, buf2, len2);
}
