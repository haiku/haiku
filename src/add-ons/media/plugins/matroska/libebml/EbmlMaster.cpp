/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This file is part of libebml.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: EbmlMaster.cpp 1178 2005-05-19 15:47:11Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#include <cassert>
#include <algorithm>

#include "ebml/EbmlMaster.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlContexts.h"
#include "ebml/MemIOCallback.h"

START_LIBEBML_NAMESPACE

EbmlMaster::EbmlMaster(const EbmlSemanticContext & aContext, const bool bSizeIsknown)
 :EbmlElement(0), Context(aContext), bChecksumUsed(bChecksumUsedByDefault)
{
	bSizeIsFinite = bSizeIsknown;
	bValueIsSet = true;
	ProcessMandatory();
}

EbmlMaster::EbmlMaster(const EbmlMaster & ElementToClone)
 :EbmlElement(ElementToClone)
 ,ElementList(ElementToClone.ListSize())
 ,Context(ElementToClone.Context)
 ,bChecksumUsed(ElementToClone.bChecksumUsed)
 ,Checksum(ElementToClone.Checksum)
{
	// add a clone of the list
	std::vector<EbmlElement *>::const_iterator Itr = ElementToClone.ElementList.begin();
	std::vector<EbmlElement *>::iterator myItr = ElementList.begin();
	while (Itr != ElementToClone.ElementList.end())
	{
		*myItr = (*Itr)->Clone();
		Itr++; myItr++;
	}

}

EbmlMaster::~EbmlMaster()
{
	assert(!bLocked); // you're trying to delete a locked element !!!

	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (!(*ElementList[Index]).IsLocked())	{
			delete ElementList[Index];
		}
	}
}

/*!
	\todo handle exception on errors
	\todo write all the Mandatory elements in the Context, otherwise assert
*/
uint32 EbmlMaster::RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact)
{
	uint32 Result = 0;
	size_t Index;

	if (!bForceRender) {
		assert(CheckMandatory());
	}

	if (!bChecksumUsed) { // old school
		for (Index = 0; Index < ElementList.size(); Index++) {
			if (!bKeepIntact && (ElementList[Index])->IsDefaultValue())
				continue;
			Result += (ElementList[Index])->Render(output, bKeepIntact, false ,bForceRender);
		}
	} else { // new school
		MemIOCallback TmpBuf(Size - 6);
		for (Index = 0; Index < ElementList.size(); Index++) {
			if (!bKeepIntact && (ElementList[Index])->IsDefaultValue())
				continue;
			(ElementList[Index])->Render(TmpBuf, bKeepIntact, false ,bForceRender);
		}
		Checksum.FillCRC32(TmpBuf.GetDataBuffer(), TmpBuf.GetDataBufferSize());
		Result += Checksum.Render(output, true, false ,bForceRender);
		output.writeFully(TmpBuf.GetDataBuffer(), TmpBuf.GetDataBufferSize());
		Result += TmpBuf.GetDataBufferSize();
	}

	return Result;
}

/*!
	\todo We might be able to forbid elements that don't exist in the context
*/
bool EbmlMaster::PushElement(EbmlElement & element)
{
	ElementList.push_back(&element);
	return true;
}

uint64 EbmlMaster::UpdateSize(bool bKeepIntact, bool bForceRender)
{
	Size = 0;

	if (!bSizeIsFinite)
		return (0-1);

	if (!bForceRender) {
		assert(CheckMandatory());
    }
	
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (!bKeepIntact && (ElementList[Index])->IsDefaultValue())
			continue;
		(ElementList[Index])->UpdateSize(bKeepIntact, bForceRender);
		uint64 SizeToAdd = (ElementList[Index])->ElementSize(bKeepIntact);
#if defined(_DEBUG) || defined(DEBUG)
		if (SizeToAdd == (0-1))
			return (0-1);
#endif // DEBUG
		Size += SizeToAdd;
	}
	if (bChecksumUsed) {
		Size += Checksum.ElementSize();
	}

	return Size;
}

uint32 EbmlMaster::WriteHead(IOCallback & output, int nSizeLength, bool bKeepIntact)
{
	SetSizeLength(nSizeLength);
	return RenderHead(output, false, bKeepIntact);
}

/*!
	\todo this code is very suspicious !
*/
uint64 EbmlMaster::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	input.setFilePointer(Size, seek_current);
	return Size;
}

/*!
	\note Hopefully no global element is mandatory
	\todo should be called for ALL EbmlMaster element on construction
*/
bool EbmlMaster::ProcessMandatory()
{
	if (Context.Size == 0)
	{
		return true;
	}

	assert(Context.MyTable != NULL);

	unsigned int EltIdx;
	for (EltIdx = 0; EltIdx < Context.Size; EltIdx++) {
		if (Context.MyTable[EltIdx].Mandatory && Context.MyTable[EltIdx].Unique) {
			assert(Context.MyTable[EltIdx].GetCallbacks.Create != NULL);
			PushElement(Context.MyTable[EltIdx].GetCallbacks.Create());
		}
	}
	return true;
}

bool EbmlMaster::CheckMandatory() const
{
	assert(Context.MyTable != NULL);

	unsigned int EltIdx;
	for (EltIdx = 0; EltIdx < Context.Size; EltIdx++) {
		if (Context.MyTable[EltIdx].Mandatory) {
			if (FindElt(Context.MyTable[EltIdx].GetCallbacks) == NULL) {
#if defined(_DEBUG) || defined(DEBUG)
				// you are missing this Mandatory element
// 				const char * MissingName = Context.MyTable[EltIdx].GetCallbacks.DebugName;
#endif // DEBUG
				return false;
			}
		}
	}

	return true;
}

std::vector<std::string> EbmlMaster::FindAllMissingElements()
{	
	assert(Context.MyTable != NULL);

	std::vector<std::string> missingElements;

	for (size_t ChildElementNo = 0; ChildElementNo < ElementList.size(); ChildElementNo++) {
   		EbmlElement *childElement = ElementList[ChildElementNo];
		if (!childElement->ValueIsSet()) {
			std::string missingValue;
			missingValue = "The Child Element \"";
			missingValue.append(childElement->Generic().DebugName);
			missingValue.append("\" of EbmlMaster \"");
			missingValue.append(this->Generic().DebugName);
			missingValue.append("\", does not have a value set.");
			missingElements.push_back(missingValue);
		}

		if (childElement->IsMaster()) {
			EbmlMaster *childMaster = (EbmlMaster *)childElement;

			std::vector<std::string> childMissingElements = childMaster->FindAllMissingElements();
			for (size_t s = 0; s < childMissingElements.size(); s++)
				missingElements.push_back(childMissingElements[s]);
		}
	}
	unsigned int EltIdx;
	for (EltIdx = 0; EltIdx < Context.Size; EltIdx++) {
		if (Context.MyTable[EltIdx].Mandatory) {
			if (FindElt(Context.MyTable[EltIdx].GetCallbacks) == NULL) {
				std::string missingElement;
				missingElement = "Missing element \"";
				missingElement.append(Context.MyTable[EltIdx].GetCallbacks.DebugName);				
				missingElement.append("\" in EbmlMaster \"");
				missingElement.append(Context.MasterElt->DebugName);
				missingElement.append("\"");
				missingElements.push_back(missingElement);
			}
		}
	}

	return missingElements;
}

EbmlElement *EbmlMaster::FindElt(const EbmlCallbacks & Callbacks) const
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		EbmlElement * tmp = ElementList[Index];
		if (EbmlId(*tmp) == Callbacks.GlobalId)
			return tmp;
	}

	return NULL;
}

EbmlElement *EbmlMaster::FindFirstElt(const EbmlCallbacks & Callbacks, const bool bCreateIfNull)
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (EbmlId(*(ElementList[Index])) == Callbacks.GlobalId)
			return ElementList[Index];
	}
	
	if (bCreateIfNull && Callbacks.Create != NULL) {
		// add the element
		EbmlElement *NewElt = &(Callbacks.Create());
		if (NewElt == NULL)
			return NULL;

		if (!PushElement(*NewElt)) {
			delete NewElt;
			NewElt = NULL;
		}
		return NewElt;
	}
	
	return NULL;
}

EbmlElement *EbmlMaster::FindFirstElt(const EbmlCallbacks & Callbacks) const
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if (EbmlId(*(ElementList[Index])) == Callbacks.GlobalId)
			return ElementList[Index];
	}
	
	return NULL;
}

/*!
	\todo only return elements that are from the same type !
	\todo the element might be the unique in the context !
*/
EbmlElement *EbmlMaster::FindNextElt(const EbmlElement & PastElt, const bool bCreateIfNull)
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if ((ElementList[Index]) == &PastElt) {
			// found past element, new one is :
			Index++;
			break;
		}
	}

	while (Index < ElementList.size()) {
		if (PastElt.Generic().GlobalId == ElementList[Index]->Generic().GlobalId)
			break;
		Index++;
	}

	if (Index != ElementList.size())
		return ElementList[Index];

	if (bCreateIfNull && PastElt.Generic().Create != NULL) {
		// add the element
		EbmlElement *NewElt = &(PastElt.Generic().Create());
		if (NewElt == NULL)
			return NULL;

		if (!PushElement(*NewElt)) {
			delete NewElt;
			NewElt = NULL;
		}
		return NewElt;
	}

	return NULL;
}

EbmlElement *EbmlMaster::FindNextElt(const EbmlElement & PastElt) const
{
	size_t Index;
	
	for (Index = 0; Index < ElementList.size(); Index++) {
		if ((ElementList[Index]) == &PastElt) {
			// found past element, new one is :
			Index++;
			break;
		}
	}

	while (Index < ElementList.size()) {
		if (PastElt.Generic().GlobalId == ElementList[Index]->Generic().GlobalId)
			return ElementList[Index];
		Index++;
	}

	return NULL;
}

EbmlElement *EbmlMaster::AddNewElt(const EbmlCallbacks & Callbacks)
{
	// add the element
	EbmlElement *NewElt = &(Callbacks.Create());
	if (NewElt == NULL)
		return NULL;

	if (!PushElement(*NewElt)) {
		delete NewElt;
		NewElt = NULL;
	}
	return NewElt;
}

void EbmlMaster::Sort()
{
	std::sort(ElementList.begin(), ElementList.end(), EbmlElement::CompareElements);
}

/*!
	\brief Method to help reading a Master element and all subsequent children quickly
	\todo add an option to discard even unknown elements
	\todo handle when a mandatory element is not found
*/
void EbmlMaster::Read(EbmlStream & inDataStream, const EbmlSemanticContext & sContext, int & UpperEltFound, EbmlElement * & FoundElt, bool AllowDummyElt, ScopeMode ReadFully)
{
	if (ReadFully != SCOPE_NO_DATA)
	{
		EbmlElement * ElementLevelA;
		// remove all existing elements, including the mandatory ones...
		size_t Index;
		for (Index=0; Index<ElementList.size(); Index++) {
			if (!(*ElementList[Index]).IsLocked()) {
				delete ElementList[Index];
			}
		}
		ElementList.clear();
		uint64 MaxSizeToRead = Size;

		// read blocks and discard the ones we don't care about
		if (MaxSizeToRead > 0) {
			inDataStream.I_O().setFilePointer(SizePosition + SizeLength, seek_beginning);
			ElementLevelA = inDataStream.FindNextElement(sContext, UpperEltFound, MaxSizeToRead, AllowDummyElt);
			while (ElementLevelA != NULL && MaxSizeToRead > 0 && UpperEltFound <= 0) {
				MaxSizeToRead = GetEndPosition() - ElementLevelA->GetEndPosition(); // even if it's the default value
				if (!AllowDummyElt && ElementLevelA->IsDummy()) {
					ElementLevelA->SkipData(inDataStream, sContext);
					delete ElementLevelA; // forget this unknown element
				} else {
					// more logical to do it afterward
					ElementList.push_back(ElementLevelA);

					ElementLevelA->Read(inDataStream, ElementLevelA->Generic().Context, UpperEltFound, FoundElt, AllowDummyElt, ReadFully);

					// just in case
					ElementLevelA->SkipData(inDataStream, ElementLevelA->Generic().Context);
				}

				if (UpperEltFound > 0) {
					UpperEltFound--;
					if (UpperEltFound > 0 || MaxSizeToRead <= 0)
						goto processCrc;
					ElementLevelA = FoundElt;
					continue;
				} 
				
				if (UpperEltFound < 0) {
					UpperEltFound++;
					if (UpperEltFound < 0)
						goto processCrc;
				}

				if (MaxSizeToRead <= 0) {
					goto processCrc;// this level is finished
				}
				
				ElementLevelA = inDataStream.FindNextElement(sContext, UpperEltFound, MaxSizeToRead, AllowDummyElt);
			}
			if (UpperEltFound > 0) {
				FoundElt = ElementLevelA;
			}
		}
	processCrc:
		for (Index=0; Index<ElementList.size(); Index++) {
			if (ElementList[Index]->Generic().GlobalId == EbmlCrc32::ClassInfos.GlobalId) {
				bChecksumUsed = true;
				// remove the element
				Checksum = *(static_cast<EbmlCrc32*>(ElementList[Index]));
				delete ElementList[Index];
				Remove(Index--);
			}
		}
		bValueIsSet = true;
	}
}

void EbmlMaster::Remove(size_t Index)
{
	if (Index < ElementList.size()) {
		std::vector<EbmlElement *>::iterator Itr = ElementList.begin();
		while (Index-- > 0) {
			Itr++;
		}

		ElementList.erase(Itr);
	}
}

bool EbmlMaster::VerifyChecksum() const
{
	if (!bChecksumUsed)
		return true;

	EbmlCrc32 aChecksum;
	/// \todo remove the Checksum if it's in the list
	/// \todo find another way when not all default values are saved or (unknown from the reader !!!)
	MemIOCallback TmpBuf(Size - 6);
	for (size_t Index = 0; Index < ElementList.size(); Index++) {
		(ElementList[Index])->Render(TmpBuf, true, false, true);
	}
	aChecksum.FillCRC32(TmpBuf.GetDataBuffer(), TmpBuf.GetDataBufferSize());
	return (aChecksum.GetCrc32() == Checksum.GetCrc32());
}

bool EbmlMaster::InsertElement(EbmlElement & element, size_t position)
{
	std::vector<EbmlElement *>::iterator Itr = ElementList.begin();
	while (Itr != ElementList.end() && position--)
	{
		Itr++;
	}
	if ((Itr == ElementList.end()) && position)
		return false;

	ElementList.insert(Itr, &element);
	return true;
}

bool EbmlMaster::InsertElement(EbmlElement & element, const EbmlElement & before)
{
	std::vector<EbmlElement *>::iterator Itr = ElementList.begin();
	while (Itr != ElementList.end() && *Itr != &before)
	{
		Itr++;
	}
	if (Itr == ElementList.end())
		return false;

	ElementList.insert(Itr, &element);
	return true;
}


END_LIBEBML_NAMESPACE
