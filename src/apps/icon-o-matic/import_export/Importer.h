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


namespace BPrivate {
namespace Icon {

class Icon;

}	// namespace Icon
}	// namespace BPrivate

using namespace BPrivate::Icon;

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
