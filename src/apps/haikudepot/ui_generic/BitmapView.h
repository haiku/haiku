/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_VIEW_H
#define BITMAP_VIEW_H


#include <View.h>


class BitmapView : public BView {
public:
								BitmapView(const char* name);

	virtual						~BitmapView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

			void				SetBitmap(const BBitmap* bitmap);
			void				SetScaleBitmap(bool scaleBitmap);

protected:
	virtual void				DrawBackground(BRect& bounds,
									BRect updateRect);

private:
			const BBitmap*		fBitmap;
			bool				fScaleBitmap;
};


#endif // BITMAP_VIEW_H
