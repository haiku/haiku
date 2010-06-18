/*
 * Copyright (c) 2010, David McPaul
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BitParser.h"

#include <math.h>

BitParser::BitParser(uint8 *bits, uint32 bitLength) {
	this->bits = bits;
	this->bitLength = bitLength;
	this->index = 0;
}

BitParser::BitParser() {
	bits = NULL;
	bitLength = 0;
	index = 1;
}

BitParser::~BitParser() {
	bits = NULL;
}

void
BitParser::Init(uint8 *bits, uint32 bitLength) {
	this->bits = bits;
	this->bitLength = bitLength;
	this->index = 1;
}

void
BitParser::Skip(uint8 length) {
	if (bitLength >= length) {

		while (length-- > 0) {
			index++;
			bitLength--;

			if (index > 8) {
				index = 1;
				bits++;
			}
		}
	}
}

uint32
BitParser::GetValue(uint8 length) {
	uint32 result = 0;
	
	if (bitLength < length) {
		return 0;
	}
	
	while (length-- > 0) {
		result = result << 1;
		result += (*bits) & (uint8)(pow(2, 8-index)) ? 1 : 0;
		index++;
		bitLength--;

		if (index > 8) {
			index = 1;
			bits++;
		}
	}
	
	return result;
}
