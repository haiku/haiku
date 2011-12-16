/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: EbmlElement.cpp 1232 2005-10-15 15:56:52Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#include <cassert>

#include "ebml/EbmlElement.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlVoid.h"
#include "ebml/EbmlDummy.h"
#include "ebml/EbmlContexts.h"

START_LIBEBML_NAMESPACE

/*!
	\todo handle more than CodedSize of 5
*/
int CodedSizeLength(uint64 Length, unsigned int SizeLength, bool bSizeFinite)
{
	int CodedSize;
	if (bSizeFinite) {
		// prepare the head of the size (000...01xxxxxx)
		// optimal size
		if (Length < 127) // 2^7 - 1
			CodedSize = 1;
		else if (Length < 16383) // 2^14 - 1
			CodedSize = 2;
		else if (Length < 2097151L) // 2^21 - 1
			CodedSize = 3;
		else if (Length < 268435455L) // 2^28 - 1
			CodedSize = 4;
		else CodedSize = 5;
	} else {
		if (Length <= 127) // 2^7 - 1
			CodedSize = 1;
		else if (Length <= 16383) // 2^14 - 1
			CodedSize = 2;
		else if (Length <= 2097151L) // 2^21 - 1
			CodedSize = 3;
		else if (Length <= 268435455L) // 2^28 - 1
			CodedSize = 4;
		else CodedSize = 5;
	}

	if (SizeLength > 0 && CodedSize < SizeLength) {
		// defined size
		CodedSize = SizeLength;
	}

	return CodedSize;
}

/*!
	\todo handle more than CodedSize of 5
*/
int CodedSizeLengthSigned(int64 Length, unsigned int SizeLength)
{
	unsigned int CodedSize;
	// prepare the head of the size (000...01xxxxxx)
	// optimal size
	if (Length > -64 && Length < 64) // 2^6
		CodedSize = 1;
	else if (Length > -8192 && Length < 8192) // 2^13
		CodedSize = 2;
	else if (Length > -1048576L && Length < 1048576L) // 2^20
		CodedSize = 3;
	else if (Length > -134217728L && Length < 134217728L) // 2^27
		CodedSize = 4;
	else CodedSize = 5;

	if (SizeLength > 0 && CodedSize < SizeLength) {
		// defined size
		CodedSize = SizeLength;
	}

	return CodedSize;
}

int CodedValueLength(uint64 Length, int CodedSize, binary * OutBuffer)
{
	int _SizeMask = 0xFF;
	OutBuffer[0] = 1 << (8 - CodedSize);
	for (int i=1; i<CodedSize; i++) {
		OutBuffer[CodedSize-i] = Length & 0xFF;
		Length >>= 8;
		_SizeMask >>= 1;
	}
	// first one use a OR with the "EBML size head"
	OutBuffer[0] |= Length & 0xFF & _SizeMask;
	return CodedSize;
}

int CodedValueLengthSigned(int64 Length, int CodedSize, binary * OutBuffer)
{
	if (Length > -64 && Length < 64) // 2^6
		Length += 63;
	else if (Length > -8192 && Length < 8192) // 2^13
		Length += 8191;
	else if (Length > -1048576L && Length < 1048576L) // 2^20
		Length += 1048575L;
	else if (Length > -134217728L && Length < 134217728L) // 2^27
		Length += 134217727L;

	return CodedValueLength(Length, CodedSize, OutBuffer);
}

uint64 ReadCodedSizeValue(const binary * InBuffer, uint32 & BufferSize, uint64 & SizeUnknown)
{
	binary SizeBitMask = 1 << 7;
	uint64 Result = 0x7F;
	unsigned int SizeIdx, PossibleSizeLength = 0;
	binary PossibleSize[8];

	SizeUnknown = 0x7F; // the last bit is discarded when computing the size
	for (SizeIdx = 0; SizeIdx < BufferSize && SizeIdx < 8; SizeIdx++) {
		if (InBuffer[0] & (SizeBitMask >> SizeIdx)) {
			// ID found
			PossibleSizeLength = SizeIdx + 1;
			SizeBitMask >>= SizeIdx;
			for (SizeIdx = 0; SizeIdx < PossibleSizeLength; SizeIdx++) {
				PossibleSize[SizeIdx] = InBuffer[SizeIdx];
			}
			for (SizeIdx = 0; SizeIdx < PossibleSizeLength - 1; SizeIdx++) {
				Result <<= 7;
				Result |= 0xFF;
			}

			Result = 0;
			Result |= PossibleSize[0] & ~SizeBitMask;
			for (unsigned int i = 1; i<PossibleSizeLength; i++) {
				Result <<= 8;
				Result |= PossibleSize[i];
			}

			BufferSize = PossibleSizeLength;

			return Result;
		}
		SizeUnknown <<= 7;
		SizeUnknown |= 0xFF;
	}

	BufferSize = 0;
	return 0;
}

int64 ReadCodedSizeSignedValue(const binary * InBuffer, uint32 & BufferSize, uint64 & SizeUnknown)
{
	int64 Result = ReadCodedSizeValue(InBuffer, BufferSize, SizeUnknown);

	if (BufferSize != 0)
	{
		switch (BufferSize)
		{
		case 1:
			Result -= 63;
			break;
		case 2:
			Result -= 8191;
			break;
		case 3:
			Result -= 1048575L;
			break;
		case 4:
			Result -= 134217727L;
			break;
		}
	}

	return Result;
}

EbmlElement::EbmlElement(const uint64 aDefaultSize, bool bValueSet)
 :DefaultSize(aDefaultSize)
 ,SizeLength(0) ///< write optimal size by default
 ,bSizeIsFinite(true)
 ,ElementPosition(0)
 ,SizePosition(0)
 ,bValueIsSet(bValueSet)
 ,DefaultIsSet(false)
 ,bLocked(false)
{
	Size = DefaultSize;
}

EbmlElement::EbmlElement(const EbmlElement & ElementToClone)
 :Size(ElementToClone.Size)
 ,DefaultSize(ElementToClone.DefaultSize)
 ,SizeLength(ElementToClone.SizeLength)
 ,bSizeIsFinite(ElementToClone.bSizeIsFinite)
 ,ElementPosition(ElementToClone.ElementPosition)
 ,SizePosition(ElementToClone.SizePosition)
 ,bValueIsSet(ElementToClone.bValueIsSet)
 ,DefaultIsSet(ElementToClone.DefaultIsSet)
 ,bLocked(ElementToClone.bLocked)
{
}

/*!
	\todo this method is deprecated and should be called FindThisID
	\todo replace the new RawElement with the appropriate class (when known)
*/
EbmlElement * EbmlElement::FindNextID(IOCallback & DataStream, const EbmlCallbacks & ClassInfos, const uint64 MaxDataSize)
{
	binary PossibleId[4];
	int PossibleID_Length = 0;
	binary PossibleSize[8]; // we don't support size stored in more than 64 bits
	uint32 PossibleSizeLength = 0;
	uint64 SizeUnknown;
	uint64 SizeFound;
	bool bElementFound = false;
	
	binary BitMask;
	uint64 aElementPosition, aSizePosition;
	while (!bElementFound) {
		// read ID
		aElementPosition = DataStream.getFilePointer();
		uint32 ReadSize = 0;
		BitMask = 1 << 7;
		while (1) {
			ReadSize += DataStream.read(&PossibleId[PossibleID_Length], 1);
			if (ReadSize == uint32(PossibleID_Length)) {
				return NULL; // no more data ?
			}
			if (++PossibleID_Length > 4) {
				return NULL; // we don't support element IDs over class D
			}
			if (PossibleId[0] & BitMask) {
				// this is the last octet of the ID
				// check whether that's the one we're looking for
/*			if (PossibleID == ClassInfos.GlobalId) {
					break;
				} else {
					/// \todo This element should be skipped (use a context ?)
				}*/
				bElementFound = true; /// \todo not exactly the one we're looking for
				break;
			}
			BitMask >>= 1;
		}
		
		// read the data size
		aSizePosition = DataStream.getFilePointer();
		uint32 _SizeLength;
		do {
			if (PossibleSizeLength >= 8)
				// Size is larger than 8 bytes
				return NULL;

			ReadSize += DataStream.read(&PossibleSize[PossibleSizeLength++], 1);
			_SizeLength = PossibleSizeLength;
			SizeFound = ReadCodedSizeValue(&PossibleSize[0], _SizeLength, SizeUnknown);
		} while (_SizeLength == 0);
	}
	
	EbmlElement *Result = NULL;
	EbmlId PossibleID(PossibleId, PossibleID_Length);
	if (PossibleID == ClassInfos.GlobalId) {
		// the element is the one expected
		Result = &ClassInfos.Create();
	} else {
		/// \todo find the element in the context
		Result = new EbmlDummy(PossibleID);
	}
	
	Result->SetSizeLength(PossibleSizeLength);
	
	Result->Size = SizeFound;

	if (!Result->ValidateSize() || (SizeFound != SizeUnknown && MaxDataSize < Result->Size)) {
			delete Result;
			return NULL;
	}

	// check if the size is not all 1s
	if (SizeFound == SizeUnknown) {
		// Size of this element is unknown
		// only possible for Master elements
		if (!Result->SetSizeInfinite()) {
			/// \todo the element is not allowed to be infinite
			delete Result;
			return NULL;
		}
	} else Result->SetSizeInfinite(false);
	Result->ElementPosition = aElementPosition;
	Result->SizePosition = aSizePosition;
	
	return Result;
}


/*!
	\todo replace the new RawElement with the appropriate class (when known)
	\todo skip data for Dummy elements when they are not allowed
	\todo better check of the size checking for upper elements (using a list of size for each level)
	\param LowLevel Will be returned with the level of the element found compared to the context given
*/
EbmlElement * EbmlElement::FindNextElement(IOCallback & DataStream, const EbmlSemanticContext & Context, int & UpperLevel, 
			uint64 MaxDataSize, bool AllowDummyElt, unsigned int MaxLowerLevel)
{
	int PossibleID_Length = 0;
	binary PossibleIdNSize[16];
	int PossibleSizeLength;
	uint64 SizeUnknown;
	int ReadIndex = 0; // trick for the algo, start index at 0
	uint32 ReadSize = 0;
	uint64 SizeFound;
	int SizeIdx;
	bool bFound;
	int UpperLevel_original = UpperLevel;
	
	do {
		// read a potential ID
		do {
			assert(ReadIndex < 16);
			// build the ID with the current Read Buffer
			bFound = false;
			binary IdBitMask = 1 << 7;
			for (SizeIdx = 0; SizeIdx < ReadIndex && SizeIdx < 4; SizeIdx++) {
				if (PossibleIdNSize[0] & (IdBitMask >> SizeIdx)) {
					// ID found
					PossibleID_Length = SizeIdx + 1;
					IdBitMask >>= SizeIdx;
					bFound = true;
					break;
				}
			}
			if (bFound) {
				break;
			}

			if (ReadIndex >= 4) {
				// ID not found
				// shift left the read octets
				memmove(&PossibleIdNSize[0],&PossibleIdNSize[1], --ReadIndex);
			}

			if (DataStream.read(&PossibleIdNSize[ReadIndex++], 1) == 0) {
				return NULL; // no more data ?
			}
			ReadSize++;

		} while (!bFound && MaxDataSize > ReadSize);

		SizeIdx = ReadIndex;
		ReadIndex -= PossibleID_Length;

		// read the data size
		uint32 _SizeLength;
		PossibleSizeLength = ReadIndex;
		while (1)
		{
			_SizeLength = PossibleSizeLength;
			SizeFound = ReadCodedSizeValue(&PossibleIdNSize[PossibleID_Length], _SizeLength, SizeUnknown);
			if (_SizeLength != 0) {
				bFound = true;
				break;
			}
			if (PossibleSizeLength >= 8) {
				bFound = false;
				break;
			}
			ReadSize += DataStream.read(&PossibleIdNSize[SizeIdx++], 1);
			PossibleSizeLength++;
		}

		if (bFound) {
			// find the element in the context and use the correct creator
			EbmlId PossibleID(PossibleIdNSize, PossibleID_Length);
			EbmlElement * Result = CreateElementUsingContext(PossibleID, Context, UpperLevel, false, AllowDummyElt, MaxLowerLevel);
			///< \todo continue is misplaced
			if (Result != NULL) {
				if (AllowDummyElt || !Result->IsDummy()) {
					Result->SetSizeLength(_SizeLength);
					
					Result->Size = SizeFound;
					// UpperLevel values
					// -1 : global element
					//  0 : child
					//  1 : same level
					//  + : further parent
					if (Result->ValidateSize() && (UpperLevel > 0 || MaxDataSize == 0 || MaxDataSize >= (PossibleID_Length + PossibleSizeLength + SizeFound))) {
						if (SizeFound == SizeUnknown) {
							Result->SetSizeInfinite();
						}

						Result->SizePosition = DataStream.getFilePointer() - SizeIdx + PossibleID.Length;
						Result->ElementPosition = Result->SizePosition - PossibleID.Length;
						// place the file at the beggining of the data
						DataStream.setFilePointer(Result->SizePosition + _SizeLength);
						return Result;
					}
				}
				delete Result;
			}
		}

		// recover all the data in the buffer minus one byte
		ReadIndex = SizeIdx - 1;
		memmove(&PossibleIdNSize[0], &PossibleIdNSize[1], ReadIndex);
		UpperLevel = UpperLevel_original;
	} while ( MaxDataSize > DataStream.getFilePointer() - SizeIdx + PossibleID_Length );

	return NULL;
}

/*!
	\todo what happens if we are in a upper element with a known size ?
*/
EbmlElement * EbmlElement::SkipData(EbmlStream & DataStream, const EbmlSemanticContext & Context, EbmlElement * TestReadElt, bool AllowDummyElt)
{
	EbmlElement * Result = NULL;
	if (bSizeIsFinite) {
		assert(TestReadElt == NULL);
		assert(ElementPosition < SizePosition);
		DataStream.I_O().setFilePointer(SizePosition + CodedSizeLength(Size, SizeLength, bSizeIsFinite) + Size, seek_beginning);
//		DataStream.I_O().setFilePointer(Size, seek_current);
	} else {
		/////////////////////////////////////////////////
		// read elements until an upper element is found
		/////////////////////////////////////////////////
		bool bEndFound = false;
		while (!bEndFound && Result == NULL) {
			// read an element
			/// \todo 0xFF... and true should be configurable
//			EbmlElement * NewElt;
			if (TestReadElt == NULL) {
				int bUpperElement = 0; // trick to call FindNextID correctly
				Result = DataStream.FindNextElement(Context, bUpperElement, 0xFFFFFFFFL, AllowDummyElt);
			} else {
				Result = TestReadElt;
			}
			
			if (Result != NULL) {
				unsigned int EltIndex;
				// data known in this Master's context
				for (EltIndex = 0; EltIndex < Context.Size; EltIndex++) {
					if (EbmlId(*Result) == Context.MyTable[EltIndex].GetCallbacks.GlobalId) {
						// skip the data with its own context
						Result = Result->SkipData(DataStream, Context.MyTable[EltIndex].GetCallbacks.Context, NULL);
						break; // let's go to the next ID
					}
				}

				if (EltIndex >= Context.Size) {
					if (Context.UpTable != NULL) {
						Result = SkipData(DataStream, *Context.UpTable, Result);
					} else {
						assert(Context.GetGlobalContext != NULL);
						if (Context != Context.GetGlobalContext()) {
							Result = SkipData(DataStream, Context.GetGlobalContext(), Result);
						} else {
							bEndFound = true;
						}
					}
				}
			} else {
				bEndFound = true;
			}
		}
	}
	return Result;
}

EbmlElement *EbmlElement::CreateElementUsingContext(const EbmlId & aID, const EbmlSemanticContext & Context,
													int & LowLevel, bool IsGlobalContext, bool bAllowDummy, unsigned int MaxLowerLevel)
{
	unsigned int ContextIndex;
	EbmlElement *Result = NULL;

	// elements at the current level
	for (ContextIndex = 0; ContextIndex < Context.Size; ContextIndex++) {
		if (aID == Context.MyTable[ContextIndex].GetCallbacks.GlobalId) {
			return &Context.MyTable[ContextIndex].GetCallbacks.Create();
		}
	}

	// global elements
	assert(Context.GetGlobalContext != NULL); // global should always exist, at least the EBML ones
	const EbmlSemanticContext & tstContext = Context.GetGlobalContext();
	if (tstContext != Context) {
		LowLevel--;
		MaxLowerLevel--;
		// recursive is good, but be carefull...
		Result = CreateElementUsingContext(aID, tstContext, LowLevel, true, bAllowDummy, MaxLowerLevel);
		if (Result != NULL) {
			return Result;
		}
		LowLevel++;
		MaxLowerLevel++;
	} else {
		return NULL;
	}

	// parent elements
	if (Context.MasterElt != NULL && aID == Context.MasterElt->GlobalId) {
		LowLevel++; // already one level up (same as context)
		return &Context.MasterElt->Create();
	}

	// check whether it's not part of an upper context
	if (Context.UpTable != NULL) {
		LowLevel++;
		MaxLowerLevel++;
		return CreateElementUsingContext(aID, *Context.UpTable, LowLevel, IsGlobalContext, bAllowDummy, MaxLowerLevel);
	}

	if (!IsGlobalContext && bAllowDummy) {
		LowLevel = 0;
		Result = new EbmlDummy(aID);
	}

	return Result;
}

/*!
	\todo verify that the size written is the same as the data written
*/
uint32 EbmlElement::Render(IOCallback & output, bool bKeepIntact, bool bKeepPosition, bool bForceRender)
{
	assert(bValueIsSet || (bKeepIntact && DefaultISset())); // an element is been rendered without a value set !!!
		                 // it may be a mandatory element without a default value
	try {
		if (!bKeepIntact && IsDefaultValue()) {
			return 0;
		}
#if defined(_DEBUG) || defined(DEBUG)
		uint64 SupposedSize = UpdateSize(bKeepIntact, bForceRender);
#endif // _DEBUG
		uint32 result = RenderHead(output, bForceRender, bKeepIntact, bKeepPosition);
		uint64 WrittenSize = RenderData(output, bForceRender, bKeepIntact);
#if defined(_DEBUG) || defined(DEBUG)
	if (SupposedSize != (0-1)) assert(WrittenSize == SupposedSize);
#endif // DEBUG
		result += WrittenSize;
		return result;
	} catch (std::exception & ex) {
// 		const char * What = ex.what();
		assert(false); // we should never be here !
		return 0;
	}
}

/*!
	\todo store the position of the Size writing for elements with unknown size
	\todo handle exceptions on errors
	\todo handle CodeSize bigger than 5 bytes
*/
uint32 EbmlElement::RenderHead(IOCallback & output, bool bForceRender, bool bKeepIntact, bool bKeepPosition)
{
	if (EbmlId(*this).Length <= 0 || EbmlId(*this).Length > 4)
		return 0;
	
	UpdateSize(bKeepIntact, bForceRender);
	
	return MakeRenderHead(output, bKeepPosition);
}
	
uint32 EbmlElement::MakeRenderHead(IOCallback & output, bool bKeepPosition)
{
	binary FinalHead[4+8]; // Class D + 64 bits coded size
	unsigned int FinalHeadSize;
	
	FinalHeadSize = EbmlId(*this).Length;
	EbmlId(*this).Fill(FinalHead);

	int CodedSize = CodedSizeLength(Size, SizeLength, bSizeIsFinite);
	CodedValueLength(Size, CodedSize, &FinalHead[FinalHeadSize]);
	FinalHeadSize += CodedSize;
	
	output.writeFully(FinalHead, FinalHeadSize);
	if (!bKeepPosition) {
		ElementPosition = output.getFilePointer() - FinalHeadSize;
		SizePosition = ElementPosition + EbmlId(*this).Length;
	}
	
	return FinalHeadSize;
}

uint64 EbmlElement::ElementSize(bool bKeepIntact) const
{
	if (!bKeepIntact && IsDefaultValue())
		return 0; // won't be saved
	return Size + EbmlId(*this).Length + CodedSizeLength(Size, SizeLength, bSizeIsFinite);
}

bool EbmlElement::CompareElements(const EbmlElement *A, const EbmlElement *B)
{
	if (EbmlId(*A) == EbmlId(*B))
		return *A < *B;
	else
		return false;
}

void EbmlElement::Read(EbmlStream & inDataStream, const EbmlSemanticContext & Context, int & UpperEltFound, EbmlElement * & FoundElt, bool AllowDummyElt, ScopeMode ReadFully)
{
	ReadData(inDataStream.I_O(), ReadFully);
}

bool EbmlElement::ForceSize(uint64 NewSize)
{
	if (bSizeIsFinite) {
		return false;
	}

	int OldSizeLen = CodedSizeLength(Size, SizeLength, bSizeIsFinite);
	uint64 OldSize = Size;

	Size = NewSize;

	if (CodedSizeLength(Size, SizeLength, bSizeIsFinite) == OldSizeLen) {
		bSizeIsFinite = true;
		return true;
	}
	Size = OldSize;

	return false;
}

uint32 EbmlElement::OverwriteHead(IOCallback & output, bool bKeepPosition)
{
	if (ElementPosition == 0) {
		return 0; // the element has not been written
	}

	uint64 CurrentPosition = output.getFilePointer();
	output.setFilePointer(GetElementPosition());
	uint32 Result = MakeRenderHead(output, bKeepPosition);
	output.setFilePointer(CurrentPosition);
	return Result;
}

uint32 EbmlElement::VoidMe(IOCallback & output, bool bKeepIntact)
{
	if (ElementPosition == 0) {
		return 0; // the element has not been written
	}

	EbmlVoid Dummy;
	return Dummy.Overwrite(*this, output, bKeepIntact);
}

END_LIBEBML_NAMESPACE
