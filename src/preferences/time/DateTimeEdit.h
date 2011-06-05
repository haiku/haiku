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


#include <DateTime.h>
#include <Locale.h>
#include <String.h>

#include "SectionEdit.h"


class TTimeEdit : public TSectionEdit {
public:
								TTimeEdit(const char* name,	uint32 sections);
	virtual						~TTimeEdit();

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

	virtual	void				BuildDispatch(BMessage* message);

			void				SetTime(int32 hour, int32 minute, int32 second);

private:
			void				_UpdateFields();
			void				_CheckRange();
			bool				_IsValidDoubleDigit(int32 value);
			int32				_SectionValue(int32 index) const;

			BDateTime			fTime;
			bigtime_t			fLastKeyDownTime;
			int32				fLastKeyDownInt;

			BString				fText;

			// TODO: morph the following into a proper class
			BDateElement*		fFields;
			int					fFieldCount;
			int*				fFieldPositions;
			int					fFieldPosCount;
};


class TDateEdit : public TSectionEdit {
public:
								TDateEdit(const char* name, uint32 sections);
	virtual						~TDateEdit();
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

	virtual void				BuildDispatch(BMessage* message);
	

			void				SetDate(int32 year, int32 month, int32 day);

private:
			void				_UpdateFields();
			void				_CheckRange();
			bool				_IsValidDoubleDigit(int32 value);
			int32				_SectionValue(int32 index) const;
			
			BDate				fDate;
			bigtime_t			fLastKeyDownTime;
			int32				fLastKeyDownInt;

			BString				fText;

			// TODO: morph the following into a proper class
			BDateElement*		fFields;
			int					fFieldCount;
			int*				fFieldPositions;
			int					fFieldPosCount;
};


#endif	// _DATE_TIME_EDIT_H

