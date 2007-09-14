/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		probably Mike Berg <mike@agamemnon.homelinux.net>
 *		and/or Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 *
 */
#ifndef DATETIME_H
#define DATETIME_H


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

		void			CheckRange();

		virtual void	DoUpPress();
		virtual void	DoDownPress();

		virtual void	BuildDispatch(BMessage *message);

		void			SetTime(uint32 hour, uint32 minute, uint32 second);
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
		
		void			CheckRange();
		
		virtual void	DoUpPress();
		virtual void	DoDownPress();
		
		virtual void	BuildDispatch(BMessage *message);
		
		void			SetDate(uint32 year, uint32 month, uint32 day);
};

#endif // DATETIME_H

