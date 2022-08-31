/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICON_MENU_ITEM_H_
#define _ICON_MENU_ITEM_H_

#include <MenuItem.h>


class BBitmap;


class IconMenuItem : public BMenuItem {
	public:
		IconMenuItem(BBitmap*, const char* title,
			BMessage*, bool drawText = true, bool purge = false);
		IconMenuItem(BBitmap*, BMenu*, bool drawText = true,
			bool purge = false);
		virtual ~IconMenuItem();

		void Reset(BBitmap*, bool purge = false);

		virtual	void DrawContent();
		virtual	void Highlight(bool isHighlighted);
		virtual	void GetContentSize(float* width, float* height);

	protected:
		void DrawIcon();

	private:
		BBitmap*	fIcon;
		bool		fDrawText;
		bool		fPurge;
};

#endif // _ICON_MENU_ITEM_H_
