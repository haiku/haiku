#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "MP3ReaderPlugin.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE ((void)0)
#endif

/* see
 * http://www.dv.co.yu/mpgscript/mpeghdr.htm
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

// b_mpeg_id_table[mpeg_version_index][layer_index]
static const int32 b_mpeg_id_table[4][4] =
{
	{ 0, B_MPEG_2_5_AUDIO_LAYER_3, B_MPEG_2_5_AUDIO_LAYER_2, B_MPEG_2_5_AUDIO_LAYER_1 },
	{ 0, 0, 0, 0 },
	{ 0, B_MPEG_2_AUDIO_LAYER_3, B_MPEG_2_AUDIO_LAYER_2, B_MPEG_2_AUDIO_LAYER_1 },
	{ 0, B_MPEG_1_AUDIO_LAYER_3, B_MPEG_1_AUDIO_LAYER_2, B_MPEG_1_AUDIO_LAYER_1 },
};

// frame_rate_table[mpeg_version_index][sampling_rate_index]
static const int frame_rate_table[4][4] =
{
	{ 11025, 12000, 8000, 0},	// mpeg version 2.5
	{ 0, 0, 0, 0 },
	{ 22050, 24000, 16000, 0},	// mpeg version 2
	{ 44100, 48000, 32000, 0}	// mpeg version 1
};

// frame_sample_count_table[layer_index]
static const int frame_sample_count_table[4] = { 0, 1152, 1152, 384 };

static const int MAX_CHUNK_SIZE = 5200;

struct mp3data
{
	int64	position;
	char *	chunkBuffer;

	int64	duration;	// usec
	int64	frameCount; // PCM frames
	int32	frameRate;
	
	media_format format;
};

struct mp3Reader::xing_vbr_info
{
	int32	frameRate;
	int64	encodedFramesCount;
	int64	byteCount;
	int32	vbrScale;
	bool	hasSeekpoints;
	uint8	seekpoints[100];

	int64	duration;	// usec
	int64	frameCount; // PCM frames
};

struct mp3Reader::fhg_vbr_info
{
};

mp3Reader::mp3Reader()
 :	fXingVbrInfo(0),
	fFhgVbrInfo(0)
{
	TRACE("mp3Reader::mp3Reader\n");
}

mp3Reader::~mp3Reader()
{
	delete fXingVbrInfo;
	delete fFhgVbrInfo;
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
	
	*streamCount = 1;
	return B_OK;
}


status_t
mp3Reader::AllocateCookie(int32 streamNumber, void **cookie)
{
	TRACE("mp3Reader::AllocateCookie\n");

	mp3data *data = new mp3data;
	data->chunkBuffer = new char[MAX_CHUNK_SIZE];
	data->position = 0;

	uint8 header[4];
	Source()->ReadAt(fDataStart, header, sizeof(header));
	int mpeg_version_index = (header[1] >> 3) & 0x03;
	int layer_index = (header[1] >> 1) & 0x03;
	
	int bit_rate;
	int frame_size;
	
	if (fXingVbrInfo && fXingVbrInfo->frameCount != -1 && fXingVbrInfo->duration != -1) {
		TRACE("mp3Reader::AllocateCookie: using timing info from VBR header\n");
		bit_rate = (fXingVbrInfo->byteCount * 8 * 1000000) / fXingVbrInfo->duration; // average bit rate
		frame_size = fXingVbrInfo->byteCount / fXingVbrInfo->encodedFramesCount;	 // average frame size
		data->duration = fXingVbrInfo->duration;
		data->frameCount = fXingVbrInfo->frameCount;
		data->frameRate = fXingVbrInfo->frameRate;
	} else {
		TRACE("mp3Reader::AllocateCookie: assuming CBR, calculating timing info from file\n");
		int sampling_rate_index = (header[2] >> 2) & 0x03;
		int bitrate_index = (header[2] >> 4) & 0x0f;
		int samples_per_chunk = frame_sample_count_table[layer_index];
		bit_rate = 1000 * bit_rate_table[mpeg_version_index][layer_index][bitrate_index];
		frame_size = GetFrameLength(header);
		data->frameRate = frame_rate_table[mpeg_version_index][sampling_rate_index];
		data->frameCount = samples_per_chunk * (fDataSize / frame_size);
	 	data->duration = (data->frameCount * 1000000) / data->frameRate;
	 }

	TRACE("mp3Reader::AllocateCookie: frameRate %ld, frameCount %Ld, duration %.6f\n",
		data->frameRate, data->frameCount, data->duration / 1000000.0);

	BMediaFormats formats;
	media_format_description description;
	description.family = B_MPEG_FORMAT_FAMILY;
	description.u.mpeg.id = b_mpeg_id_table[mpeg_version_index][layer_index];
	formats.GetFormatFor(description, &data->format);
	
	data->format.u.encoded_audio.encoding = media_encoded_audio_format::B_ANY;
	data->format.u.encoded_audio.bit_rate = bit_rate;
	data->format.u.encoded_audio.frame_size = frame_size;
//	data->format.u.encoded_audio.output.frame_rate = data->frameRate;
//	data->format.u.encoded_audio.output.channel_count = 2;

	// store the cookie
	*cookie = data;
	return B_OK;
}


status_t
mp3Reader::FreeCookie(void *cookie)
{
	TRACE("mp3Reader::FreeCookie\n");
	mp3data *data = reinterpret_cast<mp3data *>(cookie);
	delete data->chunkBuffer;
	delete data;
	
	return B_OK;
}


status_t
mp3Reader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	mp3data *data = reinterpret_cast<mp3data *>(cookie);

	*frameCount = data->frameCount;
	*duration = data->duration;
	*format = data->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
mp3Reader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	if (!fSeekableSource)
		return B_ERROR;

	mp3data *data = reinterpret_cast<mp3data *>(cookie);
	int64 pos;
	
	// this isn't very accurate

	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		pos = fXingVbrInfo ? XingSeekPoint(*frame / (float)data->frameCount) : -1;
		if (pos < 0)
			pos = (*frame * fDataSize) / data->frameCount;
		TRACE("mp3Reader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
		*time = (*frame * data->duration) / data->frameCount;
		TRACE("mp3Reader::Seek newtime %Ld\n", *time);
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		pos = fXingVbrInfo ? XingSeekPoint(*time / (float)data->duration) : -1;
		if (pos < 0)
			pos = (*time * fDataSize) / data->duration;
		TRACE("mp3Reader::Seek to time %Ld, pos %Ld\n", *time, pos);
		*frame = (*time * data->frameCount) / data->duration;
		TRACE("mp3Reader::Seek newframe %Ld\n", *frame);
	} else {
		return B_ERROR;
	}
	
	// We ignore B_MEDIA_SEEK_CLOSEST_FORWARD, B_MEDIA_SEEK_CLOSEST_BACKWARD

	uint8 buffer[16000];
	if (pos > fDataSize - 16000)
		pos = fDataSize - 16000;
	if (pos < 0)
		pos = 0;
	int64 size = fDataSize - pos;
	if (size > 16000)
		size = 16000;
	if (size != Source()->ReadAt(fDataStart + pos, buffer, size)) {
		TRACE("mp3Reader::Seek: unexpected read error\n");
		return B_ERROR;
	}
	int32 end = size - 4;
	int32 ofs;
	for (ofs = 0; ofs < end; ofs++) {
		if (buffer[ofs] != 0xff) // quick check
			continue;
		if (IsValidStream(&buffer[ofs], size - ofs))
			break;
	}
	if (ofs == end) {
		TRACE("mp3Reader::Seek: couldn't synchronize\n");
		return B_ERROR;
	}
	data->position = pos + ofs;

	TRACE("mp3Reader::Seek: synchronized at position %Ld\n", data->position);
	return B_OK;
}


status_t
mp3Reader::GetNextChunk(void *cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	mp3data *data = reinterpret_cast<mp3data *>(cookie);
	
	int64 maxbytes = fDataSize - data->position;
	if (maxbytes < 4)
		return B_ERROR;

	if (4 != Source()->ReadAt(fDataStart + data->position, data->chunkBuffer, 4)) {
		TRACE("mp3Reader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	data->position += 4;
	maxbytes -= 4;
	
	int size = GetFrameLength(data->chunkBuffer) - 4;
	if (size <= 0) {
		TRACE("mp3Reader::GetNextChunk: invalid frame encountered\n");
		// try to synchronize again here!
		return B_ERROR;
	}

	if (size > maxbytes)
		size = maxbytes;
		
	if (size != Source()->ReadAt(fDataStart + data->position, data->chunkBuffer + 4, size)) {
		TRACE("mp3Reader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	data->position += size;
		
	*chunkBuffer = data->chunkBuffer;
	*chunkSize = size + 4;

	return B_OK;
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
				ParseXingVbrHeader(offset + pos);
				goto skip_header;
			}

			hdr_length = GetInfoCbrLength(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a Info CBR header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				goto skip_header;
			}
			
			hdr_length = GetFraunhoferVbrLength(&buf[pos]);
			if (hdr_length > 0) {
				TRACE("mp3ReaderPlugin::ParseFile found a Fraunhofer VBR header of %d bytes at position %Ld\n", hdr_length, offset + pos);
				ParseFraunhoferVbrHeader(offset + pos);
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
			fDataSize = offset + pos + fDataStart;
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

	if (xing_header[0] != 'X') return -1;
	if (xing_header[1] != 'i') return -1;
	if (xing_header[2] != 'n') return -1;
	if (xing_header[3] != 'g') return -1;

	return GetFrameLength(header);
}

int
mp3Reader::GetInfoCbrLength(uint8 *header)
{
	int h_id	= (header[1] >> 3) & 1;
	int h_mode	= (header[3] >> 6) & 3;
	uint8 *info_header;
	
	// determine offset of header
	if(h_id) // mpeg1
		info_header = (h_mode != 3) ? (header + 36) : (header + 21);
	else	 // mpeg2
		info_header = (h_mode != 3) ? (header + 21) : (header + 13);

	if (info_header[0] != 'I') return -1;
	if (info_header[1] != 'n') return -1;
	if (info_header[2] != 'f') return -1;
	if (info_header[3] != 'o') return -1;

	return GetFrameLength(header);
}

void
mp3Reader::ParseXingVbrHeader(int64 pos)
{
	static const int sr_table[2][4] = { { 22050, 24000, 16000, 0 }, { 44100, 48000, 32000, 0 } };
	static const int FRAMES_FLAG = 0x0001;
	static const int BYTES_FLAG  = 0x0002;
	static const int TOC_FLAG    = 0x0004;
	static const int VBR_SCALE_FLAG = 0x0008;
	uint8 header[200];
	uint8 *xing_header;

	Source()->ReadAt(pos, header, sizeof(header));

	int layer_index = (header[1] >> 1) & 3;
	int h_id		= (header[1] >> 3) & 1;
	int h_sr_index	= (header[2] >> 2) & 3;
	int h_mode		= (header[3] >> 6) & 3;

	// determine offset of header
	if(h_id) // mpeg1
		xing_header = (h_mode != 3) ? (header + 36) : (header + 21);
	else	 // mpeg2
		xing_header = (h_mode != 3) ? (header + 21) : (header + 13);
		
	xing_header += 4; // skip ID
	
	int flags = B_BENDIAN_TO_HOST_INT32(*(uint32 *)xing_header);
	xing_header += 4;

	if (fXingVbrInfo) {
		TRACE("mp3Reader::ParseXingVbrHeader: Error, already found a header\n");
		return;
	}

	fXingVbrInfo = new xing_vbr_info;

	fXingVbrInfo->frameRate = sr_table[h_id][h_sr_index];
	if (flags & FRAMES_FLAG) {
		fXingVbrInfo->encodedFramesCount = (int64)(uint32)B_BENDIAN_TO_HOST_INT32(*(uint32 *)xing_header);
		xing_header += 4;
	} else {
		fXingVbrInfo->encodedFramesCount = -1;
	}

	if (flags & BYTES_FLAG) {
		fXingVbrInfo->byteCount = (int64)(uint32)B_BENDIAN_TO_HOST_INT32(*(uint32 *)xing_header);
		xing_header += 4;
	} else {
		fXingVbrInfo->byteCount = -1;
	}

	if (flags & TOC_FLAG) {
		fXingVbrInfo->hasSeekpoints = true;
		memcpy(fXingVbrInfo->seekpoints, xing_header, 100);
		xing_header += 100;
	} else {
		fXingVbrInfo->hasSeekpoints = false;
	}

	if (flags & VBR_SCALE_FLAG) {
		fXingVbrInfo->vbrScale = B_BENDIAN_TO_HOST_INT32(*(uint32 *)xing_header);
		xing_header += 4;
	} else {
		fXingVbrInfo->vbrScale = -1;
	}
	
	 // mpeg frame (chunk) size is is constant and always 384 samples (frames) for
	 // Layer I and 1152 samples for Layer II and Layer III
	 if (fXingVbrInfo->encodedFramesCount != -1) {
	 	fXingVbrInfo->frameCount = fXingVbrInfo->encodedFramesCount * frame_sample_count_table[layer_index];
	 	fXingVbrInfo->duration = (fXingVbrInfo->frameCount * 1000000) / fXingVbrInfo->frameRate;
	 } else {
	 	fXingVbrInfo->duration = -1;
	 	fXingVbrInfo->frameCount = -1;
	 }

	TRACE("mp3Reader::ParseXingVbrHeader: %Ld encoded frames, %Ld bytes, %s seekpoints, vbrscale %ld\n",
		fXingVbrInfo->encodedFramesCount, fXingVbrInfo->byteCount,
		fXingVbrInfo->hasSeekpoints ? "has" : "no", fXingVbrInfo->vbrScale);
	TRACE("mp3Reader::ParseXingVbrHeader: frameRate %ld, frameCount %Ld, duration %.6f\n",
		fXingVbrInfo->frameRate, fXingVbrInfo->frameCount, fXingVbrInfo->duration / 1000000.0);
}

int64
mp3Reader::XingSeekPoint(float percent)
{
	if (!fXingVbrInfo || !fXingVbrInfo->hasSeekpoints || fXingVbrInfo->byteCount == -1)
		return -1;

	int a;
	int64 point;
	float fa, fb, fx;

	if (percent < 0.0f)
		percent = 0.0f;
	if (percent > 100.0f)
		percent = 100.0f;

	a = (int)percent;
	if (a > 99)
		a = 99;
	fa = fXingVbrInfo->seekpoints[a];
	if (a < 99)
	    fb = fXingVbrInfo->seekpoints[a + 1];
	else
    	fb = 256.0f;

	fx = fa + (fb - fa) * (percent - a);
	
	point = (int64)((1.0f / 256.0f) * fx * fXingVbrInfo->byteCount);
	TRACE("mp3Reader::XingSeekPoint for %.8f%% is %Ld\n", percent, point);
	return point;
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

void
mp3Reader::ParseFraunhoferVbrHeader(int64 pos)
{
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
	// avoid detecting mp3 in a container format like AVI or mov

	// To detect an mp3 file, we seek into the middle,
	// and search for a valid sequence of 3 frame headers.
	// A mp3 frame has a maximum length of 2881 bytes, we
	// load a block of 16kB and use it to search.

	const int32 search_size = 16384;
	int64	offset;
	int32	size;
	uint8	buf[search_size];

	size = 8;
	offset = 0;
	if (size != Source()->ReadAt(offset, buf, size)) {
		TRACE("mp3ReaderPlugin::IsMp3File reading %ld bytes at offset %Ld failed\n", size, offset);
		return false;
	}
	
	// avoid reading some common formats that might have an embedded mp3 stream
	// RIFF, AVI or WAV
	if (buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F')
		return false;
	// Ogg Vorbis
	if (buf[0] == 'O' && buf[1] == 'g' && buf[2] == 'g' && buf[3] == 'S')
		return false;
	// Real Media
	if (buf[0] == '.' && buf[1] == 'R' && buf[2] == 'M' && buf[3] == 'F')
		return false;
	// Quicktime
	if (buf[4] == 'm' && buf[5] == 'o' && buf[6] == 'o' && buf[7] == 'v')
		return false;
	// ASF 1 (first few bytes of GUID)
	if (buf[0] == 0x30 && buf[1] == 0x26 && buf[2] == 0xb2 && buf[3] == 0x75
		&& buf[4] == 0x8e && buf[5] == 0x66 && buf[6] == 0xcf && buf[7] == 0x11)
		return false;
	// ASF 2.0 (first few bytes of GUID)
	if (buf[0] == 0xd1 && buf[1] == 0x29 && buf[2] == 0xe2 && buf[3] == 0xd6
		&& buf[4] == 0xda && buf[5] == 0x35 && buf[6] == 0xd1 && buf[7] == 0x11)
		return false;

	// search for a valid mpeg audio frame header
	// sequence in the middle of the file
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
	int bitrate_index = (h[2] >> 4) & 0x0f;
	int sampling_rate_index = (h[2] >> 2) & 0x03;
	int padding = (h[2] >> 1) & 0x01;
	/* no interested in the other bits */
	
	int bitrate = bit_rate_table[mpeg_version_index][layer_index][bitrate_index];
	int framerate = frame_rate_table[mpeg_version_index][sampling_rate_index];
	
	if (!bitrate || !framerate)
		return -1;	

	int length;	
	if (layer_index == 3) // layer 1
		length = ((144 * 1000 * bitrate) / framerate) + (padding * 4);
	else // layer 2 & 3
		length = ((144 * 1000 * bitrate) / framerate) + padding;

#if 0
	TRACE("%s %s, %s crc, bit rate %d, frame rate %d, padding %d, frame length %d\n",
		mpeg_version_index == 0 ? "mpeg 2.5" : (mpeg_version_index == 2 ? "mpeg 2" : "mpeg 1"),
		layer_index == 3 ? "layer 1" : (layer_index == 2 ? "layer 2" : "layer 3"),
		(h[1] & 0x01) ? "no" : "has",
		bitrate, framerate, padding, length);
#endif

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
