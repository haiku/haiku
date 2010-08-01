/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */
#ifndef _TZ_DISPLAY_H
#define _TZ_DISPLAY_H


#include <View.h>
#include <String.h>


class TTZDisplay : public BView {
public:
								TTZDisplay(BRect frame, const char* name,
									const char* label);
	virtual						~TTZDisplay();

	virtual	void				AttachedToWindow();
	virtual	void				ResizeToPreferred();
	virtual	void				Draw(BRect updateRect);

			const char*			Label() const;
			void				SetLabel(const char* label);

			const char*			Text() const;
			void				SetText(const char* text);

			const char*			Time() const;
			void				SetTime(const char* time);

private:
			BString 			fLabel;
			BString 			fText;
			BString 			fTime;
};


#endif	// _TZ_DISPLAY_H

