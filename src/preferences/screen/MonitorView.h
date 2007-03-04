/*
 * Copyright 2001-2005, Haiku.
 * Copyright 2002, Thomas Kurschel.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Thomas Kurschel
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef MONITOR_VIEW_H
#define MONITOR_VIEW_H


#include <View.h>


class MonitorView : public BView {
	public:
		MonitorView(BRect frame, char *name, int32 screenWidth, int32 screenHeight);
		~MonitorView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);
		virtual void MessageReceived(BMessage *message);
		virtual void MouseDown(BPoint point);

		void SetResolution(int32 width, int32 height);

	private:
		BRect MonitorBounds();

		rgb_color	fDesktopColor;
		int32		fWidth;
		int32		fHeight;
};

#endif	/* MONITOR_VIEW_H */
