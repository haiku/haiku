/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef _SECTION_EDIT_H
#define _SECTION_EDIT_H


#include <Control.h>


class BBitmap;
class BList;


class TSectionEdit : public BControl {
public:
								TSectionEdit(const char* name,
									uint32 sections);
	virtual						~TSectionEdit();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			BSize				MaxSize();
			BSize				MinSize();
			BSize				PreferredSize();

			uint32				CountSections() const;
			int32				FocusIndex() const;
			BRect				SectionArea() const;

protected:
	virtual	void				DrawBorder(const BRect& updateRect);
	virtual	void				DrawSection(uint32 index, BRect bounds,
									bool isFocus) {}
	virtual	void				DrawSeparator(uint32 index, BRect bounds) {}

			BRect				FrameForSection(uint32 index);
			BRect				FrameForSeparator(uint32 index);

	virtual	void				SectionFocus(uint32 index) {}
	virtual	void				SectionChange(uint32 index, uint32 value) {}
	virtual	void				SetSections(BRect area) {}
	
	virtual	float				SeparatorWidth() = 0;
	virtual	float				MinSectionWidth() = 0;
	virtual float				PreferredHeight() = 0;
	
	virtual	void				DoUpPress() {}
	virtual	void				DoDownPress() {}

	virtual	void				DispatchMessage();
	virtual	void				BuildDispatch(BMessage* message) = 0;
			
			BRect				fUpRect;
			BRect				fDownRect;

			int32				fFocus;
			uint32				fSectionCount;
			uint32				fHoldValue;
};


#endif	// _SECTION_EDIT_H
