/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef _TZ_DISPLAY_H
#define _TZ_DISPLAY_H


#include <String.h>
#include <View.h>
#include <stdio.h>


class TTZDisplay : public BView {
public:
								TTZDisplay(const char* name,
									const char* label);
	virtual						~TTZDisplay();

	virtual	void				AttachedToWindow();
	virtual	void				ResizeToPreferred();
	virtual	void				Draw(BRect updateRect);
	
	virtual BSize				MaxSize();
	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();

			const char*			Label() const;
			void				SetLabel(const char* label);

			const char*			Text() const;
			void				SetText(const char* text);

			const char*			Time() const;
			void				SetTime(const char* time);

private:
			BSize				_CalcPrefSize();

			BString 			fLabel;
			BString 			fText;
			BString 			fTime;
};


#endif	// _TZ_DISPLAY_H

