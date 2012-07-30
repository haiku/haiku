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
#ifndef _VIEW_STATE_H
#define _VIEW_STATE_H


#include <DataIO.h>
#include <String.h>


namespace BPrivate {

const int32 kColumnStateArchiveVersion = 22;
	// bump version when layout or size changes

class BColumn {
	public:
		BColumn(const char* title, float offset, float width,
			alignment align, const char* attributeName, uint32 attrType,
			const char* displayAs, bool statField, bool editable);
		BColumn(const char* title, float offset, float width,
			alignment align, const char* attributeName, uint32 attrType,
			bool statField, bool editable);
		~BColumn();

		BColumn(BMallocIO* stream, int32 version, bool endianSwap = false);
		BColumn(const BMessage &, int32 index = 0);
		static BColumn* InstantiateFromStream(BMallocIO* stream,
			bool endianSwap = false);
		static BColumn* InstantiateFromMessage(const BMessage &archive,
			int32 index = 0);
		void ArchiveToStream(BMallocIO* stream) const;
		void ArchiveToMessage(BMessage &) const;

		const char* Title() const;
		float 		Offset() const;
		float 		Width() const;
		alignment 	Alignment() const;
		const char* AttrName() const;
		uint32 		AttrType() const;
		const char* DisplayAs() const;
		uint32 		AttrHash() const;
		bool 		StatField() const;
		bool 		Editable() const;

		void 		SetOffset(float);
		void 		SetWidth(float);

	private:
		void _Init(const char* title, float offset, float width,
			alignment align, const char* attributeName, uint32 attrType,
			const char* displayAs, bool statField, bool editable);
		static BColumn* _Sanitize(BColumn* column);

		BString 	fTitle;
		float 		fOffset;
		float 		fWidth;
		alignment 	fAlignment;
		BString 	fAttrName;
		BString 	fDisplayAs;
		uint32 		fAttrHash;
		uint32 		fAttrType;
		bool 		fStatField;
		bool 		fEditable;
};


const int32 kViewStateArchiveVersion = 11;
	// bump version when layout or size changes

class BViewState {
	public:
		BViewState();

		BViewState(BMallocIO* stream, bool endianSwap = false);
		BViewState(const BMessage &message);
		static BViewState* InstantiateFromStream(BMallocIO* stream,
			bool endianSwap = false);
		static BViewState* InstantiateFromMessage(const BMessage &message);
		void ArchiveToStream(BMallocIO* stream) const;
		void ArchiveToMessage(BMessage &message) const;

		uint32 ViewMode() const;
		uint32 LastIconMode() const;
		uint32 IconSize() const;
		uint32 LastIconSize() const;
		BPoint ListOrigin() const;
		BPoint IconOrigin() const;
		uint32 PrimarySort() const;
		uint32 SecondarySort() const;
		uint32 PrimarySortType() const;
		uint32 SecondarySortType() const;
		bool ReverseSort() const;

		void SetViewMode(uint32);
		void SetLastIconMode(uint32);
		void SetIconSize(uint32);
		void SetLastIconSize(uint32);
		void SetListOrigin(BPoint);
		void SetIconOrigin(BPoint);
		void SetPrimarySort(uint32);
		void SetSecondarySort(uint32);
		void SetPrimarySortType(uint32);
		void SetSecondarySortType(uint32);
		void SetReverseSort(bool);

		bool StateNeedsSaving();

	private:
		static BViewState* _Sanitize(BViewState* state, bool fixOnly = false);

		uint32	fViewMode;
		uint32	fLastIconMode;
		uint32	fIconSize;
		uint32	fLastIconSize;
		BPoint	fListOrigin;
		BPoint	fIconOrigin;
		uint32	fPrimarySortAttr;
		uint32	fSecondarySortAttr;
		uint32	fPrimarySortType;
		uint32	fSecondarySortType;
		bool	fReverseSort;
		
		void _Init();
		void _StorePreviousState();
		
		uint32	fPreviousViewMode;
		uint32	fPreviousLastIconMode;
		uint32	fPreviousIconSize;
		uint32	fPreviousLastIconSize;
		BPoint	fPreviousListOrigin;
		BPoint	fPreviousIconOrigin;
		uint32	fPreviousPrimarySortAttr;
		uint32	fPreviousSecondarySortAttr;
		uint32	fPreviousPrimarySortType;
		uint32	fPreviousSecondarySortType;
		bool	fPreviousReverseSort;
};


inline const char*
BColumn::Title() const
{
	return fTitle.String();
}


inline float
BColumn::Offset() const
{
	return fOffset;
}


inline float
BColumn::Width() const
{
	return fWidth;
}


inline alignment
BColumn::Alignment() const
{
	return fAlignment;
}


inline const char*
BColumn::AttrName() const
{
	return fAttrName.String();
}


inline uint32
BColumn::AttrHash() const
{
	return fAttrHash;
}


inline uint32
BColumn::AttrType() const
{
	return fAttrType;
}


inline const char*
BColumn::DisplayAs() const
{
	return fDisplayAs.String();
}


inline bool
BColumn::StatField() const
{
	return fStatField;
}


inline bool
BColumn::Editable() const
{
	return fEditable;
}


inline void
BColumn::SetWidth(float w)
{
	fWidth = w;
}


inline void
BColumn::SetOffset(float o)
{
	fOffset = o;
}


inline uint32
BViewState::ViewMode() const
{
	return fViewMode;
}


inline uint32
BViewState::LastIconMode() const
{
	return fLastIconMode;
}


inline uint32
BViewState::IconSize() const
{
	return fIconSize;
}


inline uint32
BViewState::LastIconSize() const
{
	return fLastIconSize;
}


inline BPoint
BViewState::ListOrigin() const
{
	return fListOrigin;
}


inline BPoint
BViewState::IconOrigin() const
{
	return fIconOrigin;
}


inline uint32
BViewState::PrimarySort() const
{
	return fPrimarySortAttr;
}


inline uint32
BViewState::SecondarySort() const
{
	return fSecondarySortAttr;
}


inline uint32
BViewState::PrimarySortType() const
{
	return fPrimarySortType;
}

inline uint32
BViewState::SecondarySortType() const
{
	return fSecondarySortType;
}

inline bool
BViewState::ReverseSort() const
{
	return fReverseSort;
}


inline void
BViewState::SetViewMode(uint32 mode)
{
	fViewMode = mode;
}


inline void
BViewState::SetLastIconMode(uint32 mode)
{
	fLastIconMode = mode;
}


inline void
BViewState::SetIconSize(uint32 size)
{
	fIconSize = size;
}


inline void
BViewState::SetLastIconSize(uint32 size)
{
	fLastIconSize = size;
}


inline void
BViewState::SetListOrigin(BPoint newOrigin)
{
	fListOrigin = newOrigin;
}

inline void
BViewState::SetIconOrigin(BPoint newOrigin)
{
	fIconOrigin = newOrigin;
}

inline void
BViewState::SetPrimarySort(uint32 attr)
{
	fPrimarySortAttr = attr;
}


inline void
BViewState::SetSecondarySort(uint32 attr)
{
	fSecondarySortAttr = attr;
}


inline void
BViewState::SetPrimarySortType(uint32 type)
{
	fPrimarySortType = type;
}


inline void
BViewState::SetSecondarySortType(uint32 type)
{
	fSecondarySortType = type;
}


inline void
BViewState::SetReverseSort(bool on)
{
	fReverseSort = on;
}


inline bool
BViewState::StateNeedsSaving()
{
	return (fPreviousViewMode != fViewMode)
		|| (fPreviousLastIconMode != fLastIconMode)
		|| (fPreviousIconSize != fIconSize)
		|| (fPreviousLastIconSize != fLastIconSize)
		|| (fPreviousListOrigin != fListOrigin)
		|| (fPreviousIconOrigin != fIconOrigin)
		|| (fPreviousPrimarySortAttr != fPrimarySortAttr)
		|| (fPreviousSecondarySortAttr != fSecondarySortAttr)
		|| (fPreviousPrimarySortType != fPrimarySortType)
		|| (fPreviousSecondarySortType != fSecondarySortType)
		|| (fPreviousReverseSort != fReverseSort);
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// _VIEW_STATE_H
