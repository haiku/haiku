/*
 * Copyright 2006-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_TEXT_SNIFFER_ADDON_H
#define _MIME_TEXT_SNIFFER_ADDON_H


#include <MimeSnifferAddon.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


class DatabaseLocation;


class TextSnifferAddon : public BMimeSnifferAddon {
public:
								TextSnifferAddon(
									DatabaseLocation* databaseLocation);
	virtual						~TextSnifferAddon();

	virtual	size_t				MinimalBufferSize();

	virtual	float				GuessMimeType(const char* fileName,
									BMimeType* type);
	virtual	float				GuessMimeType(BFile* file,
									const void* buffer, int32 length,
									BMimeType* type);

private:
			DatabaseLocation*	fDatabaseLocation;
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_TEXT_SNIFFER_ADDON_H
