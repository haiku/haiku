/******************************************************************************
/
/	File:			avCodec.cpp
/
/	Description:	FFMpeg Generic Decoder for BeOS Release 5.
/
/	Disclaimer:		This decoder is based on undocumented APIs.
/					Therefore it is likely, if not certain, to break in future
/					versions of BeOS. This piece of software is in no way
/					connected to or even supported by Be, Inc.
/
/	Copyright (C) 2001 François Revol. All Rights Reserved.
/   Some code from MPEG I Decoder from Carlos Hasan.
/   Had some help from Axeld
/
******************************************************************************/

#include <Debug.h>
#include <OS.h>
#include <Bitmap.h>
#include <string.h>
#include "avcodec.h"

struct wave_format_ex
{
	uint16 format_tag;
	uint16 channels;
	uint32 frames_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	// extra_data[extra_size]
} _PACKED;

/* uncommenting will make Decode() set the current thread priority to time sharing, so it won't totally freeze 
 * if you busy-loop in there (to help debug with CD Manager
 */
//#define UNREAL

#define CLOSE_ON_RESET

const char *media_type_name(int type)
{
	switch (type) {
		case B_MEDIA_NO_TYPE:
			return "B_MEDIA_NO_TYPE";
		case B_MEDIA_RAW_AUDIO:
			return "B_MEDIA_RAW_AUDIO";
		case B_MEDIA_RAW_VIDEO:
			return "B_MEDIA_RAW_VIDEO";
		case B_MEDIA_VBL:
			return "B_MEDIA_VBL";
		case B_MEDIA_TIMECODE:
			return "B_MEDIA_TIMECODE";
		case B_MEDIA_MIDI:
			return "B_MEDIA_MIDI";
		case B_MEDIA_TEXT:
			return "B_MEDIA_TEXT";
		case B_MEDIA_HTML:
			return "B_MEDIA_HTML";
		case B_MEDIA_MULTISTREAM:
			return "B_MEDIA_MULTISTREAM";
		case B_MEDIA_PARAMETERS:
			return "B_MEDIA_PARAMETERS";
		case B_MEDIA_ENCODED_AUDIO:
			return "B_MEDIA_ENCODED_AUDIO";
		case B_MEDIA_ENCODED_VIDEO:
			return "B_MEDIA_ENCODED_VIDEO";
		case B_MEDIA_UNKNOWN_TYPE:
		default:
			return "B_MEDIA_UNKNOWN_TYPE";
	}
}

char make_printable_char(uchar c)
{
//	c &= 0x7F;
	if (c >= 0x20 && c < 0x7F)
		return c;
	return '.';
}

void print_hex(unsigned char *buff, int len)
{
	int i, j;
	for(i=0; i<len+7;i+=8) {
		for (j=0; j<8; j++) {
			if (i+j < len)
				printf("%02X", buff[i+j]);
			else
				printf("  ");
			if (j==3)
				printf(" ");
		}
		printf("\t");
		for (j=0; j<8; j++) {
			if (i+j < len)
				printf("%c", make_printable_char(buff[i+j]));
			else
				printf(" ");
		}
		printf("\n");
	}

}

void print_media_header(media_header *mh)
{
	printf("media_header {%s, size_used: %ld, start_time: %lld (%02d:%02d.%02d), field_sequence=%lu, user_data_type: .4s, file_pos: %ld, orig_size: %ld, data_offset: %ld}\n", 
				media_type_name(mh->type), mh->size_used, mh->start_time, 
				int((mh->start_time / 60000000) % 60),
				int((mh->start_time / 1000000) % 60),
				int((mh->start_time / 10000) % 100), 
				(long)mh->u.raw_video.field_sequence,
				//&(mh->user_data_type), 
				(long)mh->file_pos,
				(long)mh->orig_size,
				mh->data_offset);
}

void print_media_decode_info(media_decode_info *info)
{
	if (info)
		printf("media_decode_info {time_to_decode: %lld, file_format_data_size: %ld, codec_data_size: %ld}\n", info->time_to_decode, info->file_format_data_size, info->codec_data_size);
	else
		printf("media_decode_info (null)\n");
}

avCodec::avCodec()
	:	fHeader(),
		fInfo(),
		fInputFormat(),
		fOutputVideoFormat(),
		fOutputAudioFormat(),
		fFrame(0),
		isAudio(false),
		fData(NULL),
		fOffset(0L),
		fSize(0L),
		fCodec(NULL),
		ffc(NULL),
		fCodecInitDone(false),
		conv_func(NULL),
		fExtraData(NULL),
		fExtraDataSize(0),
		fBlockAlign(0),
		fOutputBuffer(0)
{
	PRINT(("[%c] avCodec::avCodec()\n", isAudio?('a'):('v')));


	static volatile vint32 ff_init_count = 0;
	static bool ff_init_done = false;
	PRINT(("avCodec::register_decoder()\n"));

	// prevent multiple inits
	if (atomic_add(&ff_init_count, 1) > 1) {
	puts("NOT first init !");
		atomic_add(&ff_init_count, -1);
		// spin until the thread that is initing is done
		while (!ff_init_done)
			snooze(20000);
	} else {
	puts("first init !");
		avcodec_init();
		avcodec_register_all();
		ff_init_done = true;
	}

//	memset(&opicture, 0, sizeof(AVPicture));
	ffc = avcodec_alloc_context();
	ffpicture = avcodec_alloc_frame();
//	ffpicture->type = FF_BUFFER_TYPE_USER;
	opicture = avcodec_alloc_frame();
}


avCodec::~avCodec()
{
	PRINT(("[%c] avCodec::~avCodec()\n", isAudio?('a'):('v')));

	if(fCodecInitDone)
		avcodec_close(ffc);

	free(opicture);
	free(ffpicture);
	free(ffc);
	
	delete [] fExtraData;
	delete [] fOutputBuffer;
}


void avCodec::GetCodecInfo(media_codec_info *mci)
{
	sprintf(mci->short_name, "ff:%s", fCodec->name);
	sprintf(mci->pretty_name, "%s (libavcodec %s)", gCodecTable[ffcodec_index_in_table].prettyname, fCodec->name);
}


status_t avCodec::Setup(media_format *input_format, const void *in_info, int32 in_size)
{
	if (input_format->type != B_MEDIA_ENCODED_AUDIO && input_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_ERROR;
		
	isAudio = (input_format->type == B_MEDIA_ENCODED_AUDIO);

	if (isAudio)
		fOutputBuffer = new char[100000];

#if DEBUG
	char buffer[1024];
	string_for_format(*input_format, buffer, sizeof(buffer));
	PRINT(("[%c]   input_format=%s\n", isAudio?('a'):('v'), buffer));
	PRINT(("[%c]   in_info_size=%ld\n", isAudio?('a'):('v'), in_size));
	print_hex((uchar *)in_info, in_size);
	PRINT(("[%c]   user_data_type=%08lx\n", isAudio?('a'):('v'), input_format->user_data_type));
	print_hex((uchar *)input_format->user_data, 48);
//	PRINT(("[%c]   meta_data_size=%ld\n", isAudio?('a'):('v'), input_format->meta_data_size));
#else
	(void)(in_info);
	(void)(in_size);
#endif

	media_format_description descr;
	for (int32 i = 0; gCodecTable[i].id; i++)
	{
		ffcodec_index_in_table = i;
		uint32 cid;
		
		if(BMediaFormats().GetCodeFor(*input_format,gCodecTable[i].family,&descr) == B_OK
		   && gCodecTable[i].type == input_format->type)
			{
			PRINT(("  codec id = \"%c%c%c%c\"\n",	(descr.u.avi.codec >> 24) & 0xff,
													(descr.u.avi.codec >> 16) & 0xff,
													(descr.u.avi.codec >> 8) & 0xff,
													descr.u.avi.codec & 0xff));
			switch(gCodecTable[i].family) {
				case B_WAV_FORMAT_FAMILY:
					cid = descr.u.wav.codec;
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
				default:
				puts("ERR family");
					return B_ERROR;
			}
			if (gCodecTable[i].family == descr.family && gCodecTable[i].fourcc == cid) {
				fCodec = avcodec_find_decoder(gCodecTable[i].id);
				if (!fCodec) {
					printf("avCodec: unable to find the correct ffmpeg decoder (id = %d)!!!\n",gCodecTable[i].id);
					return B_ERROR;
				}
				
//				ffc->codec_id = gCodecTable[i].id;

				if (gCodecTable[i].family == B_WAV_FORMAT_FAMILY) {
					const wave_format_ex *wfmt_data = (const wave_format_ex *)input_format->MetaData();
					int wfmt_size = input_format->MetaDataSize();
					if (wfmt_data && wfmt_size) {
						fBlockAlign = wfmt_data->block_align;
						fExtraDataSize = wfmt_data->extra_size;
						printf("############# fExtraDataSize %d\n", fExtraDataSize);
						printf("############# fBlockAlign %d\n", fBlockAlign);
						printf("############# wfmt_size %d\n", wfmt_size);
						printf("############# wave_format_ex %ld\n", sizeof(wave_format_ex));
						if (fExtraDataSize) {
							fExtraData = new char [fExtraDataSize];
							memcpy(fExtraData, wfmt_data + 1, fExtraDataSize);

							uint8 *p = (uint8 *)fExtraData;
							printf("extra_data: %d: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
								fExtraDataSize, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
						}
//						static const unsigned char wma_extradata[6] = { 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 };
//						fExtraData = (void *)wma_extradata;
//						fExtraDataSize = 6;
					}
				}

				fInputFormat = *input_format;
				return B_OK;
			}
		}
	}
	printf("avCodec::Setup failed!\n");
	return B_ERROR;
}

status_t avCodec::Seek(uint32 in_towhat,int64 in_requiredFrame, int64 *inout_frame,
	bigtime_t in_requiredTime, bigtime_t *inout_time)
{

	// reset the ffmpeg codec
	// to flush buffers, so we keep the sync

#ifdef CLOSE_ON_RESET

	if (isAudio && fCodecInitDone)
	{
		fCodecInitDone = false;
		avcodec_close(ffc);
		fCodecInitDone = (avcodec_open(ffc, fCodec) >= 0);
		fData = NULL;
		fSize = 0LL;
		fOffset = 0;
	}

#endif /* CLOSE_ON_RESET */

	fFrame = *inout_frame;

	return B_OK;
}


status_t avCodec::NegotiateOutputFormat(media_format *inout_format)
{
//	int ffPixelFmt;
	enum PixelFormat ffPixelFmt;
	
	PRINT(("[%c] avCodec::Format()\n", isAudio?('a'):('v')));

#if DEBUG
	char buffer[1024];
	string_for_format(*inout_format, buffer, sizeof(buffer));
	PRINT(("[%c]  in_format=%s\n", isAudio?('a'):('v'), buffer));
#endif

	// close any previous instance
	if (fCodecInitDone) {
		fCodecInitDone = false;
		avcodec_close(ffc);
	}

	if (isAudio) {
		fOutputAudioFormat = media_raw_audio_format::wildcard;
		fOutputAudioFormat.format = media_raw_audio_format::B_AUDIO_SHORT;
		fOutputAudioFormat.byte_order = B_MEDIA_HOST_ENDIAN;
		fOutputAudioFormat.frame_rate = fInputFormat.u.encoded_audio.output.frame_rate;
		fOutputAudioFormat.channel_count = fInputFormat.u.encoded_audio.output.channel_count;
		fOutputAudioFormat.buffer_size = 1024 * fInputFormat.u.encoded_audio.output.channel_count;
		inout_format->type = B_MEDIA_RAW_AUDIO;
		inout_format->u.raw_audio = fOutputAudioFormat;

		ffc->bit_rate = (int) fInputFormat.u.encoded_audio.bit_rate;
		ffc->sample_rate = (int) fInputFormat.u.encoded_audio.output.frame_rate;
		ffc->channels = fInputFormat.u.encoded_audio.output.channel_count;

		ffc->block_align = fBlockAlign;
		ffc->extradata = fExtraData;
		ffc->extradata_size = fExtraDataSize;

		if (avcodec_open(ffc, fCodec) >= 0)
			fCodecInitDone = true;

		if (ffc->codec_id == CODEC_ID_WMAV1) {
			printf("########################### WMA1\n");
		}
		if (ffc->codec_id == CODEC_ID_WMAV2) {
			printf("########################### WMA2\n");
		}
		if (ffc->codec_id == CODEC_ID_MP2) {
			printf("########################### MP3\n");
		}

		PRINT(("audio: bit_rate = %d, sample_rate = %d, chans = %d\n", ffc->bit_rate, ffc->sample_rate, ffc->channels));

		fStartTime = 0;
		fOutputFrameCount = fOutputAudioFormat.buffer_size / (2 * fOutputAudioFormat.channel_count);
		fOutputFrameRate = fOutputAudioFormat.frame_rate;
		fChunkBuffer = 0;
		fChunkBufferOffset = 0;
		fChunkBufferSize = 0;
		fOutputBufferOffset = 0;
		fOutputBufferSize = 0;

	} else {	// no AUDIO_CODEC

		fOutputVideoFormat = fInputFormat.u.encoded_video.output;

		ffc->width = fOutputVideoFormat.display.line_width;
		ffc->height = fOutputVideoFormat.display.line_count;
		ffc->frame_rate = (int)(fOutputVideoFormat.field_rate * ffc->frame_rate_base);

		// make MediaPlayer happy (if not in rgb32 screen depth and no overlay,
		// it will only ask for YCbCr, which DrawBitmap doesn't handle, so the default
		// colordepth is RGB32)
		if (inout_format->u.raw_video.display.format == B_YCbCr422)
			fOutputVideoFormat.display.format = B_YCbCr422;
		else
			fOutputVideoFormat.display.format = B_RGB32;

		// search for a pixel-format the codec handles
		while (!fCodecInitDone)
		{
			// find the proper ff pixel format and the convertion func
			conv_func = resolve_colorspace(fOutputVideoFormat.display.format, &ffPixelFmt);
			PRINT(("ffc = %08lx, fCodec = %08lx, conv_func = %08lx\n", ffc, fCodec, conv_func));
			if (!conv_func)
			{
				PRINT(("no conv_func (1) !\n"));
//				return B_ERROR;
			}
//			ffc->pix_fmt = ffPixelFmt = 0; // DEBUG !!!
//			ffc->pix_fmt = 
//			ffPixelFmt = PIX_FMT_YUV420P; // DEBUG !!!
//			ffc->pix_fmt = ffPixelFmt = PIX_FMT_YUV420P; // DEBUG !!!
			if (avcodec_open(ffc, fCodec) >= 0)
				fCodecInitDone = true;
			else
				printf("avCodec: error in avcodec_open()\n");

			conv_func = resolve_colorspace(fOutputVideoFormat.display.format, &ffc->pix_fmt);
			PRINT(("ffc = %08lx, fCodec = %08lx, conv_func = %08lx, ffcolspace= %s\n", ffc, fCodec, conv_func, pixfmt_to_string(ffc->pix_fmt)));
			if (!conv_func)
			{
				PRINT(("no conv_func (2) !\n"));
				return B_ERROR;
			}
			PRINT(("PIXFMT: %s -> %s\n", pixfmt_to_string(ffPixelFmt), pixfmt_to_string(ffc->pix_fmt)));
		}

		if (fOutputVideoFormat.display.format == B_YCbCr422)
			fOutputVideoFormat.display.bytes_per_row = 2 * fOutputVideoFormat.display.line_width;
		else
			fOutputVideoFormat.display.bytes_per_row = 4 * fOutputVideoFormat.display.line_width;

		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = fOutputVideoFormat;
	
	}

	inout_format->require_flags = 0;
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	
#if DEBUG		
	string_for_format(*inout_format, buffer, sizeof(buffer));
	PRINT(("[%c]  out_format=%s\n", isAudio?('a'):('v'), buffer));
#endif

	return B_OK;
}

status_t avCodec::Decode(void *out_buffer, int64 *out_frameCount, media_header *mh,
	media_decode_info *info)
{
	if (out_buffer == NULL || out_frameCount == NULL || mh == NULL || !fCodecInitDone)
		return B_BAD_VALUE;

	int len = 0;
	int got_picture = 0;
	
#ifdef DO_PROFILING
	bigtime_t prof_t1, prof_t2, prof_t3;
	static bigtime_t diff1 = 0, diff2 = 0;
	static long prof_cnt = 0;
#endif

#ifdef UNREAL
	set_thread_priority(find_thread(NULL), B_NORMAL_PRIORITY);
#endif

	PRINT(("[%c] avCodec::Decode()\n", isAudio?('a'):('v')));

#if DEBUG
//		asm("int     $0x0");
#endif

	if (isAudio) {

		mh->start_time = fStartTime;

		printf("audio start_time %.6f\n", mh->start_time / 1000000.0);

		char *output_buffer = (char *)out_buffer;
		*out_frameCount = 0;
		while (*out_frameCount < fOutputFrameCount) {
			if (fOutputBufferSize < 0) {
				printf("############ fOutputBufferSize %ld\n", fOutputBufferSize);
				fOutputBufferSize = 0;
			}
			if (fChunkBufferSize < 0) {
				printf("############ fChunkBufferSize %ld\n", fChunkBufferSize);
				fChunkBufferSize = 0;
			}
		
			if (fOutputBufferSize > 0) {
				int32 frames = min_c(fOutputFrameCount - *out_frameCount, fOutputBufferSize / 4);
				memcpy(output_buffer, fOutputBuffer + fOutputBufferOffset, frames * 4);
				fOutputBufferOffset += frames * 4;
				fOutputBufferSize -= frames * 4;
				output_buffer += frames * 4;
				*out_frameCount += frames;
				fStartTime += (bigtime_t)((1000000LL * frames) / fOutputFrameRate);
				continue;
			}
			if (fChunkBufferSize == 0) {
				media_header mh;
				status_t err;
				err = GetNextChunk((void **)&fChunkBuffer, &fChunkBufferSize, &mh);
				if (err != B_OK || fChunkBufferSize < 0) {
					printf("GetNextChunk error\n");
					fChunkBufferSize = 0;
					break;
				}
				fChunkBufferOffset = 0;
				fStartTime = mh.start_time;
				continue;
			}
			if (fOutputBufferSize == 0) {
				int len, out_size;
				len = avcodec_decode_audio(ffc, (short *)fOutputBuffer, &out_size, (uint8_t *)fChunkBuffer + fChunkBufferOffset, fChunkBufferSize);
				if (len < 0) {
					printf("########### audio decode error, fChunkBufferSize %ld, fChunkBufferOffset %ld\n", fChunkBufferSize, fChunkBufferOffset);
					out_size = 0;
					len = 0;
					fChunkBufferOffset = 0;
					fChunkBufferSize = 0;
				} else
					printf("audio decode: len %d, out_size %d\n", len, out_size);
				fChunkBufferOffset += len;
				fChunkBufferSize -= len;
				fOutputBufferOffset = 0;
				fOutputBufferSize = out_size;			
			}
		}

	} else {	// no AUDIO_CODEC
	// VIDEO
		status_t err;

#ifdef DEBUG
//print_media_header(mh);
//print_media_decode_info(info);
#endif
		mh->type = B_MEDIA_RAW_VIDEO;
//		mh->start_time = (bigtime_t) (1000000.0 * fFrame / fOutputVideoFormat.field_rate);
		mh->start_time = fHeader.start_time;
		mh->file_pos = mh->orig_size = 0;
		mh->u.raw_video.field_gamma = 1.0;
		mh->u.raw_video.field_sequence = fFrame;
		mh->u.raw_video.field_number = 0;
		mh->u.raw_video.pulldown_number = 0;
		mh->u.raw_video.first_active_line = 1;
		mh->u.raw_video.line_count = fOutputVideoFormat.display.line_count;
/**/
		PRINT(("[%c] start_time=%02d:%02d.%02d field_sequence=%u\n",
				isAudio?('a'):('v'),
				int((mh->start_time / 60000000) % 60),
				int((mh->start_time / 1000000) % 60),
				int((mh->start_time / 10000) % 100),
				mh->u.raw_video.field_sequence));
/**/			
		fSize = 0L;
		err = GetNextChunk(&fData, &fSize, &fHeader);
		if (err != B_OK) {
			PRINT(("avCodec::Decode(): error 0x%08lx from GetNextChunk()\n", err));
			return B_ERROR;
		}
	PRINT(("avCodec::Decode(): passed GetNextChunk(), fSize=%ld\n", fSize));

#ifdef DEBUG
/*
	puts("Chunk:");
	for(i=0; i<2;i+=8) {
		char *buff = (char *)fData;
		printf("%08lX %08lX\t", *(long *)&buff[i], *(long *)&buff[i+4]);
		printf("%c%c%c%c%c%c%c%c\n", (buff[i] & 0x7F), (buff[i+1] & 0x7F), (buff[i+2] & 0x7F), (buff[i+3] & 0x7F), (buff[i+4] & 0x7F), (buff[i+5] & 0x7F), (buff[i+6] & 0x7F), (buff[i+7] & 0x7F));
	}
*/
#endif

		// mmu_man 11/15/2002
		//ffc->frame_number = fFrame;

#ifdef DO_PROFILING
		prof_t1 = system_time();
#endif

//	PRINT(("avCodec::Decode(): before avcodec_decode_video()\n"));


//		len = avcodec_decode_video(ffc, &ffpicture, &got_picture, (uint8_t *)fData, fSize);
		len = avcodec_decode_video(ffc, ffpicture, &got_picture, (uint8_t *)fData, fSize);

//	PRINT(("avCodec::Decode(): after avcodec_decode_video()\n"));

//printf("FFDEC: PTS = %d:%d:%d.%d - ffc->frame_number = %ld ffc->frame_rate = %ld\n", (int)(ffc->pts / (60*60*1000000)), (int)(ffc->pts / (60*1000000)), (int)(ffc->pts / (1000000)), (int)(ffc->pts % 1000000), ffc->frame_number, ffc->frame_rate);
//printf("FFDEC: PTS = %d:%d:%d.%d - ffc->frame_number = %ld ffc->frame_rate = %ld\n", (int)(ffpicture->pts / (60*60*1000000)), (int)(ffpicture->pts / (60*1000000)), (int)(ffpicture->pts / (1000000)), (int)(ffpicture->pts % 1000000), ffc->frame_number, ffc->frame_rate);

		if (len < 0)
			printf("[%c] avCodec: error in decoding frame %lld\n", isAudio?('a'):('v'), *out_frameCount);

		if (got_picture)
		{
#ifdef DO_PROFILING
			prof_t2 = system_time();
#endif
			PRINT(("ONE FRAME OUT !! len=%ld sz=%ld (%s)\n", len, fSize, pixfmt_to_string(ffc->pix_fmt)));
/*
			opicture.data[0] = (uint8_t *)out_buffer;
			opicture.linesize[0] = fOutputVideoFormat.display.bytes_per_row;
			
			(*conv_func)(&ffpicture, &opicture, fOutputVideoFormat.display.line_width, fOutputVideoFormat.display.line_count);
*/
			opicture->data[0] = (uint8_t *)out_buffer;
			opicture->linesize[0] = fOutputVideoFormat.display.bytes_per_row;
			
//	PRINT(("avCodec::Decode(): before conv_func()\n"));
			(*conv_func)(ffpicture, opicture, fOutputVideoFormat.display.line_width, fOutputVideoFormat.display.line_count);
//	PRINT(("avCodec::Decode(): after conv_func()\n"));
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
			if (!(fFrame % 10))
				printf("[%c] profile: d1 = %lld, d2 = %lld (%ld)\n", isAudio?('a'):('v'), diff1/prof_cnt, diff2/prof_cnt, fFrame);
#endif
		}
	}
//	PRINT(("END of avCodec::Decode()\n"));
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
			case B_AVI_FORMAT_FAMILY:
				description.u.avi.codec = gCodecTable[i].fourcc;
				break;
			case B_MPEG_FORMAT_FAMILY:
				description.u.mpeg.id = gCodecTable[i].fourcc;
				break;
			case B_QUICKTIME_FORMAT_FAMILY:
				description.u.quicktime.codec = gCodecTable[i].fourcc;
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

