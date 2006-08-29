/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef SVG_IMPORTER_H
#define SVG_IMPORTER_H

#include "Importer.h"

struct entry_ref;

class SVGImporter : public Importer {
 public:
								SVGImporter();
	virtual						~SVGImporter();

			status_t			Import(Icon* icon,
									   const entry_ref* ref);
};

#endif // SVG_IMPORTER_H
