/***********************************************************************
 * AUTHOR: John Hedditch <jhedditc@physics.adelaide.edu.au>
 *   FILE: MediaFormats.cpp
 *  DESCR: The BMediaFormats class - doesn't use hash tables yet.
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
	media_format			format;
	media_format_description	format_description;
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
	int32		i,imax;
	new BPrivate::addon_list[desc_count]	*item;

	CALLED();
        if( descs == NULL || ioformat==NULL ) return B_BAD_VALUE;
        if( desc_count < 1) return B_BAD_VALUE;

	if(flags & B_EXCLUSIVE) then
	{
        	imax=s_formats->CountItems()

        	for(i=0;i<imax;i++)
        	{
                	if(((addon_list *)s_formats->ItemAt(i)).format == io_format)
                	{
                        	return B_ERROR;
                	}
        	}
	};

	for(i=0;i<desc_count;i++)
	{
                item[i].format=io_format;
		item[i].description = descs[i];
		s_formats->AddItem( (void *)item[i] ); 
	}
	return B_OK;
}

				
status_t 
BMediaFormats::GetFormatFor(const media_format_description & desc,
				media_format * out_format)
{
	int32           i,imax;	
	CALLED();

        imax=s_formats->CountItems()
        for(i=0;i<imax;i++)
	{
		if(((addon_list *)s_formats->ItemAt(i)).description == desc)
		{
			out_format=((addon_list *)s_formats->ItemAt(i)).format;
			return B_OK;
		}
	}

	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetBeOSFormatFor(uint32 fourcc, 
				media_format * out_format,
				media_type type)
{
	media_format_description	desc;

	CALLED()
	memset(desc, 0, sizeof(*desc);
	desc.family = B_BEOS_FORMAT_FAMILY;
	desc.u.beos.format=fourcc;
	switch (GetFormatFor(desc, out_format))
	{
		case B_ERROR:
		{
			return B_ERROR;
		}
	
		case B_OK:
		{
			out_format.type = type;
			return B_OK;
		}
	};
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetAVIFormatFor(uint32 fourcc,
							   media_format * out_format,
							   media_type type)
{
	media_format_description	desc;

	CALLED()
	memset(desc, 0, sizeof(*desc);
	desc.family = B_AVI_FORMAT_FAMILY;
	desc.u.avi.codec=fourcc;
	switch (GetFormatFor(desc, out_format))
	{
		case B_ERROR:
		{
			return B_ERROR;
		}
	
		case B_OK:
		{
			out_format.type = type;
			return B_OK;
		}
	};
	return B_ERROR;
}


/* static */ status_t 
BMediaFormats::GetQuicktimeFormatFor(uint32 vendor,
					 uint32 fourcc, 
					 media_format * out_format,
					 media_type type)
{
	media_format_description	desc;

	CALLED()
	memset(desc, 0, sizeof(*desc);
	desc.family = B_QUICKTIME_FORMAT_FAMILY;
	desc.u.quicktime.codec=fourcc;
	desc.u.quicktime.vendor=vendor;
	switch (GetFormatFor(desc, out_format))
	{
		case B_ERROR:
		{
			return B_ERROR;
		}
	
		case B_OK:
		{
			out_format.type = type;
			return B_OK;
		}
	};
	return B_ERROR;
}


status_t 
BMediaFormats::GetCodeFor(const media_format & format,
						  media_format_family family,
						  media_format_description * out_description)
{
        int32           i,imax;
        CALLED();

        imax=s_formats->CountItems()
        for(i=0;i<imax;i++)
        {
                if(((addon_list *)s_formats->ItemAt(i)).format == format)
                {
			if(((addon_list *)s_formats->ItemAt(i)).description.family == family)
			{
				 out_description = ((addon_list *)s_formats->ItemAt(i)).description;
                       		 return B_OK;
			}
                }
        }

        return B_MEDIA_BAD_FORMAT;

}


status_t 
BMediaFormats::RewindFormats()
{
	CALLED();
	while(!(s_lock->Lock()))
	{
		snooze(1000);
	}
	s_cleared=0;
	s_lock->Unlock();	

	return B_OK;
}


status_t 
BMediaFormats::GetNextFormat(media_format * out_format,
				 media_format_description * out_description)
{
	CALLED();
	while(!(s_lock->Lock()))
	{
		snooze(1000);
	}

	s_cleared++;

	if(s_cleared>(s_formats.CountItems())) 
	{
		s_lock->Unlock();
		return B_BAD_INDEX;
	}
	else
	{
		out_format = ((addon_list *)s_formats->ItemAt(s_cleared)).format;
		out_description = ((addon_list *)s_formats->ItemAt(s_cleared)).description;
		s_lock->Unlock();
		return B_OK;
	};
}

//	You need to lock/unlock (only) when using RewindFormats()/GetNextFormat()
bool
BMediaFormats::Lock()
{
	CALLED();
	return ( !s_lock->IsLocked() && s_lock->Lock() );
}

void 
BMediaFormats::Unlock()
{
	CALLED();
	return s_lock->Unlock;
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
	
	CALLED();
	if (!(a.family == b.family)) return false;
	switch(a.family)
	{
		case B_BEOS_FORMAT_FAMILY:
		{
			if(!(a.u.beos.format == b.u.beos.format)) return false;
		}
		
		case B_QUICKTIME_FORMAT_FAMILY:
		{
			if(!(a.u.quicktime.codec == b.u.quicktime.codec)) return false;
			if(!(a.u.quicktime.vendor == b.u.quicktime.vendor)) return false;
		}

		case B_AVI_FORMAT_FAMILY:
		{
			if(!(a.u.avi.codec == b.u.avi.codec)) return false;
		}

		case B_ASF_FORMAT_FAMILY:
		{
			if(!(a.u.asf.guid == b.u.asf.guid)) return false;
		}

		case B_AVR_FORMAT_FAMILY:
		{
			if(!(a.u.avr.id == b.u.avr.id)) return false;
		}

		case B_MPEG_FORMAT_FAMILY:
		{
			if(!(a.u.mpeg.id == b.u.mpeg.id)) return false;
		}

		case B_WAV_FORMAT_FAMILY:
		{
			if(!(a.u.wav.codec == b.u.wav.codec)) return false;
		}

		case B_AIFF_FORMAT_FAMILY:
		{
			if(!(a.u.aiff.codec == b.u.aiff.codec)) return false;
		}

		case B_MISC_FORMAT_FAMILY:
		{
			if(!(a.u.misc.file_format == b.u.misc.file_format)) return false;
			if(!(a.u.misc.codec == b.u.misc.codec)) return false;
		}

	};	
	return true;
}

bool operator<(const media_format_description & a, const media_format_description & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const GUID & a, const GUID & b)
{
	int		indx;
	CALLED();
	for(indx=0;indx<16;indx++)
	{
		if(!(a.data[indx]==b.data[indx])) return false;
	}
	return true;
}

bool operator<(const GUID & a, const GUID & b)
{
	UNIMPLEMENTED();
	return false;
}


