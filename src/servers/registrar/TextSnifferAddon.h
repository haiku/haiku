/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_SNIFFER_ADDON_H
#define TEXT_SNIFFER_ADDON_H

#include <MimeSnifferAddon.h>

class TextSnifferAddon : public BMimeSnifferAddon {
public:
								TextSnifferAddon();
	virtual						~TextSnifferAddon();

	virtual	size_t				MinimalBufferSize();

	virtual	float				GuessMimeType(const char* fileName,
									BMimeType* type);
	virtual	float				GuessMimeType(BFile* file,
									const void* buffer, int32 length,
									BMimeType* type);
};

#endif	// TEXT_SNIFFER_ADDON_H
