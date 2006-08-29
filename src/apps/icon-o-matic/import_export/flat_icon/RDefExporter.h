/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef RDEF_EXPORTER_H
#define RDEF_EXPORTER_H

#include "FlatIconExporter.h"

class RDefExporter : public FlatIconExporter {
 public:
								RDefExporter();
	virtual						~RDefExporter();

	// FlatIconExporter interface
	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream);

	virtual	const char*			MIMEType();

 private:
			status_t			_Export(const uint8* source,
										size_t sourceSize,
										BPositionIO* stream) const;
};

#endif // RDEF_EXPORTER_H
