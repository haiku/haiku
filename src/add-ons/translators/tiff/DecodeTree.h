/*****************************************************************************/
// DecodeTree
// Written by Michael Wilber, OBOS Translation Kit Team
//
// DecodeTree.h
//
// This object is used for fast decoding of Huffman encoded data
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef DECODE_TREE_H
#define DECODE_TREE_H

#include <SupportDefs.h>
#include "BitReader.h"

// structure used to populate
// the decode trees
struct HuffmanEncoding {
	uint16 value;
	uint8 encoding;
	uint8 length;
};

// node used by the decode tree
struct DecodeNode {
	int16 value;
	DecodeNode *branches[2];
};

class DecodeTree {
public:
	DecodeTree(bool bwhite);
	~DecodeTree();
	
	// returns B_OK if the tree was initialized without error,
	// returns B_ERROR or B_NO_MEMORY if unable to initialize
	status_t InitCheck() const { return finitStatus; };
	
	// Decodes nbits bits from encdata, starting with the 
	// highest order bit.  If successful, returns the corresponding 
	// value for the encdata and bitsread contains the 
	// number of bits from encdata that were decoded.  If not successful,
	// B_ERROR, B_NO_MEMORY or B_BAD_VALUE is returned
	status_t GetValue(BitReader &stream) const;

private:
	status_t LoadBlackEncodings();
	status_t LoadWhiteEncodings();
	status_t AddEncodings(const HuffmanEncoding *pencs, uint32 length);
	status_t AddEncoding(uint16 encoding, uint8 length, uint16 value);
	
	status_t finitStatus;
	DecodeNode *fptop;
};

#endif
