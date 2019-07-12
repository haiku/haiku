/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "cdda.h"

#include <KernelExport.h>
#include <device/scsi.h>

#include <algorithm>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


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

enum {
	kTrackID	= 0x80,
	kArtistID	= 0x81,
	kMessageID	= 0x85,
};

static const uint32 kBufferSize = 16384;
static const uint32 kSenseSize = 1024;


//	#pragma mark - string functions


static char *
copy_string(const char *string)
{
	if (string == NULL || !string[0])
		return NULL;

	return strdup(string);
}


static char *
to_utf8(const char* string)
{
	char buffer[256];
	size_t out = 0;

	// TODO: assume CP1252 or ISO-8859-1 character set for now
	while (uint32 c = (uint8)string[0]) {

		if (c < 0x80) {
			if (out >= sizeof(buffer) - 1)
				break;
			// ASCII character: no change needed
			buffer[out++] = c;
		} else {
			if (c < 0xA0) {
				// Windows CP-1252 - Use a lookup table
				static const uint32 lookup[] = {
					0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
					0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
					0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
					0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
				};

				c = lookup[c - 0x80];
			}

			// Convert to 2 or 3-byte representation
			if (c == 0) {
				// invalid character, ignore
			} else if (c < 0x800) {
				if (out >= sizeof(buffer) - 2)
					break;
				buffer[out++] = 0xc0 | (c >> 6);
				buffer[out++] = 0x80 | (c & 0x3f);
			} else {
				if (out >= sizeof(buffer) - 3)
					break;
				buffer[out++] = 0xe0 | (c >> 12);
				buffer[out++] = 0x80 | ((c >> 6) & 0x3f);
				buffer[out++] = 0x80 | (c & 0x3f);
			}
		}

		string++;
	}
	buffer[out++] = '\0';

	char *copy = (char *)malloc(out);
	if (copy == NULL)
		return NULL;

	memcpy(copy, buffer, out);
	return copy;
}


static bool
is_garbage(char c)
{
	return isspace(c) || c == '-' || c == '/' || c == '\\';
}


static void
sanitize_string(char *&string)
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

	if (!string[0]) {
		// free string if it's empty
		free(string);
		string = NULL;
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


static void
cut_string(char *string, const char *cut)
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


static void
sanitize_album(cdtext &text)
{
	cut_string(text.album, text.artist);
	sanitize_string(text.album);

	if (text.album != NULL && !strcasecmp(text.album, "My CD")) {
		// don't laugh, people really do that!
		free(text.album);
		text.album = NULL;
	}

	if ((text.artist == NULL || text.artist[0] == '\0') && text.album != NULL) {
		// try to extract artist from album
		char *space = strstr(text.album, "  ");
		if (space != NULL) {
			space[0] = '\0';
			text.artist = text.album;
			text.album = copy_string(space + 2);

			sanitize_string(text.artist);
			sanitize_string(text.album);
		}
	}
}


static void
sanitize_titles(cdtext &text)
{
	for (uint8 i = 0; i < text.track_count; i++) {
		cut_string(text.titles[i], "(Album Version)");
		sanitize_string(text.titles[i]);
		sanitize_string(text.artists[i]);

		if (text.artists[i] != NULL && text.artist != NULL
			&& !strcasecmp(text.artists[i], text.artist)) {
			// if the title artist is the same as the main artist, remove it
			free(text.artists[i]);
			text.artists[i] = NULL;
		}

		if (text.titles[i] != NULL && text.titles[i][0] == '\t' && i > 0)
			text.titles[i] = copy_string(text.titles[i - 1]);
	}
}


static bool
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


static void
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


static void
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


cdtext::cdtext()
	:
	artist(NULL),
	album(NULL),
	genre(NULL),
	track_count(0)
{
	memset(titles, 0, sizeof(titles));
	memset(artists, 0, sizeof(artists));
}


cdtext::~cdtext()
{
	free(album);
	free(artist);
	free(genre);

	for (uint8 i = 0; i < track_count; i++) {
		free(titles[i]);
		free(artists[i]);
	}
}


static bool
is_string_id(uint8 id)
{
	return id >= kTrackID && id <= kMessageID;
}


/*!	Parses a \a pack data into the provided text buffer; the corresponding
	track number will be left in \a track, and the type of the data in \a id.
	The pack data is explained in SCSI MMC-3.

	\a id, \a track, and \a state must stay constant between calls to this
	function. \a state must be initialized to zero for the first call.
*/
static bool
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
#if 0
		dprintf("%u.%u.%u, %u.%u.%u, ", pack->id, pack->track, pack->number,
			pack->double_byte, pack->block_number, pack->character_position);
		for (int32 i = 0; i < 12; i++) {
			if (isprint(pack->text[i]))
				dprintf("%c", pack->text[i]);
			else
				dprintf("-");
		}
		dprintf("\n");
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

	return true;
}


static void
dump_cdtext(cdtext &text)
{
	if (text.album)
		dprintf("Album:    \"%s\"\n", text.album);
	if (text.artist)
		dprintf("Artist:   \"%s\"\n", text.artist);
	for (uint8 i = 0; i < text.track_count; i++) {
		dprintf("Track %02u: \"%s\"%s%s%s\n", i + 1, text.titles[i],
			text.artists[i] ? " (" : "", text.artists[i] ? text.artists[i] : "",
			text.artists[i] ? ")" : "");
	}
}


static void
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

		dprintf("%02u. %02u:%02u.%02u (length %02u:%02u.%02u)\n",
			track.track_number, start.minute, start.second, start.frame,
			length.minute, length.second, length.frame);
	}
}


static status_t
read_frames(int fd, off_t firstFrame, uint8 *buffer, size_t count)
{
	size_t framesLeft = count;

	while (framesLeft > 0) {
		// If the initial count was >= 32, and not a multiple of 8, and the
		// ioctl fails, we switch to reading 8 frames at a time. However the
		// last read can read between 1 and 7 frames only, to not overflow
		// the buffer.
		count = std::min(count, framesLeft);

		scsi_read_cd read;
		read.start_m = firstFrame / kFramesPerMinute;
		read.start_s = (firstFrame / kFramesPerSecond) % 60;
		read.start_f = firstFrame % kFramesPerSecond;

		read.length_m = count / kFramesPerMinute;
		read.length_s = (count / kFramesPerSecond) % 60;
		read.length_f = count % kFramesPerSecond;

		read.buffer_length = count * kFrameSize;
		read.buffer = (char *)buffer;
		read.play = false;

		if (ioctl(fd, B_SCSI_READ_CD, &read, sizeof(scsi_read_cd)) < 0) {
			// drive couldn't read data - try again to read with a smaller block size
			if (count == 1)
				return errno;

			if (count >= 32)
				count = 8;
			else
				count = 1;

			continue;
		}

		buffer += count * kFrameSize;
		framesLeft -= count;
		firstFrame += count;
	}

	return B_OK;
}


static status_t
read_table_of_contents(int fd, uint32 track, uint8 format, uint8 *buffer,
	size_t bufferSize)
{
	raw_device_command raw;
	uint8 *senseData = (uint8 *)malloc(kSenseSize);
	if (senseData == NULL)
		return B_NO_MEMORY;

	memset(&raw, 0, sizeof(raw_device_command));
	memset(senseData, 0, kSenseSize);
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
	raw.sense_data_length = sizeof(kSenseSize);

	if (ioctl(fd, B_RAW_DEVICE_COMMAND, &raw, sizeof(raw)) == 0
		&& raw.scsi_status == 0 && raw.cam_status == 1) {
		free(senseData);
		return B_OK;
	}

	free(senseData);
	return B_ERROR;
}


//	#pragma mark - exported functions


status_t
read_cdtext(int fd, struct cdtext &cdtext)
{
	uint8 *buffer = (uint8 *)malloc(kBufferSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	// do it twice, just in case...
	// (at least my CD-ROM sometimes returned broken data on first try)
	read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_CD_TEXT, buffer,
		kBufferSize);
	if (read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_CD_TEXT, buffer,
			kBufferSize) != B_OK) {
		free(buffer);
		return B_ERROR;
	}

	scsi_toc_general *header = (scsi_toc_general *)buffer;

	uint32 packLength = B_BENDIAN_TO_HOST_INT16(header->data_length) - 2;
	cdtext_pack_data *pack = (cdtext_pack_data *)(header + 1);
	cdtext_pack_data *lastPack = NULL;
	uint8 state = 0;
	uint8 track = 0;
	uint8 id = 0;
	char text[256];

	// TODO: determine encoding!

	while (true) {
		size_t length = sizeof(text);

		if (!parse_pack_data(pack, packLength, lastPack, id, track,
				state, text, length))
			break;

		switch (id) {
			case kTrackID:
				if (track == 0) {
					if (cdtext.album == NULL)
						cdtext.album = to_utf8(text);
				} else if (track <= kMaxTracks) {
					if (cdtext.titles[track - 1] == NULL)
						cdtext.titles[track - 1] = to_utf8(text);
					if (track > cdtext.track_count)
						cdtext.track_count = track;
				}
				break;

			case kArtistID:
				if (track == 0) {
					if (cdtext.artist == NULL)
						cdtext.artist = to_utf8(text);
				} else if (track <= kMaxTracks) {
					if (cdtext.artists[track - 1] == NULL)
						cdtext.artists[track - 1] = to_utf8(text);
				}
				break;

			default:
				if (is_string_id(id))
					dprintf("UNKNOWN %u: \"%s\"\n", id, text);
				break;
		}
	}

	free(buffer);

	if (cdtext.artist == NULL && cdtext.album == NULL)
		return B_ERROR;

	for (int i = 0; i < cdtext.track_count; i++) {
		if (cdtext.titles[i] == NULL)
			return B_ERROR;
	}

	sanitize_string(cdtext.artist);
	sanitize_album(cdtext);
	sanitize_titles(cdtext);
	correct_case(cdtext);

	dump_cdtext(cdtext);
	return B_OK;
}


status_t
read_table_of_contents(int fd, scsi_toc_toc *toc, size_t length)
{
	status_t status = read_table_of_contents(fd, 1, SCSI_TOC_FORMAT_TOC,
		(uint8*)toc, length);
	if (status < B_OK)
		return status;

	// make sure the values in the TOC make sense

	int32 lastTrack = toc->last_track + 1 - toc->first_track;
	size_t dataLength = B_BENDIAN_TO_HOST_INT16(toc->data_length) + 2;
	if (dataLength < sizeof(scsi_toc_toc) || lastTrack <= 0)
		return B_BAD_DATA;

	if (length > dataLength)
		length = dataLength;

	length -= sizeof(scsi_toc_general);

	if (lastTrack * sizeof(scsi_toc_track) > length)
		toc->last_track = length / sizeof(scsi_toc_track) + toc->first_track;

	dump_toc(toc);
	return B_OK;
}


status_t
read_cdda_data(int fd, off_t endFrame, off_t offset, void *data, size_t length,
	off_t bufferOffset, void *buffer, size_t bufferSize)
{
	if (bufferOffset >= 0 && bufferOffset <= offset + (off_t)length
		&& bufferOffset + (off_t)bufferSize > offset) {
		if (offset >= bufferOffset) {
			// buffer reaches into the beginning of the request
			off_t dataOffset = offset - bufferOffset;
			size_t bytes = min_c(bufferSize - dataOffset, length);
			if (user_memcpy(data, (uint8 *)buffer + dataOffset, bytes) < B_OK)
				return B_BAD_ADDRESS;

			data = (void *)((uint8 *)data + bytes);
			length -= bytes;
			offset += bytes;
		} else if (offset < bufferOffset
			&& offset + length < bufferOffset + bufferSize) {
			// buffer overlaps at the end of the request
			off_t dataOffset = bufferOffset - offset;
			size_t bytes = length - dataOffset;
			if (user_memcpy((uint8 *)data + dataOffset, buffer, bytes) < B_OK)
				return B_BAD_ADDRESS;

			length -= bytes;
		}
		// we don't handle the case where we would need to split the request
	}

	while (length > 0) {
		off_t frame = offset / kFrameSize;
		uint32 count = bufferSize / kFrameSize;
		if (frame + count > endFrame)
			count = endFrame - frame;

		status_t status = read_frames(fd, frame, (uint8 *)buffer, count);
		if (status < B_OK)
			return status;

		off_t dataOffset = offset % kFrameSize;
		size_t bytes = bufferSize - dataOffset;
		if (bytes > length)
			bytes = length;

		if (user_memcpy(data, (uint8 *)buffer + dataOffset, bytes) < B_OK)
			return B_BAD_ADDRESS;

		data = (void *)((uint8 *)data + bytes);
		length -= bytes;
		offset += bytes;
	}

	return B_OK;
}
