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
	CALLED();
}

// final
media_destination::media_destination(const media_destination &clone)
	: port(clone.port),
	id(clone.id)
{
	CALLED();
}

// final
media_destination &
media_destination::operator=(const media_destination &clone)
{
	CALLED();
	port = clone.port;
	id = clone.id;
	return *this;
}

// final & verfied
media_destination::media_destination()
	: port(-1),
	id(-1)
{
	CALLED();
}

// final
media_destination::~media_destination()
{
	CALLED();
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
	CALLED();
}

// final
media_source::media_source(const media_source &clone)
	: port(clone.port),
	id(clone.id)
{
	CALLED();
}

// final
media_source &
media_source::operator=(const media_source &clone)
{
	CALLED();
	port = clone.port;
	id = clone.id;
	return *this;
}

// final & verfied
media_source::media_source()
	: port(-1),
	id(-1)
{
	CALLED();
}

// final
media_source::~media_source()
{
	CALLED();
}

// final & verfied
media_source media_source::null(-1,-1);

/*************************************************************
 * 
 *************************************************************/

// final
bool operator==(const media_destination & a, const media_destination & b)
{
	CALLED();
	return (a.port == b.port) && (a.id == b.id);
}

// final
bool operator!=(const media_destination & a, const media_destination & b)
{
	CALLED();
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
	CALLED();
	return (a.port == b.port) && (a.id == b.id);
}

// final
bool operator!=(const media_source & a, const media_source & b)
{
	CALLED();
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
	CALLED();
	return (a.node == b.node) && (a.port == b.port) && (a.kind == b.kind);
}

// final
bool operator!=(const media_node & a, const media_node & b)
{
	CALLED();
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

media_encoded_audio_format media_encoded_audio_format::wildcard = {0};

media_video_display_info media_video_display_info::wildcard = {(color_space)0};

media_raw_video_format media_raw_video_format::wildcard = {0};

media_encoded_video_format media_encoded_video_format::wildcard = {0};

media_multistream_format media_multistream_format::wildcard = {0};

/*************************************************************
 * media_format
 *************************************************************/

bool
media_format::Matches(const media_format *otherFormat) const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
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
	status_t dummy;

	return dummy;
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
	int32 dummy;

	return dummy;
}

// final
media_format::media_format()
{
	CALLED();
	memset(this,0x00,sizeof(*this));
	//meta_data, meta_data_size, meta_data_area, use_area, 
	//team, and thisPtr are currently only used by decoders
	//when communicating with the file reader; they're not
	//currently for public use.
	
	//If any field is 0, it's treated as a wildcard
}


// final
media_format::media_format(const media_format &other)
{
	CALLED();
	*this = other;
}

// final
media_format::~media_format()
{
	CALLED();
}


// final
media_format &
media_format::operator=(const media_format &clone)
{
	CALLED();
	memset(this,0x00,sizeof(*this));

	type = clone.type;
	user_data_type = clone.user_data_type;

	memcpy(user_data,clone.user_data,sizeof(media_format::user_data));

	require_flags = clone.require_flags;
	deny_flags = clone.deny_flags;

	memcpy(u._reserved_,clone.u._reserved_,sizeof(media_format::u._reserved_));
	return *this;
}

/*************************************************************
 * 
 *************************************************************/


bool operator==(const media_raw_audio_format & a, const media_raw_audio_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_multi_audio_info & a, const media_multi_audio_info & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_multi_audio_format & a, const media_multi_audio_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_encoded_audio_format & a, const media_encoded_audio_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_video_display_info & a, const media_video_display_info & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_raw_video_format & a, const media_raw_video_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_encoded_video_format & a, const media_encoded_video_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_multistream_format::vid_info & a, const media_multistream_format::vid_info & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_multistream_format::avi_info & a, const media_multistream_format::avi_info & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_multistream_format & a, const media_multistream_format & b)
{
	UNIMPLEMENTED();
	return false;
}

bool operator==(const media_format & a, const media_format & b)
{
	UNIMPLEMENTED();
	return false;
}

/*************************************************************
 * 
 *************************************************************/

/* return true if a and b are compatible (accounting for wildcards) */
bool format_is_compatible(const media_format & a, const media_format & b)	/* a is the format you want to feed to something accepting b */
{
	UNIMPLEMENTED();
	return false;
}

bool string_for_format(const media_format & f, char * buf, size_t size)
{
	UNIMPLEMENTED();
	return false;
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


