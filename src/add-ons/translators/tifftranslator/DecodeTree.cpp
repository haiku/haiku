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

DecodeTree::DecodeTree()
{
	fptop = new DecodeNode;
	if (fptop) {		
		fptop->value = -1;
		fptop->branches[0] = NULL;
		fptop->branches[1] = NULL;
	}
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
DecodeTree::AddEncoding(uint16 encoding, uint8 length, uint16 value)
{
	if (!fptop)
		return B_NO_MEMORY;
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
DecodeTree::GetValue(uint16 encdata, uint8 nbits, uint8 &bitsread) const
{
	if (!fptop)
		return B_NO_MEMORY;
	if (!nbits || nbits > 16)
		return B_BAD_VALUE;

	uint32 branch;
	DecodeNode *pnode = fptop;
	bitsread = 0;
	while (bitsread < nbits) {
	
		branch = (encdata >> (16 - (bitsread + 1))) & 1;
		if (!pnode->branches[branch])
			// if I can't go any further, I've either successfully
			// decoded some bits or I'm returning -1
			return pnode->value;
		else {
			pnode = pnode->branches[branch];
			bitsread++;
			if (bitsread == nbits)
				return pnode->value;
		}
	}
	
	return B_ERROR;
}


