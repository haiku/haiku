/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SourceExporter.h"

#include <stdio.h>
#include <string.h>

#include <DataIO.h>

// constructor
SourceExporter::SourceExporter()
	: FlatIconExporter()
{
}

// destructor
SourceExporter::~SourceExporter()
{
}

// Export
status_t
SourceExporter::Export(const Icon* icon, BPositionIO* stream)
{
	BMallocIO buffer;
	status_t ret = FlatIconExporter::Export(icon, &buffer);
	if (ret < B_OK)
		return ret;

	return _Export((const uint8*)buffer.Buffer(), buffer.BufferLength(),
		stream);
}

// MIMEType
const char*
SourceExporter::MIMEType()
{
	return "text/x-source-code";
}

// #pragma mark -

// _Export
status_t
SourceExporter::_Export(const uint8* source, size_t sourceSize,
	BPositionIO* stream) const
{
	char buffer[2048];
	// write header
	sprintf(buffer, "\nconst unsigned char kIconName[] = {\n");
	size_t size = strlen(buffer);

	ssize_t written = stream->Write(buffer, size);
	if (written < 0)
		return (status_t)written;
	if (written < (ssize_t)size)
		return B_ERROR;

	status_t ret = B_OK;
	const uint8* b = source;

	// print one line (12 values)
	while (sourceSize > 12) {
		sprintf(buffer, "	0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x"
						", 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x"
						", 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x,\n",
						b[0], b[1], b[2], b[3],
						b[4], b[5], b[6], b[7],
						b[8], b[9], b[10], b[11]);

		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
			break;
		}

		sourceSize -= 12;
		b += 12;
	}
	// last line (up to 12 values)
	if (ret >= B_OK && sourceSize > 0) {
		for (size_t i = 0; i < sourceSize; i++) {
			if (i == 0)
				sprintf(buffer, "	0x%.2x", b[i]);
			else
				sprintf(buffer, ", 0x%.2x", b[i]);
			size = strlen(buffer);
			written = stream->Write(buffer, size);
			if (written != (ssize_t)size) {
				if (written >= 0)
					ret = B_ERROR;
				else
					ret = (status_t)written;
				break;
			}
		}
	}
	if (ret >= B_OK) {
		// finish
		sprintf(buffer, "\n};\n");
		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
		}
	}

	return ret;
}

