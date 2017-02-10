/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <MediaFormats.h>

#include <stdio.h>
#include <string.h>


static const char *
get_family_string(media_format_family family)
{
	switch (family) {
		case B_BEOS_FORMAT_FAMILY:
			return "BeOS";
		case B_ANY_FORMAT_FAMILY:
			return "any";
		case B_QUICKTIME_FORMAT_FAMILY:
			return "QuickTime";
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
// these are OpenBeOS specific:
//		case B_OGG_FORMAT_FAMILY:
//			return "OGG";
//		case B_META_FORMAT_FAMILY:
//			return "meta";
		default:
			return "unknown";
	}
}


static const char *
get_mpeg_string(int32 id)
{
	switch (id) {
		case B_MPEG_ANY:
			return "any";
		case B_MPEG_1_AUDIO_LAYER_1:
			return "mpeg 1 audio layer 1";
		case B_MPEG_1_AUDIO_LAYER_2:
			return "mpeg 1 audio layer 2";
		case B_MPEG_1_AUDIO_LAYER_3:
			return "mpeg 1 audio layer 3 (mp3)";
		case B_MPEG_1_VIDEO:
			return "mpeg 1 video";
// these are OpenBeOS specific:
/*		case B_MPEG_2_AUDIO_LAYER_1:
			return "mpeg 2 audio layer 1";
		case B_MPEG_2_AUDIO_LAYER_2:
			return "mpeg 2 audio layer 2";
		case B_MPEG_2_AUDIO_LAYER_3:
			return "mpeg 2 audio layer 3";
		case B_MPEG_2_VIDEO:
			return "mpeg 2 video";
		case B_MPEG_2_5_AUDIO_LAYER_1:
			return "mpeg 2.5 audio layer 1";
		case B_MPEG_2_5_AUDIO_LAYER_2:
			return "mpeg 2.5 audio layer 2";
		case B_MPEG_2_5_AUDIO_LAYER_3:
			return "mpeg 2.5 audio layer 3";
*/		default:
			return "unknown";
	}
}


static void
print_fourcc(uint32 id)
{
	char string[5];
	for (int32 i = 0; i < 4; i++) {
		uint8 c = uint8((id >> (24 - i * 8)) & 0xff);
		if (c < ' ' || c > 0x7f)
			string[i] = '.';
		else
			string[i] = (char)c;
	}
	string[4] = '\0';
	printf("%s (0x%lx)\n", string, id);
}


void
dump_media_format_description(media_format_description &description)
{
	printf("media_format_description:\n");
	printf("\tfamily:\t%s\n", get_family_string(description.family));
	
	switch (description.family) {
		case B_BEOS_FORMAT_FAMILY:
			printf("\tformat:\t");
			print_fourcc(description.u.beos.format);
			break;
		case B_AVI_FORMAT_FAMILY:
		case B_AIFF_FORMAT_FAMILY:
		case B_WAV_FORMAT_FAMILY:
			printf("\tcodec:\t");
			print_fourcc(description.u.avi.codec);
			break;
		case B_AVR_FORMAT_FAMILY:
			printf("\tid:\t");
			print_fourcc(description.u.avr.id);
			break;
		case B_QUICKTIME_FORMAT_FAMILY:
			printf("\tcodec:\t");
			print_fourcc(description.u.quicktime.codec);
			printf("\tvendor:\t");
			print_fourcc(description.u.quicktime.vendor);
			break;
		case B_MISC_FORMAT_FAMILY:
			printf("\tcodec:\t\t");
			print_fourcc(description.u.misc.file_format);
			printf("\tfile format:\t");
			print_fourcc(description.u.misc.codec);
			break;
		case B_MPEG_FORMAT_FAMILY:
			printf("\ttype:\t%s\n", get_mpeg_string(description.u.mpeg.id));
			break;
		case B_ASF_FORMAT_FAMILY:
			// note, this is endian depended - you shouldn't do it this way for real...
			printf("\tguid:\t0x%Lx%Lx\n", *(uint64 *)&description.u.asf.guid.data[0], *(uint64 *)&description.u.asf.guid.data[8]);
		default:
			break;
	}
}


int
main(int argc, char **argv)
{
	BMediaFormats formats;

	status_t status = formats.InitCheck();
	if (status != B_OK) {
		fprintf(stderr, "BMediaFormats::InitCheck() failed: %s\n", strerror(status));
		return -1;
	}

	media_format format;
	status = formats.GetAVIFormatFor('DIVX', &format, B_MEDIA_ENCODED_VIDEO);
	if (status != B_OK) {
		fprintf(stderr, "BMediaFormats::GetAVIFormatFor() failed: %s\n", strerror(status));
		return -1;
	}
	
	media_format_description description;
	status = formats.GetCodeFor(format, B_AVI_FORMAT_FAMILY, &description);
	if (status != B_OK) {
		fprintf(stderr, "BMediaFormats::GetCodeFor() failed: %s\n", strerror(status));
		return -1;
	}
	dump_media_format_description(description);

	char desc[256];
	string_for_format(format, desc, sizeof(desc));
	printf("\tformat:\t%s\n", desc);

	status = formats.GetCodeFor(format, B_MPEG_FORMAT_FAMILY, &description);
	if (status == B_OK) {
		fprintf(stderr, "BMediaFormats::GetCodeFor() succeded with wrong family!\n");
		return -1;
	}

	puts("\n***** all supported formats *****");

	// Rewind() should only work when the formats object is locked
	status = formats.RewindFormats();
	if (status == B_OK) {
		fprintf(stderr, "BMediaFormats::RewindFormats() succeded unlocked!\n");
		return -1;
	}

	if (!formats.Lock()) {
		fprintf(stderr, "BMediaFormats::Lock() failed!\n");
		return -1;		
	}

	status = formats.RewindFormats();
	if (status != B_OK) {
		fprintf(stderr, "BMediaFormats::RewindFormats() failed: %s\n", strerror(status));
		return -1;
	}

	int32 count = 0;
	while ((status = formats.GetNextFormat(&format, &description)) == B_OK) {
		dump_media_format_description(description);
		string_for_format(format, desc, sizeof(desc));
		printf("\tformat:\t%s\n", desc);
		count++;
	}
	if (status != B_BAD_INDEX)
		fprintf(stderr, "BMediaFormats::GetNextFormat() failed: %s\n", strerror(status));

	printf("***** %ld supported formats *****\n", count);

	formats.Unlock();
	return 0;
}
