/*
 * Copyright 2001-2009, Haiku.
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
							MonitorView(BRect frame, const char* name,
								int32 screenWidth, int32 screenHeight);
	virtual					~MonitorView();

	virtual	void			AttachedToWindow();
	virtual	void			Draw(BRect updateRect);
	virtual	void			MessageReceived(BMessage *message);
	virtual	void			MouseDown(BPoint point);

			void			SetResolution(int32 width, int32 height);
			void			SetMaxResolution(int32 width, int32 height);

private:
			BRect			_MonitorBounds();
			void			_UpdateDPI();

			rgb_color		fBackgroundColor;
			rgb_color		fDesktopColor;
			int32			fMaxWidth;
			int32			fMaxHeight;
			int32			fWidth;
			int32			fHeight;
			int32			fDPI;
};

#endif	/* MONITOR_VIEW_H */
