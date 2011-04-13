// Author: Michael Wilber
// Copyright (C) Haiku, uses the MIT license
#ifndef ICONVIEW_H
#define ICONVIEW_H


#include <Bitmap.h>
#include <Mime.h>
#include <Path.h>
#include <View.h>


class IconView : public BView {
public:
							IconView(icon_size iconSize = B_LARGE_ICON);

							~IconView();

			status_t		InitCheck() const;
	virtual	void			Draw(BRect area);

			void			DrawIcon(bool draw);
			status_t		SetIcon(const BPath& path,
								icon_size iconSize = B_LARGE_ICON);

private:
			void			_SetSize();

			icon_size		fIconSize;
			BBitmap*		fIconBitmap;
			bool			fDrawIcon;
};

#endif // #ifndef ICONVIEW_H
