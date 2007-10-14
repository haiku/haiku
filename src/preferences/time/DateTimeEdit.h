/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */
#ifndef DATETIME_H
#define DATETIME_H


#include "DateTime.h"
#include "SectionEdit.h"


class TTimeEdit : public TSectionEdit {
	public:
						TTimeEdit(BRect frame, const char *name, uint32 sections);
		virtual			~TTimeEdit();

		virtual void	InitView();
		virtual void	DrawSection(uint32 index, bool isfocus);
		virtual void	DrawSeperator(uint32 index);

		virtual void	SetSections(BRect area);
		virtual void	SectionFocus(uint32 index);
		virtual float	SeperatorWidth() const;

		virtual void	DoUpPress();
		virtual void	DoDownPress();

		virtual void	BuildDispatch(BMessage *message);

		void			SetTime(int32 hour, int32 minute, int32 second);

	private:
		void			_CheckRange();
		int32			_SectionValue(int32 index) const;

	private:
		BTime			fTime;
};


class TDateEdit : public TSectionEdit {
	public:
						TDateEdit(BRect frame, const char *name, uint32 sections);
		virtual			~TDateEdit();
		
		virtual void	InitView();
		virtual void	DrawSection(uint32 index, bool isfocus);
		virtual void	DrawSeperator(uint32 index);
		
		virtual void	SetSections(BRect area);
		virtual void	SectionFocus(uint32 index);
		virtual float	SeperatorWidth() const;
		
		virtual void	DoUpPress();
		virtual void	DoDownPress();
		
		virtual void	BuildDispatch(BMessage *message);
		
		void			SetDate(int32 year, int32 month, int32 day);

	private:
		void			_CheckRange();
		int32			_SectionValue(int32 index) const;

	private:
		BDate			fDate;
};

#endif // DATETIME_H

