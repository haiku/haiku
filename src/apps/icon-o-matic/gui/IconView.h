/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ICON_VIEW_H
#define ICON_VIEW_H


#include "Icon.h"

#include <View.h>


class BBitmap;

_BEGIN_ICON_NAMESPACE
	class IconRenderer;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE

class IconView : public BView,
				 public IconListener {
 public:
								IconView(BRect frame, const char* name);
	virtual						~IconView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	// IconListener interface
	virtual	void				AreaInvalidated(const BRect& area);

	// IconView
			void				SetIcon(Icon* icon);
			void				SetIconBGColor(const rgb_color& color);

 private:
			BBitmap*			fBitmap;
			Icon*				fIcon;
			IconRenderer*		fRenderer;
			BRect				fDirtyIconArea;

			double				fScale;
};

#endif // ICON_VIEW_H
