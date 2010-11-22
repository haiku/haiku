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


class Importer {
 public:
								Importer();
	virtual						~Importer();

			status_t			Init(Icon* icon);

			int32				StyleIndexFor(int32 savedIndex) const;
			int32				PathIndexFor(int32 savedIndex) const;

 private:
			int32				fStyleIndexOffset;
			int32				fPathIndexOffset;
};

#endif // IMPORTER_H
