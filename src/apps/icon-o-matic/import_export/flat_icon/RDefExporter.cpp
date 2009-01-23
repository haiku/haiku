/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RDefExporter.h"

#include <stdio.h>
#include <string.h>

#include <DataIO.h>

// constructor
RDefExporter::RDefExporter()
	: FlatIconExporter()
{
}

// destructor
RDefExporter::~RDefExporter()
{
}

// Export
status_t
RDefExporter::Export(const Icon* icon, BPositionIO* stream)
{
	BMallocIO buffer;
	status_t ret = FlatIconExporter::Export(icon, &buffer);
	if (ret < B_OK)
		return ret;

	return _Export((const uint8*)buffer.Buffer(), buffer.BufferLength(), stream);
}

// MIMEType
const char*
RDefExporter::MIMEType()
{
	return "text/x-vnd.Be.ResourceDef";
}

// #pragma mark -

// _Export
status_t
RDefExporter::_Export(const uint8* source, size_t sourceSize, BPositionIO* stream) const
{
	char buffer[2048];
	// write header
	sprintf(buffer, "\nresource(<your resource id here>) #'VICN' array {\n");
	size_t size = strlen(buffer);

	ssize_t written = stream->Write(buffer, size);
	if (written < 0)
		return (status_t)written;
	if (written < (ssize_t)size)
		return B_ERROR;

	status_t ret = B_OK;
	const uint8* b = source;

	// print one line (32 values)
	while (sourceSize >= 32) {
		sprintf(buffer, "	$\"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X"
						"%.2X%.2X%.2X%.2X\"\n",
						b[0], b[1], b[2], b[3],
						b[4], b[5], b[6], b[7],
						b[8], b[9], b[10], b[11],
						b[12], b[13], b[14], b[15],
						b[16], b[17], b[18], b[19],
						b[20], b[21], b[22], b[23],
						b[24], b[25], b[26], b[27],
						b[28], b[29], b[30], b[31]);

		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
			break;
		}

		sourceSize -= 32;
		b += 32;
	}
	// beginning of last line
	if (ret >= B_OK && sourceSize > 0) {
		sprintf(buffer, "	$\"");
		size = strlen(buffer);
		written = stream->Write(buffer, size);
		if (written != (ssize_t)size) {
			if (written >= 0)
				ret = B_ERROR;
			else
				ret = (status_t)written;
		}
	}
	// last line (up to 32 values)
	bool endQuotes = sourceSize > 0;
	if (ret >= B_OK && sourceSize > 0) {
		for (size_t i = 0; i < sourceSize; i++) {
			sprintf(buffer, "%.2X", b[i]);
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
		sprintf(buffer, endQuotes ? "\"\n};\n" : "};\n");
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

