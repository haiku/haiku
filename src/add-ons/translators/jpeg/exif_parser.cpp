/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "exif_parser.h"

#include <Catalog.h>
#include <ctype.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>

#include <Catalog.h>
#include <Message.h>

#include <ReadHelper.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "exit_parser"


using std::set;

enum {
	TAG_EXIF_OFFSET		= 0x8769,
	TAG_SUB_DIR_OFFSET	= 0xa005,

	TAG_MAKER			= 0x10f,
	TAG_MODEL			= 0x110,
	TAG_ORIENTATION		= 0x112,
	TAG_EXPOSURE_TIME	= 0x829a,
	TAG_ISO				= 0x8827,
};

static const convert_tag kDefaultTags[] = {
	{TAG_MAKER,			B_ANY_TYPE,		"Maker"},
	{TAG_MODEL,			B_ANY_TYPE,		"Model"},
	{TAG_ORIENTATION,	B_INT32_TYPE,	"Orientation"},
	{TAG_EXPOSURE_TIME,	B_DOUBLE_TYPE,	"ExposureTime"},
	{TAG_ISO,			B_INT32_TYPE,	"ISO"},
};
static const size_t kNumDefaultTags = sizeof(kDefaultTags)
	/ sizeof(kDefaultTags[0]);


static status_t parse_tiff_directory(TReadHelper& read, set<off_t>& visited,
	BMessage& target, const convert_tag* tags, size_t tagCount);


static status_t
add_to_message(TReadHelper& source, BMessage& target, tiff_tag& tag,
	const char* name, type_code type)
{
	type_code defaultType = B_INT32_TYPE;
	double doubleValue = 0.0;
	int32 intValue = 0;

	switch (tag.type) {
		case TIFF_STRING_TYPE:
		{
			if (type != B_ANY_TYPE && type != B_STRING_TYPE)
				return B_BAD_VALUE;

			char* buffer = (char*)malloc(tag.length);
			if (buffer == NULL)
				return B_NO_MEMORY;

			source(buffer, tag.length);

			// remove trailing spaces
			int32 i = tag.length;
			while ((--i > 0 && isspace(buffer[i])) || !buffer[i]) {
				buffer[i] = '\0';
			}

			status_t status = target.AddString(name, buffer);
			free(buffer);

			return status;
		}

		case TIFF_UNDEFINED_TYPE:
		{
			if (type != B_ANY_TYPE && type != B_STRING_TYPE && type != B_RAW_TYPE)
				return B_BAD_VALUE;

			char* buffer = (char*)malloc(tag.length);
			if (buffer == NULL)
				return B_NO_MEMORY;

			source(buffer, tag.length);

			status_t status;
			if (type == B_STRING_TYPE)
				status = target.AddString(name, buffer);
			else
				status = target.AddData(name, B_RAW_TYPE, buffer, tag.length);

			free(buffer);

			return status;
		}

		// unsigned
		case TIFF_UINT8_TYPE:
			intValue = source.Next<uint8>();
			break;
		case TIFF_UINT16_TYPE:
			defaultType = B_INT32_TYPE;
			intValue = source.Next<uint16>();
			break;
		case TIFF_UINT32_TYPE:
			defaultType = B_INT32_TYPE;
			intValue = source.Next<uint32>();
			break;
		case TIFF_UFRACTION_TYPE:
		{
			defaultType = B_DOUBLE_TYPE;
			double value = source.Next<uint32>();
			doubleValue = value / source.Next<uint32>();
			break;
		}

		// signed
		case TIFF_INT8_TYPE:
			intValue = source.Next<int8>();
			break;
		case TIFF_INT16_TYPE:
			intValue = source.Next<int16>();
			break;
		case TIFF_INT32_TYPE:
			intValue = source.Next<int32>();
			break;
		case TIFF_FRACTION_TYPE:
		{
			defaultType = B_DOUBLE_TYPE;
			double value = source.Next<int32>();
			doubleValue = value / source.Next<int32>();
		}

		// floating point
		case TIFF_FLOAT_TYPE:
			defaultType = B_FLOAT_TYPE;
			doubleValue = source.Next<float>();
			break;
		case TIFF_DOUBLE_TYPE:
			defaultType = B_DOUBLE_TYPE;
			doubleValue = source.Next<double>();
			break;

		default:
			return B_BAD_VALUE;
	}

	if (defaultType == B_INT32_TYPE)
		doubleValue = intValue;
	else
		intValue = int32(doubleValue + 0.5);

	if (type == B_ANY_TYPE)
		type = defaultType;

	switch (type) {
		case B_INT32_TYPE:
			return target.AddInt32(name, intValue);
		case B_FLOAT_TYPE:
			return target.AddFloat(name, doubleValue);
		case B_DOUBLE_TYPE:
			return target.AddDouble(name, doubleValue);

		default:
			return B_BAD_VALUE;
	}
}


static const convert_tag*
find_convert_tag(uint16 id, const convert_tag* tags, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (tags[i].tag == id)
			return &tags[i];
	}

	return NULL;
}


/*!
	Reads a TIFF tag and positions the file stream to its data section
*/
void
parse_tiff_tag(TReadHelper& read, tiff_tag& tag, off_t& offset)
{
	read(tag.tag);
	read(tag.type);
	read(tag.length);

	offset = read.Position() + 4;

	uint32 length = tag.length;

	switch (tag.type) {
		case TIFF_UINT16_TYPE:
		case TIFF_INT16_TYPE:
			length *= 2;
			break;

		case TIFF_UINT32_TYPE:
		case TIFF_INT32_TYPE:
		case TIFF_FLOAT_TYPE:
			length *= 4;
			break;

		case TIFF_UFRACTION_TYPE:
		case TIFF_FRACTION_TYPE:
		case TIFF_DOUBLE_TYPE:
			length *= 8;
			break;

		default:
			break;
	}

	if (length > 4) {
		uint32 position;
		read(position);

		read.Seek(position, SEEK_SET);
	}
}


static status_t
parse_tiff_directory(TReadHelper& read, set<off_t>& visited, off_t offset,
	BMessage& target, const convert_tag* convertTags, size_t convertTagCount)
{
	if (visited.find(offset) != visited.end()) {
		// The EXIF data is obviously corrupt
		return B_BAD_DATA;
	}

	read.Seek(offset, SEEK_SET);
	visited.insert(offset);

	uint16 tags;
	read(tags);
	if (tags > 512)
		return B_BAD_DATA;

	while (tags--) {
		off_t nextOffset;
		tiff_tag tag;
		parse_tiff_tag(read, tag, nextOffset);

		//printf("TAG %u\n", tag.tag);

		switch (tag.tag) {
			case TAG_EXIF_OFFSET:
			case TAG_SUB_DIR_OFFSET:
			{
				status_t status = parse_tiff_directory(read, visited, target,
					convertTags, convertTagCount);
				if (status < B_OK)
					return status;
				break;
			}

			default:
				const convert_tag* convertTag = find_convert_tag(tag.tag,
					convertTags, convertTagCount);
				if (convertTag != NULL) {
					add_to_message(read, target, tag, convertTag->name,
						convertTag->type);
				}
				break;
		}

		if (visited.find(nextOffset) != visited.end())
			return B_BAD_DATA;

		read.Seek(nextOffset, SEEK_SET);
		visited.insert(nextOffset);
	}

	return B_OK;
}


static status_t
parse_tiff_directory(TReadHelper& read, set<off_t>& visited, BMessage& target,
	const convert_tag* tags, size_t tagCount)
{
	while (true) {
		int32 offset;
		read(offset);
		if (offset == 0)
			break;

		status_t status = parse_tiff_directory(read, visited, offset, target,
			tags, tagCount);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark -


/*!	Converts the EXIF data that starts in \a source to a BMessage in \a target.
	If the EXIF data is corrupt, this function will return an appropriate error
	code. Nevertheless, there might be some data ending up in \a target that
	was parsed until this point.
*/
status_t
convert_exif_to_message(BPositionIO& source, BMessage& target,
	const convert_tag* tags, size_t tagCount)
{
	TReadHelper read(source);

	uint16 endian;
	read(endian);
	if (endian != 'MM' && endian != 'II')
		return B_BAD_TYPE;

#if B_HOST_IS_LENDIAN
	read.SetSwap(endian == 'MM');
#else
	read.SetSwap(endian == 'II');
#endif

	int16 magic;
	read(magic);
	if (magic != 42)
		return B_BAD_TYPE;

	set<off_t> visitedOffsets;
	return parse_tiff_directory(read, visitedOffsets, target, tags, tagCount);
}


status_t
convert_exif_to_message(BPositionIO& source, BMessage& target)
{
	return convert_exif_to_message(source, target, kDefaultTags,
		kNumDefaultTags);
}
