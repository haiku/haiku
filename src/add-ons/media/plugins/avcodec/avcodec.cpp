/*
 * Copyright (C) 2001 Carlos Hasan
 * Copyright (C) 2001 François Revol
 * Copyright (C) 2001 Axel Dörfler
 * Copyright (C) 2004 Marcus Overhagen
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//! libavcodec based decoder for Haiku

#include <Debug.h>
#include <OS.h>
#include <Bitmap.h>
#include <string.h>

#include "avcodecplugin.h"

#define DO_PROFILING 0

#undef TRACE
//#define TRACE_AV_CODEC
#ifdef TRACE_AV_CODEC
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif

struct wave_format_ex {
	uint16 format_tag;
	uint16 channels;
	uint32 frames_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	// extra_data[extra_size]
} _PACKED;

static bigtime_t diff1 = 0, diff2 = 0;
static long prof_cnt = 0;

// uncommenting will make Decode() set the current thread priority to time
// sharing, so it won't totally freeze if you busy-loop in there (to help debug
// with CD Manager)
//#define UNREAL
 
avCodec::avCodec()
	:	fHeader(),
		fInfo(),
		fInputFormat(),
		fOutputVideoFormat(),
		fFrame(0),
		isAudio(false),
		fCodec(NULL),
		ffc(NULL),
		fCodecInitDone(false),
		conv_func(NULL),
		fExtraData(NULL),
		fExtraDataSize(0),
		fBlockAlign(0),
		fOutputBuffer(0)
{
	TRACE("avCodec::avCodec()\n");

	// prevent multiple inits
	static volatile vint32 ff_init_count = 0;
	static bool ff_init_done = false;
	if (atomic_add(&ff_init_count, 1) > 1) {
		atomic_add(&ff_init_count, -1);
		// spin until the thread that is initing is done
		while (!ff_init_done)
			snooze(20000);
	} else {
		avcodec_init();
		avcodec_register_all();
		ff_init_done = true;
	}

	ffc = avcodec_alloc_context();
	ffpicture = avcodec_alloc_frame();
	opicture = avcodec_alloc_frame();
}


avCodec::~avCodec()
{
	TRACE("[%c] avCodec::~avCodec()\n", isAudio?('a'):('v'));

#ifdef DO_PROFILING
	if (prof_cnt > 0) {
			printf("[%c] profile: d1 = %lld, d2 = %lld (%Ld)\n",
				isAudio?('a'):('v'), diff1/prof_cnt, diff2/prof_cnt,
				fFrame);
	}
#endif

	if(fCodecInitDone)
		avcodec_close(ffc);

	free(opicture);
	free(ffpicture);
	free(ffc);
	
	delete [] fExtraData;
	delete [] fOutputBuffer;
}


void
avCodec::GetCodecInfo(media_codec_info *mci)
{
	sprintf(mci->short_name, "ff:%s", fCodec->name);
	sprintf(mci->pretty_name, "%s (libavcodec %s)",
		gCodecTable[ffcodec_index_in_table].prettyname, fCodec->name);
}


status_t
avCodec::Setup(media_format *ioEncodedFormat, const void *infoBuffer,
	size_t infoSize)
{
	if (ioEncodedFormat->type != B_MEDIA_ENCODED_AUDIO
		&& ioEncodedFormat->type != B_MEDIA_ENCODED_VIDEO)
		return B_ERROR;
		
	isAudio = (ioEncodedFormat->type == B_MEDIA_ENCODED_AUDIO);
	TRACE("[%c] avCodec::Setup()\n", isAudio?('a'):('v'));

	if (isAudio && !fOutputBuffer)
		fOutputBuffer = new char[AVCODEC_MAX_AUDIO_FRAME_SIZE];

//#if DEBUG
	char buffer[1024];
	string_for_format(*ioEncodedFormat, buffer, sizeof(buffer));
	TRACE("[%c]   input_format=%s\n", isAudio?('a'):('v'), buffer);
	TRACE("[%c]   infoSize=%ld\n", isAudio?('a'):('v'), infoSize);
	TRACE("[%c]   user_data_type=%08lx\n", isAudio?('a'):('v'),	ioEncodedFormat->user_data_type);
	TRACE("[%c]   meta_data_size=%ld\n", isAudio?('a'):('v'), ioEncodedFormat->MetaDataSize());
	TRACE("[%c]   info_size=%ld\n", isAudio?('a'):('v'), infoSize);
//#endif

	media_format_description descr;
	for (int32 i = 0; gCodecTable[i].id; i++) {
		ffcodec_index_in_table = i;
		uint64 cid;
		
		if (BMediaFormats().GetCodeFor(*ioEncodedFormat, gCodecTable[i].family,
				&descr) == B_OK
		    && gCodecTable[i].type == ioEncodedFormat->type) {
			switch(gCodecTable[i].family) {
				case B_WAV_FORMAT_FAMILY:
					cid = descr.u.wav.codec;
					break;
				case B_AIFF_FORMAT_FAMILY:
					cid = descr.u.aiff.codec;
					break;
				case B_AVI_FORMAT_FAMILY:
					cid = descr.u.avi.codec;
					break;
				case B_MPEG_FORMAT_FAMILY:
					cid = descr.u.mpeg.id;
					break;
				case B_QUICKTIME_FORMAT_FAMILY:
					cid = descr.u.quicktime.codec;
					break;
				case B_MISC_FORMAT_FAMILY:
					cid = (((uint64)descr.u.misc.file_format) << 32)
						| descr.u.misc.codec;
					break;
				default:
					puts("ERR family");
					return B_ERROR;
			}
			TRACE("  0x%04lx codec id = \"%c%c%c%c\"\n", uint32(cid), (char)((cid >> 24) & 0xff),
				(char)((cid >> 16) & 0xff), (char)((cid >> 8) & 0xff),
				(char)(cid & 0xff));

			if (gCodecTable[i].family == descr.family
				&& gCodecTable[i].fourcc == cid) {
				fCodec = avcodec_find_decoder(gCodecTable[i].id);
				if (!fCodec) {
					TRACE("avCodec: unable to find the correct ffmpeg decoder "
						"(id = %d)!!!\n",gCodecTable[i].id);
					return B_ERROR;
				}
				TRACE("avCodec: found decoder %s\n",fCodec->name);
				
				if (gCodecTable[i].family == B_WAV_FORMAT_FAMILY && infoSize == sizeof(wave_format_ex)) {
					const wave_format_ex *wfmt_data
						= (const wave_format_ex *)infoBuffer;
					size_t wfmt_size = infoSize;
					if (wfmt_data && wfmt_size) {
						fBlockAlign = wfmt_data->block_align;
						fExtraDataSize = wfmt_data->extra_size;
						if (fExtraDataSize) {
							fExtraData = new char [fExtraDataSize];
							memcpy(fExtraData, wfmt_data + 1, fExtraDataSize);
						}
					}
				} else {
					fBlockAlign = ioEncodedFormat->u.encoded_audio.output.buffer_size;
					TRACE("avCodec: extra data size %ld\n",infoSize);
					fExtraDataSize = infoSize;
					if (fExtraDataSize) {
						fExtraData = new char [fExtraDataSize];
						memcpy(fExtraData, infoBuffer, fExtraDataSize);
					}
				}

				fInputFormat = *ioEncodedFormat;
				return B_OK;
			}
		}
	}
	printf("avCodec::Setup failed!\n");
	return B_ERROR;
}


status_t
avCodec::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	// reset the ffmpeg codec
	// to flush buffers, so we keep the sync
	if (isAudio && fCodecInitDone) {
		fCodecInitDone = false;
		avcodec_close(ffc);
		fCodecInitDone = (avcodec_open(ffc, fCodec) >= 0);
	}
	
	if (seekTo == B_MEDIA_SEEK_TO_TIME) {
		TRACE("avCodec::Seek by time ");
		TRACE("from frame %Ld and time %.6f TO Required Time %.6f. ", fFrame, fStartTime / 1000000.0, seekTime / 1000000.0);

		*frame = (int64)(seekTime * fOutputFrameRate / 1000000LL);
		*time = seekTime;
	} else if (seekTo == B_MEDIA_SEEK_TO_FRAME) {
		TRACE("avCodec::Seek by Frame ");
		TRACE("from time %.6f and frame %Ld TO Required Frame %Ld. ", fStartTime / 1000000.0, fFrame, seekFrame);

		*time = (bigtime_t)(seekFrame * 1000000LL / fOutputFrameRate);
		*frame = seekFrame;
	} else
		return B_BAD_VALUE;

	fFrame = *frame;
	fStartTime = *time;
	TRACE("so new frame is %Ld at time %.6f\n", *frame, *time / 1000000.0);
	return B_OK;
}


status_t
avCodec::NegotiateOutputFormat(media_format *inout_format)
{
	TRACE("[%c] avCodec::NegotiateOutputFormat()\n", isAudio?('a'):('v'));

	int result;
	
#if DEBUG
	char buffer[1024];
	string_for_format(*inout_format, buffer, sizeof(buffer));
	TRACE("[%c]  in_format=%s\n", isAudio?('a'):('v'), buffer);
#endif

	if (isAudio) {
		media_multi_audio_format outputAudioFormat;
		outputAudioFormat = media_raw_audio_format::wildcard;
		outputAudioFormat.format = media_raw_audio_format::B_AUDIO_SHORT;
		outputAudioFormat.byte_order = B_MEDIA_HOST_ENDIAN;
		outputAudioFormat.frame_rate
			= fInputFormat.u.encoded_audio.output.frame_rate;
		outputAudioFormat.channel_count
			= fInputFormat.u.encoded_audio.output.channel_count;
		outputAudioFormat.buffer_size
			= 1024 * fInputFormat.u.encoded_audio.output.channel_count;
		inout_format->type = B_MEDIA_RAW_AUDIO;
		inout_format->u.raw_audio = outputAudioFormat;

		ffc->bit_rate = (int) fInputFormat.u.encoded_audio.bit_rate;
		ffc->sample_rate = (int) fInputFormat.u.encoded_audio.output.frame_rate;
		ffc->channels = fInputFormat.u.encoded_audio.output.channel_count;
		ffc->block_align = fBlockAlign;
		ffc->extradata = (uint8_t *)fExtraData;
		ffc->extradata_size = fExtraDataSize;

		TRACE("bit_rate %d, sample_rate %d, channels %d, block_align %d, "
			"extradata_size %d\n", ffc->bit_rate, ffc->sample_rate,
			ffc->channels, ffc->block_align, ffc->extradata_size);

		// close any previous instance
		if (fCodecInitDone) {
			fCodecInitDone = false;
			avcodec_close(ffc);
		}

		// open new
		result = avcodec_open(ffc, fCodec);
		fCodecInitDone = (result >= 0);

		TRACE("audio: bit_rate = %d, sample_rate = %d, chans = %d Init = %d\n",
			ffc->bit_rate, ffc->sample_rate, ffc->channels, result);

		fStartTime = 0;
		fOutputFrameSize = 2 * outputAudioFormat.channel_count;
		fOutputFrameCount = outputAudioFormat.buffer_size / fOutputFrameSize;
		fOutputFrameRate = outputAudioFormat.frame_rate;
		fChunkBuffer = 0;
		fChunkBufferOffset = 0;
		fChunkBufferSize = 0;
		fOutputBufferOffset = 0;
		fOutputBufferSize = 0;
		
		inout_format->require_flags = 0;
		inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

		if (!fCodecInitDone) {
			TRACE("avcodec_open() failed!\n");
			return B_ERROR;
		}

		return B_OK;

	} else {	// VIDEO

		fOutputVideoFormat = fInputFormat.u.encoded_video.output;

		ffc->width = fOutputVideoFormat.display.line_width;
		ffc->height = fOutputVideoFormat.display.line_count;
//		ffc->frame_rate = (int)(fOutputVideoFormat.field_rate
//			* ffc->frame_rate_base);
		
		fOutputFrameRate = fOutputVideoFormat.field_rate;
		
		ffc->extradata = (uint8_t *)fExtraData;
		ffc->extradata_size = fExtraDataSize;

//		if (fInputFormat.MetaDataSize() > 0) {
//			ffc->extradata = (uint8_t *)fInputFormat.MetaData();
//			ffc->extradata_size = fInputFormat.MetaDataSize();
//		}

		TRACE("#### requested video format 0x%x\n",
			inout_format->u.raw_video.display.format);

		// make MediaPlayer happy (if not in rgb32 screen depth and no overlay,
		// it will only ask for YCbCr, which DrawBitmap doesn't handle, so the
		// default colordepth is RGB32)
		if (inout_format->u.raw_video.display.format == B_YCbCr422)
			fOutputVideoFormat.display.format = B_YCbCr422;
		else
			fOutputVideoFormat.display.format = B_RGB32;

		// search for a pixel-format the codec handles
		// XXX We should try this a couple of times until it succeeds, each time
		// XXX using another format pixel-format that is supported by the
		// XXX decoder. But libavcodec doesn't seem to offer any way to tell the
		// XXX decoder which format it should use.
		conv_func = 0;
		for (int i = 0; i < 1; i++) { // iterate over supported codec formats
			// close any previous instance
			if (fCodecInitDone) {
				fCodecInitDone = false;
				avcodec_close(ffc);
			}
			// XXX set n-th ffc->pix_fmt here
			if (avcodec_open(ffc, fCodec) >= 0) {
				fCodecInitDone = true;

				conv_func = resolve_colorspace(fOutputVideoFormat.display.format, ffc->pix_fmt);
			}
			if (conv_func != 0)
				break;
		}

		if (!fCodecInitDone) {
			TRACE("avcodec_open() failed to init codec!\n");
			return B_ERROR;
		}

		if (!conv_func) {
			TRACE("no conv_func found or decoder has not set the pixel format yet!\n");
		}

		if (fOutputVideoFormat.display.format == B_YCbCr422) {
			fOutputVideoFormat.display.bytes_per_row
				= 2 * fOutputVideoFormat.display.line_width;
		} else {
			fOutputVideoFormat.display.bytes_per_row
				= 4 * fOutputVideoFormat.display.line_width;
		}

		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = fOutputVideoFormat;

		inout_format->require_flags = 0;
		inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

	#if DEBUG		
		string_for_format(*inout_format, buffer, sizeof(buffer));
		TRACE("[%c]  out_format=%s\n", isAudio?('a'):('v'), buffer);
	#endif

		TRACE("#### returned  video format 0x%x\n",
			inout_format->u.raw_video.display.format);

		return B_OK;
	}
}


status_t
avCodec::Decode(void *out_buffer, int64 *out_frameCount,
				media_header *mh, media_decode_info *info)
{
	const void *data;

	if (!fCodecInitDone)
		return B_BAD_VALUE;

#ifdef DO_PROFILING
	bigtime_t prof_t1, prof_t2, prof_t3;
#endif

#ifdef UNREAL
	set_thread_priority(find_thread(NULL), B_NORMAL_PRIORITY);
#endif

//	TRACE("[%c] avCodec::Decode() for time %Ld\n", isAudio?('a'):('v'), fStartTime);

	mh->start_time = fStartTime;

	if (isAudio) {

//		TRACE("audio start_time %.6f\n", mh->start_time / 1000000.0);

		char *output_buffer = (char *)out_buffer;
		*out_frameCount = 0;
		while (*out_frameCount < fOutputFrameCount) {
			if (fOutputBufferSize < 0) {
				TRACE("############ fOutputBufferSize %ld\n",
					fOutputBufferSize);
				fOutputBufferSize = 0;
			}
			if (fChunkBufferSize < 0) {
				TRACE("############ fChunkBufferSize %ld\n",
					fChunkBufferSize);
				fChunkBufferSize = 0;
			}
		
			if (fOutputBufferSize > 0) {
				int32 frames = min_c(fOutputFrameCount - *out_frameCount,
					fOutputBufferSize / fOutputFrameSize);
				memcpy(output_buffer, fOutputBuffer + fOutputBufferOffset,
					frames * fOutputFrameSize);
				fOutputBufferOffset += frames * fOutputFrameSize;
				fOutputBufferSize -= frames * fOutputFrameSize;
				output_buffer += frames * fOutputFrameSize;
				*out_frameCount += frames;
				fStartTime += (bigtime_t)((1000000LL * frames) / fOutputFrameRate);
				continue;
			}
			if (fChunkBufferSize == 0) {
				media_header chunk_mh;
				status_t err;
				err = GetNextChunk(&fChunkBuffer, &fChunkBufferSize, &chunk_mh);
				if (err != B_OK || fChunkBufferSize < 0) {
					TRACE("GetNextChunk error %ld\n",fChunkBufferSize);
					fChunkBufferSize = 0;
					break;
				}
				fChunkBufferOffset = 0;
				fStartTime = chunk_mh.start_time;
				if (*out_frameCount == 0)
					mh->start_time = chunk_mh.start_time;
				continue;
			}
			if (fOutputBufferSize == 0) {
				int len;
				int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
				len = avcodec_decode_audio2(ffc, (short *)fOutputBuffer,
					&out_size, (uint8_t*)fChunkBuffer + fChunkBufferOffset,
					fChunkBufferSize);
				if (len < 0) {
					TRACE("########### audio decode error, "
						"fChunkBufferSize %ld, fChunkBufferOffset %ld\n",
						fChunkBufferSize, fChunkBufferOffset);
					out_size = 0;
					len = 0;
					fChunkBufferOffset = 0;
					fChunkBufferSize = 0;
//				} else {
//					TRACE("audio decode: len %d, out_size %d\n", len, out_size);
				}
				fChunkBufferOffset += len;
				fChunkBufferSize -= len;
				fOutputBufferOffset = 0;
				fOutputBufferSize = out_size;			
			}
		}
		fFrame += *out_frameCount;

//		TRACE("Played %Ld frames at time %Ld\n",*out_frameCount, mh->start_time);

	} else {	// Video

		media_header chunk_mh;
		status_t err;
		size_t size;

		err = GetNextChunk(&data, &size, &chunk_mh);
		if (err != B_OK) {
			TRACE("avCodec::Decode(): error 0x%08lx from GetNextChunk()\n",	err);
			return err;
		}

		mh->type = B_MEDIA_RAW_VIDEO;
//		mh->start_time = chunk_mh.start_time;
		mh->file_pos = 0;
		mh->orig_size = 0;
		mh->u.raw_video.field_gamma = 1.0;
		mh->u.raw_video.field_sequence = fFrame;
		mh->u.raw_video.field_number = 0;
		mh->u.raw_video.pulldown_number = 0;
		mh->u.raw_video.first_active_line = 1;
		mh->u.raw_video.line_count = fOutputVideoFormat.display.line_count;

		TRACE("[%c] start_time=%02d:%02d.%02d field_sequence=%lu\n",
			isAudio ? ('a') : ('v'),
			int((mh->start_time / 60000000) % 60),
			int((mh->start_time / 1000000) % 60),
			int((mh->start_time / 10000) % 100),
			mh->u.raw_video.field_sequence);

#ifdef DO_PROFILING
		prof_t1 = system_time();
#endif

		int got_picture = 0;
		int len;
		len = avcodec_decode_video(ffc, ffpicture, &got_picture,
			(uint8_t *)data, size);

//TRACE("FFDEC: PTS = %d:%d:%d.%d - ffc->frame_number = %ld "
//	"ffc->frame_rate = %ld\n", (int)(ffc->pts / (60*60*1000000)),
//	(int)(ffc->pts / (60*1000000)), (int)(ffc->pts / (1000000)),
//	(int)(ffc->pts % 1000000), ffc->frame_number, ffc->frame_rate);
//TRACE("FFDEC: PTS = %d:%d:%d.%d - ffc->frame_number = %ld "
//	"ffc->frame_rate = %ld\n", (int)(ffpicture->pts / (60*60*1000000)),
//	(int)(ffpicture->pts / (60*1000000)), (int)(ffpicture->pts / (1000000)),
//	(int)(ffpicture->pts % 1000000), ffc->frame_number, ffc->frame_rate);

		if (len < 0) {
			printf("[%c] avCodec: error in decoding frame %lld\n",
				isAudio?('a'):('v'), *out_frameCount);
		}

		if (got_picture) {
#ifdef DO_PROFILING
			prof_t2 = system_time();
#endif
//			TRACE("ONE FRAME OUT !! len=%d size=%ld (%s)\n", len, size,
//				pixfmt_to_string(ffc->pix_fmt));

			// Some decoders do not set pix_fmt until they have decoded 1 frame				
			if (conv_func == 0) {
				conv_func = resolve_colorspace(fOutputVideoFormat.display.format, ffc->pix_fmt);
			}
			opicture->data[0] = (uint8_t *)out_buffer;
			opicture->linesize[0] = fOutputVideoFormat.display.bytes_per_row;
			
			if (conv_func) {
				(*conv_func)(ffpicture, opicture,
					fOutputVideoFormat.display.line_width,
					fOutputVideoFormat.display.line_count);
			}
#ifdef DEBUG
			dump_ffframe(ffpicture, "ffpict");
//			dump_ffframe(opicture, "opict");
#endif
			*out_frameCount = 1;				
			fFrame++;

#ifdef DO_PROFILING
			prof_t3 = system_time();
			diff1 += prof_t2 - prof_t1;
			diff2 += prof_t3 - prof_t2;
			prof_cnt++;
			if (!(fFrame % 10)) {
				TRACE("[%c] profile: d1 = %lld, d2 = %lld (%Ld)\n",
					isAudio?('a'):('v'), diff1/prof_cnt, diff2/prof_cnt,
					fFrame);
			}
#endif
		}
	}

	fStartTime = (bigtime_t) (1000000LL * fFrame / fOutputFrameRate);

	return B_OK;
}

status_t
avCodecPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	BMediaFormats mediaFormats;
	if (B_OK != mediaFormats.InitCheck())
		return B_ERROR;

	for (int i = 0; i < num_codecs; i++) {
		media_format_description description;
		description.family = gCodecTable[i].family;
		switch(description.family) {
			case B_WAV_FORMAT_FAMILY:
				description.u.wav.codec = gCodecTable[i].fourcc;
				break;
			case B_AIFF_FORMAT_FAMILY:
				description.u.aiff.codec = gCodecTable[i].fourcc;
				break;
			case B_AVI_FORMAT_FAMILY:
				description.u.avi.codec = gCodecTable[i].fourcc;
				break;
			case B_MPEG_FORMAT_FAMILY:
				description.u.mpeg.id = gCodecTable[i].fourcc;
				break;
			case B_QUICKTIME_FORMAT_FAMILY:
				description.u.quicktime.codec = gCodecTable[i].fourcc;
				break;
			case B_MISC_FORMAT_FAMILY:
				description.u.misc.file_format =
					(uint32)(gCodecTable[i].fourcc >> 32);
				description.u.misc.codec = (uint32) gCodecTable[i].fourcc;
				break;
			default:
				break;
		}
		media_format format;
		format.type = gCodecTable[i].type;
		format.require_flags = 0;
		format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		if (B_OK != mediaFormats.MakeFormatFor(&description, 1, &format))
			return B_ERROR;
		avcodec_formats[i] = format;
	}

	*formats = avcodec_formats;
	*count = num_codecs;
	return B_OK;
}


Decoder *
avCodecPlugin::NewDecoder(uint index)
{
	return new avCodec();
}


MediaPlugin *instantiate_plugin()
{
	return new avCodecPlugin;
}

