// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the Haiku license.
//
//
//  File:			MethodMenuItem.h
//  Authors:		Jérôme Duval,
//
//  Description:	Input Server
//  Created:		October 19, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef METHOD_MENUITEM_H_
#define METHOD_MENUITEM_H_

#include <Bitmap.h>
#include <MenuItem.h>

#define MENUITEM_ICON_SIZE 16

class MethodMenuItem : public BMenuItem {
	public:
		MethodMenuItem(void *cookie, const char *label, const uchar *icon, BMenu *subMenu, BMessenger &messenger);
		MethodMenuItem(void *cookie, const char *label, const uchar *icon);

		virtual ~MethodMenuItem();

		virtual void DrawContent();
		virtual void GetContentSize(float *width, float *height);

		void SetName(const char *name);
		const char *Name() { return Label(); };

		void SetIcon(const uchar *icon);
		const uchar *Icon() { return(uchar *)fIcon.Bits(); };

		void *Cookie() { return fCookie; };
	private:
		BBitmap fIcon;
		void *fCookie;
		BMessenger fMessenger;
};

#endif
