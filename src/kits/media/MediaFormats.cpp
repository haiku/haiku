/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaFormats.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFormats.h>
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
	UNIMPLEMENTED();
}

_media_format_description::~_media_format_description()
{
	UNIMPLEMENTED();
}

_media_format_description::_media_format_description(const _media_format_description & other)
{
	UNIMPLEMENTED();
}

_media_format_description & 
_media_format_description::operator=(const _media_format_description & other)
{
	UNIMPLEMENTED();
	return *this;
}

/*************************************************************
 * 
 *************************************************************/


namespace BPrivate {

class addon_list
{
};

	
void dec_load_hook(void *arg, image_id imgid)
{
	UNIMPLEMENTED();
};

void extractor_load_hook(void *arg, image_id imgid)
{
	UNIMPLEMENTED();
};

class Extractor
{
};

}

/*************************************************************
 * public BMediaFormats
 *************************************************************/

BMediaFormats::BMediaFormats()
{
	UNIMPLEMENTED();
}

/* virtual */
BMediaFormats::~BMediaFormats()
{
	UNIMPLEMENTED();
}

status_t 
BMediaFormats::InitCheck()
{
	UNIMPLEMENTED();

	return B_OK;
}

status_t 
BMediaFormats::MakeFormatFor(const media_format_description * descs,
							 int32 desc_count,
							 media_format * io_format,
							 uint32 flags,
							 void * _reserved)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

				
status_t 
BMediaFormats::GetFormatFor(const media_format_description & desc,
							media_format * out_format)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetBeOSFormatFor(uint32 fourcc, 
								media_format * out_format,
								media_type type)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetAVIFormatFor(uint32 fourcc,
							   media_format * out_format,
							   media_type type)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetQuicktimeFormatFor(uint32 vendor,
									 uint32 fourcc, 
									 media_format * out_format,
									 media_type type)
{
	UNIMPLEMENTED();
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
	UNIMPLEMENTED();
	return B_ERROR;
}


status_t 
BMediaFormats::GetNextFormat(media_format * out_format,
							 media_format_description * out_description)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

//	You need to lock/unlock (only) when using RewindFormats()/GetNextFormat()
bool
BMediaFormats::Lock()
{
	UNIMPLEMENTED();
	return true;
}

void 
BMediaFormats::Unlock()
{
	UNIMPLEMENTED();
}

/* --- begin deprecated API --- */
status_t 
BMediaFormats::MakeFormatFor(const media_format_description & desc,
							 const media_format & in_format,
							  media_format * out_format)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

/*************************************************************
 * private BMediaFormats
 *************************************************************/

void 
BMediaFormats::clear_formats()
{
	UNIMPLEMENTED();
}

/* static */ void 
BMediaFormats::ex_clear_formats_imp()
{
	UNIMPLEMENTED();
}

/* static */ void 
BMediaFormats::clear_formats_imp()
{
	UNIMPLEMENTED();
}

status_t 
BMediaFormats::get_formats()
{
	UNIMPLEMENTED();
	return B_ERROR;
}

/* static */ status_t 
BMediaFormats::get_formats_imp()
{
	UNIMPLEMENTED();
	return B_ERROR;
}

/* static */ BMessenger & 
BMediaFormats::get_server()
{
	UNIMPLEMENTED();
	static BMessenger dummy;
	return dummy;
}

/* static */ status_t 
BMediaFormats::bind_addon(
				const char * addon,
				const media_format * formats,
				int32 count)
{
	UNIMPLEMENTED();
	return B_OK;
}
				
/* static */ bool 
BMediaFormats::is_bound(
				const char * addon,
				const media_format * formats,
				int32 count)
{
	UNIMPLEMENTED();
	return false;
}
				

/* static */ status_t 
BMediaFormats::find_addons(
				const media_format * format,
				BPrivate::addon_list & addons)
{
	UNIMPLEMENTED();
	return B_ERROR;
}

/*************************************************************
 * static BMediaFormats variables
 *************************************************************/

int32 BMediaFormats::s_cleared;
BMessenger BMediaFormats::s_server;
BList BMediaFormats::s_formats;
BLocker BMediaFormats::s_lock;

/*************************************************************
 * 
 *************************************************************/

bool operator==(const media_format_description & a, const media_format_description & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator<(const media_format_description & a, const media_format_description & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const GUID & a, const GUID & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator<(const GUID & a, const GUID & b)
{
	UNIMPLEMENTED();
	return false;
}

