#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include "MP3ReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

/* see
 * http://www.multiweb.cz/twoinches/MP3inside.htm
 * http://www.id3.org/id3v2.3.0.txt
 * http://www.id3.org/lyrics3.html
 * http://www.id3.org/lyrics3200.html
 */

// bit_rate_table[mpeg_version_index][layer_index][bitrate_index]
static const int bit_rate_table[4][4][16] =
{
	{ // mpeg version 2.5
		{ }, // undefined layer
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, // layer 3
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 }, // layer 2
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 } // layer 1
	},
	{ // undefined version
		{ },
		{ },
		{ },
		{ }
	},
	{ // mpeg version 2
		{ },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }
	},
	{ // mpeg version 1
		{ },
		{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 },
		{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
		{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}
	}
};

// sampling_rate_table[mpeg_version_index][sampling_rate_index]
static const int sampling_rate_table[4][4] =
{
	{ 11025, 12000, 8000, 0},	// mpeg version 2.5
	{ 0, 0, 0, 0 },
	{ 22050, 24000, 16000, 0},	// mpeg version 2
	{ 44100, 48000, 32000, 0}	// mpeg version 1
};


struct mp3data
{
	int64	position;
	char *	chunkBuffer;
	
};

mp3Reader::mp3Reader()
{
	TRACE("mp3Reader::mp3Reader\n");
}

mp3Reader::~mp3Reader()
{
}
      
const char *
mp3Reader::Copyright()
{
	return "mp3 reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
mp3Reader::Sniff(int32 *streamCount)
{
	TRACE("mp3Reader::Sniff\n");
	
	fSeekableSource = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!fSeekableSource) {
		TRACE("mp3Reader::Sniff: non seekable sources not supported\n");
		return B_ERROR;
	}

	fFileSize = Source()->Seek(0, SEEK_END);

	TRACE("mp3Reader::Sniff: file size is %Ld bytes\n", fFileSize);
	
	if (!IsMp3File()) {
		TRACE("mp3Reader::Sniff: non recognized as mp3 file\n");
		return B_ERROR;
	}

	TRACE("mp3Reader::Sniff: looks like an mp3 file\n");
	
	if (!ParseFile()) {
		TRACE("mp3Reader::Sniff: parsing file failed\n");
		return B_ERROR;
	}
	

	//fDataSize = Source()->Seek(0, SEEK_END);
	//if (sizeof(fRawHeader) != Source()->ReadAt(0, &fRawHeader, sizeof(fRawHeader))) {

	*streamCount = 1;
	return B_OK;
}


status_t
mp3Reader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("mp3Reader::AllocateCookie\n");

	mp3data *data = new mp3data;
/*
	data->position = 0;
	data->datasize = fDataSize;
	data->framesize = (B_LENDIAN_TO_HOST_INT16(fRawHeader.common.bits_per_sample) / 8) * B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->fps = B_LENDIAN_TO_HOST_INT32(fRawHeader.common.samples_per_sec) / B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->buffersize = (65536 / data->framesize) * data->framesize;
	data->buffer = malloc(data->buffersize);
	data->framecount = fDataSize / data->framesize;
	data->duration = (data->framecount * 1000000LL) / data->fps;

	TRACE(" framesize %ld\n", data->framesize);
	TRACE(" fps %ld\n", data->fps);
	TRACE(" buffersize %ld\n", data->buffersize);
	TRACE(" framecount %Ld\n", data->framecount);
	TRACE(" duration %Ld\n", data->duration);

	memset(&data->format, 0, sizeof(data->format));
	data->format.type = B_MEDIA_RAW_AUDIO;
	data->format.u.raw_audio.frame_rate = data->fps;
	data->format.u.raw_audio.channel_count = B_LENDIAN_TO_HOST_INT16(fRawHeader.common.channels);
	data->format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT; // XXX fixme
	data->format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	data->format.u.raw_audio.buffer_size = data->buffersize;
*/	
	// store the cookie
	*cookie = data;
	return B_OK;
}


status_t
mp3Reader::FreeCookie(void *cookie)
{
	TRACE("mp3Reader::FreeCookie\n");
	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*
	free(data->buffer);
*/
	delete data;
	
	return B_OK;
}


status_t
mp3Reader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*	
	*frameCount = data->framecount;
	*duration = data->duration;
	*format = data->format;
	*infoBuffer = 0;
	*infoSize = 0;
*/
	return B_OK;
}


status_t
mp3Reader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	if (!fSeekableSource)
		return B_ERROR;

//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
/*
	uint64 pos;
	
	if (frame) {
		pos = *frame * data->framesize;
		TRACE("mp3Reader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
	} else if (time) {
		pos = (*time * data->fps) / 1000000LL;
		TRACE("mp3Reader::Seek to time %Ld, pos %Ld\n", *time, pos);
		*time = (pos * 1000000LL) / data->fps;
		TRACE("mp3Reader::Seek newtime %Ld\n", *time);
	} else {
		return B_ERROR;
	}
	
	if (pos < 0 || pos > data->datasize) {
		TRACE("mp3Reader::Seek invalid position %Ld\n", pos);
		return B_ERROR;
	}
	
	data->position = pos;
*/
	return B_OK;
}


status_t
mp3Reader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
//	mp3data *data = reinterpret_cast<mp3data *>(cookie);
	status_t result = B_OK;
/*
	int32 readsize = data->datasize - data->position;
	if (readsize > data->buffersize) {
		readsize = data->buffersize;
		result = B_LAST_BUFFER_ERROR;
	}
		
	if (readsize != Source()->ReadAt(44 + data->position, data->buffer, readsize)) {
		TRACE("mp3Reader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	
	if (mediaHeader) {
		// memset(mediaHeader, 0, sizeof(*mediaHeader));
		// mediaHeader->type = B_MEDIA_RAW_AUDIO;
		// mediaHeader->size_used = buffersize;
		// mediaHeader->start_time = ((data->position / data->framesize) * 1000000LL) / data->fps;
		// mediaHeader->file_pos = 44 + data->position;
	}
	
	data->position += readsize;
	*chunkBuffer = data->buffer;
	*chunkSize = readsize;
*/
	return result;
}

bool
mp3Reader::ParseFile()
{
	// Since we already know that this is an mp3 file,
	// detect the real (mp3 audio) data start and end
	// and find VBR or other headers and tags

	const int32 search_size = 16384;
	const int32 padding_size = 128; // Get???Length() functions need some bytes to look into
	int64	offset;
	int32	size;
	uint8	buf[search_size];

	fDataStart = -1;

	for (offset = 0; offset < fFileSize; )  {
		int64 maxsize = fFileSize - offset;
		size = (search_size < maxsize) ? search_size : maxsize;

		if (size != Source()->ReadAt(offset, buf, size)) {
			TRACE("mp3ReaderPlugin::ParseFile reading %ld bytes at offset %Ld failed\n", size, offset);
			return false;
		}

		int skip_bytes = 0;

		// since the various Get???Length() functions need to check a few bytes
		// (10 for ID3V2, about 40 for VBR), we stop searching before buffer end
		int32 end = size - padding_size;
		for (int32 pos = 0; pos < end; ) {
			int hdr_length;
			
			// A Xing or Fraunhofer VBR header is embedded into a valid
			// mp3 frame that contains silence. We need to first check
			// for these headers before we can search for the start of a stream.
			
			hdr_length = GetXingVbrLength(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a Xing VBR header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				goto skip_header;
			}

			hdr_length = GetFraunhoferVbrLength(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a Fraunhofer VBR header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				goto skip_header;
			}
			
			hdr_length = GetLameVbrLength(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a Lame VBR header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				goto skip_header;
			}
			
			hdr_length = GetId3v2Length(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a ID3V2 header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				goto skip_header;
			}
		
			if (IsValidStream(&buf[pos], size - pos)) {
				fDataStart = offset + pos;
				break;
			}
			
			pos++;
			continue;
			
			skip_header:
				int skip_max = end - pos;
				skip_bytes = (skip_max < hdr_length) ? skip_max : hdr_length;
				pos += skip_bytes;
				skip_bytes = hdr_length - skip_bytes;
		}
		if (fDataStart != -1)
			break;

		if (skip_bytes) {
			offset += skip_bytes;
			skip_bytes = 0;
		} else {
			offset += (search_size - padding_size);
		}
	}

	fDataSize = fFileSize - fDataStart;
	
	TRACE("found mp3 audio data at file position %Ld, maximum data length is %Ld\n", fDataStart, fDataSize);

	// search for a ID3 V1 tag
	offset = fFileSize - 128;
	size = 128;
	if (offset > 0) {
		if (size != Source()->ReadAt(offset, buf, size)) {
			TRACE("mp3ReaderPlugin::ParseFile reading %ld bytes at offset %Ld failed\n", size, offset);
			return false;
		}
		if (buf[0] == 'T'&& buf[1] == 'A' && buf[2] == 'G') {
			TRACE("mp3ReaderPlugin::ParseFile found a ID3V1 header of 128 bytes at position %Ld\n", offset);
			fDataSize -= 128;
		}
	}

	// search for a lyrics tag
	// maximum size is 5100 bytes, and a 128 byte ID3V1 tag is always appended
	// starts with "LYRICSBEGIN", end with "LYRICS200" or "LYRICSEND"
	offset = fFileSize - 5300;
	size = 5300;
	if (offset < 0) {
		offset = 0;
		size = fFileSize;
	}
	if (size != Source()->ReadAt(offset, buf, size)) {
		TRACE("mp3ReaderPlugin::ParseFile reading %ld bytes at offset %Ld failed\n", size, offset);
		return false;
	}
	for (int pos = 0; pos < size; pos++) {
		if (buf[pos] != 'L')
			continue;
		if (0 == memcmp(&buf[pos], "LYRICSBEGIN", 11)) {
			TRACE("mp3ReaderPlugin::ParseFile found a Lyrics header at position %Ld\n", offset + pos);
			fDataSize = fDataStart - offset + pos;
		}
	}
	
	// might search for APE tags, too

	TRACE("found mp3 audio data at file position %Ld, data length is %Ld\n", fDataStart, fDataSize);
	
	return true;
}

int
mp3Reader::GetXingVbrLength(uint8 *header)
{
	int h_id	= (header[1] >> 3) & 1;
	int h_mode	= (header[3] >> 6) & 3;
	uint8 *xing_header;
	
	// determine offset of header
	if(h_id) // mpeg1
		xing_header = (h_mode != 3) ? (header + 36) : (header + 21);
	else	 // mpeg2
		xing_header = (h_mode != 3) ? (header + 21) : (header + 13);

	// note: in CBR files, there is an 'Info' tag of the same format
	if (xing_header[0] != 'X') return -1;
	if (xing_header[1] != 'i') return -1;
	if (xing_header[2] != 'n') return -1;
	if (xing_header[3] != 'g') return -1;

	return GetFrameLength(header);
}

int
mp3Reader::GetLameVbrLength(uint8 *header)
{
	return -1;
}

int
mp3Reader::GetFraunhoferVbrLength(uint8 *header)
{
	if (header[0] != 0xff) return -1;
	if (header[36] != 'V') return -1;
	if (header[37] != 'B') return -1;
	if (header[38] != 'R') return -1;
	if (header[39] != 'I') return -1;

	return GetFrameLength(header);
}

int
mp3Reader::GetId3v2Length(uint8 *buffer)
{
	if ((buffer[0] == 'I') && /* magic */
	    (buffer[1] == 'D') &&
	    (buffer[2] == '3') &&
	    (buffer[3] != 0xff) && (buffer[4] != 0xff) && /* version */
	    /* flags */
	    (!(buffer[6] & 0x80)) && (!(buffer[7] & 0x80)) && /* the MSB in each byte in size is 0, to avoid */
	    (!(buffer[8] & 0x80)) && (!(buffer[9] & 0x80))) { /* making a buggy mpeg header */
		return ((buffer[6] << 21)|(buffer[7] << 14)|(buffer[8] << 7)|(buffer[9])) + 10;
	}
	return B_ENTRY_NOT_FOUND;
}

bool
mp3Reader::IsMp3File()
{
	// To detect an mp3 file, we seek into the middle,
	// and search for a valid sequence of 3 frame headers.
	// A mp3 frame has a maximum length of 2881 bytes, we
	// load a block of 16kB and use it to search.

	const int32 search_size = 16384;
	int64	offset;
	int32	size;
	uint8	buf[search_size];

	size = search_size;
	offset = fFileSize / 2 - search_size / 2;
	if (size > fFileSize) {
		size = fFileSize;
		offset = 0;
	}

	TRACE("searching for mp3 frame headers at %Ld in %ld bytes\n", offset, size);
	
	if (size != Source()->ReadAt(offset, buf, size)) {
		TRACE("mp3ReaderPlugin::IsMp3File reading %ld bytes at offset %Ld failed\n", size, offset);
		return false;
	}

	int32 end = size - 4;
	for (int32 pos = 0; pos < end; pos++) {
		if (buf[pos] != 0xff) // quick check
			continue;
		if (IsValidStream(&buf[pos], size - pos))
			return true;
	}
	return false;
}

bool
mp3Reader::IsValidStream(uint8 *buffer, int size)
{
	// check 3 consecutive frame headers to make sure
	// that the length encoded in the header is correct,
	// and also that mpeg version and layer do not change
	int length1 = GetFrameLength(buffer);
	if (length1 < 0 || (length1 + 4) > size)
		return false;
	int version_index1 = (buffer[1] >> 3) & 0x03;
	int layer_index1 = (buffer[1] >> 1) & 0x03;
	int length2 = GetFrameLength(buffer + length1);
	if (length2 < 0 || (length1 + length2 + 4) > size)
		return false;
	int version_index2 = (buffer[length1 + 1] >> 3) & 0x03;
	int layer_index2 = (buffer[length1 + 1] >> 1) & 0x03;
	if (version_index1 != version_index2 || layer_index1 != layer_index1)
		return false;
	int length3 = GetFrameLength(buffer + length1 + length2);
	if (length3 < 0)
		return false;
	int version_index3 = (buffer[length1 + length2 + 1] >> 3) & 0x03;
	int layer_index3 = (buffer[length1 + length2 + 1] >> 1) & 0x03;
	if (version_index2 != version_index3 || layer_index2 != layer_index3)
		return false;
	return true;
}

int
mp3Reader::GetFrameLength(void *header)
{
	uint8 *h = (uint8 *)header;

	if (h[0] != 0xff)
		return -1;
	if ((h[1] & 0xe0) != 0xe0)
		return -1;
	
	int mpeg_version_index = (h[1] >> 3) & 0x03;
	int layer_index = (h[1] >> 1) & 0x03;
	int no_crc = (h[1] & 0x01);
	/* VBR files seem to use CRC in info frames, probably to make the
	 * decoder discard them with a wrong CRC...
	 */
	int bitrate_index = (h[2] >> 4) & 0x0f;
	int sampling_rate_index = (h[2] >> 2) & 0x03;
	int padding = (h[2] >> 1) & 0x01;
	/* no interested in the other bits */
	
	int bitrate = bit_rate_table[mpeg_version_index][layer_index][bitrate_index];
	int samplingrate = sampling_rate_table[mpeg_version_index][sampling_rate_index];
	
	if (!bitrate || !samplingrate)
		return -1;	

	int length;	
	if (layer_index == 3) // layer 1
		length = ((144 * 1000 * bitrate) / samplingrate) + (padding * 4);
	else // layer 2 & 3
		length = ((144 * 1000 * bitrate) / samplingrate) + padding;
	
	TRACE("%s %s, %s crc, bit rate %d, sampling rate %d, padding %d, frame length %d\n",
		mpeg_version_index == 0 ? "mpeg 2.5" : (mpeg_version_index == 2 ? "mpeg 2" : "mpeg 1"),
		layer_index == 3 ? "layer 1" : (layer_index == 2 ? "layer 2" : "layer 3"),
		no_crc ? "no" : "has",
		bitrate, samplingrate, padding, length);
	
	return length;
}

Reader *
mp3ReaderPlugin::NewReader()
{
	return new mp3Reader;
}


MediaPlugin *
instantiate_plugin()
{
	return new mp3ReaderPlugin;
}
