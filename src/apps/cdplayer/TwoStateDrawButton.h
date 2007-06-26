/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef _TWOSTATE_DRAWBUTTON_H
#define _TWOSTATE_DRAWBUTTON_H


#include <Bitmap.h>
#include <Button.h>
#include <View.h>
#include <Window.h>


class TwoStateDrawButton : public BButton {
	public:
		TwoStateDrawButton(BRect frame, const char *name, BBitmap *upone, 
			BBitmap *downone, BBitmap *uptwo, BBitmap *downtwo, 
			BMessage *msg, const int32 &resize, const int32 &flags);

		~TwoStateDrawButton();

		void Draw(BRect update);

		void SetBitmaps(BBitmap *upone, BBitmap *downone, BBitmap *uptwo, 
			BBitmap *downtwo);

		void SetBitmaps(const int32 state, BBitmap *up, BBitmap *down);
		void ResizeToPreferred();
		void SetDisabled(BBitmap *disabledone, BBitmap *disabledtwo);
		void MouseUp(BPoint pt);

		int32 GetState() { return fButtonState ? 1 : 0; };
		void SetState(int32 value);

	private:
		BBitmap	*fUpOne,
				*fDownOne,
				*fUpTwo,
				*fDownTwo,
				*fDisabledOne,
				*fDisabledTwo;

		bool fButtonState;
			// true if in state two
};

#endif	// _TWOSTATE_DRAWBUTTON_H
