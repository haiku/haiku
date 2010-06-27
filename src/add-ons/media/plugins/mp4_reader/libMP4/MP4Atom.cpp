/*
 * Copyright (c) 2005, David McPaul
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


#include "MP4Atom.h"

#include <stdio.h>


AtomBase::AtomBase(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType,
	uint64 patomSize)
{
	theStream = pStream;
	streamOffset = pstreamOffset;
	atomType = patomType;
	atomSize = patomSize;
	parentAtom = NULL;
}


AtomBase::~AtomBase()
{
	theStream = NULL;
	parentAtom = NULL;
}


char *
AtomBase::GetAtomTypeAsFourcc()
{
	fourcc[0] = (char)((atomType >> 24) & 0xff);
	fourcc[1] = (char)((atomType >> 16) & 0xff);
	fourcc[2] = (char)((atomType >> 8) & 0xff);
	fourcc[3] = (char)((atomType >> 0) & 0xff);
	fourcc[4] = '\0';

	return fourcc;
}


const char *
AtomBase::GetAtomName()
{
	const char *_result;
	_result = OnGetAtomName();
	if (_result) {
		return _result;
	}
	
	fourcc[0] = (char)((atomType >> 24) & 0xff);
	fourcc[1] = (char)((atomType >> 16) & 0xff);
	fourcc[2] = (char)((atomType >> 8) & 0xff);
	fourcc[3] = (char)((atomType >> 0) & 0xff);
	fourcc[4] = '\0';

	return fourcc;
}


const char *
AtomBase::OnGetAtomName()
{
	return NULL;
}


void
AtomBase::ProcessMetaData()
//	ProcessMetaData() - Reads in the basic Atom Meta Data
//				- Calls OnProcessMetaData()
//				- Calls ProcessMetaData on each child atom
//				(ensures stream is correct for child via offset)
{
	SetAtomOffset(GetStream()->Position());
	
	OnProcessMetaData();

	MoveToEnd();
}


void
AtomBase::OnProcessMetaData()
{
	MoveToEnd();
}


bool
AtomBase::MoveToEnd()
{
	off_t NewPosition = streamOffset + atomSize;

	if (GetStream()->Position() != NewPosition) {
		return (GetStream()->Seek(NewPosition,0) > 0);
	}
	return true;
}


uint64
AtomBase::GetBytesRemaining()
{
	off_t EndPosition = streamOffset + atomSize;
	off_t CurrPosition = GetStream()->Position();
	
	if (CurrPosition > EndPosition) {
		printf("ERROR: Read past atom boundary by %Ld bytes\n",
			CurrPosition - EndPosition);
		return 0;
	}
	
	return (EndPosition - CurrPosition);
}


void
AtomBase::DisplayAtoms()
{
	uint32 aindent = 0;
	DisplayAtoms(aindent);
}


void
AtomBase::DisplayAtoms(uint32 pindent)
{
	Indent(pindent);
	printf("(%s)\n",GetAtomName());
}


void
AtomBase::Indent(uint32 pindent)
{
	for (uint32 i=0;i<pindent;i++) {
		printf("-");
	}
}


bool
AtomBase::IsKnown()
{
	return (OnGetAtomName() != NULL);
}


void
AtomBase::ReadArrayHeader(array_header *pHeader)
{
	Read(&pHeader->NoEntries);
}


BPositionIO *
AtomBase::OnGetStream()
{
	// default implementation
	return theStream;
}


BPositionIO *
AtomBase::GetStream()
{
	return OnGetStream();
}


void
AtomBase::Read(uint64	*value)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,sizeof(uint64));
	
	// Assert((bytes_read == sizeof(uint64),"Read Error");
	
	*value = B_BENDIAN_TO_HOST_INT64(*value);
}


void
AtomBase::Read(uint32	*value)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,sizeof(uint32));
	
	// Assert((bytes_read == sizeof(uint32),"Read Error");
	
	*value = B_BENDIAN_TO_HOST_INT32(*value);
}


void
AtomBase::Read(int32	*value)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,sizeof(int32));
	
	// Assert((bytes_read == sizeof(int32),"Read Error");
	
	*value = B_BENDIAN_TO_HOST_INT32(*value);
}


void
AtomBase::Read(uint16	*value)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,sizeof(uint16));
	
	// Assert((bytes_read == sizeof(uint16),"Read Error");
	
	*value = B_BENDIAN_TO_HOST_INT16(*value);
}


void
AtomBase::Read(uint8	*value)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,sizeof(uint8));
	
	// Assert((bytes_read == sizeof(uint8),"Read Error");
}


void
AtomBase::Read(char	*value, uint32 maxread)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,maxread);

	// Assert((bytes_read == maxread,"Read Error");
}


void
AtomBase::Read(uint8 *value, uint32 maxread)
{
	uint32	bytes_read;
	
	bytes_read = GetStream()->Read(value,maxread);

	// Assert((bytes_read == maxread,"Read Error");
}


uint64
AtomBase::GetBits(uint64 buffer, uint8 startBit, uint8 totalBits)
{
	// startBit should range from 0-63, totalBits should range from 1-64
	if ((startBit < 64) && (totalBits > 0) && (totalBits <= 64) 
		&& (startBit + totalBits <= 64)) {
		// Ok pull from the buffer the bits wanted.
		buffer = buffer << startBit;
		buffer = buffer >> (64 - (totalBits + startBit) + startBit);
		
		return buffer;
	}
	
	return 0L;
}


uint32
AtomBase::GetBits(uint32 buffer, uint8 startBit, uint8 totalBits)
{
	// startBit should range from 0-31, totalBits should range from 1-32
	if ((startBit < 32) && (totalBits > 0) && (totalBits <= 32)
		&& (startBit + totalBits <= 32)) {
		// Ok pull from the buffer the bits wanted.
		buffer = buffer << startBit;
		buffer = buffer >> (32 - (startBit + totalBits) + startBit);
		
		return buffer;
	}

	return 0;
}


FullAtom::FullAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType,
	uint64 patomSize)
	:
	AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}


FullAtom::~FullAtom()
{
}


void
FullAtom::OnProcessMetaData()
{
	Read(&Version);
	Read(&Flags1);
	Read(&Flags2);
	Read(&Flags3);
}


AtomContainer::AtomContainer(BPositionIO *pStream, off_t pstreamOffset,
	uint32 patomType, uint64 patomSize)
	:
	AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	TotalChildren = 0;
}


AtomContainer::~AtomContainer()
{
}


void
AtomContainer::DisplayAtoms(uint32 pindent)
{
	Indent(pindent);
	printf("%ld:(%s)\n",TotalChildren,GetAtomName());
	pindent++;
	// for each child
	for (uint32 i = 0;i < TotalChildren;i++) {
		atomChildren[i]->DisplayAtoms(pindent);
	}

}


void
AtomContainer::ProcessMetaData()
{
	SetAtomOffset(GetStream()->Position());
	
	OnProcessMetaData();

	AtomBase *aChild;
	while (IsEndOfAtom() == false) {
		aChild = GetAtom(GetStream());
		if (AddChild(aChild)) {
			aChild->ProcessMetaData();
		}
	}

	OnChildProcessingComplete();

	MoveToEnd();
}


bool
AtomContainer::AddChild(AtomBase *pChildAtom)
{
	if (pChildAtom) {
		pChildAtom->SetParent(this);
		atomChildren.push_back(pChildAtom);
		TotalChildren++;
		return true;
	}
	return false;
}


AtomBase *AtomContainer::GetChildAtom(uint32 patomType, uint32 offset)
{
	for (uint32 i=0;i<TotalChildren;i++) {
		if (atomChildren[i]->IsType(patomType)) {
			// found match, skip if offset non zero.
			if (offset == 0) {
				return atomChildren[i];
			} else {
				offset--;
			}
		} else {
			if (atomChildren[i]->IsContainer()) {
				// search container
				AtomBase *aAtomBase = dynamic_cast<AtomContainer *>
					(atomChildren[i])->GetChildAtom(patomType, offset);
				if (aAtomBase) {
					// found in container
					return aAtomBase;
				}
				// not found
			}
		}
	}
	return NULL;
}


void
AtomContainer::OnProcessMetaData()
{
}
