#include <stdio.h>

#include "MOVAtom.h"

AtomBase::AtomBase(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize)
{
	theStream = pStream;
	streamOffset = pstreamOffset;
	atomType = patomType;
	atomSize = patomSize;
}

AtomBase::~AtomBase()
{
	theStream = NULL;
}

char *AtomBase::getAtomName()
{
	char *_result;
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

char *AtomBase::OnGetAtomName()
{
	return NULL;
}

void AtomBase::ProcessMetaData()
//	ProcessMetaData() - Reads in the basic Atom Meta Data
//				- Calls OnProcessMetaData()
//				- Calls ProcessMetaData on each child atom
//				(ensures stream is correct for child via offset)
{
	setAtomOffset(theStream->Position());
	
	OnProcessMetaData();

	MoveToEnd();
}

void AtomBase::OnProcessMetaData()
{
	MoveToEnd();
}

bool AtomBase::MoveToEnd()
{
	off_t NewPosition = streamOffset + atomSize;

	if (theStream->Position() != NewPosition) {
		return (theStream->Seek(NewPosition,0) > 0);
	}
	return true;
}

void	AtomBase::DisplayAtoms()
{
	uint32 aindent = 0;
	DisplayAtoms(aindent);
}

void	AtomBase::DisplayAtoms(uint32 pindent)
{
	Indent(pindent);
	printf("%s\n",getAtomName());
}

void	AtomBase::Indent(uint32 pindent)
{
	for (uint32 i=0;i<pindent;i++) {
		printf("-");
	}
}

bool AtomBase::IsKnown()
{
	return (OnGetAtomName() != NULL);
}

void AtomBase::ReadArrayHeader(array_header *pHeader)
{
	theStream->Read(pHeader,sizeof(array_header));

	pHeader->NoEntries = B_BENDIAN_TO_HOST_INT32(pHeader->NoEntries);
}

AtomContainer::AtomContainer(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	TotalChildren = 0;
}

AtomContainer::~AtomContainer()
{
}

void	AtomContainer::DisplayAtoms(uint32 pindent)
{
	Indent(pindent);
	printf("%ld:%s\n",TotalChildren,getAtomName());
	pindent++;
	// for each child
	for (uint32 i = 0;i < TotalChildren;i++) {
		atomChildren[i]->DisplayAtoms(pindent);
	}

}

void	AtomContainer::ProcessMetaData()
{
	setAtomOffset(theStream->Position());
	
	OnProcessMetaData();

	AtomBase *aChild;
	while (IsEndOfAtom() == false) {
		aChild = getAtom(theStream);
		if (AddChild(aChild)) {
			aChild->ProcessMetaData();
		}
	}

	MoveToEnd();
}

bool	AtomContainer::AddChild(AtomBase *pChildAtom)
{
	if (pChildAtom) {
		atomChildren[TotalChildren++] = pChildAtom;
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
				AtomBase *aAtomBase = (dynamic_cast<AtomContainer *>(atomChildren[i])->GetChildAtom(patomType, offset));
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

void	AtomContainer::OnProcessMetaData()
{
}
