/* MIME Sniffer Add-On Protocol
 *
 * Copyright 2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_SNIFFER_ADDON_H
#define _MIME_SNIFFER_ADDON_H

#include <SupportDefs.h>

class BFile;
class BMimeType;

// **********************************
// *** WARNING: EXPERIMENTAL API! ***
// **********************************

class BMimeSnifferAddon {
public:
								BMimeSnifferAddon();
	virtual						~BMimeSnifferAddon();

	virtual	size_t				MinimalBufferSize();

	virtual	float				GuessMimeType(const char* fileName,
									BMimeType* type);
	virtual	float				GuessMimeType(BFile* file,
									const void* buffer, int32 length,
									BMimeType* type);
};

#endif	// _MIME_SNIFFER_ADDON_H
