/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaDefs.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDefs.h>
#include <MediaNode.h>
#include <string.h>
#include "debug.h"

/*************************************************************
 * media_destination
 *************************************************************/

// final
media_destination::media_destination(port_id port,
									 int32 id)
	: port(port),
	id(id)
{
}

// final
media_destination::media_destination(const media_destination &clone)
	: port(clone.port),
	id(clone.id)
{
}

// final
media_destination &
media_destination::operator=(const media_destination &clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}

// final & verfied
media_destination::media_destination()
	: port(-1),
	id(-1)
{
}

// final
media_destination::~media_destination()
{
}

// final & verfied
media_destination media_destination::null(-1,-1);

/*************************************************************
 * media_source
 *************************************************************/

// final
media_source::media_source(port_id port,
						   int32 id)
	: port(port),
	id(id)
{
}

// final
media_source::media_source(const media_source &clone)
	: port(clone.port),
	id(clone.id)
{
}

// final
media_source &
media_source::operator=(const media_source &clone)
{
	port = clone.port;
	id = clone.id;
	return *this;
}

// final & verfied
media_source::media_source()
	: port(-1),
	id(-1)
{
}

// final
media_source::~media_source()
{
}

// final & verfied
media_source media_source::null(-1,-1);

/*************************************************************
 * 
 *************************************************************/

// final
bool operator==(const media_destination & a, const media_destination & b)
{
	return (a.port == b.port) && (a.id == b.id);
}

// final
bool operator!=(const media_destination & a, const media_destination & b)
{
	return (a.port != b.port) || (a.id != b.id);
}

bool operator<(const media_destination & a, const media_destination & b)
{
	UNIMPLEMENTED();
	return false;
}

// final
bool operator==(const media_source & a, const media_source & b)
{
	return (a.port == b.port) && (a.id == b.id);
}

// final
bool operator!=(const media_source & a, const media_source & b)
{
	return (a.port != b.port) || (a.id != b.id);
}

bool operator<(const media_source & a, const media_source & b)
{
	UNIMPLEMENTED();
	return false;
}

// final
bool operator==(const media_node & a, const media_node & b)
{
	return (a.node == b.node) && (a.port == b.port) && (a.kind == b.kind);
}

// final
bool operator!=(const media_node & a, const media_node & b)
{
	return (a.node != b.node) || (a.port != b.port) || (a.kind != b.kind);
}

bool operator<(const media_node & a, const media_node & b)
{
	UNIMPLEMENTED();
	return false;
}


/*************************************************************
 * 
 *************************************************************/

media_multi_audio_format media_raw_audio_format::wildcard;

media_multi_audio_format media_multi_audio_format::wildcard;

media_encoded_audio_format media_encoded_audio_format::wildcard = {{0}};

media_video_display_info media_video_display_info::wildcard = {(color_space)0};

media_raw_video_format media_raw_video_format::wildcard = {0};

media_encoded_video_format media_encoded_video_format::wildcard = {{0}};

media_multistream_format media_multistream_format::wildcard = {0};

/*************************************************************
 * media_format
 *************************************************************/

bool
media_format::Matches(const media_format *otherFormat) const
{
	return format_is_compatible(*this, *otherFormat);
}


void
media_format::SpecializeTo(const media_format *otherFormat)
{
	UNIMPLEMENTED();
}


status_t
media_format::SetMetaData(const void *data,
						  size_t size)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


const void *
media_format::MetaData() const
{
	UNIMPLEMENTED();
	return NULL;
}


int32
media_format::MetaDataSize() const
{
	UNIMPLEMENTED();

	return 0;
}

// final
media_format::media_format()
{
	memset(this, 0x00, sizeof(*this));
	//meta_data, meta_data_size, meta_data_area, use_area, 
	//team, and thisPtr are currently only used by decoders
	//when communicating with the file reader; they're not
	//currently for public use.
	
	//If any field is 0, it's treated as a wildcard
}


// final
media_format::media_format(const media_format &other)
{
	*this = other;
}

// final
media_format::~media_format()
{
}


// final
media_format &
media_format::operator=(const media_format &clone)
{
	memset(this, 0x00, sizeof(*this));

	type = clone.type;
	user_data_type = clone.user_data_type;

	memcpy(user_data, clone.user_data, sizeof(media_format::user_data));

	require_flags = clone.require_flags;
	deny_flags = clone.deny_flags;

	memcpy(u._reserved_, clone.u._reserved_, sizeof(media_format::u._reserved_));
	return *this;
}

/*************************************************************
 * 
 *************************************************************/


bool operator==(const media_raw_audio_format & a, const media_raw_audio_format & b)
{
	return (   a.frame_rate == b.frame_rate
			&& a.channel_count == b.channel_count
			&& a.format == b.format
			&& a.byte_order == b.byte_order
			&& a.buffer_size == b.buffer_size);
}

bool operator==(const media_multi_audio_info & a, const media_multi_audio_info & b)
{
	return (   a.channel_mask == b.channel_mask
			&& a.valid_bits == b.valid_bits
			&& a.matrix_mask == b.matrix_mask);
}

bool operator==(const media_multi_audio_format & a, const media_multi_audio_format & b)
{
	return (   (media_raw_audio_format)a == (media_raw_audio_format)b
			&& (media_multi_audio_info)a == (media_multi_audio_info)b);
}

bool operator==(const media_encoded_audio_format & a, const media_encoded_audio_format & b)
{
	return (   a.output == b.output
			&& a.encoding == b.encoding
			&& a.bit_rate == b.bit_rate
			&& a.frame_size == b.frame_size
			&& a.multi_info == b.multi_info);
}

bool operator==(const media_video_display_info & a, const media_video_display_info & b)
{
	return (   a.format == b.format
			&& a.line_width == b.line_width
			&& a.line_count == b.line_count
			&& a.bytes_per_row == b.bytes_per_row
			&& a.pixel_offset == b.pixel_offset
			&& a.line_offset == b.line_offset
			&& a.flags == b.flags);
}

bool operator==(const media_raw_video_format & a, const media_raw_video_format & b)
{
	return (   a.field_rate == b.field_rate
			&& a.interlace == b.interlace
			&& a.first_active == b.first_active
			&& a.last_active == b.last_active
			&& a.orientation == b.orientation
			&& a.pixel_width_aspect == b.pixel_width_aspect
			&& a.pixel_height_aspect == b.pixel_height_aspect
			&& a.display == b.display);
}

bool operator==(const media_encoded_video_format & a, const media_encoded_video_format & b)
{
	return (   a.output == b.output
			&& a.avg_bit_rate == b.avg_bit_rate
			&& a.max_bit_rate == b.max_bit_rate
			&& a.encoding == b.encoding
			&& a.frame_size == b.frame_size
			&& a.forward_history == b.forward_history
			&& a.backward_history == b.backward_history);
}

bool operator==(const media_multistream_format::vid_info & a, const media_multistream_format::vid_info & b)
{
	return (   a.frame_rate == b.frame_rate
			&& a.width == b.width
			&& a.height == b.height
			&& a.space == b.space
			&& a.sampling_rate == b.sampling_rate
			&& a.sample_format == b.sample_format
			&& a.byte_order == b.byte_order
			&& a.channel_count == b.channel_count);
}

bool operator==(const media_multistream_format::avi_info & a, const media_multistream_format::avi_info & b)
{
	return (   a.us_per_frame == b.us_per_frame
			&& a.width == b.width
			&& a.height == b.height
			&& a.type_count == b.type_count
			&& a.types[0] == b.types[0]
			&& a.types[1] == b.types[1]
			&& a.types[2] == b.types[2]
			&& a.types[3] == b.types[3]
			&& a.types[4] == b.types[4]);
}

bool operator==(const media_multistream_format & a, const media_multistream_format & b)
{
	if (a.avg_bit_rate != b.avg_bit_rate
		|| a.max_bit_rate != b.max_bit_rate
		|| a.avg_chunk_size != b.avg_chunk_size
		|| a.max_chunk_size != b.max_chunk_size
		|| a.format != b.format
		|| a.flags != b.flags)
		return false;

	switch (a.format) {
		case media_multistream_format::B_VID:
			return a.u.vid == b.u.vid;
			
		case media_multistream_format::B_AVI:
			return a.u.avi == b.u.avi;

		default:
			return true; // XXX really?
	}
}

bool operator==(const media_format & a, const media_format & b)
{
	if (a.type != b.type
		|| a.user_data_type != b.user_data_type
		// XXX compare user_data[48] ?
		|| a.require_flags != b.require_flags
		|| a.deny_flags != b.deny_flags)
		return false;
		
	switch (a.type) {
		case B_MEDIA_RAW_AUDIO:
			return a.u.raw_audio == b.u.raw_audio;
			
		case B_MEDIA_RAW_VIDEO:
			return a.u.raw_video == b.u.raw_video;
			
		case B_MEDIA_MULTISTREAM:
			return a.u.multistream == b.u.multistream;
			
		case B_MEDIA_ENCODED_AUDIO:
			return a.u.encoded_audio == b.u.encoded_audio;
			
		case B_MEDIA_ENCODED_VIDEO:
			return a.u.encoded_video == b.u.encoded_video;
			
		default:
			return true; // XXX really?
	}
}

/*************************************************************
 * 
 *************************************************************/

/* return true if a and b are compatible (accounting for wildcards) */
bool format_is_compatible(const media_format & a, const media_format & b)	/* a is the format you want to feed to something accepting b */
{
	UNIMPLEMENTED();
	if (a.type == b.type)
		return true;
	return false;
}

bool string_for_format(const media_format & f, char * buf, size_t size)
{
	UNIMPLEMENTED();
	strcpy(buf, "No string_for_format string!");
	return true;
}

/*************************************************************
 * 
 *************************************************************/

bool operator==(const media_file_format_id & a, const media_file_format_id & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator<(const media_file_format_id & a, const media_file_format_id & b)
{
	UNIMPLEMENTED();
	return false;
}

/*************************************************************
 * 
 *************************************************************/

//
// Use this function iterate through available file format writers
//
status_t get_next_file_format(int32 *cookie, media_file_format *mfi)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


/*************************************************************
 * 
 *************************************************************/

// final & verified
const char * B_MEDIA_SERVER_SIGNATURE = "application/x-vnd.Be.media-server";

/*************************************************************
 * 
 *************************************************************/

struct dormant_node_info
{
};

struct buffer_clone_info
{
};

/*************************************************************
 * 
 *************************************************************/


//	If you for some reason need to get rid of the media_server (and friends)
//	use these functions, rather than sending messages yourself.
//	The callback will be called for various stages of the process, with 100 meaning completely done
//	The callback should always return TRUE for the time being.
status_t shutdown_media_server(bigtime_t timeout = B_INFINITE_TIMEOUT, bool (*progress)(int stage, const char * message, void * cookie) = NULL, void * cookie = NULL)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t launch_media_server(uint32 flags = 0)
{
	UNIMPLEMENTED();
	return B_OK;
}

//	Given an image_id, prepare that image_id for realtime media
//	If the kind of media indicated by "flags" is not enabled for real-time,
//	B_MEDIA_REALTIME_DISABLED is returned.
//	If there are not enough system resources to enable real-time performance,
//	B_MEDIA_REALTIME_UNAVAILABLE is returned.
status_t media_realtime_init_image(image_id image, uint32 flags)
{
	UNIMPLEMENTED();
	return B_OK;
}

//	Given a thread ID, and an optional indication of what the thread is
//	doing in "flags", prepare the thread for real-time media performance.
//	Currently, this means locking the thread stack, up to size_used bytes,
//	or all of it if 0 is passed. Typically, you will not be using all
//	256 kB of the stack, so you should pass some smaller value you determine
//	from profiling the thread; typically in the 32-64kB range.
//	Return values are the same as for media_prepare_realtime_image().
status_t media_realtime_init_thread(thread_id thread, size_t stack_used, uint32 flags)
{
	UNIMPLEMENTED();
	return B_OK;
}

/*************************************************************
 * media_encode_info
 *************************************************************/


media_encode_info::media_encode_info()
{
	UNIMPLEMENTED();
}


media_decode_info::media_decode_info()
{
	UNIMPLEMENTED();
}


