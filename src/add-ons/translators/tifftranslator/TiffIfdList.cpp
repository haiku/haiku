/*****************************************************************************/
// TiffIfdList
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffIfdList.cpp
//
// This object is for storing all IFD entries from a TIFF file
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

#include "TiffIfdList.h"

status_t
TiffIfdList::LoadIfds(uint32 offset, BPositionIO &io, swap_action swp)
{
	Empty();
	
	TiffIfdNode *pnode = NULL;
	while (offset) {
		// Create new node at the end of the list
		if (!fpfirst) {
			fpfirst = new TiffIfdNode;
			pnode = fpfirst;
		} else {
			pnode->pnext = new TiffIfdNode;
			pnode = pnode->pnext;
		}
		if (!pnode) {
			finitStatus = B_NO_MEMORY;
			break;
		}
		
		// Create new TiffIfd and check its status
		pnode->pnext = NULL;
		pnode->pifd = new TiffIfd(offset, io, swp);
		if (!pnode->pifd) {
			finitStatus = B_NO_MEMORY;
			break;
		}
		if (pnode->pifd->InitCheck() != B_OK) {
			finitStatus = pnode->pifd->InitCheck();
			break;
		}
		
		offset = pnode->pifd->GetNextIfdOffset();
		fcount++;
	}
	
	if (finitStatus != B_OK)
		fcount = 0;
	
	return finitStatus;	
}

TiffIfdList::TiffIfdList(uint32 offset, BPositionIO &io, swap_action swp)
{
	fpfirst = NULL;
	finitStatus = B_OK;
	fcount = 0;
	
	LoadIfds(offset, io, swp);
}

TiffIfdList::~TiffIfdList()
{
	Empty();
}

void
TiffIfdList::Empty()
{
	TiffIfdNode *pnode, *pdelme;
	pnode = fpfirst;
	
	while (pnode) {
		pdelme = pnode;
		pnode = pnode->pnext;
		
		delete pdelme->pifd;
		delete pdelme;
	}
	fpfirst = NULL;
	fcount = 0;
	finitStatus = B_OK;
}

// index is zero based
TiffIfd *
TiffIfdList::GetIfd(int32 index)
{
	if (index < 0 || index >= fcount)
		return NULL;
	
	TiffIfdNode *pnode = fpfirst;
	while (index--)
		pnode = pnode->pnext;
	
	return pnode->pifd;
}

