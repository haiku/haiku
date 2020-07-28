/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_VIEW_H
#define BITMAP_VIEW_H


#include "SharedBitmap.h"

#include <View.h>


class BitmapView : public BView {
public:
								BitmapView(const char* name);

	virtual						~BitmapView();

	virtual	void				Draw(BRect updateRect);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

			void				SetBitmap(SharedBitmap* bitmap,
									BitmapSize bitmapSize = BITMAP_SIZE_ANY);
			void				UnsetBitmap();
			void				SetScaleBitmap(bool scaleBitmap);

private:
			BitmapRef			fReference;
			BitmapSize			fBitmapSize;
			const BBitmap*		fBitmap;
			bool				fScaleBitmap;
};


#endif // BITMAP_VIEW_H
