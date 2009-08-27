/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ALERT_VIEW_H
#define ALERT_VIEW_H


#include <String.h>
#include <View.h>

class BBitmap;
class BStringView;


class AlertView : public BView {
	public:
		AlertView(BRect frame, const char* name);

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void Pulse();
		virtual void KeyDown(const char* bytes, int32 numBytes);

	private:
		void UpdateCountdownView();
		BBitmap* InitIcon();

		BStringView*	fCountdownView;
		BBitmap*		fBitmap;
		int32			fSeconds;
};

#endif	/* ALERT_VIEW_H */
