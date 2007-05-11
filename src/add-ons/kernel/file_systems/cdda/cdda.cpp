/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <bus/scsi/scsi_cmds.h>
#include <device/scsi.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define kFramesPerSecond 75
#define kFramesPerMinute (kFramesPerSecond * 60)

struct cdtext_pack_data {
	uint8	id;
	uint8	track;
	uint8	number;
	uint8	character_position : 4;
	uint8	block_number : 3;
	uint8	double_byte : 1;
	char	text[12];
	uint8	crc[2];
} _PACKED;


static const uint8 kMaxTracks = 0x63;
enum {
	kTrackID	= 0x80,
	kArtistID	= 0x81,
	kMessageID	= 0x85,
};

struct cdtext {
	char *artist;
	char *album;
	char *titles[kMaxTracks];
	char *artists[kMaxTracks];
	uint8 track_count;
	char *genre;
};


//	#pragma mark - string functions


bool
is_garbage(char c)
{
	return isspace(c) || c == '-' || c == '/' || c == '\\';
}


void
sanitize_string(char *string)
{
	if (string == NULL)
		return;

	// strip garbage at the start

	uint32 length = strlen(string);
	uint32 garbage = 0;
	while (is_garbage(string[garbage])) {
		garbage++;
	}

	length -= garbage;
	if (garbage)
		memmove(string, string + garbage, length + 1);

	// strip garbage from the end	

	while (length > 1 && isspace(string[length - 1])) {
		string[--length] = '\0';
	}
}


//! Finds the first occurrence of \a find in \a string, ignores case.
static char*
find_string(const char *string, const char *find)
{
	if (string == NULL || find == NULL)
		return NULL;

	char first = tolower(find[0]);
	if (first == '\0')
		return (char *)string;

	int32 findLength = strlen(find) - 1;
	find++;

	for (; string[0]; string++) {
		if (tolower(string[0]) != first)
			continue;
		if (strncasecmp(string + 1, find, findLength) == 0)
			return (char *)string;
	}

	return NULL;
}


void
cut_string(char *string, char *cut)
{
	if (string == NULL || cut == NULL)
		return;

	char *found = find_string(string, cut);
	if (found != NULL) {
		uint32 foundLength = strlen(found);
		uint32 cutLength = strlen(cut);
		memmove(found, found + cutLength, foundLength + 1 - cutLength);
	}
}


void
sanitize_album(cdtext &text)
{
	cut_string(text.album, text.artist);
	sanitize_string(text.album);

	if ((text.artist == NULL || !text.artist[0]) && text.album != NULL) {
		// try to extract artist from album
		char *space = strstr(text.album, "  ");
		if (space != NULL) {
			space[0] = '\0';
			text.artist = text.album;
			text.album = strdup(space + 2);

			sanitize_string(text.artist);
			sanitize_string(text.album);
		}
	}
}


void
sanitize_titles(cdtext &text)
{
	for (uint8 i = 0; i < text.track_count; i++) {
		cut_string(text.titles[i], "(Album Version)");
		sanitize_string(text.titles[i]);
		sanitize_string(text.artists[i]);
		if (!strcasecmp(text.artists[i], text.artist)) {
			// if the title artist is the same as the main artist, remove it
			free(text.artists[i]);
			text.artists[i] = NULL;
		}

		if (text.titles[i] != NULL && text.titles[i][0] == '\t' && i > 0)
			text.titles[i] = strdup(text.titles[i - 1]);
	}
}


bool
single_case(const char *string, bool &upper, bool &first)
{
	if (string == NULL)
		return true;

	while (string[0]) {
		while (!isalpha(string[0])) {
			string++;
		}

		if (first) {
			upper = isupper(string[0]) != 0;
			first = false;
		} else if ((isupper(string[0]) != 0) ^ upper)
			return false;

		string++;
	}

	return true;
}


void
capitalize_string(char *string)
{
	if (string == NULL)
		return;

	bool newWord = isalpha(string[0]) || isspace(string[0]);
	while (string[0]) {
		if (isalpha(string[0])) {
			if (newWord) {
				string[0] = toupper(string[0]);
				newWord = false;
			} else
				string[0] = tolower(string[0]);
		} else if (string[0] != '\'')
			newWord = true;

		string++;
	}
}


void
correct_case(cdtext &text)
{
	// check if all titles share a single case
	bool first = true;
	bool upper;
	if (!single_case(text.album, upper, first)
		|| !single_case(text.artist, upper, first))
		return;

	for (int32 i = 0; i < text.track_count; i++) {
		if (!single_case(text.titles[i], upper, first)
			|| !single_case(text.artists[i], upper, first))
			return;
	}

	// If we get here, everything has a single case; we fix that
	// and capitalize each word

	capitalize_string(text.album);
	capitalize_string(text.artist);
	for (int32 i = 0; i < text.track_count; i++) {
		capitalize_string(text.titles[i]);
		capitalize_string(text.artists[i]);
	}
}


//	#pragma mark - CD-Text


bool
is_string_id(uint8 id)
{
	return id >= kTrackID && id <= kMessageID;
}


bool
parse_pack_data(cdtext_pack_data *&pack, uint32 &packLeft,
	cdtext_pack_data *&lastPack, uint8 &id, uint8 &track, uint8 &state,
	char *buffer, size_t &length)
{
	if (packLeft < sizeof(cdtext_pack_data))
		return false;

	uint8 number = pack->number;
	size_t size = length;

	if (state != 0) {
		// we had a terminated string and a missing track
		track++;
		memcpy(buffer, lastPack->text + state, 12 - state);
		if (pack->track - track == 1)
			state = 0;
		else
			state += strnlen(buffer, 12 - state);
		return true;
	}

	id = pack->id;
	track = pack->track;
	buffer[0] = '\0';
	length = 0;

	size_t position = pack->character_position;
	if (position > 0 && lastPack != NULL) {
		memcpy(buffer, &lastPack->text[12 - position], position);
		length = position;
	}

	while (id == pack->id && track == pack->track) {
#if 1
		printf("%u.%u.%u, %u.%u.%u, ", pack->id, pack->track, pack->number,
			pack->double_byte, pack->block_number, pack->character_position);
		for (int32 i = 0; i < 12; i++) {
			if (isprint(pack->text[i]))
				putchar(pack->text[i]);
		}
		putchar('\n');
#endif
		if (is_string_id(id)) {
			// TODO: support double byte characters
			if (length + 12 < size) {
				memcpy(buffer + length, pack->text, 12);
				length += 12;
			}
		}

		packLeft -= sizeof(cdtext_pack_data);
		if (packLeft < sizeof(cdtext_pack_data))
			return false;

		lastPack = pack;
		number++;
		pack++;

		if (pack->number != number)
			return false;
	}

	if (id == pack->id) {
		length -= pack->character_position;
		if (length >= size)
			length = size - 1;
		buffer[length] = '\0';

		if (pack->track > lastPack->track + 1) {
			// there is a missing track
			for (int32 i = 0; i < 12; i++) {
				if (lastPack->text[i] == '\0') {
					state = i + (lastPack->double_byte ? 2 : 1);
					break;
				}
			}
		}
	}

	// TODO: convert text to UTF-8
	return true;
}


void
dump_cdtext(cdtext_pack_data *pack, size_t packLength)
{
	cdtext_pack_data *lastPack = NULL;
	char buffer[256];
	uint8 state = 0;
	cdtext text;

	memset(&text, 0, sizeof(cdtext));

	while (true) {
		size_t length = sizeof(buffer);
		uint8 id, track;

		if (!parse_pack_data(pack, packLength, lastPack, id, track,
				state, buffer, length))
			break;

		switch (id) {
			case kTrackID:
				if (track == 0) {
					if (text.album == NULL)
						text.album = strdup(buffer);
				} else if (track <= kMaxTracks) {
					if (text.titles[track - 1] == NULL)
						text.titles[track - 1] = strdup(buffer);
					if (track > text.track_count)
						text.track_count = track;
				}
				break;

			case kArtistID:
				if (track == 0) {
					if (text.artist == NULL)
						text.artist = strdup(buffer);
				} else if (track <= kMaxTracks) {
					if (text.artists[track - 1] == NULL)
						text.artists[track - 1] = strdup(buffer);
				}
				break;

			default:
				if (is_string_id(id))
					printf("UNKNOWN %u: \"%s\"\n", id, buffer);
				break;
		}
	}

	sanitize_string(text.artist);
	sanitize_album(text);
	sanitize_titles(text);
	correct_case(text);

	if (text.album)
		printf("Album:    \"%s\"\n", text.album);
	if (text.artist)
		printf("Artist:   \"%s\"\n", text.artist);
	for (uint8 i = 0; i < text.track_count; i++) {
		printf("Track %02u: \"%s\"%s%s%s\n", i + 1, text.titles[i],
			text.artists[i] ? " (" : "", text.artists[i] ? text.artists[i] : "",
			text.artists[i] ? ")" : "");
	}
}


void
dump_toc(scsi_toc_toc *toc)
{
	int32 numTracks = toc->last_track + 1 - toc->first_track;

	for (int32 i = 0; i < numTracks; i++) {
		scsi_toc_track& track = toc->tracks[i];
		scsi_cd_msf& next = toc->tracks[i + 1].start.time;
			// the last track is always lead-out
		scsi_cd_msf& start = toc->tracks[i].start.time;
		scsi_cd_msf length;

		uint64 diff = next.minute * kFramesPerMinute
			+ next.second * kFramesPerSecond + next.frame
			- start.minute * kFramesPerMinute
			- start.second * kFramesPerSecond - start.frame;
		length.minute = diff / kFramesPerMinute;
		length.second = (diff % kFramesPerMinute) / kFramesPerSecond;
		length.frame = diff % kFramesPerSecond;

		printf("%02u. %02u:%02u.%02u (length %02u:%02u.%02u)\n",
			track.track_number, start.minute, start.second, start.frame,
			length.minute, length.second, length.frame);
	}
}


status_t
read_table_of_contents(int fd, uint32 track, uint8 format, uint8 *buffer,
	size_t bufferSize)
{
	raw_device_command raw;
	uint8 senseData[1024];

	memset(&raw, 0, sizeof(raw_device_command));
	memset(senseData, 0, sizeof(senseData));
	memset(buffer, 0, bufferSize);

	scsi_cmd_read_toc &toc = *(scsi_cmd_read_toc*)&raw.command;
	toc.opcode = SCSI_OP_READ_TOC;
	toc.time = 1;
	toc.format = format;
	toc.track = track;
	toc.allocation_length = B_HOST_TO_BENDIAN_INT16(bufferSize);

	raw.command_length = 10;
	raw.flags = B_RAW_DEVICE_DATA_IN | B_RAW_DEVICE_REPORT_RESIDUAL
		| B_RAW_DEVICE_SHORT_READ_VALID;
	raw.scsi_status = 0;
	raw.cam_status = 0;
	raw.data = buffer;
	raw.data_length = bufferSize;
	raw.timeout = 10000000LL;	// 10 secs
	raw.sense_data = senseData;
	raw.sense_data_length = sizeof(senseData);

	if (ioctl(fd, B_RAW_DEVICE_COMMAND, &raw) == 0) {
		if (raw.scsi_status == 0 && raw.cam_status == 1) {
			puts("success!\n");
		} else {
			puts("failure!\n");
		}
	}

	return 0;
}


void
dump_block(const uint8 *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;)
	{
		int start = i;

		printf(prefix);
		for (; i < start+16; i++)
		{
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + 16; i++)
		{
			if (i < size)
			{
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			}
			else
				break;
		}
		printf("\n");
	}
}


int
main(int argc, char** argv)
{
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		return -1;

	uint8 buffer[1024];
	scsi_toc_general *header = (scsi_toc_general *)buffer;

	read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_TOC, buffer, sizeof(buffer));
	header->data_length = B_BENDIAN_TO_HOST_INT16(header->data_length);
	printf("TOC header %u, %d, %d\n", header->data_length, header->first, header->last);
	//dump_block(buffer, header->data_length + 2, "TOC");
	dump_toc((scsi_toc_toc *)buffer);

	read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_CD_TEXT, buffer, sizeof(buffer));
	read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_CD_TEXT, buffer, sizeof(buffer));
	header->data_length = B_BENDIAN_TO_HOST_INT16(header->data_length);
	printf("TEXT header %u, %d, %d\n", header->data_length, header->first, header->last);
	//dump_block(buffer, header->data_length + 2, "TEXT");

	dump_cdtext((cdtext_pack_data *)(buffer + 4), header->data_length - 2);

	close(fd);
}
