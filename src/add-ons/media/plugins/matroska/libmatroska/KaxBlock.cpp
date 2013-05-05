/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxBlock.cpp 1265 2007-01-14 17:20:35Z mosu $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Julien Coloos    <suiryc @ users.sf.net>
*/
#include <cassert>

//#include <streams.h>

#include "matroska/KaxBlock.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlockData.h"
#include "matroska/KaxCluster.h"

START_LIBMATROSKA_NAMESPACE

#if MATROSKA_VERSION == 1
const EbmlSemantic KaxBlockGroup_ContextList[6] =
#else // MATROSKA_VERSION
const EbmlSemantic KaxBlockGroup_ContextList[9] =
#endif // MATROSKA_VERSION
{
	EbmlSemantic(true,  true,  KaxBlock::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true,  KaxBlockVirtual::ClassInfos),
#endif // MATROSKA_VERSION
	EbmlSemantic(false, true,  KaxBlockDuration::ClassInfos),
	EbmlSemantic(false, true,  KaxSlices::ClassInfos),
	EbmlSemantic(true,  true,  KaxReferencePriority::ClassInfos),
	EbmlSemantic(false, false, KaxReferenceBlock::ClassInfos),
#if MATROSKA_VERSION >= 2
	EbmlSemantic(false, true,  KaxReferenceVirtual::ClassInfos),
	EbmlSemantic(false, true,  KaxCodecState::ClassInfos),
#endif // MATROSKA_VERSION
	EbmlSemantic(false, true,  KaxBlockAdditions::ClassInfos),
};

const EbmlSemantic KaxBlockAdditions_ContextList[1] =
{
	EbmlSemantic(true,  false,  KaxBlockMore::ClassInfos)
};

const EbmlSemantic KaxBlockMore_ContextList[2] =
{
	EbmlSemantic(true,  true,  KaxBlockAddID::ClassInfos),
	EbmlSemantic(true,  true,  KaxBlockAdditional::ClassInfos)
};

EbmlId KaxBlockGroup_TheId     (0xA0, 1);
EbmlId KaxBlock_TheId          (0xA1, 1);
EbmlId KaxSimpleBlock_TheId    (0xA3, 1);
EbmlId KaxBlockDuration_TheId  (0x9B, 1);
#if MATROSKA_VERSION >= 2
EbmlId KaxBlockVirtual_TheId   (0xA2, 1);
EbmlId KaxCodecState_TheId     (0xA4, 1);
#endif // MATROSKA_VERSION
EbmlId KaxBlockAdditions_TheId (0x75A1, 2);
EbmlId KaxBlockMore_TheId      (0xA6, 1);
EbmlId KaxBlockAddID_TheId     (0xEE, 1);
EbmlId KaxBlockAdditional_TheId(0xA5, 1);

const EbmlSemanticContext KaxBlockGroup_Context = EbmlSemanticContext(countof(KaxBlockGroup_ContextList), KaxBlockGroup_ContextList, &KaxCluster_Context, *GetKaxGlobal_Context, &KaxBlockGroup::ClassInfos);
const EbmlSemanticContext KaxBlock_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxBlock::ClassInfos);
const EbmlSemanticContext KaxBlockDuration_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxBlockDuration::ClassInfos);
#if MATROSKA_VERSION >= 2
const EbmlSemanticContext KaxSimpleBlock_Context = EbmlSemanticContext(0, NULL, &KaxCluster_Context, *GetKaxGlobal_Context, &KaxSimpleBlock::ClassInfos);
const EbmlSemanticContext KaxBlockVirtual_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxBlockVirtual::ClassInfos);
const EbmlSemanticContext KaxCodecState_Context = EbmlSemanticContext(0, NULL, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxCodecState::ClassInfos);
#endif // MATROSKA_VERSION
const EbmlSemanticContext KaxBlockAdditions_Context = EbmlSemanticContext(countof(KaxBlockAdditions_ContextList), KaxBlockAdditions_ContextList, &KaxBlockGroup_Context, *GetKaxGlobal_Context, &KaxBlockAdditions::ClassInfos);
const EbmlSemanticContext KaxBlockMore_Context = EbmlSemanticContext(countof(KaxBlockMore_ContextList), KaxBlockMore_ContextList, &KaxBlockAdditions_Context, *GetKaxGlobal_Context, &KaxBlockMore::ClassInfos);
const EbmlSemanticContext KaxBlockAddID_Context = EbmlSemanticContext(0, NULL, &KaxBlockMore_Context, *GetKaxGlobal_Context, &KaxBlockAddID::ClassInfos);
const EbmlSemanticContext KaxBlockAdditional_Context = EbmlSemanticContext(0, NULL, &KaxBlockMore_Context, *GetKaxGlobal_Context, &KaxBlockAdditional::ClassInfos);

const EbmlCallbacks KaxBlockGroup::ClassInfos(KaxBlockGroup::Create, KaxBlockGroup_TheId, "BlockGroup", KaxBlockGroup_Context);
const EbmlCallbacks KaxBlock::ClassInfos(KaxBlock::Create, KaxBlock_TheId, "Block", KaxBlock_Context);
const EbmlCallbacks KaxBlockDuration::ClassInfos(KaxBlockDuration::Create, KaxBlockDuration_TheId, "BlockDuration", KaxBlockDuration_Context);
#if MATROSKA_VERSION >= 2
const EbmlCallbacks KaxSimpleBlock::ClassInfos(KaxSimpleBlock::Create, KaxSimpleBlock_TheId, "SimpleBlock", KaxSimpleBlock_Context);
const EbmlCallbacks KaxBlockVirtual::ClassInfos(KaxBlockVirtual::Create, KaxBlockVirtual_TheId, "BlockVirtual", KaxBlockVirtual_Context);
const EbmlCallbacks KaxCodecState::ClassInfos(KaxCodecState::Create, KaxCodecState_TheId, "CodecState", KaxCodecState_Context);
#endif // MATROSKA_VERSION
const EbmlCallbacks KaxBlockAdditions::ClassInfos(KaxBlockAdditions::Create, KaxBlockAdditions_TheId, "BlockAdditions", KaxBlockAdditions_Context);
const EbmlCallbacks KaxBlockMore::ClassInfos(KaxBlockMore::Create, KaxBlockMore_TheId, "BlockMore", KaxBlockMore_Context);
const EbmlCallbacks KaxBlockAddID::ClassInfos(KaxBlockAddID::Create, KaxBlockAddID_TheId, "BlockAddID", KaxBlockAddID_Context);
const EbmlCallbacks KaxBlockAdditional::ClassInfos(KaxBlockAdditional::Create, KaxBlockAdditional_TheId, "BlockAdditional", KaxBlockAdditional_Context);

DataBuffer * DataBuffer::Clone()
{
	binary *ClonedData = (binary *)malloc(mySize * sizeof(binary));
	assert(ClonedData != NULL);
	memcpy(ClonedData, myBuffer ,mySize );

	SimpleDataBuffer * result = new SimpleDataBuffer(ClonedData, mySize, 0);
	result->bValidValue = bValidValue;
	return result;
}

SimpleDataBuffer::SimpleDataBuffer(const SimpleDataBuffer & ToClone)
 :DataBuffer((binary *)malloc(ToClone.mySize * sizeof(binary)), ToClone.mySize, myFreeBuffer)
{
	assert(myBuffer != NULL);
	memcpy(myBuffer, ToClone.myBuffer ,mySize );
	bValidValue = ToClone.bValidValue;
}

bool KaxInternalBlock::ValidateSize() const
{
	return (Size >= 4); /// for the moment
}

KaxInternalBlock::~KaxInternalBlock()
{
	ReleaseFrames();
}

KaxInternalBlock::KaxInternalBlock(const KaxInternalBlock & ElementToClone)
 :EbmlBinary(ElementToClone)
 ,myBuffers(ElementToClone.myBuffers.size())
 ,Timecode(ElementToClone.Timecode)
 ,LocalTimecode(ElementToClone.LocalTimecode)
 ,bLocalTimecodeUsed(ElementToClone.bLocalTimecodeUsed)
 ,TrackNumber(ElementToClone.TrackNumber)
 ,ParentCluster(ElementToClone.ParentCluster) ///< \todo not exactly
{
	// add a clone of the list
	std::vector<DataBuffer *>::const_iterator Itr = ElementToClone.myBuffers.begin();
	std::vector<DataBuffer *>::iterator myItr = myBuffers.begin();
	while (Itr != ElementToClone.myBuffers.end())
	{
		*myItr = (*Itr)->Clone();
		Itr++; myItr++;
	}
}


KaxBlockGroup::~KaxBlockGroup()
{
//NOTE("KaxBlockGroup::~KaxBlockGroup");
}

KaxBlockGroup::KaxBlockGroup()
 :EbmlMaster(KaxBlockGroup_Context)
 ,ParentCluster(NULL)
 ,ParentTrack(NULL)
{}

KaxBlockAdditions::KaxBlockAdditions()
 :EbmlMaster(KaxBlockAdditions_Context)
{}

KaxBlockMore::KaxBlockMore()
 :EbmlMaster(KaxBlockMore_Context)
{}

/*!
	\todo handle flags
	\todo hardcoded limit of the number of frames in a lace should be a parameter
	\return true if more frames can be added to this Block
*/
bool KaxInternalBlock::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing, bool invisible)
{
	bValueIsSet = true;
	if (myBuffers.size() == 0) {
		// first frame
		Timecode = timecode;
		TrackNumber = track.TrackNumber();
		mInvisible = invisible;
		mLacing = lacing;
	}
	myBuffers.push_back(&buffer);

	// we don't allow more than 8 frames in a Block because the overhead improvement is minimal
	if (myBuffers.size() >= 8 || lacing == LACING_NONE)
		return false;

	if (lacing == LACING_XIPH)
		// decide whether a new frame can be added or not
		// a frame in a lace is not efficient when the place necessary to code it in a lace is bigger 
		// than the size of a simple Block. That means more than 6 bytes (4 in struct + 2 for EBML) to code the size
		return (buffer.Size() < 6*0xFF);
	else
		return true;
}

/*!
       \return Returns the lacing type that produces the smallest footprint.
*/
LacingType KaxInternalBlock::GetBestLacingType() const {
	int XiphLacingSize, EbmlLacingSize, i;
	bool SameSize = true;

	if (myBuffers.size() <= 1)
		return LACING_NONE;

	XiphLacingSize = 1; // Number of laces is stored in 1 byte.
	EbmlLacingSize = 1;
	for (i = 0; i < (int)myBuffers.size() - 1; i++) {
		if (myBuffers[i]->Size() != myBuffers[i + 1]->Size())
			SameSize = false;
		XiphLacingSize += myBuffers[i]->Size() / 255 + 1;
	}
	EbmlLacingSize += CodedSizeLength(myBuffers[0]->Size(), 0, bSizeIsFinite);
	for (i = 1; i < (int)myBuffers.size() - 1; i++)
		EbmlLacingSize += CodedSizeLengthSigned(int64(myBuffers[i]->Size()) - int64(myBuffers[i - 1]->Size()), 0);
	if (SameSize)
		return LACING_FIXED;
	else if (XiphLacingSize < EbmlLacingSize)
		return LACING_XIPH;
	else
		return LACING_EBML;
}

uint64 KaxInternalBlock::UpdateSize(bool bSaveDefault, bool bForceRender)
{
	LacingType LacingHere;
	assert(Data == NULL); // Data is not used for KaxInternalBlock
	assert(TrackNumber < 0x4000); // no more allowed for the moment
	unsigned int i;

	// compute the final size of the data
	switch (myBuffers.size()) {
		case 0:
			Size = 0;
			break;
		case 1:
			Size = 4 + myBuffers[0]->Size();
			break;
		default:
			Size = 4 + 1; // 1 for the lacing head
			if (mLacing == LACING_AUTO)
				LacingHere = GetBestLacingType();
			else
				LacingHere = mLacing;
			switch (LacingHere)
			{
			case LACING_XIPH:
				for (i=0; i<myBuffers.size()-1; i++) {
					Size += myBuffers[i]->Size() + (myBuffers[i]->Size() / 0xFF + 1);
				}
				break;
			case LACING_EBML:
				Size += myBuffers[0]->Size() + CodedSizeLength(myBuffers[0]->Size(), 0, bSizeIsFinite);
				for (i=1; i<myBuffers.size()-1; i++) {
					Size += myBuffers[i]->Size() 
						+ CodedSizeLengthSigned(int64(myBuffers[i]->Size()) - int64(myBuffers[i-1]->Size()), 0);;
				}
				break;
			case LACING_FIXED:
				for (i=0; i<myBuffers.size()-1; i++) {
					Size += myBuffers[i]->Size();
				}
				break;
			default:
				assert(0);
			}
			// Size of the last frame (not in lace)
			Size += myBuffers[i]->Size();
			break;
	}

	if (TrackNumber >= 0x80)
		Size++; // the size will be coded with one more octet

	return Size;
}

#if MATROSKA_VERSION >= 2
KaxBlockVirtual::KaxBlockVirtual(const KaxBlockVirtual & ElementToClone)
 :EbmlBinary(ElementToClone)
 ,Timecode(ElementToClone.Timecode)
 ,TrackNumber(ElementToClone.TrackNumber)
 ,ParentCluster(ElementToClone.ParentCluster) ///< \todo not exactly
{
	Data = DataBlock;
}

uint64 KaxBlockVirtual::UpdateSize(bool bSaveDefault, bool bForceRender)
{
	assert(TrackNumber < 0x4000);
	binary *cursor = Data;
	// fill data
	if (TrackNumber < 0x80) {
		*cursor++ = TrackNumber | 0x80; // set the first bit to 1 
	} else {
		*cursor++ = (TrackNumber >> 8) | 0x40; // set the second bit to 1
		*cursor++ = TrackNumber & 0xFF;
	}

	assert(ParentCluster != NULL);
	int16 ActualTimecode = ParentCluster->GetBlockLocalTimecode(Timecode);
	big_int16 b16(ActualTimecode);
	b16.Fill(cursor);
	cursor += 2;

	*cursor++ = 0; // flags

	return Size;
}
#endif // MATROSKA_VERSION

/*!
	\todo more optimisation is possible (render the Block head and don't copy the buffer in memory, care should be taken with the allocation of Data)
	\todo the actual timecode to write should be retrieved from the Cluster from here
*/
uint32 KaxInternalBlock::RenderData(IOCallback & output, bool bForceRender, bool bSaveDefault)
{
	if (myBuffers.size() == 0) {
		return 0;
	} else {
		assert(TrackNumber < 0x4000);
		binary BlockHead[5], *cursor = BlockHead;
		unsigned int i;

		if (myBuffers.size() == 1) {
			Size = 4;
			mLacing = LACING_NONE;
		} else {
			if (mLacing == LACING_NONE)
				mLacing = LACING_EBML; // supposedly the best of all
			Size = 4 + 1; // 1 for the lacing head (number of laced elements)
		}
		if (TrackNumber > 0x80)
			Size++;

		// write Block Head
		if (TrackNumber < 0x80) {
			*cursor++ = TrackNumber | 0x80; // set the first bit to 1 
		} else {
			*cursor++ = (TrackNumber >> 8) | 0x40; // set the second bit to 1
			*cursor++ = TrackNumber & 0xFF;
		}

		assert(ParentCluster != NULL);
		int16 ActualTimecode = ParentCluster->GetBlockLocalTimecode(Timecode);
		big_int16 b16(ActualTimecode);
		b16.Fill(cursor);
		cursor += 2;

		*cursor = 0; // flags

		if (mLacing == LACING_AUTO) {
			mLacing = GetBestLacingType();
		}

		// invisible flag
		if (mInvisible)
			*cursor = 0x08;

		if (bIsSimple) {
			if (bIsKeyframe)
				*cursor |= 0x80;
			if (bIsDiscardable)
				*cursor |= 0x01;
		}
		
		// lacing flag
		switch (mLacing)
		{
		case LACING_XIPH:
			*cursor++ |= 0x02;
			break;
		case LACING_EBML:
			*cursor++ |= 0x06;
			break;
		case LACING_FIXED:
			*cursor++ |= 0x04;
			break;
		case LACING_NONE:
			break;
	    default:
			assert(0);
		}

		output.writeFully(BlockHead, 4 + ((TrackNumber > 0x80) ? 1 : 0));

		binary tmpValue;
		switch (mLacing)
		{
		case LACING_XIPH:
			// number of laces
			tmpValue = myBuffers.size()-1;
			output.writeFully(&tmpValue, 1);

			// set the size of each member in the lace
			for (i=0; i<myBuffers.size()-1; i++) {
				tmpValue = 0xFF;
				uint16 tmpSize = myBuffers[i]->Size();
				while (tmpSize >= 0xFF) {
					output.writeFully(&tmpValue, 1);
					Size++;
					tmpSize -= 0xFF;
				}
				tmpValue = binary(tmpSize);
				output.writeFully(&tmpValue, 1);
				Size++;
			}
			break;
		case LACING_EBML:
			// number of laces
			tmpValue = myBuffers.size()-1;
			output.writeFully(&tmpValue, 1);

			{
				int64 _Size;
				int _CodedSize;
				binary _FinalHead[8]; // 64 bits max coded size

				_Size = myBuffers[0]->Size();

				_CodedSize = CodedSizeLength(_Size, 0, bSizeIsFinite);

				// first size in the lace is not a signed
				CodedValueLength(_Size, _CodedSize, _FinalHead);
				output.writeFully(_FinalHead, _CodedSize);
				Size += _CodedSize;

				// set the size of each member in the lace
				for (i=1; i<myBuffers.size()-1; i++) {
					_Size = int64(myBuffers[i]->Size()) - int64(myBuffers[i-1]->Size());
					_CodedSize = CodedSizeLengthSigned(_Size, 0);
					CodedValueLengthSigned(_Size, _CodedSize, _FinalHead);
					output.writeFully(_FinalHead, _CodedSize);
					Size += _CodedSize;
				}
			}
			break;
		case LACING_FIXED:
			// number of laces
			tmpValue = myBuffers.size()-1;
			output.writeFully(&tmpValue, 1);
			break;
		case LACING_NONE:
			break;
	    default:
			assert(0);
		}

		// put the data of each frame
		for (i=0; i<myBuffers.size(); i++) {
			output.writeFully(myBuffers[i]->Buffer(), myBuffers[i]->Size());
			Size += myBuffers[i]->Size();
		}
	}

	return Size;
}

uint64 KaxInternalBlock::ReadInternalHead(IOCallback & input)
{
	binary Buffer[5], *cursor = Buffer;
	uint64 Result = input.read(cursor, 4);
	if (Result != 4)
		return Result;
	
	// update internal values
	TrackNumber = *cursor++;
	if ((TrackNumber & 0x80) == 0) {
		// there is extra data
		if ((TrackNumber & 0x40) == 0) {
			// We don't support track numbers that large !
			return Result;
		}
		Result += input.read(&Buffer[4], 1);
		TrackNumber = (TrackNumber & 0x3F) << 8;
		TrackNumber += *cursor++;
	} else {
		TrackNumber &= 0x7F;
	}

    
	big_int16 b16;
	b16.Eval(cursor);
	assert(ParentCluster != NULL);
	Timecode = ParentCluster->GetBlockGlobalTimecode(int16(b16));
	bLocalTimecodeUsed = false;
	cursor += 2;

	return Result;
}

/*!
	\todo better zero copy handling
*/
uint64 KaxInternalBlock::ReadData(IOCallback & input, ScopeMode ReadFully)
{
	uint64 Result;

	FirstFrameLocation = input.getFilePointer(); // will be updated accordingly below

	if (ReadFully == SCOPE_ALL_DATA)
	{
		Result = EbmlBinary::ReadData(input, ReadFully);
		binary *cursor = Data;
		uint8 BlockHeadSize = 4;

		// update internal values
		TrackNumber = *cursor++;
		if ((TrackNumber & 0x80) == 0) {
			// there is extra data
			if ((TrackNumber & 0x40) == 0) {
				// We don't support track numbers that large !
				return Result;
			}
			TrackNumber = (TrackNumber & 0x3F) << 8;
			TrackNumber += *cursor++;
			BlockHeadSize++;
		} else {
			TrackNumber &= 0x7F;
		}

		big_int16 b16;
		b16.Eval(cursor);
		LocalTimecode = int16(b16);
		bLocalTimecodeUsed = true;
		cursor += 2;

		if (EbmlId(*this) == KaxSimpleBlock::ClassInfos.GlobalId) {
			bIsKeyframe = (*cursor & 0x80) != 0;
			bIsDiscardable = (*cursor & 0x01) != 0;
		}
		mInvisible = (*cursor & 0x08) >> 3;
		mLacing = LacingType((*cursor++ & 0x06) >> 1);

		// put all Frames in the list
		if (mLacing == LACING_NONE) {
			FirstFrameLocation += cursor - Data;
			DataBuffer * soloFrame = new DataBuffer(cursor, Size - BlockHeadSize);
			myBuffers.push_back(soloFrame);
			SizeList.resize(1);
			SizeList[0] = Size - BlockHeadSize;
		} else {
			// read the number of frames in the lace
			uint32 LastBufferSize = Size - BlockHeadSize - 1; // 1 for number of frame
			uint8 FrameNum = *cursor++; // number of frames in the lace - 1
			// read the list of frame sizes
			uint8 Index;
			int32 FrameSize;
			uint32 SizeRead;
			uint64 SizeUnknown;

			SizeList.resize(FrameNum + 1);

			switch (mLacing)
			{
			case LACING_XIPH:
				for (Index=0; Index<FrameNum; Index++) {
					// get the size of the frame
					FrameSize = 0;
					do {
						FrameSize += uint8(*cursor);
						LastBufferSize--;
					} while (*cursor++ == 0xFF);
					SizeList[Index] = FrameSize;
					LastBufferSize -= FrameSize;
				}
				SizeList[Index] = LastBufferSize;
				break;
			case LACING_EBML:
				SizeRead = LastBufferSize;
				FrameSize = ReadCodedSizeValue(cursor, SizeRead, SizeUnknown);
				SizeList[0] = FrameSize;
				cursor += SizeRead;
				LastBufferSize -= FrameSize + SizeRead;

				for (Index=1; Index<FrameNum; Index++) {
					// get the size of the frame
					SizeRead = LastBufferSize;
					FrameSize += ReadCodedSizeSignedValue(cursor, SizeRead, SizeUnknown);
					SizeList[Index] = FrameSize;
					cursor += SizeRead;
					LastBufferSize -= FrameSize + SizeRead;
				}
				SizeList[Index] = LastBufferSize;
				break;
			case LACING_FIXED:
				for (Index=0; Index<=FrameNum; Index++) {
					// get the size of the frame
					SizeList[Index] = LastBufferSize / (FrameNum + 1);
				}
				break;
			default: // other lacing not supported
				assert(0);
			}

			FirstFrameLocation += cursor - Data;

			for (Index=0; Index<=FrameNum; Index++) {
				DataBuffer * lacedFrame = new DataBuffer(cursor, SizeList[Index]);
				myBuffers.push_back(lacedFrame);
				cursor += SizeList[Index];
			}
		}
		bValueIsSet = true;
	}
	else if (ReadFully == SCOPE_PARTIAL_DATA)
	{
		binary _TempHead[5];
		Result = input.read(_TempHead, 5);
		binary *cursor = _TempHead;
		binary *_tmpBuf;
		uint8 BlockHeadSize = 4;

		// update internal values
		TrackNumber = *cursor++;
		if ((TrackNumber & 0x80) == 0) {
			// there is extra data
			if ((TrackNumber & 0x40) == 0) {
				// We don't support track numbers that large !
				return Result;
			}
			TrackNumber = (TrackNumber & 0x3F) << 8;
			TrackNumber += *cursor++;
			BlockHeadSize++;
		} else {
			TrackNumber &= 0x7F;
		}

		big_int16 b16;
		b16.Eval(cursor);
		LocalTimecode = int16(b16);
		bLocalTimecodeUsed = true;
		cursor += 2;

		if (EbmlId(*this) == KaxSimpleBlock::ClassInfos.GlobalId) {
			bIsKeyframe = (*cursor & 0x80) != 0;
			bIsDiscardable = (*cursor & 0x01) != 0;
		}
		mInvisible = (*cursor & 0x08) >> 3;
		mLacing = LacingType((*cursor++ & 0x06) >> 1);
		if (cursor == &_TempHead[4])
		{
			_TempHead[0] = _TempHead[4];
		} else {
			Result += input.read(_TempHead, 1);
		}

		FirstFrameLocation += cursor - _TempHead;

		// put all Frames in the list
		if (mLacing != LACING_NONE) {
			// read the number of frames in the lace
			uint32 LastBufferSize = Size - BlockHeadSize - 1; // 1 for number of frame
			uint8 FrameNum = _TempHead[0]; // number of frames in the lace - 1
			// read the list of frame sizes
			uint8 Index;
			int32 FrameSize;
			uint32 SizeRead;
			uint64 SizeUnknown;

			SizeList.resize(FrameNum + 1);

			switch (mLacing)
			{
			case LACING_XIPH:
				for (Index=0; Index<FrameNum; Index++) {
					// get the size of the frame
					FrameSize = 0;
					do {
						Result += input.read(_TempHead, 1);
						FrameSize += uint8(_TempHead[0]);
						LastBufferSize--;

						FirstFrameLocation++;
					} while (_TempHead[0] == 0xFF);

					FirstFrameLocation++;
					SizeList[Index] = FrameSize;
					LastBufferSize -= FrameSize;
				}
				SizeList[Index] = LastBufferSize;
				break;
			case LACING_EBML:
				SizeRead = LastBufferSize;
				cursor = _tmpBuf = new binary[FrameNum*4]; /// \warning assume the mean size will be coded in less than 4 bytes
				Result += input.read(cursor, FrameNum*4);
				FrameSize = ReadCodedSizeValue(cursor, SizeRead, SizeUnknown);
				SizeList[0] = FrameSize;
				cursor += SizeRead;
				LastBufferSize -= FrameSize + SizeRead;

				for (Index=1; Index<FrameNum; Index++) {
					// get the size of the frame
					SizeRead = LastBufferSize;
					FrameSize += ReadCodedSizeSignedValue(cursor, SizeRead, SizeUnknown);
					SizeList[Index] = FrameSize;
					cursor += SizeRead;
					LastBufferSize -= FrameSize + SizeRead;
				}

				FirstFrameLocation += cursor - _tmpBuf;

				SizeList[Index] = LastBufferSize;
				delete [] _tmpBuf;
				break;
			case LACING_FIXED:
				for (Index=0; Index<=FrameNum; Index++) {
					// get the size of the frame
					SizeList[Index] = LastBufferSize / (FrameNum + 1);
				}
				break;
			default: // other lacing not supported
				assert(0);
			}
		} else {
			SizeList.resize(1);
			SizeList[0] = Size - BlockHeadSize;
		}
		bValueIsSet = false;
		Result = Size;
	} else {
		bValueIsSet = false;
		Result = Size;
	}

	return Result;
}

bool KaxBlockGroup::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing)
{
	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	assert(ParentCluster != NULL);
	theBlock.SetParent(*ParentCluster);
	ParentTrack = &track;
	return theBlock.AddFrame(track, timecode, buffer, lacing);
}

bool KaxBlockGroup::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockGroup & PastBlock, LacingType lacing)
{
//	assert(past_timecode < 0);

	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	assert(ParentCluster != NULL);
	theBlock.SetParent(*ParentCluster);
	ParentTrack = &track;
	bool bRes = theBlock.AddFrame(track, timecode, buffer, lacing);

	KaxReferenceBlock & thePastRef = GetChild<KaxReferenceBlock>(*this);
	thePastRef.SetReferencedBlock(PastBlock);
	thePastRef.SetParentBlock(*this);

	return bRes;
}

bool KaxBlockGroup::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockGroup & PastBlock, const KaxBlockGroup & ForwBlock, LacingType lacing)
{
//	assert(past_timecode < 0);

//	assert(forw_timecode > 0);
	
	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	assert(ParentCluster != NULL);
	theBlock.SetParent(*ParentCluster);
	ParentTrack = &track;
	bool bRes = theBlock.AddFrame(track, timecode, buffer, lacing);

	KaxReferenceBlock & thePastRef = GetChild<KaxReferenceBlock>(*this);
	thePastRef.SetReferencedBlock(PastBlock);
	thePastRef.SetParentBlock(*this);

	KaxReferenceBlock & theFutureRef = AddNewChild<KaxReferenceBlock>(*this);
	theFutureRef.SetReferencedBlock(ForwBlock);
	theFutureRef.SetParentBlock(*this);

	return bRes;
}

bool KaxBlockGroup::AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockBlob * PastBlock, const KaxBlockBlob * ForwBlock, LacingType lacing)
{
	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	assert(ParentCluster != NULL);
	theBlock.SetParent(*ParentCluster);
	ParentTrack = &track;
	bool bRes = theBlock.AddFrame(track, timecode, buffer, lacing);

	if (PastBlock != NULL)
	{
		KaxReferenceBlock & thePastRef = GetChild<KaxReferenceBlock>(*this);
		thePastRef.SetReferencedBlock(PastBlock);
		thePastRef.SetParentBlock(*this);
	}

	if (ForwBlock != NULL)
	{
		KaxReferenceBlock & theFutureRef = AddNewChild<KaxReferenceBlock>(*this);
		theFutureRef.SetReferencedBlock(ForwBlock);
		theFutureRef.SetParentBlock(*this);
	}

	return bRes;
}

/*!
	\todo we may cache the reference to the timecode block
*/
uint64 KaxBlockGroup::GlobalTimecode() const
{
	assert(ParentCluster != NULL); // impossible otherwise
	KaxInternalBlock & MyBlock = *static_cast<KaxBlock *>(this->FindElt(KaxBlock::ClassInfos));
	return MyBlock.GlobalTimecode();

}

uint16 KaxBlockGroup::TrackNumber() const
{
	KaxInternalBlock & MyBlock = *static_cast<KaxBlock *>(this->FindElt(KaxBlock::ClassInfos));
	return MyBlock.TrackNum();
}

uint64 KaxBlockGroup::ClusterPosition() const
{
	assert(ParentCluster != NULL); // impossible otherwise
	return ParentCluster->GetPosition();
}

uint64 KaxInternalBlock::ClusterPosition() const
{
	assert(ParentCluster != NULL); // impossible otherwise
	return ParentCluster->GetPosition();
}

unsigned int KaxBlockGroup::ReferenceCount() const
{
	unsigned int Result = 0;
	KaxReferenceBlock * MyBlockAdds = static_cast<KaxReferenceBlock *>(FindFirstElt(KaxReferenceBlock::ClassInfos));
	if (MyBlockAdds != NULL) {
		Result++;
		while ((MyBlockAdds = static_cast<KaxReferenceBlock *>(FindNextElt(*MyBlockAdds))) != NULL)
		{
			Result++;
		}
	}
	return Result;
}

const KaxReferenceBlock & KaxBlockGroup::Reference(unsigned int Index) const
{
	KaxReferenceBlock * MyBlockAdds = static_cast<KaxReferenceBlock *>(FindFirstElt(KaxReferenceBlock::ClassInfos));
	assert(MyBlockAdds != NULL); // call of a non existing reference
	
	while (Index != 0) {
		MyBlockAdds = static_cast<KaxReferenceBlock *>(FindNextElt(*MyBlockAdds));
		assert(MyBlockAdds != NULL);
		Index--;
	}
	return *MyBlockAdds;
}

void KaxBlockGroup::ReleaseFrames()
{
	KaxInternalBlock & MyBlock = *static_cast<KaxBlock *>(this->FindElt(KaxBlock::ClassInfos));
	MyBlock.ReleaseFrames();
}

void KaxInternalBlock::ReleaseFrames()
{
	// free the allocated Frames
	int i;
	for (i=myBuffers.size()-1; i>=0; i--) {
		if (myBuffers[i] != NULL) {
			myBuffers[i]->FreeBuffer(*myBuffers[i]);
			delete myBuffers[i];
			myBuffers[i] = NULL;
		}
	}
}

void KaxBlockGroup::SetBlockDuration(uint64 TimeLength)
{
	assert(ParentTrack != NULL);
	int64 scale = ParentTrack->GlobalTimecodeScale();
	KaxBlockDuration & myDuration = *static_cast<KaxBlockDuration *>(FindFirstElt(KaxBlockDuration::ClassInfos, true));
	*(static_cast<EbmlUInteger *>(&myDuration)) = TimeLength / uint64(scale);
}

bool KaxBlockGroup::GetBlockDuration(uint64 &TheTimecode) const
{
	KaxBlockDuration * myDuration = static_cast<KaxBlockDuration *>(FindElt(KaxBlockDuration::ClassInfos));
	if (myDuration == NULL) {
		return false;
	}

	assert(ParentTrack != NULL);
	TheTimecode = uint64(*myDuration) * ParentTrack->GlobalTimecodeScale();
	return true;
}

KaxBlockGroup::operator KaxInternalBlock &() {
	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	return theBlock;
}

void KaxBlockGroup::SetParent(KaxCluster & aParentCluster) {
	ParentCluster = &aParentCluster;
	KaxBlock & theBlock = GetChild<KaxBlock>(*this);
	theBlock.SetParent( aParentCluster );
}

void KaxInternalBlock::SetParent(KaxCluster & aParentCluster)
{
	ParentCluster = &aParentCluster;
	if (bLocalTimecodeUsed) {
		Timecode = aParentCluster.GetBlockGlobalTimecode(LocalTimecode);
		bLocalTimecodeUsed = false;
	}
}

int64 KaxInternalBlock::GetDataPosition(size_t FrameNumber)
{
	int64 _Result = -1;

	if (bValueIsSet && FrameNumber < SizeList.size())
	{
		_Result = FirstFrameLocation;
	
		size_t _Idx = 0;
		while(FrameNumber--)
		{
			_Result += SizeList[_Idx++];
		}
	}

	return _Result;
}

int64 KaxInternalBlock::GetFrameSize(size_t FrameNumber)
{
	int64 _Result = -1;

	if (/*bValueIsSet &&*/ FrameNumber < SizeList.size())
	{
		_Result = SizeList[FrameNumber];
	}

	return _Result;
}

KaxBlockBlob::operator KaxBlockGroup &()
{
	assert(!bUseSimpleBlock);
	assert(Block.group);
	return *Block.group;
}

KaxBlockBlob::operator const KaxBlockGroup &() const
{
	assert(!bUseSimpleBlock);
	assert(Block.group);
	return *Block.group;
}

KaxBlockBlob::operator KaxInternalBlock &()
{
	assert(Block.group);
#if MATROSKA_VERSION >= 2
	if (bUseSimpleBlock)
		return *Block.simpleblock;
	else
#endif
		return *Block.group;
}

KaxBlockBlob::operator const KaxInternalBlock &() const
{
	assert(Block.group);
#if MATROSKA_VERSION >= 2
	if (bUseSimpleBlock)
		return *Block.simpleblock;
	else
#endif
		return *Block.group;
}

#if MATROSKA_VERSION >= 2
KaxBlockBlob::operator KaxSimpleBlock &()
{
	assert(bUseSimpleBlock);
	assert(Block.simpleblock);
	return *Block.simpleblock;
}
#endif

bool KaxBlockBlob::AddFrameAuto(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing, const KaxBlockBlob * PastBlock, const KaxBlockBlob * ForwBlock)
{
	bool bResult = false;
#if MATROSKA_VERSION >= 2
	if ((SimpleBlockMode == BLOCK_BLOB_ALWAYS_SIMPLE) || (SimpleBlockMode == BLOCK_BLOB_SIMPLE_AUTO && PastBlock == NULL && ForwBlock == NULL)) {
		assert(bUseSimpleBlock == true);
		if (Block.simpleblock == NULL) {
			Block.simpleblock = new KaxSimpleBlock();
			Block.simpleblock->SetParent(*ParentCluster);
		}

		bResult = Block.simpleblock->AddFrame(track, timecode, buffer, lacing);
		if (PastBlock == NULL && ForwBlock == NULL) {
			Block.simpleblock->SetKeyframe(true);
			Block.simpleblock->SetDiscardable(false);
		} else {
			Block.simpleblock->SetKeyframe(false);
			if ((ForwBlock == NULL || ((const KaxInternalBlock &)*ForwBlock).GlobalTimecode() <= timecode) &&
				(PastBlock == NULL || ((const KaxInternalBlock &)*PastBlock).GlobalTimecode() <= timecode))
				Block.simpleblock->SetDiscardable(false);
			else
				Block.simpleblock->SetDiscardable(true);
		}
	}
	else
#endif
	{
		if (ReplaceSimpleByGroup()) {
			bResult = Block.group->AddFrame(track, timecode, buffer, PastBlock, ForwBlock, lacing);
		}
	}

	return bResult;
}

void KaxBlockBlob::SetParent(KaxCluster & parent_clust)
{
	ParentCluster = &parent_clust;
}

void KaxBlockBlob::SetBlockDuration(uint64 TimeLength)
{
	if (ReplaceSimpleByGroup())
		Block.group->SetBlockDuration(TimeLength);
}

bool KaxBlockBlob::ReplaceSimpleByGroup()
{
	if (SimpleBlockMode== BLOCK_BLOB_ALWAYS_SIMPLE)
		return false;

	if (!bUseSimpleBlock) {
		if (Block.group == NULL) {
			Block.group = new KaxBlockGroup();
		}
	}
#if MATROSKA_VERSION >= 2
	else 
	{

		if (Block.simpleblock != NULL) {
			KaxSimpleBlock *old_simpleblock = Block.simpleblock;
			Block.group = new KaxBlockGroup();
			// _TODO_ : move all the data to the blockgroup
			assert(false);
			// -> while(frame) AddFrame(myBuffer)
			delete old_simpleblock;
		} else {
			Block.group = new KaxBlockGroup();
		}
	}
#endif
	if (ParentCluster != NULL)
		Block.group->SetParent(*ParentCluster);

	bUseSimpleBlock = false;
	return true;
}

void KaxBlockBlob::SetBlockGroup( KaxBlockGroup &BlockRef )
{
	assert(!bUseSimpleBlock);
	Block.group = &BlockRef;
}

END_LIBMATROSKA_NAMESPACE
