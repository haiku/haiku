/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef META_FORMAT_H
#define META_FORMAT_H


#include <MediaFormats.h>


namespace BPrivate {
namespace media {

// Implementation can be found in MediaFormats.cpp

#define MEDIA_META_FORMAT_TYPE 'MeFo'
	// to be used in the MEDIA_SERVER_GET_FORMATS message reply

struct meta_format {
	meta_format();
	meta_format(const media_format_description &description,
		const media_format &format, int32 id);
	meta_format(const media_format_description &description);
	meta_format(const meta_format &other);

	bool Matches(const media_format &format, media_format_family family);
	static int CompareDescriptions(const meta_format *a, const meta_format *b);
	static int Compare(const meta_format *a, const meta_format *b);

	media_format_description description;
	media_format format;
	int32 id;
};

typedef status_t (*_MakeFormatHookFunc)(
	const media_format_description *descriptions, int32 descriptionsCount,
	media_format *format, uint32 flags);

extern _MakeFormatHookFunc _gMakeFormatHook;

}	// namespace media
}	// namespace BPrivate

#endif	/* META_FORMAT_H */
