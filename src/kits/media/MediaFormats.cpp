/*
 * Copyright 2004-2009, The Haiku Project. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler
 *		Marcus Overhagen
 */

#include <MediaFormats.h>

#include <CodecRoster.h>
#include <ObjectList.h>
#include <Message.h>
#include <Autolock.h>

#include "AddOnManager.h"
#include "DataExchange.h"
#include "FormatManager.h"
#include "MetaFormat.h"
#include "MediaDebug.h"

#include <string.h>

using namespace BPrivate::media;


static BLocker sLock;
static BObjectList<meta_format> sFormats;
static bigtime_t sLastFormatsUpdate;


status_t
get_next_encoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, media_format* _outputFormat,
	media_codec_info* _codecInfo)
{
	return BCodecKit::BCodecRoster::GetNextEncoder(cookie, fileFormat,
		inputFormat, _outputFormat, _codecInfo);
}


status_t
get_next_encoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, const media_format* outputFormat,
	media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
	media_format* _acceptedOutputFormat)
{
	return BCodecKit::BCodecRoster::GetNextEncoder(cookie, fileFormat,
		inputFormat, outputFormat, _codecInfo, _acceptedInputFormat,
			_acceptedOutputFormat);
}


status_t
get_next_encoder(int32* cookie, media_codec_info* _codecInfo)
{
	return BCodecKit::BCodecRoster::GetNextEncoder(cookie, _codecInfo);
}


bool
does_file_accept_format(const media_file_format* _fileFormat,
	media_format* format, uint32 flags)
{
	UNIMPLEMENTED();
	return false;
}


//	#pragma mark -


_media_format_description::_media_format_description()
{
	memset(this, 0, sizeof(*this));
}


_media_format_description::~_media_format_description()
{
}


_media_format_description::_media_format_description(
	const _media_format_description& other)
{
	memcpy(this, &other, sizeof(*this));
}


_media_format_description& 
_media_format_description::operator=(const _media_format_description& other)
{
	memcpy(this, &other, sizeof(*this));
	return *this;
}


bool
operator==(const media_format_description& a,
	const media_format_description& b)
{
	if (a.family != b.family)
		return false;

	switch (a.family) {
		case B_BEOS_FORMAT_FAMILY:
			return a.u.beos.format == b.u.beos.format;
		case B_QUICKTIME_FORMAT_FAMILY:
			return a.u.quicktime.codec == b.u.quicktime.codec
				&& a.u.quicktime.vendor == b.u.quicktime.vendor;
		case B_AVI_FORMAT_FAMILY:
			return a.u.avi.codec == b.u.avi.codec;
		case B_ASF_FORMAT_FAMILY:
			return a.u.asf.guid == b.u.asf.guid;
		case B_MPEG_FORMAT_FAMILY:
			return a.u.mpeg.id == b.u.mpeg.id;
		case B_WAV_FORMAT_FAMILY:
			return a.u.wav.codec == b.u.wav.codec;
		case B_AIFF_FORMAT_FAMILY:
			return a.u.aiff.codec == b.u.aiff.codec;
		case B_AVR_FORMAT_FAMILY:
			return a.u.avr.id == b.u.avr.id;
		case B_MISC_FORMAT_FAMILY:
			return a.u.misc.file_format == b.u.misc.file_format
				&& a.u.misc.codec == b.u.misc.codec;

		default:
			return false;
	}
}


bool
operator<(const media_format_description& a, const media_format_description& b)
{
	if (a.family != b.family)
		return a.family < b.family;

	switch (a.family) {
		case B_BEOS_FORMAT_FAMILY:
			return a.u.beos.format < b.u.beos.format;
		case B_QUICKTIME_FORMAT_FAMILY:
			if (a.u.quicktime.vendor == b.u.quicktime.vendor)
				return a.u.quicktime.codec < b.u.quicktime.codec;
			return a.u.quicktime.vendor < b.u.quicktime.vendor;
		case B_AVI_FORMAT_FAMILY:
			return a.u.avi.codec < b.u.avi.codec;
		case B_ASF_FORMAT_FAMILY:
			return a.u.asf.guid < b.u.asf.guid;
		case B_MPEG_FORMAT_FAMILY:
			return a.u.mpeg.id < b.u.mpeg.id;
		case B_WAV_FORMAT_FAMILY:
			return a.u.wav.codec < b.u.wav.codec;
		case B_AIFF_FORMAT_FAMILY:
			return a.u.aiff.codec < b.u.aiff.codec;
		case B_AVR_FORMAT_FAMILY:
			return a.u.avr.id < b.u.avr.id;
		case B_MISC_FORMAT_FAMILY:
			if (a.u.misc.file_format == b.u.misc.file_format)
				return a.u.misc.codec < b.u.misc.codec;
			return a.u.misc.file_format < b.u.misc.file_format;

		default:
			return true;
	}
}


bool
operator==(const GUID& a, const GUID& b)
{
	return memcmp(&a, &b, sizeof(a)) == 0;
}


bool
operator<(const GUID& a, const GUID& b)
{
	return memcmp(&a, &b, sizeof(a)) < 0;
}


//	#pragma mark -
//
//	Some (meta) formats supply functions


meta_format::meta_format()
	:
	id(0)
{
}



meta_format::meta_format(const media_format_description& description,
	const media_format& format, int32 id)
	:
	description(description),
	format(format),
	id(id)
{
}


meta_format::meta_format(const media_format_description& description)
	:
	description(description),
	id(0)
{
}


meta_format::meta_format(const meta_format& other)
	:
	description(other.description),
	format(other.format)
{
}


bool 
meta_format::Matches(const media_format& otherFormat,
	media_format_family family)
{
	if (family != description.family)
		return false;

	return format.Matches(&otherFormat);
}


int 
meta_format::CompareDescriptions(const meta_format* a, const meta_format* b)
{
	if (a->description == b->description)
		return 0;

	if (a->description < b->description)
		return -1;

	return 1;
}


int 
meta_format::Compare(const meta_format* a, const meta_format* b)
{
	int compare = CompareDescriptions(a, b);
	if (compare != 0)
		return compare;

	return a->id - b->id;
}


/** We share one global list for all BMediaFormats in the team - since the
 *	format data can change at any time, we have to update the list to ensure
 *	that we are working on the latest data set. The list is always sorted by
 *	description. The formats lock has to be held when you call this function.
 */
static status_t
update_media_formats()
{
	if (!sLock.IsLocked())
		return B_NOT_ALLOWED;

	// We want the add-ons to register themselves with the format manager, so
	// the list is up to date.
	BCodecKit::BPrivate::AddOnManager::GetInstance()->RegisterAddOns();

	BMessage reply;
	FormatManager::GetInstance()->GetFormats(sLastFormatsUpdate, reply);

	// do we need an update at all?
	bool needUpdate;
	if (reply.FindBool("need_update", &needUpdate) < B_OK)
		return B_ERROR;
	if (!needUpdate)
		return B_OK;

	// update timestamp and check if the message is okay
	type_code code;
	int32 count;
	if (reply.FindInt64("timestamp", &sLastFormatsUpdate) < B_OK
		|| reply.GetInfo("formats", &code, &count) < B_OK)
		return B_ERROR;

	// overwrite already existing formats

	int32 index = 0;
	for (; index < sFormats.CountItems() && index < count; index++) {
		meta_format* item = sFormats.ItemAt(index);

		const meta_format* newItem;
		ssize_t size;
		if (reply.FindData("formats", MEDIA_META_FORMAT_TYPE, index,
				(const void**)&newItem, &size) == B_OK)
			*item = *newItem;
	}

	// allocate additional formats

	for (; index < count; index++) {
		const meta_format* newItem;
		ssize_t size;
		if (reply.FindData("formats", MEDIA_META_FORMAT_TYPE, index,
				(const void**)&newItem, &size) == B_OK)
			sFormats.AddItem(new meta_format(*newItem));
	}

	// remove no longer used formats

	while (count < sFormats.CountItems())
		delete sFormats.RemoveItemAt(count);

	return B_OK;
}


//	#pragma mark -


BMediaFormats::BMediaFormats()
	:
	fIteratorIndex(0)	
{
}


BMediaFormats::~BMediaFormats()
{
}


status_t 
BMediaFormats::InitCheck()
{
	return sLock.Sem() >= B_OK ? B_OK : sLock.Sem();
}


status_t 
BMediaFormats::GetCodeFor(const media_format& format,
	media_format_family family,
	media_format_description* _description)
{
	BAutolock locker(sLock);

	status_t status = update_media_formats();
	if (status < B_OK)
		return status;

	// search for a matching format

	for (int32 index = sFormats.CountItems(); index-- > 0;) {
		meta_format* metaFormat = sFormats.ItemAt(index);

		if (metaFormat->Matches(format, family)) {
			*_description = metaFormat->description;
			return B_OK;
		}
	}

	return B_MEDIA_BAD_FORMAT;
}


status_t 
BMediaFormats::GetFormatFor(const media_format_description& description,
	media_format* _format)
{
	BAutolock locker(sLock);

	status_t status = update_media_formats();
	if (status < B_OK) {
		ERROR("BMediaFormats: updating formats from server failed: %s!\n",
			strerror(status));
		return status;
	}
	TRACE("search for description family = %d, a = 0x%"
		B_PRId32 "x, b = 0x%" B_PRId32 "x\n",
		description.family, description.u.misc.file_format,
		description.u.misc.codec);

	// search for a matching format description

	meta_format other(description);
	const meta_format* metaFormat = sFormats.BinarySearch(other,
		meta_format::CompareDescriptions);
	TRACE("meta format == %p\n", metaFormat);
	if (metaFormat == NULL) {
		_format->Clear(); // clear to widlcard
		return B_MEDIA_BAD_FORMAT;
	}

	// found it!
	*_format = metaFormat->format;
	return B_OK;
}


status_t 
BMediaFormats::GetBeOSFormatFor(uint32 format, 
	media_format* _format, media_type type)
{
	BMediaFormats formats;

	media_format_description description;
	description.family = B_BEOS_FORMAT_FAMILY;
	description.u.beos.format = format;

	status_t status = formats.GetFormatFor(description, _format);
	if (status < B_OK)
		return status;

	if (type != B_MEDIA_UNKNOWN_TYPE && type != _format->type)
		return B_BAD_TYPE;

	return B_OK;
}


status_t 
BMediaFormats::GetAVIFormatFor(uint32 codec,
	media_format* _format, media_type type)
{
	UNIMPLEMENTED();
	BMediaFormats formats;

	media_format_description description;
	description.family = B_AVI_FORMAT_FAMILY;
	description.u.avi.codec = codec;

	status_t status = formats.GetFormatFor(description, _format);
	if (status < B_OK)
		return status;

	if (type != B_MEDIA_UNKNOWN_TYPE && type != _format->type)
		return B_BAD_TYPE;

	return B_OK;
}


status_t 
BMediaFormats::GetQuicktimeFormatFor(uint32 vendor, uint32 codec, 
	media_format* _format, media_type type)
{
	BMediaFormats formats;

	media_format_description description;
	description.family = B_QUICKTIME_FORMAT_FAMILY;
	description.u.quicktime.vendor = vendor;
	description.u.quicktime.codec = codec;

	status_t status = formats.GetFormatFor(description, _format);
	if (status < B_OK)
		return status;

	if (type != B_MEDIA_UNKNOWN_TYPE && type != _format->type)
		return B_BAD_TYPE;

	return B_OK;
}


status_t 
BMediaFormats::RewindFormats()
{
	if (!sLock.IsLocked() || sLock.LockingThread() != find_thread(NULL)) {
		// TODO: Shouldn't we simply drop into the debugger in this case?
		return B_NOT_ALLOWED;
	}

	fIteratorIndex = 0;
	return B_OK;
}


status_t 
BMediaFormats::GetNextFormat(media_format* _format,
	media_format_description* _description)
{
	if (!sLock.IsLocked() || sLock.LockingThread() != find_thread(NULL)) {
		// TODO: Shouldn't we simply drop into the debugger in this case?
		return B_NOT_ALLOWED;
	}

	if (fIteratorIndex == 0) {
		// This is the first call, so let's make sure we have current data to
		// operate on.
		status_t status = update_media_formats();
		if (status < B_OK)
			return status;
	}

	meta_format* format = sFormats.ItemAt(fIteratorIndex++);
	if (format == NULL)
		return B_BAD_INDEX;

	return B_OK;
}


bool
BMediaFormats::Lock()
{
	return sLock.Lock();
}


void 
BMediaFormats::Unlock()
{
	sLock.Unlock();
}


status_t 
BMediaFormats::MakeFormatFor(const media_format_description* descriptions,
	int32 descriptionCount, media_format* format, uint32 flags,
	void* _reserved)
{
	return FormatManager::GetInstance()->MakeFormatFor(descriptions,
		descriptionCount, *format, flags, _reserved);
}


// #pragma mark - deprecated API


status_t 
BMediaFormats::MakeFormatFor(const media_format_description& description,
	const media_format& inFormat, media_format* _outFormat)
{
	*_outFormat = inFormat;
	return MakeFormatFor(&description, 1, _outFormat);
}
