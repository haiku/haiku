/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */
#ifndef TTZDISPLAY_H
#define TTZDISPLAY_H


#include <View.h>
#include <String.h>


class TTZDisplay : public BView {
	public:
						TTZDisplay(BRect frame, const char *name, const char *label);
		virtual			~TTZDisplay();

		virtual void 	AttachedToWindow();
		virtual void 	ResizeToPreferred();
		virtual void 	Draw(BRect updateRect);

		const char*		Label() const;
		void			SetLabel(const char *label);

		const char*		Text() const;
		void			SetText(const char *text);

		const char*		Time() const;
		void 			SetTime(int32 hour, int32 minute);

	private:
		BString 		fLabel;
		BString 		fText;
		BString 		fTime;

};

#endif	// TTZDISPLAY_H

