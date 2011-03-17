// Author: Michael Wilber
// Copyright (C) Haiku, uses the MIT license
#ifndef ICONVIEW_H
#define ICONVIEW_H


#include <Bitmap.h>
#include <Path.h>
#include <View.h>


class IconView : public BView {
public:
							IconView(const BRect& frame, const char* name,
								uint32 resize, uint32 flags);
							~IconView();
	virtual	void			Draw(BRect area);
	
			bool			DrawIcon(bool draw);
			bool			SetIcon(const BPath& path);

private:
			BBitmap*		fIconBitmap;
			bool			fDrawIcon;
};

#endif // #ifndef ICONVIEW_H
