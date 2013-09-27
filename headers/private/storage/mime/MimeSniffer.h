/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _MIME_MIME_SNIFFER_H
#define _MIME_MIME_SNIFFER_H


#include <SupportDefs.h>


class BFile;
class BMimeType;


namespace BPrivate {
namespace Storage {
namespace Mime {


class MimeSniffer {
public:
	virtual						~MimeSniffer();

	virtual	size_t				MinimalBufferSize() = 0;

	virtual	float				GuessMimeType(const char* fileName,
									BMimeType* type) = 0;
	virtual	float				GuessMimeType(BFile* file,
									const void* buffer, int32 length,
									BMimeType* type) = 0;
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_MIME_SNIFFER_H
