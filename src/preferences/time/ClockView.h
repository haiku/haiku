/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione <jscipione@gmail.com>
 */
#ifndef _CLOCK_VIEW_H
#define _CLOCK_VIEW_H


#include <View.h>


class BCheckBox;
class BRadioButton;


class ClockView : public BView {
public:
								ClockView(const char* name);
	virtual 					~ClockView();

	virtual	void			 	AttachedToWindow();
	virtual	void 				MessageReceived(BMessage* message);

			bool				CheckCanRevert();

private:
			void				_Revert();

			BCheckBox*			fShowClock;
			BCheckBox*			fShowSeconds;
			BCheckBox*			fShowDayOfWeek;
			BCheckBox*			fShowTimeZone;

			int32				fCachedShowClock;
			int32				fCachedShowSeconds;
			int32				fCachedShowDayOfWeek;
			int32				fCachedShowTimeZone;
};


#endif	// _CLOCK_VIEW_H
