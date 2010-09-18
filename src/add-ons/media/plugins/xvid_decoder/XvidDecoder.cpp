/*
 * XvidDecoder.cpp - XviD plugin for the Haiku Operating System
 *
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#define DEBUG 0
#define PROPER_SEEKING 0
#define PRINT_FOURCC 0

#define MIN_USEFUL_BYTES 1

#include "XvidDecoder.h"

//#include <fenv.h>
#include <new>
#include <setjmp.h>
#include <signal.h>
#include <string.h>

#include <ByteOrder.h>
#include <Debug.h>
#include <MediaTrack.h>
#include <OS.h>

#include <MediaDefs.h>
#include <File.h>
#include <Bitmap.h>

#include "supported_codecs.h"


using std::nothrow;


#if DEBUG
static const char*
media_type_name(int type)
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

static char
make_printable_char(uchar c)
{
	if (c >= 0x20 && c < 0x7F)
		return c;
	return '.';
}

static void
print_hex(unsigned char* buff, int len)
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

static void
print_media_header(media_header* mh)
{
	printf("media_header {%s, size_used: %ld, start_time: %lld (%02d:%02d.%02d), "
		"field_sequence=%lu, user_data_type: .4s, file_pos: %ld, orig_size: %ld, "
		"data_offset: %ld}\n", 
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

static void
print_media_decode_info(media_decode_info *info)
{
	if (info) {
		printf("media_decode_info {time_to_decode: %lld, "
			"file_format_data_size: %ld, codec_data_size: %ld}\n",
			info->time_to_decode, info->file_format_data_size,
			info->codec_data_size);
	} else
		printf("media_decode_info (null)\n");
}
#endif // DEBUG


// #pragma mark -


XvidDecoder::XvidDecoder()
	: Decoder()
	, fInputFormat()
	, fOutputVideoFormat()

	, fXvidDecoderHandle(NULL)
	, fXvidColorspace(0)

	, fFrame(0)
	, fIndexInCodecTable(-1)

	, fChunkBuffer(NULL)
	, fWrappedChunkBuffer(NULL)
	, fChunkBufferHandle(NULL)
	, fChunkBufferSize(0)
	, fLeftInChunkBuffer(0)
	, fDiscontinuity(false)
{
}


XvidDecoder::~XvidDecoder()
{
	_XvidUninit();
	delete[] fWrappedChunkBuffer;
}


void
XvidDecoder::GetCodecInfo(media_codec_info* mci)
{
	PRINT(("XvidDecoder::GetCodecInfo()\n"));

	if (mci == NULL)
		return;// B_BAD_VALUE;

	if (fIndexInCodecTable < 0)
		return;// B_NO_INIT;

	sprintf(mci->short_name, "xvid");
	sprintf(mci->pretty_name, "xvid - %s",
		gCodecTable[fIndexInCodecTable].prettyName);

	mci->id = 0;
	mci->sub_id = 0;

	return;// B_OK;
}


status_t
XvidDecoder::Setup(media_format* inputFormat, const void* inInfo,
	size_t inSize)
{
	if (inputFormat == NULL)
		return B_BAD_VALUE;

	if (inputFormat->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_VALUE;

//	PRINT(("%p->XvidDecoder::Setup()\n", this));

//#if DEBUG
//	char buffer[1024];
//	string_for_format(*inputFormat, buffer, sizeof(buffer));
//	PRINT(("   inputFormat=%s\n", buffer));
//	PRINT(("   inSize=%d\n", inSize));
//	print_hex((uchar*)inInfo, inSize);
//	PRINT(("   user_data_type=%08lx\n", (int)inputFormat->user_data_type));
//	print_hex((uchar*)inputFormat->user_data, 48);
//#endif

	uint32 codecID = 0;
	media_format_family familyID = B_ANY_FORMAT_FAMILY;

	// hacky... get the exact 4CC from there if it's in, to help xvid
	// handle broken files
//	if ((inputFormat->user_data_type == B_CODEC_TYPE_INFO)
//	 && !memcmp(inputFormat->user_data, "AVI ", 4)) {
//		codecID = ((uint32*)inputFormat->user_data)[1];
//		familyID = B_AVI_FORMAT_FAMILY;
//		PRINT(("XvidDecoder::Setup() - AVI 4CC: %4s\n",
//			inputFormat->user_data + 4));
//	}

	if (codecID == 0) {
		BMediaFormats formats;
		media_format_description descr;
		if (formats.GetCodeFor(*inputFormat, B_QUICKTIME_FORMAT_FAMILY,
				&descr) == B_OK) {
			codecID = descr.u.quicktime.codec;
			familyID = B_QUICKTIME_FORMAT_FAMILY;

			#if PRINT_FOURCC
			uint32 bigEndianID = B_HOST_TO_BENDIAN_INT32(codecID);
			printf("%p->XvidDecoder::Setup() - QT 4CC: %.4s\n", this,
				(const char*)&bigEndianID);
			#endif
		} else if (formats.GetCodeFor(*inputFormat, B_AVI_FORMAT_FAMILY,
				&descr) == B_OK) {
			codecID = descr.u.avi.codec;
			familyID = B_AVI_FORMAT_FAMILY;

			#if PRINT_FOURCC
			uint32 bigEndianID = B_HOST_TO_BENDIAN_INT32(codecID);
			printf("%p->XvidDecoder::Setup() - AVI 4CC: %.4s\n", this,
				(const char*)&bigEndianID);
			#endif
		} else if (formats.GetCodeFor(*inputFormat, B_MPEG_FORMAT_FAMILY,
				&descr) == B_OK) {
			codecID = descr.u.mpeg.id;
			familyID = B_MPEG_FORMAT_FAMILY;

			#if PRINT_FOURCC
			printf("%p->XvidDecoder::Setup() - MPEG ID: %ld\n", this, codecID);
			#endif
		}
	}

	if (codecID == 0)
		return B_ERROR;

	for (int32 i = 0; i < gSupportedCodecsCount; i++) {
		if (gCodecTable[i].family == familyID
			&& gCodecTable[i].fourcc == codecID) {
			PRINT(("%p->XvidDecoder::Setup() - found codec in the table "
				"at %ld.\n", this, i));
			fIndexInCodecTable = i;
			fInputFormat = *inputFormat;
			return B_OK;
		}
	}

	#if PRINT_FOURCC
	printf("%p->XvidDecoder::Setup() - no matching codec found in the "
		"table.\n", this);
	#endif

	return B_ERROR;
}


status_t
XvidDecoder::NegotiateOutputFormat(media_format* _inoutFormat)
{
	PRINT(("%p->XvidDecoder::NegotiateOutputFormat()\n", this));
	
	if (_inoutFormat == NULL)
		return B_BAD_VALUE;

#if DEBUG
	char buffer[1024];
	buffer[0] = 0;
	if (string_for_format(*_inoutFormat, buffer, sizeof(buffer)) == B_OK)
		PRINT(("  _inoutFormat = %s\n", buffer));
#endif

	// init our own output format from the sniffed values
	fOutputVideoFormat = fInputFormat.u.encoded_video.output;

	// suggest a default colorspace
	color_space askedFormat = _inoutFormat->u.raw_video.display.format;
	color_space supportedFormat;
	uint32 bpr;
	switch (askedFormat) {
		case B_NO_COLOR_SPACE:
			// suggest preferred format
		case B_YCbCr422:
			supportedFormat = B_YCbCr422;
			fXvidColorspace = XVID_CSP_YUY2;
			bpr = fOutputVideoFormat.display.line_width * 2;
			break;
		case B_YCbCr420:
			supportedFormat = askedFormat;
			fXvidColorspace = XVID_CSP_I420;
			bpr = fOutputVideoFormat.display.line_width
				+ (fOutputVideoFormat.display.line_width + 1) / 2;
			break;
		case B_RGB32_BIG:
			supportedFormat = askedFormat;
			fXvidColorspace = XVID_CSP_RGBA;
			bpr = fOutputVideoFormat.display.line_width * 4;
			break;
		case B_RGB24:
			supportedFormat = askedFormat;
			fXvidColorspace = XVID_CSP_BGR;
			bpr = fOutputVideoFormat.display.line_width * 3;
			break;
		case B_RGB16:
			supportedFormat = askedFormat;
			fXvidColorspace = XVID_CSP_RGB565;
			bpr = fOutputVideoFormat.display.line_width * 2;
			break;
		case B_RGB15:
			supportedFormat = askedFormat;
			fXvidColorspace = XVID_CSP_RGB555;
			bpr = fOutputVideoFormat.display.line_width * 2;
			break;

		default:
			fprintf(stderr, "XvidDecoder::NegotiateOutputFormat() - Application "
				"asked for unsupported colorspace, using fallback "
				"of B_RGB32.\n");
		case B_RGB32:
			supportedFormat = B_RGB32;
			fXvidColorspace = XVID_CSP_BGRA;
			bpr = fOutputVideoFormat.display.line_width * 4;
			break;
	}

	// enforce the colorspace we support
	fOutputVideoFormat.display.format = supportedFormat;
	_inoutFormat->u.raw_video.display.format = supportedFormat;

	// enforce supported bytes per row
	bpr = max_c(_inoutFormat->u.raw_video.display.bytes_per_row,
			((bpr + 3) / 4) * 4);
	fOutputVideoFormat.display.bytes_per_row = bpr;
	_inoutFormat->u.raw_video.display.bytes_per_row = bpr;

	_inoutFormat->type = B_MEDIA_RAW_VIDEO;
	_inoutFormat->u.raw_video = fOutputVideoFormat;
	_inoutFormat->require_flags = 0;
	_inoutFormat->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;

#if DEBUG
	if (string_for_format(*_inoutFormat, buffer, sizeof(buffer)) == B_OK)
		PRINT(("%p->XvidDecoder: out_format=%s\n", this, buffer));
#endif

	return _XvidInit() == 0 ? B_OK : B_ERROR;
}


status_t
XvidDecoder::Decode(void* outBuffer, int64* outFrameCount, media_header* mh,
	media_decode_info* info)
{
	if (outBuffer == NULL || outFrameCount == NULL || mh == NULL)
		return B_BAD_VALUE;
	
//	PRINT((%p->"XvidDecoder::Decode()\n", this));

	*outFrameCount = 0;
	
	// are we in a hurry ?
	bool hurryUp = (!info || (info->time_to_decode > 0)) ? false : true;

	mh->type = B_MEDIA_UNKNOWN_TYPE;
	mh->start_time = 0;
	mh->size_used = 0;
	mh->file_pos = 0;
	mh->orig_size = 0;
	mh->data_offset = 0;
	mh->u.raw_video.field_gamma = 1.0;
	mh->u.raw_video.field_sequence = 0;
	mh->u.raw_video.field_number = 0;
	mh->u.raw_video.pulldown_number = 0;
	mh->u.raw_video.first_active_line = 1;
	mh->u.raw_video.line_count = 0;

	#if DEBUG
	int32 chunk = 0;
	#endif

	status_t ret = B_OK;

	do {
		if (fLeftInChunkBuffer <= MIN_USEFUL_BYTES) {
			uint8* leftOverBuffer = NULL;
			size_t leftOverBufferSize = 0;
			if (fLeftInChunkBuffer > 0) {
				// we need to wrap the chunk buffer
				// create a temporary buffer to hold the remaining data
				leftOverBufferSize = fLeftInChunkBuffer;
				leftOverBuffer = new (nothrow) uint8[leftOverBufferSize];
				if (!leftOverBuffer)
					ret = B_NO_MEMORY;
				else {
					memcpy(leftOverBuffer, fChunkBufferHandle,
						leftOverBufferSize);
				}
			}
			// we don't need the previous wrapped buffer anymore in
			// case we had it
			delete[] fWrappedChunkBuffer;
			fWrappedChunkBuffer = NULL;

			// read new data
			if (ret >= B_OK)
				ret = GetNextChunk(&fChunkBuffer, &fChunkBufferSize, mh);
			if (ret >= B_OK) {
				if (leftOverBuffer) {
					fWrappedChunkBuffer = new (nothrow) uint8[fChunkBufferSize
						+ leftOverBufferSize];
					if (!fWrappedChunkBuffer)
						ret = B_NO_MEMORY;
					else {
						// copy the left over buffer to the beginning of the
						// wrapped chunk buffer
						memcpy(fWrappedChunkBuffer, leftOverBuffer,
							leftOverBufferSize);
						// copy the new chunk buffer after that
						memcpy(fWrappedChunkBuffer + leftOverBufferSize,
							fChunkBuffer, fChunkBufferSize);

						fChunkBufferSize += leftOverBufferSize;
						fLeftInChunkBuffer = fChunkBufferSize;
						fChunkBufferHandle = (const char*)fWrappedChunkBuffer;
					}
				} else {
					fLeftInChunkBuffer = fChunkBufferSize;
					fChunkBufferHandle = (const char*)fChunkBuffer;
				}
			}
			if (ret < B_OK) {
				fChunkBufferSize = 0;
				fChunkBuffer = NULL;
				fChunkBufferHandle = NULL;
			}
			delete[] leftOverBuffer;
		}

		// Check if there is a negative number of useful bytes left in buffer
		// This means we went too far
		if (!fChunkBufferHandle || fLeftInChunkBuffer < 0) {
			break;
		}

		xvid_dec_stats_t xvidDecoderStats;

		// This loop is needed to handle VOL/NVOP reading
		do {
			// Decode frame
			int usedBytes = _XvidDecode((uchar*)fChunkBufferHandle,
				(uchar*)outBuffer, fLeftInChunkBuffer, &xvidDecoderStats,
				hurryUp);

			// Resize image buffer if needed
			if (xvidDecoderStats.type == XVID_TYPE_VOL) {

				// Check if old buffer is smaller
				if ((int)(fOutputVideoFormat.display.line_width
					* fOutputVideoFormat.display.line_count)
					< xvidDecoderStats.data.vol.width
					* xvidDecoderStats.data.vol.height) {

					fprintf(stderr, "XvidDecoder::Decode() - image size "
						"changed: %dx%d\n",
						xvidDecoderStats.data.vol.width,
						xvidDecoderStats.data.vol.height);

					return B_ERROR;
				}
			}

			// Update buffer pointers
			if (usedBytes > 0) {
				fChunkBufferHandle += usedBytes;
				fLeftInChunkBuffer -= usedBytes;
			}

//			PRINT((%p->"XvidDecoder::Decode() - chunk %d: %d bytes consumed, "
//				"%d bytes in buffer\n", this, chunk++, usedBytes,
//				fLeftInChunkBuffer));

		} while (xvidDecoderStats.type <= 0
			&& fLeftInChunkBuffer > MIN_USEFUL_BYTES);

		if (xvidDecoderStats.type > XVID_TYPE_NOTHING) {
			// got a full frame
			mh->size_used = fOutputVideoFormat.display.line_count
				* fOutputVideoFormat.display.bytes_per_row;
			break;
		}

	} while (fLeftInChunkBuffer > MIN_USEFUL_BYTES || ret >= B_OK);

	if (ret != B_OK) {
		PRINT(("%p->XvidDecoder::Decode() - error: %s\n", this,
			strerror(ret)));
		return ret;
	}

	mh->type = B_MEDIA_RAW_VIDEO;
	mh->start_time = (bigtime_t) (1000000.0 * fFrame
		/ fOutputVideoFormat.field_rate);
	mh->u.raw_video.field_sequence = fFrame;
	mh->u.raw_video.first_active_line = 1;
	mh->u.raw_video.line_count = fOutputVideoFormat.display.line_count;

	*outFrameCount = 1;

	fFrame++;

	PRINT(("%p->XvidDecoder::Decode() - start_time=%02d:%02d.%02d "
		"field_sequence=%u\n", this,
		int((mh->start_time / 60000000) % 60),
		int((mh->start_time / 1000000) % 60),
		int((mh->start_time / 10000) % 100),
		mh->u.raw_video.field_sequence));

//#if DEBUG
//	print_media_header(mh);
//	print_media_decode_info(info);
//#endif

	return B_OK;
}


status_t
XvidDecoder::SeekedTo(int64 frame, bigtime_t time)
{
	PRINT(("%p->XvidDecoder::SeekedTo(frame=%lld, time=%lld)\n",
		this, frame, time));

	fFrame = frame;

	fChunkBuffer = NULL;
	fChunkBufferHandle = NULL;
	fChunkBufferSize = 0;
	fLeftInChunkBuffer = 0;

	// this will cause the xvid core to discard any cached stuff
	fDiscontinuity = true;

	return B_OK;
}


// #pragma mark -


static bool
init_xvid(bool useAssembler, int debugLevel)
{
	// XviD core initialization
	xvid_gbl_init_t xvidGlobalInit;

	// reset the structure with zeros
	memset(&xvidGlobalInit, 0, sizeof(xvid_gbl_init_t));

	// version
	xvidGlobalInit.version = XVID_VERSION;

	// assembly usage setting
	if (useAssembler)
		xvidGlobalInit.cpu_flags = 0;
	else
		xvidGlobalInit.cpu_flags = XVID_CPU_FORCE;

	// debug level
	xvidGlobalInit.debug = debugLevel;

	xvid_global(NULL, 0, &xvidGlobalInit, NULL);

	return true;
}


static bool sXvidInitialized = init_xvid(true, 0);


// _XvidInit
int
XvidDecoder::_XvidInit()
{
	if (fXvidDecoderHandle != NULL)
		return 0;

	// XviD decoder initialization
	xvid_dec_create_t xvidDecoderCreate;

	// reset the structure with zeros
	memset(&xvidDecoderCreate, 0, sizeof(xvid_dec_create_t));

	// version
	xvidDecoderCreate.version = XVID_VERSION;

	// bitmap dimensions -- xvidcore will resize when needed
	xvidDecoderCreate.width = 0;
	xvidDecoderCreate.height = 0;

	int ret = xvid_decore(NULL, XVID_DEC_CREATE, &xvidDecoderCreate, NULL);

	if (ret == 0)
		fXvidDecoderHandle = xvidDecoderCreate.handle;

	return ret;
}

// _XvidUninit
int
XvidDecoder::_XvidUninit()
{
	if (fXvidDecoderHandle == NULL)
		return 0;

	int ret = xvid_decore(fXvidDecoderHandle, XVID_DEC_DESTROY, NULL, NULL);
	if (ret == 0) {
		fXvidDecoderHandle = NULL;
		fXvidColorspace = 0;
	}
	return ret;
}

// handle_fp_exeption
static void
handle_fp_exeption(int sig, void* opaque)
{
	printf("_XvidDecode(): WARNING: Xvid decoder raised SIGFPE exception.\n");

// TODO: enable when fenv.h is available
//	feclearexcept(FE_ALL_EXCEPT);

	//jump back before xvid_decore() and take the other branch
	siglongjmp((sigjmp_buf)opaque, 1);
}

// _XvidDecode
int
XvidDecoder::_XvidDecode(uchar *inStream, uchar *outStream, int inStreamSize,
	xvid_dec_stats_t* xvidDecoderStats, bool hurryUp)
{
	PRINT(("%p->XvidDecoder::_XvidDecode(%p, %p, %d)\n", this, inStream,
		outStream, inStreamSize));

	if (inStream == NULL || inStreamSize == 0)
		return -1;

	xvid_dec_frame_t xvid_dec_frame;

	// reset all structures
	memset(&xvid_dec_frame, 0, sizeof(xvid_dec_frame_t));
	memset(xvidDecoderStats, 0, sizeof(xvid_dec_stats_t));

	// set version
	xvid_dec_frame.version = XVID_VERSION;
	xvidDecoderStats->version = XVID_VERSION;

	// no general flags to set
	xvid_dec_frame.general = 0; //XVID_DEBLOCKY | XVID_DEBLOCKUV;
	if (hurryUp)
		xvid_dec_frame.general |= XVID_LOWDELAY;
	if (fDiscontinuity) {
		xvid_dec_frame.general |= XVID_DISCONTINUITY;
		fDiscontinuity = false;
	}

	// input stream
	xvid_dec_frame.bitstream = inStream;
	xvid_dec_frame.length = inStreamSize;

	// output frame structure
	xvid_dec_frame.output.plane[0] = outStream;
	xvid_dec_frame.output.stride[0] = outStream ?
		fOutputVideoFormat.display.bytes_per_row : 0;
	xvid_dec_frame.output.csp = fXvidColorspace;

	// prepare for possible floating point exception
	sigjmp_buf preDecoreEnv;

    struct sigaction action;
    struct sigaction oldAction;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_flags = 0;
    action.sa_handler = (__signal_func_ptr)handle_fp_exeption;
    action.sa_userdata = preDecoreEnv;
 
    if (sigaction(SIGFPE, &action, &oldAction) != 0)
    	return -1;

	int usedBytes;
	if (sigsetjmp(preDecoreEnv, 1) == 0) {
		// decode
		usedBytes = xvid_decore(fXvidDecoderHandle, XVID_DEC_DECODE,
			&xvid_dec_frame, xvidDecoderStats);
	} else {
		// make sure the state changes before calling xvid_decore again
		// or we'll cause the same exception
		usedBytes = 1;
	}

	if (sigaction(SIGFPE, &oldAction, &action) != 0)
		return -1;

	return usedBytes;
}

// #pragma mark -


status_t
XvidPlugin::GetSupportedFormats(media_format** _mediaFormatArray, size_t *_count)
{
	PRINT(("XvidDecoder::register_decoder()\n"));

	static bool codecsRegistered = false;
	if (codecsRegistered)
		return B_OK;
	codecsRegistered = true;

	PRINT(("XvidDecoder: registering %d codecs\n", gSupportedCodecsCount));

	media_format_description descr[gSupportedCodecsCount];

	for (int i = 0; i < gSupportedCodecsCount; i++) {
		descr[i].family = gCodecTable[i].family;
		switch(descr[i].family) {
			case B_AVI_FORMAT_FAMILY:
				descr[i].u.avi.codec = gCodecTable[i].fourcc;
				break;
			case B_MPEG_FORMAT_FAMILY:
				descr[i].u.mpeg.id = gCodecTable[i].fourcc;
				break;
			case B_QUICKTIME_FORMAT_FAMILY:
				descr[i].u.quicktime.codec = gCodecTable[i].fourcc;
				break;
			default:
				break;
		}
	}

	BMediaFormats formats;
	for (int i = 0; i < gSupportedCodecsCount; i++) {
		media_format format;
		format.type = B_MEDIA_ENCODED_VIDEO;
		format.u.encoded_video = media_encoded_video_format::wildcard;
		format.require_flags = 0;
		format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
		
		status_t err = formats.MakeFormatFor(&descr[i], 1, &format);
		
		if (err < B_OK) {
			fprintf(stderr, "XvidDecoder: BMediaFormats::MakeFormatFor: "
				"error %s\n", strerror(err));
			continue;
		}

		gXvidFormats[i] = format;
	}

	*_mediaFormatArray = gXvidFormats;
	*_count = gSupportedCodecsCount;

	return B_OK;
}


Decoder*
XvidPlugin::NewDecoder(uint index)
{
	return new (nothrow) XvidDecoder();
}


MediaPlugin *instantiate_plugin()
{
	return new (nothrow) XvidPlugin;
}


