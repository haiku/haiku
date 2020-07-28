/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ICON_TAR_PTR_H
#define ICON_TAR_PTR_H

#include <stdlib.h>

#include <Referenceable.h>
#include <String.h>

#include "HaikuDepotConstants.h"


/*!	The tar icon repository is able to find the icons for each package by
	scanning the tar file for suitable icon files and recording the offsets into
	the tar file when it does find a suitable icon file.  This class is used
	to return the offsets for a given package.
*/

class IconTarPtr : public BReferenceable {
public:
								IconTarPtr(const BString& name);
	virtual						~IconTarPtr();

	const	BString&			Name() const;
			bool				HasOffset(BitmapSize size) const;
			off_t				Offset(BitmapSize size) const;
			void				SetOffset(BitmapSize size, off_t value);

private:
			BString				fName;
			uint8				fOffsetsMask;
			off_t				fOffsets[5];
};

#endif // ICON_TAR_PTR_H