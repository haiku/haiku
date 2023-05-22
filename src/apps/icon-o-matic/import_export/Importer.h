/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef IMPORTER_H
#define IMPORTER_H


#include <SupportDefs.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Icon;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE

/*! Base class providing facilities for all importers.
	Child classes should have an Import function taking parameters relavent to
	the thing they import.

	\note Some importers are in the icon library.
*/
class Importer {
 public:
								Importer();
	virtual						~Importer();

			status_t			Init(Icon* icon);

	/*!
		Sometimes the thing being imported is being imported into a file that
		already has items. This means that style index zero and path index zero
		are likely already taken. These functions take in a zero-based index
		and offset them to a previously unused style/path index.
	*/

	//! @{
			int32				StyleIndexFor(int32 savedIndex) const;
			int32				PathIndexFor(int32 savedIndex) const;
	//! @}
 private:
			int32				fStyleIndexOffset;
			int32				fPathIndexOffset;
};

#endif // IMPORTER_H
