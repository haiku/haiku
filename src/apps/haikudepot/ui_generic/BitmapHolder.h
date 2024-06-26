/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BITMAP_HOLDER_H
#define BITMAP_HOLDER_H


#include <Bitmap.h>
#include <Referenceable.h>

#include "HaikuDepotConstants.h"


/*!	This class only serves as a holder for a bitmap that is referencable.
	It is used for carrying icon images as well as icons for packages.
*/

class BitmapHolder : public BReferenceable
{
public:
								BitmapHolder(const BBitmap* bitmap);
	virtual						~BitmapHolder();

	const	BBitmap*			Bitmap() const;

private:
	const	BBitmap*			fBitmap;
};


typedef BReference<BitmapHolder> BitmapHolderRef;


#endif // BITMAP_HOLDER_H
