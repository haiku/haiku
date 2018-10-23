/*
 * Copyright (c) 2002, 2003, 2008 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include <MediaAddOn.h>
#include <string.h>
#include <stdlib.h>
#include <new>
#include "MediaDebug.h"
#include "DataExchange.h"


#define MAX_FLAVOR_IN_FORMAT_COUNT	300
#define MAX_FLAVOR_OUT_FORMAT_COUNT	300

#define FLATTEN_MAGIC 		'CODE'
#define FLATTEN_TYPECODE	'DFIT'


static char *
_newstrdup(const char *str)
{
	if (str == NULL)
		return NULL;
	int len = strlen(str) + 1;
	char *p = new(std::nothrow) char[len];
	if (p)
		memcpy(p, str, len);
	return p;
}


// #pragma mark - dormant_node_info


dormant_node_info::dormant_node_info()
	:
	addon(-1),
	flavor_id(-1)
{
	name[0] = '\0';
}


dormant_node_info::~dormant_node_info()
{
}


// #pragma mark - flavor_info


/* DO NOT IMPLEMENT */
/*
flavor_info &flavor_info::operator=(const flavor_info &other)
*/


// #pragma mark - dormant_flavor_info


dormant_flavor_info::dormant_flavor_info()
{
	name = NULL;
	info = NULL;
	kinds = 0;
	flavor_flags = 0;
	internal_id = 0;
	possible_count = 0;
	in_format_count = 0;
	in_format_flags = 0;
	in_formats = NULL;
	out_format_count = 0;
	out_format_flags = 0;
	out_formats = NULL;
}


dormant_flavor_info::~dormant_flavor_info()
{
	delete[] name;
	delete[] info;
	delete[] in_formats;
	delete[] out_formats;
}


dormant_flavor_info::dormant_flavor_info(const dormant_flavor_info &clone)
{
	name = NULL;
	info = NULL;
	in_formats = NULL;
	out_formats = NULL;

	*this = clone;
}


dormant_flavor_info &
dormant_flavor_info::operator=(const dormant_flavor_info &clone)
{
	// call operator=(const flavor_info &clone) to copy the flavor_info base class
	*this = static_cast<const flavor_info>(clone);
	// copy the dormant_node_info member variable
	node_info = clone.node_info;
	return *this;
}


dormant_flavor_info &
dormant_flavor_info::operator=(const flavor_info &clone)
{
	kinds = clone.kinds;
	flavor_flags = clone.flavor_flags;
	internal_id = clone.internal_id;
	possible_count = clone.possible_count;

	delete [] info;
	info = _newstrdup(clone.info);

	delete [] name;
	name = _newstrdup(clone.name);

	delete [] in_formats;
	in_formats = NULL;
	in_format_count = 0;
	in_format_flags = clone.in_format_flags;
	if ((kinds & B_BUFFER_CONSUMER) != 0) {
		if (clone.in_format_count >= 0
			&& clone.in_format_count <= MAX_FLAVOR_IN_FORMAT_COUNT) {
			in_formats = new(std::nothrow) media_format[clone.in_format_count];
			if (in_formats != NULL && clone.in_formats != NULL) {
				in_format_count = clone.in_format_count;
				for (int i = 0; i < in_format_count; i++) {
					const_cast<media_format &>(in_formats[i])
						= clone.in_formats[i];
				}
			}
		} else {
			fprintf(stderr, "error: dormant_flavor_info::operator= clone.in_"
				"format_count is invalid\n");
		}
	} else if (clone.in_format_count) {
		fprintf(stderr, "warning: dormant_flavor_info::operator= not "
			"B_BUFFER_CONSUMER and clone.in_format_count is != 0\n");
	}

	delete [] out_formats;
	out_formats = NULL;
	out_format_count = 0;
	out_format_flags = clone.out_format_flags;
	if (kinds & B_BUFFER_PRODUCER) {
		if (clone.out_format_count >= 0
			&& clone.out_format_count <= MAX_FLAVOR_OUT_FORMAT_COUNT) {
			out_formats = new(std::nothrow) media_format[clone.out_format_count];
			if (out_formats != NULL && clone.out_formats != NULL) {
				out_format_count = clone.out_format_count;
				for (int i = 0; i < out_format_count; i++) {
					const_cast<media_format &>(out_formats[i])
						= clone.out_formats[i];
				}
			}
		} else {
			fprintf(stderr, "error dormant_flavor_info::operator= clone.out_"
				"format_count is invalid\n");
		}
	} else if (clone.out_format_count) {
		fprintf(stderr, "warning: dormant_flavor_info::operator= not "
			"B_BUFFER_PRODUCER and clone.out_format_count is != 0\n");
	}

	// initialize node_info with default values
	dormant_node_info defaultValues;
	node_info = defaultValues;

	return *this;
}


void
dormant_flavor_info::set_name(const char *newName)
{
	delete[] name;
	name = _newstrdup(newName);
}


void
dormant_flavor_info::set_info(const char *newInfo)
{
	delete[] info;
	info = _newstrdup(newInfo);
}


void
dormant_flavor_info::add_in_format(const media_format &in_format)
{
	media_format *p = new(std::nothrow) media_format[in_format_count + 1];
	if (p) {
		for (int i = 0; i < in_format_count; i++)
			p[i] = in_formats[i];
		p[in_format_count] = in_format;
		delete [] in_formats;
		in_formats = p;
		in_format_count += 1;
	}
}


void
dormant_flavor_info::add_out_format(const media_format &out_format)
{
	media_format *p = new(std::nothrow) media_format[out_format_count + 1];
	if (p) {
		for (int i = 0; i < out_format_count; i++)
			p[i] = out_formats[i];
		p[out_format_count] = out_format;
		delete [] out_formats;
		out_formats = p;
		out_format_count += 1;
	}
}


bool
dormant_flavor_info::IsFixedSize() const
{
	return false;
}


type_code
dormant_flavor_info::TypeCode() const
{
	return FLATTEN_TYPECODE;
}


ssize_t
dormant_flavor_info::FlattenedSize() const
{
	ssize_t size = 0;
	// magic
	size += sizeof(int32);
	// size
	size += sizeof(int32);
	// struct flavor_info
	size += sizeof(int32) + strlen(name);
	size += sizeof(int32) + strlen(info);
	size += sizeof(kinds);
	size += sizeof(flavor_flags);
	size += sizeof(internal_id);
	size += sizeof(possible_count);
	size += sizeof(in_format_count);
	size += sizeof(in_format_flags);
	if (in_format_count > 0 && in_format_count <= MAX_FLAVOR_IN_FORMAT_COUNT
		&& in_formats != NULL)
		size += in_format_count * sizeof(media_format);
	size += sizeof(out_format_count);
	size += sizeof(out_format_flags);
	if (out_format_count > 0 && out_format_count <= MAX_FLAVOR_OUT_FORMAT_COUNT
		&& out_formats != NULL)
		size += out_format_count * sizeof(media_format);
	// struct dormant_node_info	node_info
	size += sizeof(node_info);

	return size;
}


status_t
dormant_flavor_info::Flatten(void *buffer, ssize_t size) const
{
	if (size < FlattenedSize())
		return B_ERROR;

	char *buf = (char *)buffer;
	int32 nameLength = name ? (int32)strlen(name) : -1;
	int32 infoLength = info ? (int32)strlen(info) : -1;
	int32 inFormatCount = 0;
	size_t inFormatSize = 0;
	int32 outFormatCount = 0;
	size_t outFormatSize = 0;

	if ((kinds & B_BUFFER_CONSUMER) != 0 && in_format_count > 0
		&& in_formats != NULL) {
		if (in_format_count <= MAX_FLAVOR_IN_FORMAT_COUNT) {
			inFormatCount = in_format_count;
			inFormatSize = in_format_count * sizeof(media_format);
		} else {
			fprintf(stderr, "error dormant_flavor_info::Flatten: "
				"in_format_count is too large\n");
			return B_ERROR;
		}
	}

	if ((kinds & B_BUFFER_PRODUCER) != 0 && out_format_count > 0
		&& out_formats != NULL) {
		if (out_format_count <= MAX_FLAVOR_OUT_FORMAT_COUNT) {
			outFormatCount = out_format_count;
			outFormatSize = out_format_count * sizeof(media_format);
		} else {
			fprintf(stderr, "error dormant_flavor_info::Flatten: "
				"out_format_count is too large\n");
			return B_ERROR;
		}
	}

	// magic
	*(int32*)buf = FLATTEN_MAGIC; buf += sizeof(int32);

	// size
	*(int32*)buf = FlattenedSize(); buf += sizeof(int32);

	// struct flavor_info
	*(int32*)buf = nameLength; buf += sizeof(int32);
	if (nameLength > 0) {
		memcpy(buf, name, nameLength);
		buf += nameLength;
	}
	*(int32*)buf = infoLength; buf += sizeof(int32);
	if (infoLength > 0) {
		memcpy(buf, info, infoLength);
		buf += infoLength;
	}

	*(uint64*)buf = kinds; buf += sizeof(uint64);
	*(uint32*)buf = flavor_flags; buf += sizeof(uint32);
	*(int32*)buf = internal_id; buf += sizeof(int32);
	*(int32*)buf = possible_count; buf += sizeof(int32);
	*(int32*)buf = inFormatCount; buf += sizeof(int32);
	*(uint32*)buf = in_format_flags; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	memcpy(buf, in_formats, inFormatSize); buf += inFormatSize;

	*(int32*)buf = outFormatCount; buf += sizeof(int32);
	*(uint32*)buf = out_format_flags; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	memcpy(buf, out_formats, outFormatSize); buf += outFormatSize;

	*(dormant_node_info*)buf = node_info; buf += sizeof(dormant_node_info);

	return B_OK;
}


status_t
dormant_flavor_info::Unflatten(type_code c, const void *buffer, ssize_t size)
{
	if (c != FLATTEN_TYPECODE)
		return B_ERROR;
	if (size < 8)
		return B_ERROR;

	const char *buf = (const char *)buffer;
	int32 nameLength;
	int32 infoLength;

	// check magic
	if (*(int32*)buf != FLATTEN_MAGIC)
		return B_ERROR;
	buf += sizeof(int32);

	// check size
	if (*(uint32*)buf > (uint32)size)
		return B_ERROR;
	buf += sizeof(int32);

	delete[] name;
	name = NULL;
	delete[] info;
	info = NULL;
	delete[] in_formats;
	in_formats = NULL;
	in_format_count = 0;
	delete[] out_formats;
	out_formats = NULL;
	out_format_count = 0;

	// struct flavor_info
	nameLength = *(int32*)buf; buf += sizeof(int32);
	if (nameLength >= 0) { // if nameLength is -1, we leave name = 0
		name = new(std::nothrow) char [nameLength + 1];
		if (name) {
			memcpy(name, buf, nameLength);
			name[nameLength] = 0;
			buf += nameLength; // XXX not save
		}
	}

	infoLength = *(int32*)buf; buf += sizeof(int32);
	if (infoLength >= 0) { // if infoLength is -1, we leave info = 0
		info = new(std::nothrow) char [infoLength + 1];
		if (info) {
			memcpy(info, buf, infoLength);
			info[infoLength] = 0;
			buf += infoLength; // XXX not save
		}
	}

	int32 count;

	kinds = *(uint64*)buf; buf += sizeof(uint64);
	flavor_flags = *(uint32*)buf; buf += sizeof(uint32);
	internal_id = *(int32*)buf; buf += sizeof(int32);
	possible_count = *(int32*)buf; buf += sizeof(int32);
	count = *(int32*)buf; buf += sizeof(int32);
	in_format_flags = *(uint32*)buf; buf += sizeof(uint32);

	if (count > 0) {
		if (count <= MAX_FLAVOR_IN_FORMAT_COUNT) {
			in_formats = new(std::nothrow) media_format[count];
			if (!in_formats)
				return B_NO_MEMORY;
			// TODO: we should not!!! make flat copies of media_format
			for (int32 i = 0; i < count; i++) {
				const_cast<media_format*>
					(&in_formats[i])->Unflatten(buf);
				buf += sizeof(media_format); // TODO: not save
			}
			in_format_count = count;
		}
	}

	count = *(int32*)buf; buf += sizeof(int32);
	out_format_flags = *(uint32*)buf; buf += sizeof(uint32);

	if (count > 0) {
		if (count <= MAX_FLAVOR_OUT_FORMAT_COUNT) {
			out_formats = new(std::nothrow) media_format[count];
			if (!out_formats)
				return B_NO_MEMORY;
			// TODO: we should not!!! make flat copies of media_format
			for (int32 i = 0; i < count; i++) {
				const_cast<media_format*>
					(&out_formats[i])->Unflatten(buf);
				buf += sizeof(media_format); // TODO: not save
			}
			out_format_count = count;
		}
	}

	node_info = *(dormant_node_info*)buf; buf += sizeof(dormant_node_info);

	return B_OK;
}


// #pragma mark - BMediaAddOn


BMediaAddOn::BMediaAddOn(image_id image)
	:
	fImage(image),
	fAddon(0)
{
	CALLED();
}


BMediaAddOn::~BMediaAddOn()
{
	CALLED();
}


status_t
BMediaAddOn::InitCheck(const char **_failureText)
{
	CALLED();
	// only to be implemented by derived classes
	*_failureText = "no error";
	return B_OK;
}


int32
BMediaAddOn::CountFlavors()
{
	CALLED();
	// only to be implemented by derived classes
	return 0;
}


status_t
BMediaAddOn::GetFlavorAt(int32 n, const flavor_info **_info)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


BMediaNode*
BMediaAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config,
	status_t *_error)
{
	CALLED();
	// only to be implemented by derived classes
	return NULL;
}


status_t
BMediaAddOn::GetConfigurationFor(BMediaNode *node, BMessage *toMessage)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


bool
BMediaAddOn::WantsAutoStart()
{
	CALLED();
	// only to be implemented by derived classes
	return false;
}


status_t
BMediaAddOn::AutoStart(int count, BMediaNode **_node, int32 *_internalID,
	bool *_hasMore)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


status_t
BMediaAddOn::SniffRef(const entry_ref &file, BMimeType *mimeType,
	float *_quality, int32 *_internalID)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


status_t
BMediaAddOn::SniffType(const BMimeType &type, float *_quality,
	int32 *_internalID)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


status_t
BMediaAddOn::GetFileFormatList(int32 flavorID,
	media_file_format *writableFormats, int32 maxWriteItems, int32 *_writeItems,
	media_file_format *readableFormats, int32 maxReadItems, int32 *_readItems,
	void *_reserved)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


status_t
BMediaAddOn::SniffTypeKind(const BMimeType &type, uint64 kinds, float *_quality,
	int32 *_internalID, void *_reserved)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


image_id
BMediaAddOn::ImageID()
{
	return fImage;
}


media_addon_id
BMediaAddOn::AddonID()
{
	return fAddon;
}


// #pragma mark - protected BMediaAddOn


status_t
BMediaAddOn::NotifyFlavorChange()
{
	CALLED();
	if (fAddon == 0)
		return B_ERROR;

	add_on_server_rescan_flavors_command command;
	command.add_on_id = fAddon;
	return SendToAddOnServer(ADD_ON_SERVER_RESCAN_ADD_ON_FLAVORS, &command,
		sizeof(command));
}


// #pragma mark - private BMediaAddOn


/*
unimplemented:
BMediaAddOn::BMediaAddOn()
BMediaAddOn::BMediaAddOn(const BMediaAddOn &clone)
BMediaAddOn & BMediaAddOn::operator=(const BMediaAddOn &clone)
*/


extern "C" {
	// declared here to remove them from the class header file
	status_t _Reserved_MediaAddOn_0__11BMediaAddOnPv(void *, void *); /* now used for BMediaAddOn::GetFileFormatList */
	status_t _Reserved_MediaAddOn_1__11BMediaAddOnPv(void *, void *); /* now used for BMediaAddOn::SniffTypeKind */
	status_t _Reserved_MediaAddOn_0__11BMediaAddOnPv(void *, void *) { return B_ERROR; }
	status_t _Reserved_MediaAddOn_1__11BMediaAddOnPv(void *, void *) { return B_ERROR; }
};

status_t BMediaAddOn::_Reserved_MediaAddOn_2(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_3(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_4(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_5(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_6(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_7(void *) { return B_ERROR; }

