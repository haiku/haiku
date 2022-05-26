/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef _DATE_TIME_EDIT_H
#define _DATE_TIME_EDIT_H

#include <Control.h>
#include <DateFormat.h>
#include <DateTime.h>
#include <Locale.h>
#include <String.h>
#include <TimeFormat.h>

class BBitmap;
class BList;


namespace BPrivate {


class SectionEdit : public BControl {
public:
								SectionEdit(const char* name,
											uint32 sections, BMessage* message);
	virtual						~SectionEdit();

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

	virtual	status_t			Invoke(BMessage* message = NULL);

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

	virtual	void				PopulateMessage(BMessage* message) = 0;

			BRect				fUpRect;
			BRect				fDownRect;

			int32				fFocus;
			uint32				fSectionCount;
			uint32				fHoldValue;
};


class TimeEdit : public SectionEdit {
public:
								TimeEdit(const char* name,	uint32 sections,
										BMessage* message);
	virtual						~TimeEdit();

	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				InitView();
	virtual	void				DrawSection(uint32 index, BRect bounds,
									bool isfocus);
	virtual	void				DrawSeparator(uint32 index, BRect bounds);

	virtual	void				SectionFocus(uint32 index);
	virtual float				MinSectionWidth();
	virtual float				SeparatorWidth();

	virtual float				PreferredHeight();
	virtual	void				DoUpPress();
	virtual	void				DoDownPress();

	virtual	void				PopulateMessage(BMessage* message);

			void				SetTime(int32 hour, int32 minute, int32 second);
			BTime				GetTime();

private:
			void				_UpdateFields();
			void				_CheckRange();
			bool				_IsValidDoubleDigit(int32 value);
			int32				_SectionValue(int32 index) const;

			BDateTime			fTime;
			BTimeFormat			fTimeFormat;
			bigtime_t			fLastKeyDownTime;
			int32				fLastKeyDownInt;

			BString				fText;

			// TODO: morph the following into a proper class
			BDateElement*		fFields;
			int					fFieldCount;
			int*				fFieldPositions;
			int					fFieldPosCount;
};


class DateEdit : public SectionEdit {
public:
								DateEdit(const char* name, uint32 sections,
										BMessage* message);
	virtual						~DateEdit();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				InitView();
	virtual void				DrawSection(uint32 index, BRect bounds, 
									bool isfocus);
	virtual void				DrawSeparator(uint32 index, BRect bounds);

	virtual void				SectionFocus(uint32 index);
	virtual float				MinSectionWidth();
	virtual float				SeparatorWidth();

	virtual float				PreferredHeight();
	virtual void				DoUpPress();
	virtual void				DoDownPress();

	virtual void				PopulateMessage(BMessage* message);


			void				SetDate(int32 year, int32 month, int32 day);
			BDate				GetDate();

private:
			void				_UpdateFields();
			void				_CheckRange();
			bool				_IsValidDoubleDigit(int32 value);
			int32				_SectionValue(int32 index) const;

			BDate				fDate;
			BDateFormat			fDateFormat;
			bigtime_t			fLastKeyDownTime;
			int32				fLastKeyDownInt;

			BString				fText;

			// TODO: morph the following into a proper class
			BDateElement*		fFields;
			int					fFieldCount;
			int*				fFieldPositions;
			int					fFieldPosCount;
};


}	// namespace BPrivate


#endif	// _DATE_TIME_EDIT_H
