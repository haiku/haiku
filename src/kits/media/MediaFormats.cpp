/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaFormats.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFormats.h>
#include <string.h>
#include "DataExchange.h"
#include "debug.h"

/*************************************************************
 * 
 *************************************************************/

status_t get_next_encoder(int32 *cookie,
						  const media_file_format *mfi,		// this comes from get_next_file_format()
						  const media_format *input_format,	// this is the type of data given to the encoder
						  media_format *output_format,		// this is the type of data encoder will output 
						  media_codec_info *ei)				// information about the encoder
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t get_next_encoder(int32 *cookie,
						  const media_file_format *mfi,
						  const media_format *input_format,
						  const media_format *output_format,
						  media_codec_info *ei,
						  media_format *accepted_input_format,
						  media_format *accepted_output_format)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t get_next_encoder(int32 *cookie, media_codec_info *ei)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


bool does_file_accept_format(const media_file_format *mfi,
                             media_format *format, uint32 flags)
{
	UNIMPLEMENTED();
	return false;
}

/*************************************************************
 * _media_format_description
 *************************************************************/

_media_format_description::_media_format_description()
{
	memset(this, 0, sizeof(*this));
}

_media_format_description::~_media_format_description()
{
}

_media_format_description::_media_format_description(const _media_format_description & other)
{
	memcpy(this, &other, sizeof(*this));
}

_media_format_description & 
_media_format_description::operator=(const _media_format_description & other)
{
	memcpy(this, &other, sizeof(*this));
	return *this;
}

/*************************************************************
 * public BMediaFormats
 *************************************************************/

BMediaFormats::BMediaFormats()
 :	fLocker(new BLocker("some BMediaFormats locker")),
	fIndex(0)	
{
}

/* virtual */
BMediaFormats::~BMediaFormats()
{
	delete fLocker;
}

status_t 
BMediaFormats::InitCheck()
{
	return B_OK;
}

status_t 
BMediaFormats::MakeFormatFor(const media_format_description * descs,
							 int32 desc_count,
							 media_format * io_format,
							 uint32 flags,
							 void * _reserved)
{
	CALLED();
	if (descs == 0 || desc_count < 1 || io_format == 0)
		return B_BAD_VALUE;
		
	return B_ERROR;
}

				
status_t 
BMediaFormats::GetFormatFor(const media_format_description & desc,
							media_format * out_format)
{
	// set to wildcard, as MakeFormatFor wants an in/out format...
	memset(out_format, 0, sizeof(*out_format));
	return MakeFormatFor(&desc, 1, out_format);
}


/* static */ status_t 
BMediaFormats::GetBeOSFormatFor(uint32 fourcc, 
								media_format * out_format,
								media_type type)
{
	media_format_description mfd;
	mfd.family = B_BEOS_FORMAT_FAMILY;
	mfd.u.beos.format = fourcc;
	memset(out_format, 0, sizeof(*out_format));
	out_format->type = type;
//	return MakeFormatFor(&mfd, 1, out_format);
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetAVIFormatFor(uint32 fourcc,
							   media_format * out_format,
							   media_type type)
{
	media_format_description mfd;
	mfd.family = B_AVI_FORMAT_FAMILY;
	mfd.u.avi.codec = fourcc;
	memset(out_format, 0, sizeof(*out_format));
	out_format->type = type;
//	return MakeFormatFor(&mfd, 1, out_format);
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetQuicktimeFormatFor(uint32 vendor,
									 uint32 fourcc, 
									 media_format * out_format,
									 media_type type)
{
	media_format_description mfd;
	mfd.family = B_QUICKTIME_FORMAT_FAMILY;
	mfd.u.quicktime.codec = fourcc;
	mfd.u.quicktime.vendor = vendor;
	memset(out_format, 0, sizeof(*out_format));
	out_format->type = type;
//	return MakeFormatFor(&mfd, 1, out_format);
	return B_ERROR;
}


status_t 
BMediaFormats::GetCodeFor(const media_format & format,
						  media_format_family family,
						  media_format_description * out_description)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaFormats::RewindFormats()
{
	if (!fLocker->IsLocked())
		return B_NOT_ALLOWED;
	fIndex = 0;
	return B_OK;
}


status_t 
BMediaFormats::GetNextFormat(media_format * out_format,
							 media_format_description * out_description)
{
	if (!fLocker->IsLocked())
		return B_NOT_ALLOWED;
		
	UNIMPLEMENTED();
	return B_ERROR;
}

//	You need to lock/unlock (only) when using RewindFormats()/GetNextFormat()
bool
BMediaFormats::Lock()
{
	return fLocker->Lock();
}

void 
BMediaFormats::Unlock()
{
	fLocker->Unlock();
}

/* --- begin deprecated API --- */
status_t 
BMediaFormats::MakeFormatFor(const media_format_description & desc,
							 const media_format & in_format,
							  media_format * out_format)
{
	*out_format = in_format;
	return MakeFormatFor(&desc, 1, out_format);
}

/*************************************************************
 * 
 *************************************************************/

bool operator==(const media_format_description & a, const media_format_description & b)
{
	if (a.family != b.family)
		return false;
	switch (a.family) {
		case B_BEOS_FORMAT_FAMILY:
			return a.u.beos.format == b.u.beos.format;
		case B_QUICKTIME_FORMAT_FAMILY:
			return a.u.quicktime.codec == b.u.quicktime.codec && a.u.quicktime.vendor == b.u.quicktime.vendor;
		case B_AVI_FORMAT_FAMILY:
			return a.u.avi.codec == b.u.avi.codec;
		case B_ASF_FORMAT_FAMILY:
			return a.u.asf.guid == b.u.asf.guid;
		case B_MPEG_FORMAT_FAMILY:
			return a.u.mpeg.id == b.u.mpeg.id;
		case B_WAV_FORMAT_FAMILY:
			return a.u.wav.codec == b.u.wav.codec;
		case B_AIFF_FORMAT_FAMILY:
			return a.u.aiff.codec == b.u.aiff.codec;
		case B_AVR_FORMAT_FAMILY:
			return a.u.avr.id == b.u.avr.id;
		case B_MISC_FORMAT_FAMILY:
			return a.u.misc.file_format == b.u.misc.file_format && a.u.misc.codec == b.u.misc.codec;
		case B_META_FORMAT_FAMILY:
			return strncmp(a.u.meta.description, b.u.meta.description, sizeof(a.u.meta.description)) == 0;
		default:
			return false;
	}
}

bool operator<(const media_format_description & a, const media_format_description & b)
{
	if (a.family != b.family)
		return a.family < b.family;
	switch (a.family) {
		case B_BEOS_FORMAT_FAMILY:
			return a.u.beos.format < b.u.beos.format;
		case B_QUICKTIME_FORMAT_FAMILY:
			return a.u.quicktime.codec < b.u.quicktime.codec || a.u.quicktime.vendor < b.u.quicktime.vendor;
		case B_AVI_FORMAT_FAMILY:
			return a.u.avi.codec < b.u.avi.codec;
		case B_ASF_FORMAT_FAMILY:
			return a.u.asf.guid < b.u.asf.guid;
		case B_MPEG_FORMAT_FAMILY:
			return a.u.mpeg.id < b.u.mpeg.id;
		case B_WAV_FORMAT_FAMILY:
			return a.u.wav.codec < b.u.wav.codec;
		case B_AIFF_FORMAT_FAMILY:
			return a.u.aiff.codec < b.u.aiff.codec;
		case B_AVR_FORMAT_FAMILY:
			return a.u.avr.id < b.u.avr.id;
		case B_MISC_FORMAT_FAMILY:
			return a.u.misc.file_format < b.u.misc.file_format || a.u.misc.codec < b.u.misc.codec;
		case B_META_FORMAT_FAMILY:
			return strncmp(a.u.meta.description, b.u.meta.description, sizeof(a.u.meta.description)) < 0;
		default:
			return true;
	}
}

bool operator==(const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(a)) == 0;
}

bool operator<(const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(a)) < 0;
}

status_t
_get_format_for_description(media_format *out_format, const media_format_description &in_desc)
{
	server_get_format_for_description_request request;
	server_get_format_for_description_reply reply;
	
	request.description = in_desc;
	
//	if (B_OK != QueryServer(SERVER_GET_FORMAT_FOR_DESCRIPTION, &request, sizeof(request), &reply, sizeof(reply)))
//		return B_ERROR;

	if (in_desc.family == B_BEOS_FORMAT_FAMILY && in_desc.u.beos.format == B_BEOS_FORMAT_RAW_AUDIO)
		reply.format.type = B_MEDIA_RAW_AUDIO;
	else
		reply.format.type = B_MEDIA_ENCODED_AUDIO;
	reply.format.u.encoded_audio.encoding = (enum media_encoded_audio_format::audio_encoding) 1;

	*out_format = reply.format;
	return B_OK;
}

status_t
_get_meta_description_for_format(media_format_description *out_desc, const media_format &in_format)
{
	server_get_meta_description_for_format_request request;
	server_get_meta_description_for_format_reply reply;
	
	request.format = in_format;
	
//	if (B_OK != QueryServer(SERVER_GET_META_DESCRIPTION_FOR_FORMAT, &request, sizeof(request), &reply, sizeof(reply)))
//		return B_ERROR;
		
	*out_desc = reply.description;
	return B_OK;
}
