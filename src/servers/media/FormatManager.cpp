#include <stdio.h>
#include <MediaFormats.h>
#include <string.h>
#include "FormatManager.h"
#include "debug.h"

const char *family_to_string(media_format_family in_family);
const char *string_for_description(const media_format_description &in_description, char *string, size_t length);

FormatManager::FormatManager()
{
}

FormatManager::~FormatManager()
{
}
	
status_t
FormatManager::GetFormatForDescription(media_format *out_format,
									   const media_format_description &in_desc)
{
	char s[128];
	printf("GetFormatForDescription, description: %s\n",
		string_for_description(in_desc, s, sizeof(s)));

	memset(out_format, 0, sizeof(*out_format));

	if (in_desc.family == B_BEOS_FORMAT_FAMILY && in_desc.u.beos.format == B_BEOS_FORMAT_RAW_AUDIO)
		out_format->type = B_MEDIA_RAW_AUDIO;
	else {
		out_format->type = B_MEDIA_ENCODED_AUDIO;
		out_format->u.encoded_audio.encoding = (enum media_encoded_audio_format::audio_encoding) in_desc.family;
	}

	string_for_format(*out_format, s, sizeof(s));
	printf("                         encoding %d, format %s\n", (int)out_format->u.encoded_audio.encoding, s);
	return B_OK;
}

status_t
FormatManager::GetDescriptionForFormat(media_format_description *out_description,
									   const media_format &in_format,
									   media_format_family in_family)
{
	printf("GetDescriptionForFormat, requested %s family\n",
		family_to_string(in_family));
	char s[128];
	string_for_format(in_format, s, sizeof(s));
	printf("                         encoding %d, format %s\n", (int)in_format.u.encoded_audio.encoding, s);


	if (in_family != B_META_FORMAT_FAMILY)
		return B_ERROR;

	out_description->family = B_META_FORMAT_FAMILY;

	switch (in_format.type) {
	case B_MEDIA_RAW_AUDIO:
		strcpy(out_description->u.meta.description, "audiocodec/raw");
		break;
	case B_MEDIA_RAW_VIDEO:
		strcpy(out_description->u.meta.description, "videocodec/raw");
		break;
	case B_MEDIA_ENCODED_AUDIO:
		switch (in_format.u.encoded_audio.encoding) {
				
			case B_WAV_FORMAT_FAMILY:
			case B_MPEG_FORMAT_FAMILY:
				strcpy(out_description->u.meta.description, "audiocodec/mpeg1layer3");
				break;
	
			default:
				return B_ERROR;
		}
		break;
	case B_MEDIA_ENCODED_VIDEO:
		switch (in_format.u.encoded_video.encoding) {
				
			case B_MPEG_FORMAT_FAMILY:
				strcpy(out_description->u.meta.description, "videocodec/mpeg");
				break;
	
			default:
				return B_ERROR;
		}
		break;
	case B_MEDIA_UNKNOWN_TYPE:
		debugger("FormatManager::GetDescriptionForFormat");
		strcpy(out_description->u.meta.description, "unknown");
		break;
	default:
		debugger("FormatManager::GetDescriptionForFormat");
	}
	printf("                         description: %s\n",
		string_for_description(*out_description, s, sizeof(s)));
	return B_OK;
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
		case B_OGG_FORMAT_FAMILY:
			return "OGG";
		case B_MISC_FORMAT_FAMILY:
			return "misc";
		case B_META_FORMAT_FAMILY:
			return "meta";
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
		case B_OGG_FORMAT_FAMILY:
			snprintf(string, length, "OGG format");
			break;
		case B_MISC_FORMAT_FAMILY:
			snprintf(string, length, "misc format, file-format id 0x%lx, codec id 0x%lx", desc.u.misc.file_format, desc.u.misc.codec);
			break;
		case B_META_FORMAT_FAMILY:
			snprintf(string, length, "meta format, description %s", desc.u.meta.description);
			break;
		default:
			snprintf(string, length, "unknown format");
			break;
	}
	return string;
}
