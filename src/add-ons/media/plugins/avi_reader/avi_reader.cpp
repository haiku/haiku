#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include <MediaFormats.h>
#include "RawFormats.h"
#include "avi_reader.h"

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

// http://web.archive.org/web/20030618161228/http://www.microsoft.com/Developer/PRODINFO/directx/dxm/help/ds/FiltDev/DV_Data_AVI_File_Format.htm
// http://mediaxw.sourceforge.net/files/doc/Video%20for%20Windows%20Reference%20-%20Chapter%204%20-%20AVI%20Files.pdf


struct avi_cookie
{
	unsigned	stream;
	char *		buffer;
	unsigned	buffer_size;

	int64		frame_count;
	bigtime_t 	duration;
	media_format format;

	bool		audio;

	// audio only:	
	int64		byte_pos;
	uint32		bytes_per_sec_rate;
	uint32		bytes_per_sec_scale;

	// video only:
	uint32		frame_pos;
	uint32		frames_per_sec_rate;
	uint32		frames_per_sec_scale;
	uint32		line_count;
};


aviReader::aviReader()
 :	fFile(0)
{
	TRACE("aviReader::aviReader\n");
}


aviReader::~aviReader()
{
 	delete fFile;
}

      
const char *
aviReader::Copyright()
{
	return "AVI & OpenDML reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
aviReader::Sniff(int32 *streamCount)
{
	TRACE("aviReader::Sniff\n");
	
	BPositionIO *pos_io_source;

	pos_io_source = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!pos_io_source) {
		TRACE("aviReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}
	
	if (!OpenDMLFile::IsSupported(pos_io_source)) {
		TRACE("aviReader::Sniff: unsupported file type\n");
		return B_ERROR;
	}
	
	TRACE("aviReader::Sniff: this stream seems to be supported\n");
	
	fFile = new OpenDMLFile();
	if (B_OK != fFile->SetTo(pos_io_source)) {
		TRACE("aviReader::Sniff: can't setup OpenDMLFile\n");
		return B_ERROR;
	}
	
	*streamCount = fFile->StreamCount();
	return B_OK;
}

void
aviReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_VIDEO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "audio/x-avi");
	strcpy(mff->file_extension, "avi");
	strcpy(mff->short_name,  "AVI");
	strcpy(mff->pretty_name, "Audio/Video Interleaved (AVI) file format");
}

status_t
aviReader::AllocateCookie(int32 streamNumber, void **_cookie)
{
	avi_cookie *cookie = new avi_cookie;
	*_cookie = cookie;
	
	cookie->stream = streamNumber;
	cookie->buffer = 0;
	cookie->buffer_size = 0;

	BMediaFormats formats;
	media_format *format = &cookie->format;
	media_format_description description;
	
	const avi_stream_header *stream_header;
	stream_header = fFile->StreamFormat(cookie->stream);
	if (!stream_header) {
		TRACE("aviReader::GetStreamInfo: stream %d has no header\n", cookie->stream);
		delete cookie;
		return B_ERROR;
	}
	
	TRACE("aviReader::AllocateCookie: stream %ld (%s)\n", streamNumber, fFile->IsAudio(cookie->stream) ? "audio" : fFile->IsVideo(cookie->stream)  ? "video" : "unknown");

	if (fFile->IsAudio(cookie->stream)) {
		const wave_format_ex *audio_format = fFile->AudioFormat(cookie->stream);
		if (!audio_format) {
			TRACE("aviReader::GetStreamInfo: audio stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		if (audio_format->format_tag == 0x0001) {// PCM
			cookie->frame_count = stream_header->length / ((stream_header->sample_size + 7) / 8);
			TRACE("frame_count %Ld (is PCM)\n", cookie->frame_count);
		} else if (stream_header->rate) { // not PCM
			cookie->frame_count = (stream_header->length * (int64)audio_format->frames_per_sec) / stream_header->rate;
			TRACE("frame_count %Ld (using rate)\n", cookie->frame_count);
		} else { // not PCM
			cookie->frame_count = (stream_header->length * (int64)audio_format->frames_per_sec) / (stream_header->sample_size * audio_format->avg_bytes_per_sec);
			TRACE("frame_count %Ld (using fallback)\n", cookie->frame_count);
		}

		if (stream_header->rate && stream_header->scale) {
			cookie->duration = (1000000LL * (int64)stream_header->length * (int64)stream_header->scale) / stream_header->rate;
			TRACE("duration %.6f (%Ld) (using scale & rate)\n", cookie->duration / 1E6, cookie->duration);
		} else if (stream_header->rate) {
			cookie->duration = (1000000LL * (int64)stream_header->length) / stream_header->rate;
			TRACE("duration %.6f (%Ld) (using rate)\n", cookie->duration / 1E6, cookie->duration);
		} else {
			cookie->duration = fFile->Duration();
			TRACE("duration %.6f (%Ld) (using fallback)\n", cookie->duration / 1E6, cookie->duration);
		}
		
		cookie->audio = true;
		cookie->byte_pos = 0;

		if (stream_header->scale && stream_header->rate && stream_header->sample_size) {
			cookie->bytes_per_sec_rate = stream_header->rate * stream_header->sample_size;
			cookie->bytes_per_sec_scale = stream_header->scale;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using both)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (audio_format->avg_bytes_per_sec) {
			cookie->bytes_per_sec_rate = audio_format->avg_bytes_per_sec;
			cookie->bytes_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using avg_bytes_per_sec)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else if (stream_header->rate) {
			cookie->bytes_per_sec_rate = stream_header->rate;
			cookie->bytes_per_sec_scale = 1;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using rate)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		} else {
			cookie->bytes_per_sec_rate = 128000;
			cookie->bytes_per_sec_scale = 8;
			TRACE("bytes_per_sec_rate %ld, bytes_per_sec_scale %ld (using fallback)\n", cookie->bytes_per_sec_rate, cookie->bytes_per_sec_scale);
		}

		if (audio_format->format_tag == 0x0001) {
			// a raw PCM format
			description.family = B_BEOS_FORMAT_FAMILY;
			description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
			if (B_OK != formats.GetFormatFor(description, format)) 
				format->type = B_MEDIA_RAW_AUDIO;
			format->u.raw_audio.frame_rate = audio_format->frames_per_sec;
			format->u.raw_audio.channel_count = audio_format->channels;
			if (audio_format->bits_per_sample <= 8)
				format->u.raw_audio.format = B_AUDIO_FORMAT_UINT8;
			else if (audio_format->bits_per_sample <= 16)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT16;
			else if (audio_format->bits_per_sample <= 24)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT24;
			else if (audio_format->bits_per_sample <= 32)
				format->u.raw_audio.format = B_AUDIO_FORMAT_INT32;
			else {
				TRACE("WavReader::AllocateCookie: unhandled bits per sample %d\n", audio_format->bits_per_sample);
				return B_ERROR;
			}
			format->u.raw_audio.format |= B_AUDIO_FORMAT_CHANNEL_ORDER_WAVE;
			format->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
			format->u.raw_audio.buffer_size = stream_header->suggested_buffer_size;
		} else {
			// some encoded format
			description.family = B_WAV_FORMAT_FAMILY;
			description.u.wav.codec = audio_format->format_tag;
			if (B_OK != formats.GetFormatFor(description, format)) 
				format->type = B_MEDIA_ENCODED_AUDIO;
			format->u.encoded_audio.bit_rate = 8 * audio_format->avg_bytes_per_sec;
			TRACE("bit_rate %.3f\n", format->u.encoded_audio.bit_rate);
			format->u.encoded_audio.output.frame_rate = audio_format->frames_per_sec;
			format->u.encoded_audio.output.channel_count = audio_format->channels;
		}
		// this doesn't seem to work (it's not even a fourcc)
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = audio_format->format_tag; format->user_data[4] = 0;
		
		// put the wave_format_ex struct, including extra data, into the format meta data.
		size_t size;
		const void *data = fFile->AudioFormat(cookie->stream, &size);
		format->SetMetaData(data, size);
		
		uint8 *p = 18 + (uint8 *)data;
		TRACE("extra_data: %ld: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			size - 18, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
	
		return B_OK;
	}

	if (fFile->IsVideo(cookie->stream)) {
		const bitmap_info_header *video_format = fFile->VideoFormat(cookie->stream);
		if (!video_format) {
			TRACE("aviReader::GetStreamInfo: video stream %d has no format\n", cookie->stream);
			delete cookie;
			return B_ERROR;
		}
		
		cookie->audio = false;
		cookie->frame_pos = 0;
		cookie->line_count = fFile->AviMainHeader()->height;
		
		if (stream_header->scale && stream_header->rate) {
			cookie->frames_per_sec_rate = stream_header->rate;
			cookie->frames_per_sec_scale = stream_header->scale;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using both)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else if (fFile->AviMainHeader()->micro_sec_per_frame) {
			cookie->frames_per_sec_rate = 1000000;
			cookie->frames_per_sec_scale = fFile->AviMainHeader()->micro_sec_per_frame;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using micro_sec_per_frame)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		} else {
			cookie->frames_per_sec_rate = 25;
			cookie->frames_per_sec_scale = 1;
			TRACE("frames_per_sec_rate %ld, frames_per_sec_scale %ld (using fallback)\n", cookie->frames_per_sec_rate, cookie->frames_per_sec_scale);
		}

		cookie->frame_count = stream_header->length;
		cookie->duration = (cookie->frame_count * (int64)cookie->frames_per_sec_scale * 1000000LL) / cookie->frames_per_sec_rate;
		
		TRACE("frame_count %Ld\n", cookie->frame_count);
		TRACE("duration %.6f (%Ld)\n", cookie->duration / 1E6, cookie->duration);

		description.family = B_AVI_FORMAT_FAMILY;
		description.u.avi.codec = stream_header->fourcc_handler;
		if (B_OK != formats.GetFormatFor(description, format)) 
			format->type = B_MEDIA_ENCODED_VIDEO;
			
		format->user_data_type = B_CODEC_TYPE_INFO;
		*(uint32 *)format->user_data = stream_header->fourcc_handler; format->user_data[4] = 0;
		format->u.encoded_video.max_bit_rate = 8 * fFile->AviMainHeader()->max_bytes_per_sec;
		format->u.encoded_video.avg_bit_rate = format->u.encoded_video.max_bit_rate / 2; // XXX fix this
		format->u.encoded_video.output.field_rate = cookie->frames_per_sec_rate / (float)cookie->frames_per_sec_scale;
		format->u.encoded_video.output.interlace = 1; // 1: progressive
		format->u.encoded_video.output.first_active = 0;
		format->u.encoded_video.output.last_active = cookie->line_count - 1;
		format->u.encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		format->u.encoded_video.output.pixel_width_aspect = 1;
		format->u.encoded_video.output.pixel_height_aspect = 1;
		// format->u.encoded_video.output.display.format = 0;
		format->u.encoded_video.output.display.line_width = fFile->AviMainHeader()->width;
		format->u.encoded_video.output.display.line_count = cookie->line_count;
		format->u.encoded_video.output.display.bytes_per_row = 0; // format->u.encoded_video.output.display.line_width * 4;
		format->u.encoded_video.output.display.pixel_offset = 0;
		format->u.encoded_video.output.display.line_offset = 0;
		format->u.encoded_video.output.display.flags = 0;
		
		TRACE("max_bit_rate %.3f\n", format->u.encoded_video.max_bit_rate);
		TRACE("field_rate   %.3f\n", format->u.encoded_video.output.field_rate);

		return B_OK;
	}

	delete cookie;
	return B_ERROR;
}


status_t
aviReader::FreeCookie(void *_cookie)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	delete [] cookie->buffer;

	delete cookie;
	return B_OK;
}


status_t
aviReader::GetStreamInfo(void *_cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, void **infoBuffer, int32 *infoSize)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	*frameCount = cookie->frame_count;
	*duration = cookie->duration;
	*format = cookie->format;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
aviReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{

	return B_OK;
}


status_t
aviReader::GetNextChunk(void *_cookie,
						void **chunkBuffer, int32 *chunkSize,
						media_header *mediaHeader)
{
	avi_cookie *cookie = (avi_cookie *)_cookie;

	int64 start; uint32 size; bool keyframe;
	if (!fFile->GetNextChunkInfo(cookie->stream, &start, &size, &keyframe))
		return B_LAST_BUFFER_ERROR;

	if (cookie->buffer_size < size) {
		delete [] cookie->buffer;
		cookie->buffer_size = (size + 15) & ~15;
		cookie->buffer = new char [cookie->buffer_size];
	}
	
	if (cookie->audio) {
		mediaHeader->start_time = (cookie->byte_pos * 1000000LL * (int64)cookie->bytes_per_sec_scale) / cookie->bytes_per_sec_rate;
		mediaHeader->type = B_MEDIA_ENCODED_AUDIO;
		mediaHeader->u.encoded_audio.buffer_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		
		cookie->byte_pos += size;
	} else {
		mediaHeader->start_time = (cookie->frame_pos * 1000000LL * (int64)cookie->frames_per_sec_scale) / cookie->frames_per_sec_rate;
		mediaHeader->type = B_MEDIA_ENCODED_VIDEO;
		mediaHeader->u.encoded_video.field_flags = keyframe ? B_MEDIA_KEY_FRAME : 0;
		mediaHeader->u.encoded_video.first_active_line = 0;
		mediaHeader->u.encoded_video.line_count = cookie->line_count;
	
		cookie->frame_pos += 1;
	}
	
	TRACE("stream %d: start_time %.6f\n", cookie->stream, mediaHeader->start_time / 1000000.0);

	*chunkBuffer = cookie->buffer;
	*chunkSize = size;
	return (int)size == fFile->Source()->ReadAt(start, cookie->buffer, size) ? B_OK : B_LAST_BUFFER_ERROR;
}


Reader *
aviReaderPlugin::NewReader()
{
	return new aviReader;
}


MediaPlugin *instantiate_plugin()
{
	return new aviReaderPlugin;
}
