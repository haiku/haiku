/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Authors: Axel Dörfler, Marcus Overhagen
*/


#include "FormatManager.h"
#include "debug.h"

#include <Autolock.h>

#include <stdio.h>
#include <string.h>


#define TIMEOUT	5000000LL
	// 5 seconds timeout for sending the reply
	// ToDo: do we really want to pause the server looper for this?
	//	would be better to offload this action to a second thread

const char *family_to_string(media_format_family in_family);
const char *string_for_description(const media_format_description &in_description, char *string, size_t length);


FormatManager::FormatManager()
	:
	fLock("format manager"),
	fLastUpdate(0),
	fNextCodecID(1000)
{
}


FormatManager::~FormatManager()
{
}


/** This method is called when BMediaFormats asks for any updates
 *	made to our format list.
 *	If there were any changes since the last time, the whole
 *	list will be sent back.
 */

void 
FormatManager::GetFormats(BMessage &message)
{
	BAutolock locker(fLock);

	bigtime_t lastUpdate;
	if (message.FindInt64("last_timestamp", &lastUpdate) == B_OK
		&& lastUpdate >= fLastUpdate) {
		// there weren't any changes since last time
		BMessage reply;
		reply.AddBool("need_update", false);

		message.SendReply(&reply, (BHandler *)NULL, TIMEOUT);
		return;
	}

	// add all meta formats to the list

	BMessage reply;
	reply.AddBool("need_update", true);
	reply.AddInt64("timestamp", system_time());

	int32 count = fList.CountItems();
	printf("FormatManager::GetFormats(): put %ld formats into message\n", count);
	for (int32 i = 0; i < count; i++) {
		meta_format *format = fList.ItemAt(i);
		reply.AddData("formats", MEDIA_META_FORMAT_TYPE, format, sizeof(meta_format));
	}

	message.SendReply(&reply, (BHandler *)NULL, TIMEOUT);
}


status_t 
FormatManager::RegisterDecoder(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	return RegisterDescriptions(descriptions, descriptionCount, format, flags, false);
}


status_t 
FormatManager::RegisterEncoder(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags)
{
	return RegisterDescriptions(descriptions, descriptionCount, format, flags, true);
}


status_t 
FormatManager::RegisterDescriptions(const media_format_description *descriptions,
	int32 descriptionCount, media_format *format, uint32 flags, bool encoder)
{
	BAutolock locker(fLock);

	int32 codec = fNextCodecID++;
	fLastUpdate = system_time();

	switch (format->type) {
		case B_MEDIA_ENCODED_AUDIO:
			format->u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)codec;
			break;
		case B_MEDIA_ENCODED_VIDEO:
			format->u.encoded_video.encoding = (media_encoded_video_format::video_encoding)codec;
			break;
		case B_MEDIA_MULTISTREAM:
			format->u.multistream.format = codec;
			break;
		default:
			// ToDo: what can we do for the raw formats?
			break;
	}

	// ToDo: support "flags" (B_SET_DEFAULT, B_EXCLUSIVE, B_NO_MERGE)!

	for (int32 i = 0; i < descriptionCount; i++) {
		meta_format *metaFormat = new meta_format(descriptions[i], *format, codec);

		if (encoder)
			fEncoderFormats.BinaryInsert(metaFormat, meta_format::Compare);
		else
			fDecoderFormats.BinaryInsert(metaFormat, meta_format::Compare);

		fList.BinaryInsert(metaFormat, meta_format::Compare);
	}

	return B_OK;
}


void 
FormatManager::UnregisterDecoder(media_format &format)
{
	UNIMPLEMENTED();
}


void 
FormatManager::UnregisterEncoder(media_format &format)
{
	UNIMPLEMENTED();
}


void
FormatManager::LoadState()
{
}


void
FormatManager::SaveState()
{
}


const char *
family_to_string(media_format_family family)
{
	switch (family) {
		case B_ANY_FORMAT_FAMILY:
			return "any";
		case B_BEOS_FORMAT_FAMILY:
			return "BeOS";
		case B_QUICKTIME_FORMAT_FAMILY:
			return "Quicktime";
		case B_AVI_FORMAT_FAMILY:
			return "AVI";
		case B_ASF_FORMAT_FAMILY:
			return "ASF";
		case B_MPEG_FORMAT_FAMILY:
			return "MPEG";
		case B_WAV_FORMAT_FAMILY:
			return "WAV";
		case B_AIFF_FORMAT_FAMILY:
			return "AIFF";
		case B_AVR_FORMAT_FAMILY:
			return "AVR";
		case B_MISC_FORMAT_FAMILY:
			return "misc";
		default:
			return "unknown";
	}
}


const char *
string_for_description(const media_format_description &desc, char *string, size_t length)
{
	switch (desc.family) {
		case B_ANY_FORMAT_FAMILY:
			snprintf(string, length, "any format");
			break;
		case B_BEOS_FORMAT_FAMILY:
			snprintf(string, length, "BeOS format, format id 0x%lx", desc.u.beos.format);
			break;
		case B_QUICKTIME_FORMAT_FAMILY:
			snprintf(string, length, "Quicktime format, vendor id 0x%lx, codec id 0x%lx", desc.u.quicktime.vendor, desc.u.quicktime.codec);
			break;
		case B_AVI_FORMAT_FAMILY:
			snprintf(string, length, "AVI format, codec id 0x%lx", desc.u.avi.codec);
			break;
		case B_ASF_FORMAT_FAMILY:
			snprintf(string, length, "ASF format, GUID %02x %02x %02x %02x %02x %02x "
				"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
				desc.u.asf.guid.data[0], desc.u.asf.guid.data[1],
				desc.u.asf.guid.data[2], desc.u.asf.guid.data[3],
				desc.u.asf.guid.data[4], desc.u.asf.guid.data[5],
				desc.u.asf.guid.data[6], desc.u.asf.guid.data[7],
				desc.u.asf.guid.data[8], desc.u.asf.guid.data[9],
				desc.u.asf.guid.data[10], desc.u.asf.guid.data[11],
				desc.u.asf.guid.data[12], desc.u.asf.guid.data[13],
				desc.u.asf.guid.data[14], desc.u.asf.guid.data[15]);
			break;
		case B_MPEG_FORMAT_FAMILY:
			snprintf(string, length, "MPEG format, id 0x%lx", desc.u.mpeg.id);
			break;
		case B_WAV_FORMAT_FAMILY:
			snprintf(string, length, "WAV format, codec id 0x%lx", desc.u.wav.codec);
			break;
		case B_AIFF_FORMAT_FAMILY:
			snprintf(string, length, "AIFF format, codec id 0x%lx", desc.u.aiff.codec);
			break;
		case B_AVR_FORMAT_FAMILY:
			snprintf(string, length, "AVR format, id 0x%lx", desc.u.avr.id);
			break;
		case B_MISC_FORMAT_FAMILY:
			snprintf(string, length, "misc format, file-format id 0x%lx, codec id 0x%lx", desc.u.misc.file_format, desc.u.misc.codec);
			break;
		default:
			snprintf(string, length, "unknown format");
			break;
	}
	return string;
}
