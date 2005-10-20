/*****************************************************************************/
// DecodeTree
// Written by Michael Wilber, OBOS Translation Kit Team
//
// DecodeTree.cpp
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

#include "DecodeTree.h"

// Encodings used to populate
// the decode trees
const HuffmanEncoding g_encWhiteTerm[] = {
	{0,   0x35,  8},  // "00110101"
	{1,   0x07,  6},  // "000111"
	{2,   0x07,  4},  // "0111"
	{3,   0x08,  4},  // "1000"
	{4,   0x0b,  4},  // "1011"
	{5,   0x0c,  4},  // "1100"
	{6,   0x0e,  4},  // "1110"
	{7,   0x0f,  4},  // "1111"
	{8,   0x13,  5},  // "10011"
	{9,   0x14,  5},  // "10100"
	{10,  0x07,  5},  // "00111"
	{11,  0x08,  5},  // "01000"
	{12,  0x08,  6},  // "001000"
	{13,  0x03,  6},  // "000011"
	{14,  0x34,  6},  // "110100"
	{15,  0x35,  6},  // "110101"
	{16,  0x2a,  6},  // "101010"
	{17,  0x2b,  6},  // "101011"
	{18,  0x27,  7},  // "0100111"
	{19,  0x0c,  7},  // "0001100"
	{20,  0x08,  7},  // "0001000"
	{21,  0x17,  7},  // "0010111"
	{22,  0x03,  7},  // "0000011"
	{23,  0x04,  7},  // "0000100"
	{24,  0x28,  7},  // "0101000"
	{25,  0x2b,  7},  // "0101011"
	{26,  0x13,  7},  // "0010011"
	{27,  0x24,  7},  // "0100100"
	{28,  0x18,  7},  // "0011000"
	{29,  0x02,  8},  // "00000010"
	{30,  0x03,  8},  // "00000011"
	{31,  0x1a,  8},  // "00011010"
	{32,  0x1b,  8},  // "00011011"
	{33,  0x12,  8},  // "00010010"
	{34,  0x13,  8},  // "00010011"
	{35,  0x14,  8},  // "00010100"
	{36,  0x15,  8},  // "00010101"
	{37,  0x16,  8},  // "00010110"
	{38,  0x17,  8},  // "00010111"
	{39,  0x28,  8},  // "00101000"
	{40,  0x29,  8},  // "00101001"
	{41,  0x2a,  8},  // "00101010"
	{42,  0x2b,  8},  // "00101011"
	{43,  0x2c,  8},  // "00101100"
	{44,  0x2d,  8},  // "00101101"
	{45,  0x04,  8},  // "00000100"
	{46,  0x05,  8},  // "00000101"
	{47,  0x0a,  8},  // "00001010"
	{48,  0x0b,  8},  // "00001011"
	{49,  0x52,  8},  // "01010010"
	{50,  0x53,  8},  // "01010011"
	{51,  0x54,  8},  // "01010100"
	{52,  0x55,  8},  // "01010101"
	{53,  0x24,  8},  // "00100100"
	{54,  0x25,  8},  // "00100101"
	{55,  0x58,  8},  // "01011000"
	{56,  0x59,  8},  // "01011001"
	{57,  0x5a,  8},  // "01011010"
	{58,  0x5b,  8},  // "01011011"
	{59,  0x4a,  8},  // "01001010"
	{60,  0x4b,  8},  // "01001011"
	{61,  0x32,  8},  // "00110010"
	{62,  0x33,  8},  // "00110011"
	{63,  0x34,  8},  // "00110100"
}; // g_encWhiteTerm

const HuffmanEncoding g_encWhiteMakeup[] = {
	{64,    0x1b,  5},  // "11011"
	{128,   0x12,  5},  // "10010"
	{192,   0x17,  6},  // "010111"
	{256,   0x37,  7},  // "0110111"
	{320,   0x36,  8},  // "00110110"
	{384,   0x37,  8},  // "00110111"
	{448,   0x64,  8},  // "01100100"
	{512,   0x65,  8},  // "01100101"
	{576,   0x68,  8},  // "01101000"
	{640,   0x67,  8},  // "01100111"
	{704,   0xcc,  9},  // "011001100"
	{768,   0xcd,  9},  // "011001101"
	{832,   0xd2,  9},  // "011010010"
	{896,   0xd3,  9},  // "011010011"
	{960,   0xd4,  9},  // "011010100"
	{1024,  0xd5,  9},  // "011010101"
	{1088,  0xd6,  9},  // "011010110"
	{1152,  0xd7,  9},  // "011010111"
	{1216,  0xd8,  9},  // "011011000"
	{1280,  0xd9,  9},  // "011011001"
	{1344,  0xda,  9},  // "011011010"
	{1408,  0xdb,  9},  // "011011011"
	{1472,  0x98,  9},  // "010011000"
	{1536,  0x99,  9},  // "010011001"
	{1600,  0x9a,  9},  // "010011010"
	{1664,  0x18,  6},  // "011000"
	{1728,  0x9b,  9},  // "010011011"
}; // g_encWhiteMakeup

const HuffmanEncoding g_encBlackTerm[] = {
	{0,   0x37,  10},  // "0000110111"
	{1,   0x02,   3},  // "010"
	{2,   0x03,   2},  // "11"
	{3,   0x02,   2},  // "10"
	{4,   0x03,   3},  // "011"
	{5,   0x03,   4},  // "0011"
	{6,   0x02,   4},  // "0010"
	{7,   0x03,   5},  // "00011"
	{8,   0x05,   6},  // "000101"
	{9,   0x04,   6},  // "000100"
	{10,  0x04,   7},  // "0000100"
	{11,  0x05,   7},  // "0000101"
	{12,  0x07,   7},  // "0000111"
	{13,  0x04,   8},  // "00000100"
	{14,  0x07,   8},  // "00000111"
	{15,  0x18,   9},  // "000011000"
	{16,  0x17,  10},  // "0000010111"
	{17,  0x18,  10},  // "0000011000"
	{18,  0x08,  10},  // "0000001000"
	{19,  0x67,  11},  // "00001100111"
	{20,  0x68,  11},  // "00001101000"
	{21,  0x6c,  11},  // "00001101100"
	{22,  0x37,  11},  // "00000110111"
	{23,  0x28,  11},  // "00000101000"
	{24,  0x17,  11},  // "00000010111"
	{25,  0x18,  11},  // "00000011000"
	{26,  0xca,  12},  // "000011001010"
	{27,  0xcb,  12},  // "000011001011"
	{28,  0xcc,  12},  // "000011001100"
	{29,  0xcd,  12},  // "000011001101"
	{30,  0x68,  12},  // "000001101000"
	{31,  0x69,  12},  // "000001101001"
	{32,  0x6a,  12},  // "000001101010"
	{33,  0x6b,  12},  // "000001101011"
	{34,  0xd2,  12},  // "000011010010"
	{35,  0xd3,  12},  // "000011010011"
	{36,  0xd4,  12},  // "000011010100"
	{37,  0xd5,  12},  // "000011010101"
	{38,  0xd6,  12},  // "000011010110"
	{39,  0xd7,  12},  // "000011010111"
	{40,  0x6c,  12},  // "000001101100"
	{41,  0x6d,  12},  // "000001101101"
	{42,  0xda,  12},  // "000011011010"
	{43,  0xdb,  12},  // "000011011011"
	{44,  0x54,  12},  // "000001010100"
	{45,  0x55,  12},  // "000001010101"
	{46,  0x56,  12},  // "000001010110"
	{47,  0x57,  12},  // "000001010111"
	{48,  0x64,  12},  // "000001100100"
	{49,  0x65,  12},  // "000001100101"
	{50,  0x52,  12},  // "000001010010"
	{51,  0x53,  12},  // "000001010011"
	{52,  0x24,  12},  // "000000100100"
	{53,  0x37,  12},  // "000000110111"
	{54,  0x38,  12},  // "000000111000"
	{55,  0x27,  12},  // "000000100111"
	{56,  0x28,  12},  // "000000101000"
	{57,  0x58,  12},  // "000001011000"
	{58,  0x59,  12},  // "000001011001"
	{59,  0x2b,  12},  // "000000101011"
	{60,  0x2c,  12},  // "000000101100"
	{61,  0x5a,  12},  // "000001011010"
	{62,  0x66,  12},  // "000001100110"
	{63,  0x67,  12},  // "000001100111"
}; // g_encBlackTerm

const HuffmanEncoding g_encBlackMakeup[] = {
	{64,    0x0f,  10},  // "0000001111"
	{128,   0xc8,  12},  // "000011001000"
	{192,   0xc9,  12},  // "000011001001"
	{256,   0x5b,  12},  // "000001011011"
	{320,   0x33,  12},  // "000000110011"
	{384,   0x34,  12},  // "000000110100"
	{448,   0x35,  12},  // "000000110101"
	{512,   0x6c,  13},  // "0000001101100"
	{576,   0x6d,  13},  // "0000001101101"
	{640,   0x4a,  13},  // "0000001001010"
	{704,   0x4b,  13},  // "0000001001011"
	{768,   0x4c,  13},  // "0000001001100"
	{832,   0x4d,  13},  // "0000001001101"
	{896,   0x72,  13},  // "0000001110010"
	{960,   0x73,  13},  // "0000001110011"
	{1024,  0x74,  13},  // "0000001110100"
	{1088,  0x75,  13},  // "0000001110101"
	{1152,  0x76,  13},  // "0000001110110"
	{1216,  0x77,  13},  // "0000001110111"
	{1280,  0x52,  13},  // "0000001010010"
	{1344,  0x53,  13},  // "0000001010011"
	{1408,  0x54,  13},  // "0000001010100"
	{1472,  0x55,  13},  // "0000001010101"
	{1536,  0x5a,  13},  // "0000001011010"
	{1600,  0x5b,  13},  // "0000001011011"
	{1664,  0x64,  13},  // "0000001100100"
	{1728,  0x65,  13},  // "0000001100101"
}; // g_encBlackMakeup

const HuffmanEncoding g_encBothMakeup[] = {
	{1792,  0x08,  11},  // "00000001000"
	{1856,  0x0c,  11},  // "00000001100"
	{1920,  0x0d,  11},  // "00000001101"
	{1984,  0x12,  12},  // "000000010010"
	{2048,  0x13,  12},  // "000000010011"
	{2112,  0x14,  12},  // "000000010100"
	{2176,  0x15,  12},  // "000000010101"
	{2240,  0x16,  12},  // "000000010110"
	{2304,  0x17,  12},  // "000000010111"
	{2368,  0x1c,  12},  // "000000011100"
	{2432,  0x1d,  12},  // "000000011101"
	{2496,  0x1e,  12},  // "000000011110"
	{2560,  0x1f,  12},  // "000000011111"
}; // g_encBothMakeup


DecodeTree::DecodeTree(bool bwhite)
{
	finitStatus = B_ERROR;
	fptop = new DecodeNode;
	if (fptop) {		
		fptop->value = -1;
		fptop->branches[0] = NULL;
		fptop->branches[1] = NULL;
		
		if (bwhite)
			finitStatus = LoadWhiteEncodings();
		else
			finitStatus = LoadBlackEncodings();
	} else
		finitStatus = B_NO_MEMORY;
}

void
destroy_node(DecodeNode *pnode)
{
	if (pnode->branches[0])
		destroy_node(pnode->branches[0]);
	if (pnode->branches[1])
		destroy_node(pnode->branches[1]);

	delete pnode;
}
DecodeTree::~DecodeTree()
{
	if (fptop) {
		destroy_node(fptop);
		fptop = NULL;
	}
}

status_t
DecodeTree::LoadBlackEncodings()
{
	status_t result = B_ERROR;
	
	result = AddEncodings(g_encBlackTerm,
		sizeof(g_encBlackTerm) / sizeof(HuffmanEncoding));
	if (result != B_OK)
		return result;
	
	result = AddEncodings(g_encBlackMakeup,
		sizeof(g_encBlackMakeup) / sizeof(HuffmanEncoding));
	if (result != B_OK)
		return result;
		
	result = AddEncodings(g_encBothMakeup,
		sizeof(g_encBothMakeup) / sizeof(HuffmanEncoding));
		
	return result;
}

status_t
DecodeTree::LoadWhiteEncodings()
{
	status_t result = B_ERROR;
	
	result = AddEncodings(g_encWhiteTerm,
		sizeof(g_encWhiteTerm) / sizeof(HuffmanEncoding));
	if (result != B_OK)
		return result;
	
	result = AddEncodings(g_encWhiteMakeup,
		sizeof(g_encWhiteMakeup) / sizeof(HuffmanEncoding));
	if (result != B_OK)
		return result;
		
	result = AddEncodings(g_encBothMakeup,
		sizeof(g_encBothMakeup) / sizeof(HuffmanEncoding));
		
	return result;
}

status_t
DecodeTree::AddEncodings(const HuffmanEncoding *pencs, uint32 length)
{
	status_t result = B_ERROR;
	for (uint32 i = 0; i < length; i++) {
		result = AddEncoding(pencs[i].encoding, pencs[i].length,
			pencs[i].value);
		if (result != B_OK)
			break;
	}
	
	return result;
}

status_t
DecodeTree::AddEncoding(uint16 encoding, uint8 length, uint16 value)
{
	if (!length || length > 16)
		return B_BAD_VALUE;
		
	DecodeNode *pcurrent = fptop, *pnext = NULL;
	uint8 bitsleft = length;
		
	while (bitsleft > 0) {
		uint32 branch = (encoding >> (bitsleft - 1)) & 1;
	
		// if a new node needs to be allocated
		if (!pcurrent->branches[branch]) {
			pcurrent->branches[branch] = new DecodeNode;
			pnext = pcurrent->branches[branch];
			if (!pnext)
				return B_NO_MEMORY;

			pnext->value = -1;	
			pnext->branches[0] = NULL;
			pnext->branches[1] = NULL;
		
		} else
			// if a node exists
			pnext = pcurrent->branches[branch];
	
		if (bitsleft == 1) {
			pnext->value = value;
			return B_OK;
		
		} else {
			pcurrent = pnext;
			bitsleft--;
		}
	}
	
	return B_ERROR;
}

status_t
DecodeTree::GetValue(BitReader &stream) const
{
	if (InitCheck() != B_OK)
		return InitCheck();

	status_t branch;
	DecodeNode *pnode = fptop;
	while (pnode) {	
		branch = stream.NextBit();
		if (branch < 0)
			return branch;

		pnode = pnode->branches[branch];
		if (!pnode)
			return B_ERROR;
		else if (pnode->value > -1)
			return pnode->value;
	}
	
	return B_ERROR;
}
