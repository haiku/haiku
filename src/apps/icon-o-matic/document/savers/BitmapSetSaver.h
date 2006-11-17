/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef BITMAP_SET_SAVER_H
#define BITMAP_SET_SAVER_H

#include "FileSaver.h"

class BitmapSetSaver : public FileSaver {
 public:
								BitmapSetSaver(const entry_ref& ref);
	virtual						~BitmapSetSaver();

	virtual	status_t			Save(Document* document);
};

#endif // BITMAP_SET_SAVER_H
