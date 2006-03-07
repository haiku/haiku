/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICON_VIEW_H
#define ICON_VIEW_H


#include <View.h>


class IconView : public BView {
	public:
		IconView(BRect rect, const char* name);
		virtual ~IconView();

		virtual void AttachedToWindow();
		virtual void Draw(BRect updateRect);

		virtual void MouseDown(BPoint where);

		void SetTo(entry_ref* file);
		void ShowIconHeap(bool show);

	private:
		BBitmap*	fIcon;
		BBitmap*	fHeapIcon;
};

#endif	// ICON_VIEW_H
