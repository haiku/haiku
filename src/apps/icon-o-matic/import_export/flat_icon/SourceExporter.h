/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SOURCE_EXPORTER_H
#define SOURCE_EXPORTER_H

#include "FlatIconExporter.h"

class SourceExporter : public FlatIconExporter {
 public:
								SourceExporter();
	virtual						~SourceExporter();

	// FlatIconExporter interface
	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream);

	virtual	const char*			MIMEType();

 private:
			status_t			_Export(const uint8* source,
										size_t sourceSize,
										BPositionIO* stream) const;
};

#endif // SOURCE_EXPORTER_H
