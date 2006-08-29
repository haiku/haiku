/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <string.h>
#include <stdlib.h>

#include <Debug.h>
#include <AppDefs.h>
#include <InterfaceDefs.h>

#include "Attributes.h"
#include "Commands.h"
#include "PoseView.h"
#include "Utilities.h"
#include "ViewState.h"

const char *kColumnVersionName = "BColumn:version";
const char *kColumnTitleName = "BColumn:fTitle";
const char *kColumnOffsetName = "BColumn:fOffset";
const char *kColumnWidthName = "BColumn:fWidth";
const char *kColumnAlignmentName = "BColumn:fAlignment";
const char *kColumnAttrName = "BColumn:fAttrName";
const char *kColumnAttrHashName = "BColumn:fAttrHash";
const char *kColumnAttrTypeName = "BColumn:fAttrType";
const char *kColumnStatFieldName = "BColumn:fStatField";
const char *kColumnEditableName = "BColumn:fEditable";

BColumn::BColumn(const char *title, float offset, float width, alignment a,
	const char *attributeName, uint32 attr_type, bool stat_field,
	bool editable)
	:	fTitle(title),
		fAttrName(attributeName)
{
	fOffset = offset;
	fWidth = width;
	fAlignment = a;
	fAttrHash = AttrHashString(attributeName, attr_type);
	fAttrType = attr_type;
	fStatField = stat_field;
	fEditable = editable;
}

BColumn::~BColumn()
{
}

BColumn::BColumn(BMallocIO *stream, bool endianSwap)
{
	StringFromStream(&fTitle, stream, endianSwap);
	stream->Read(&fOffset, sizeof(float));
	stream->Read(&fWidth, sizeof(float));
	stream->Read(&fAlignment, sizeof(alignment));
	StringFromStream(&fAttrName, stream, endianSwap);
	stream->Read(&fAttrHash, sizeof(uint32));
	stream->Read(&fAttrType, sizeof(uint32));
	stream->Read(&fStatField, sizeof(bool));
	stream->Read(&fEditable, sizeof(bool));
	
	if (endianSwap) {
		PRINT(("endian swapping column\n"));
		fOffset = B_SWAP_FLOAT(fOffset);
		fWidth = B_SWAP_FLOAT(fWidth);
		STATIC_ASSERT(sizeof(BColumn::fAlignment) == sizeof(int32));
		fAlignment = (alignment)B_SWAP_INT32(fAlignment);
		fAttrHash = B_SWAP_INT32(fAttrHash);
		fAttrType = B_SWAP_INT32(fAttrType);
	}
}

BColumn::BColumn(const BMessage &message, int32 index)
{
	message.FindString(kColumnTitleName, index, &fTitle);
	message.FindFloat(kColumnOffsetName, index, &fOffset);
	message.FindFloat(kColumnWidthName, index, &fWidth);
	message.FindInt32(kColumnAlignmentName, index, (int32 *)&fAlignment);
	message.FindString(kColumnAttrName, index, &fAttrName);
	message.FindInt32(kColumnAttrHashName, index, (int32 *)&fAttrHash);
	message.FindInt32(kColumnAttrTypeName, index, (int32 *)&fAttrType);
	message.FindBool(kColumnStatFieldName, index, &fStatField);
	message.FindBool(kColumnEditableName, index, &fEditable);
}

BColumn *
BColumn::InstantiateFromStream(BMallocIO *stream, bool endianSwap)
{
	// compare stream header in canonical form
	uint32 key = AttrHashString("BColumn", B_OBJECT_TYPE);
	int32 version = kColumnStateArchiveVersion;

	
	if (endianSwap) {
		key = SwapUInt32(key);
		version = SwapInt32(version);
	}

//	PRINT(("validating key %x, version %d\n", key, version));
	if (!ValidateStream(stream, key, version)) 
		return 0;

//	PRINT(("instantiating column, %s\n", endianSwap ? "endian swapping," : ""));
	BColumn *result = new BColumn(stream, endianSwap);
	
	// sanity-check the resulting column
	if (result->fTitle.Length() > 500
		|| result->fOffset < 0
		|| result->fOffset > 10000
		|| result->fWidth < 0
		|| result->fWidth > 10000
		|| (int32)result->fAlignment < B_ALIGN_LEFT
		|| (int32)result->fAlignment > B_ALIGN_CENTER
		|| result->fAttrName.Length() > 500) {
		PRINT(("column data not valid\n"));
		delete result;
		return 0;
	}
#if DEBUG
	else if (endianSwap)
		PRINT(("Instantiated foreign column ok\n"));
#endif

	return result;
}

BColumn *
BColumn::InstantiateFromMessage(const BMessage &message, int32 index)
{
	int32 version = kColumnStateArchiveVersion;
	int32 messageVersion;

	if (message.FindInt32(kColumnVersionName, index, &messageVersion) != B_OK)
		return NULL;

	if (version != messageVersion) 
		return NULL;

	BColumn *result = new BColumn(message, index);
	
	// sanity-check the resulting column
	if (result->fTitle.Length() > 500
		|| result->fOffset < 0
		|| result->fOffset > 10000
		|| result->fWidth < 0
		|| result->fWidth > 10000
		|| (int32)result->fAlignment < B_ALIGN_LEFT
		|| (int32)result->fAlignment > B_ALIGN_CENTER
		|| result->fAttrName.Length() > 500) {
		PRINT(("column data not valid\n"));
		delete result;
		return NULL;
	}
	return result;
}

void
BColumn::ArchiveToStream(BMallocIO *stream) const
{
	// write class identifier and version info
	uint32 key = AttrHashString("BColumn", B_OBJECT_TYPE);
	stream->Write(&key, sizeof(uint32));
	int32 version = kColumnStateArchiveVersion;
	stream->Write(&version, sizeof(int32));

//	PRINT(("ArchiveToStream column, key %x, version %d\n", key, version));

	StringToStream(&fTitle, stream);
	stream->Write(&fOffset, sizeof(float));
	stream->Write(&fWidth, sizeof(float));
	stream->Write(&fAlignment, sizeof(alignment));
	StringToStream(&fAttrName, stream);
	stream->Write(&fAttrHash, sizeof(uint32));
	stream->Write(&fAttrType, sizeof(uint32));
	stream->Write(&fStatField, sizeof(bool));
	stream->Write(&fEditable, sizeof(bool));
}

void
BColumn::ArchiveToMessage(BMessage &message) const
{
	message.AddInt32(kColumnVersionName, kColumnStateArchiveVersion);

	message.AddString(kColumnTitleName, fTitle);
	message.AddFloat(kColumnOffsetName, fOffset);
	message.AddFloat(kColumnWidthName, fWidth);
	message.AddInt32(kColumnAlignmentName, fAlignment);
	message.AddString(kColumnAttrName, fAttrName);
	message.AddInt32(kColumnAttrHashName, static_cast<int32>(fAttrHash));
	message.AddInt32(kColumnAttrTypeName, static_cast<int32>(fAttrType));
	message.AddBool(kColumnStatFieldName, fStatField);
	message.AddBool(kColumnEditableName, fEditable);
}

const char *kViewStateVersionName = "ViewState:version";
const char *kViewStateViewModeName = "ViewState:fViewMode";
const char *kViewStateLastIconModeName = "ViewState:fLastIconMode";
const char *kViewStateListOriginName = "ViewState:fListOrigin";
const char *kViewStateIconOriginName = "ViewState:fIconOrigin";
const char *kViewStatePrimarySortAttrName = "ViewState:fPrimarySortAttr";
const char *kViewStatePrimarySortTypeName = "ViewState:fPrimarySortType";
const char *kViewStateSecondarySortAttrName = "ViewState:fSecondarySortAttr";
const char *kViewStateSecondarySortTypeName = "ViewState:fSecondarySortType";
const char *kViewStateReverseSortName = "ViewState:fReverseSort";
const char *kViewStateIconSizeName = "ViewState:fIconSize";

BViewState::BViewState()
{
	fViewMode = kListMode;
	fLastIconMode = 0;
	fIconSize = 32;
	fListOrigin.Set(0, 0);
	fIconOrigin.Set(0, 0);
	fPrimarySortAttr = AttrHashString(kAttrStatName, B_STRING_TYPE);
	fPrimarySortType = B_STRING_TYPE;
	fSecondarySortAttr = 0;
	fSecondarySortType = 0;
	fReverseSort = false;
	fStateNeedsSaving = false;
}

BViewState::BViewState(BMallocIO *stream, bool endianSwap)
{
	stream->Read(&fViewMode, sizeof(uint32));
	stream->Read(&fLastIconMode, sizeof(uint32));
	stream->Read(&fListOrigin, sizeof(BPoint));
	stream->Read(&fIconOrigin, sizeof(BPoint));
	stream->Read(&fPrimarySortAttr, sizeof(uint32));
	stream->Read(&fPrimarySortType, sizeof(uint32));
	stream->Read(&fSecondarySortAttr, sizeof(uint32));
	stream->Read(&fSecondarySortType, sizeof(uint32));
	stream->Read(&fReverseSort, sizeof(bool));
	stream->Read(&fIconSize, sizeof(uint32));
	
	if (endianSwap) {
		PRINT(("endian swapping view state\n"));
		fViewMode = B_SWAP_INT32(fViewMode);
		fLastIconMode = B_SWAP_INT32(fLastIconMode);
		fIconSize = B_SWAP_INT32(fIconSize);
		swap_data(B_POINT_TYPE, &fListOrigin, sizeof(fListOrigin), B_SWAP_ALWAYS);
		swap_data(B_POINT_TYPE, &fIconOrigin, sizeof(fIconOrigin), B_SWAP_ALWAYS);
		fPrimarySortAttr = B_SWAP_INT32(fPrimarySortAttr);
		fSecondarySortAttr = B_SWAP_INT32(fSecondarySortAttr);
		fPrimarySortType = B_SWAP_INT32(fPrimarySortType);
		fSecondarySortType = B_SWAP_INT32(fSecondarySortType);
	}

	// assure a sane state
	if (fIconSize < 16)	
		fIconSize = 16; 
	if (fIconSize > 64)	
		fIconSize = 64; 

	fStateNeedsSaving = false;
}

BViewState::BViewState(const BMessage &message)
{
	message.FindInt32(kViewStateViewModeName, (int32 *)&fViewMode);
	message.FindInt32(kViewStateLastIconModeName, (int32 *)&fLastIconMode);
	message.FindPoint(kViewStateListOriginName, &fListOrigin);
	message.FindPoint(kViewStateIconOriginName, &fIconOrigin);
	message.FindInt32(kViewStatePrimarySortAttrName, (int32 *)&fPrimarySortAttr);
	message.FindInt32(kViewStatePrimarySortTypeName, (int32 *)&fPrimarySortType);
	message.FindInt32(kViewStateSecondarySortAttrName, (int32 *)&fSecondarySortAttr);
	message.FindInt32(kViewStateSecondarySortTypeName, (int32 *)&fSecondarySortType);
	message.FindBool(kViewStateReverseSortName, &fReverseSort);
	message.FindInt32(kViewStateIconSizeName, (int32 *)&fIconSize);
	
	// assure a sane state
	if (fIconSize < 16)	
		fIconSize = 16; 
	if (fIconSize > 64)	
		fIconSize = 64; 

	fStateNeedsSaving = false;
}

void
BViewState::ArchiveToStream(BMallocIO *stream) const
{
	// write class identifier and verison info
	uint32 key = AttrHashString("BViewState", B_OBJECT_TYPE);
	stream->Write(&key, sizeof(key));
	int32 version = kViewStateArchiveVersion;
	stream->Write(&version, sizeof(version));

	stream->Write(&fViewMode, sizeof(uint32));
	stream->Write(&fLastIconMode, sizeof(uint32));
	stream->Write(&fListOrigin, sizeof(BPoint));
	stream->Write(&fIconOrigin, sizeof(BPoint));
	stream->Write(&fPrimarySortAttr, sizeof(uint32));
	stream->Write(&fPrimarySortType, sizeof(uint32));
	stream->Write(&fSecondarySortAttr, sizeof(uint32));
	stream->Write(&fSecondarySortType, sizeof(uint32));
	stream->Write(&fReverseSort, sizeof(bool));
	stream->Write(&fIconSize, sizeof(uint32));
}


void
BViewState::ArchiveToMessage(BMessage &message) const
{
	message.AddInt32(kViewStateVersionName, kViewStateArchiveVersion);
	
	message.AddInt32(kViewStateViewModeName, static_cast<int32>(fViewMode));
	message.AddInt32(kViewStateLastIconModeName, static_cast<int32>(fLastIconMode));
	message.AddPoint(kViewStateListOriginName, fListOrigin);
	message.AddPoint(kViewStateIconOriginName, fIconOrigin);
	message.AddInt32(kViewStatePrimarySortAttrName, static_cast<int32>(fPrimarySortAttr));
	message.AddInt32(kViewStatePrimarySortTypeName, static_cast<int32>(fPrimarySortType));
	message.AddInt32(kViewStateSecondarySortAttrName, static_cast<int32>(fSecondarySortAttr));
	message.AddInt32(kViewStateSecondarySortTypeName, static_cast<int32>(fSecondarySortType));
	message.AddBool(kViewStateReverseSortName, fReverseSort);
	message.AddInt32(kViewStateIconSizeName, static_cast<int32>(fIconSize));
}



BViewState *
BViewState::InstantiateFromStream(BMallocIO *stream, bool endianSwap)
{
	// compare stream header in canonical form
	uint32 key = AttrHashString("BViewState", B_OBJECT_TYPE);
	int32 version = kViewStateArchiveVersion;
	
	if (endianSwap) {
		key = SwapUInt32(key);
		version = SwapInt32(version);
	}

	if (!ValidateStream(stream, key, version)) 
		return NULL;

	BViewState *result = new BViewState(stream, endianSwap);
	
	// do a sanity check here
	if ((result->fViewMode != kListMode
			&& result->fViewMode != kIconMode
			&& result->fViewMode != kMiniIconMode
			&& result->fViewMode != kScaleIconMode
			&& result->fViewMode != 0)
		|| (result->fLastIconMode != kListMode
			&& result->fLastIconMode != kIconMode
			&& result->fLastIconMode != kMiniIconMode
			&& result->fLastIconMode != kScaleIconMode
			&& result->fLastIconMode != 0)) {
		
		PRINT(("Bad data instantiating ViewState, view mode %x, lastIconMode %x\n",
			result->fViewMode, result->fLastIconMode));

		delete result;
		return NULL;
	}
#if DEBUG
	else if (endianSwap)
		PRINT(("Instantiated foreign view state ok\n"));
#endif
	return result;
}

BViewState *
BViewState::InstantiateFromMessage(const BMessage &message)
{
	int32 version = kViewStateArchiveVersion;
	
	int32 messageVersion;

	if (message.FindInt32(kViewStateVersionName, &messageVersion) != B_OK)
		return NULL;

	if (version != messageVersion) 
		return NULL;

	BViewState *result = new BViewState(message);
	
	// do a sanity check here
	if ((result->fViewMode != kListMode
			&& result->fViewMode != kIconMode
			&& result->fViewMode != kMiniIconMode
			&& result->fViewMode != kScaleIconMode
			&& result->fViewMode != 0)
		|| (result->fLastIconMode != kListMode
			&& result->fLastIconMode != kIconMode
			&& result->fLastIconMode != kMiniIconMode
			&& result->fLastIconMode != kScaleIconMode
			&& result->fLastIconMode != 0)) {
	
		PRINT(("Bad data instantiating ViewState, view mode %x, lastIconMode %x\n",
			result->fViewMode, result->fLastIconMode));

		delete result;
		return NULL;
	}
	return result;
}
