/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_DEFS_H
#define _MEDIA_DEFS_H


/*! Basic data types and defines for the Media Kit. */


#include <OS.h>
#include <ByteOrder.h>

#if defined(__cplusplus)
#	include <GraphicsDefs.h>
#	include <Looper.h>
class media_node;
#else
struct media_node;
#endif


#define B_MEDIA_NAME_LENGTH 64


/* Notification messages 'what' fields */
enum {
	/* Note that BMediaNode::node_error also belongs in here! */
	B_MEDIA_WILDCARD		= 'TRWC',
									/* Used to match any notification in */
									/* Start/StopWatching */
	B_MEDIA_NODE_CREATED	= 'TRIA',
									/* "media_node_id" (multiple items) */
	B_MEDIA_NODE_DELETED,			/* "media_node_id" (multiple items) */
	B_MEDIA_CONNECTION_MADE,		/* "output", "input", "format" */
	B_MEDIA_CONNECTION_BROKEN,		/* "source", "destination" */
	B_MEDIA_BUFFER_CREATED,			/* "clone_info" -- handled by */
									/* BMediaRoster */
	B_MEDIA_BUFFER_DELETED,			/* "media_buffer_id" -- handled by */
									/* BMediaRoster */
	B_MEDIA_TRANSPORT_STATE,		/* "state", "location", "realtime" */
	B_MEDIA_PARAMETER_CHANGED,		/* N "node", "parameter" */
	B_MEDIA_FORMAT_CHANGED,			/* N "source", "destination", "format" */
	B_MEDIA_WEB_CHANGED,			/* N "node" */
	B_MEDIA_DEFAULT_CHANGED,		/* "default", "node" -- handled by */
									/* BMediaRoster */
	B_MEDIA_NEW_PARAMETER_VALUE,	/* N "node", "parameter", "when", */
									/* "value" */
	B_MEDIA_NODE_STOPPED,			/* N "node", "when" */
	B_MEDIA_FLAVORS_CHANGED,		/* "be:addon_id", "be:new_count", */
									/* "be:gone_count" */
	B_MEDIA_SERVER_STARTED,
	B_MEDIA_SERVER_QUIT
};


enum media_type {
	B_MEDIA_NO_TYPE			= -1,
	B_MEDIA_UNKNOWN_TYPE	= 0,
	B_MEDIA_RAW_AUDIO		= 1,	/* uncompressed raw_audio */
	B_MEDIA_RAW_VIDEO,				/* uncompressed raw_video */
	B_MEDIA_VBL,					/* raw data from VBL area, 1600/line */
	B_MEDIA_TIMECODE,				/* data format TBD */
	B_MEDIA_MIDI,
	B_MEDIA_TEXT,					/* typically closed captioning */
	B_MEDIA_HTML,
	B_MEDIA_MULTISTREAM,			/* AVI, etc */
	B_MEDIA_PARAMETERS,				/* BControllable change data */
	B_MEDIA_ENCODED_AUDIO,			/* MP3, AC-3, ... */
	B_MEDIA_ENCODED_VIDEO,			/* H.264, Theora, ... */
	B_MEDIA_PRIVATE			= 90000,
									/* This are reserved. */
	B_MEDIA_FIRST_USER_TYPE	= 100000
									/* Use something bigger than this for */
									/* experimentation with your own media */
									/* formats. */
};


enum node_kind {
	B_BUFFER_PRODUCER		= 0x1,
	B_BUFFER_CONSUMER		= 0x2,
	B_TIME_SOURCE			= 0x4,
	B_CONTROLLABLE			= 0x8,
	B_FILE_INTERFACE		= 0x10,
	B_ENTITY_INTERFACE		= 0x20,

	/* Set these flags for nodes that are suitable as default system nodes. */
	B_PHYSICAL_INPUT		= 0x10000,
	B_PHYSICAL_OUTPUT		= 0x20000,
	B_SYSTEM_MIXER			= 0x40000
};


enum video_orientation {
	/* Which pixel is first and how do we scan each "line"? */
	B_VIDEO_TOP_LEFT_RIGHT	= 1,	/* This is the typical progressive scan */
									/* format */
	B_VIDEO_BOTTOM_LEFT_RIGHT		/* This is how BMP and TGA might scan */
};


/* data */
enum media_flags
{
	B_MEDIA_FLAGS_VERSION			= 1,
									/* uint32, greater for newer versions */
	B_MEDIA_FLAGS_PRIVATE			= 0x40000000
									/* private to Haiku */
};


/* for producer status */
enum media_producer_status {
	B_DATA_NOT_AVAILABLE			= 1,
	B_DATA_AVAILABLE				= 2,
	B_PRODUCER_STOPPED				= 3
};


/* realtime flags */
enum media_realtime_flags {
	B_MEDIA_REALTIME_ALLOCATOR		= 0x1,
	B_MEDIA_REALTIME_AUDIO			= 0x2,
	B_MEDIA_REALTIME_VIDEO			= 0x4,
	B_MEDIA_REALTIME_ANYKIND		= 0xffff
};

enum media_frame_flags {
	B_MEDIA_KEY_FRAME				= 0x1
};

#define B_MEDIA_ANY_QUALITY			0.0f
#define B_MEDIA_LOW_QUALITY			0.1f
#define B_MEDIA_MEDIUM_QUALITY		0.5f
#define B_MEDIA_HIGH_QUALITY		1.0f


#ifndef _MULTI_AUDIO_H	/* #define in protocol header */
enum media_multi_channels {
	B_CHANNEL_LEFT					= 0x00001,
	B_CHANNEL_RIGHT					= 0x00002,
	B_CHANNEL_CENTER				= 0x00004,	/* 5.1+ or fake surround */
	B_CHANNEL_SUB					= 0x00008,	/* 5.1+ */
	B_CHANNEL_REARLEFT				= 0x00010,	/* quad surround or 5.1+ */
	B_CHANNEL_REARRIGHT				= 0x00020,	/* quad surround or 5.1+ */
	B_CHANNEL_FRONT_LEFT_CENTER		= 0x00040,
	B_CHANNEL_FRONT_RIGHT_CENTER	= 0x00080,
	B_CHANNEL_BACK_CENTER			= 0x00100,	/* 6.1 or fake surround */
	B_CHANNEL_SIDE_LEFT				= 0x00200,
	B_CHANNEL_SIDE_RIGHT			= 0x00400,
	B_CHANNEL_TOP_CENTER			= 0x00800,
	B_CHANNEL_TOP_FRONT_LEFT		= 0x01000,
	B_CHANNEL_TOP_FRONT_CENTER		= 0x02000,
	B_CHANNEL_TOP_FRONT_RIGHT		= 0x04000,
	B_CHANNEL_TOP_BACK_LEFT			= 0x08000,
	B_CHANNEL_TOP_BACK_CENTER		= 0x10000,
	B_CHANNEL_TOP_BACK_RIGHT		= 0x20000
};


enum media_multi_matrix {
	B_MATRIX_PROLOGIC_LR			= 0x1,
	B_MATRIX_AMBISONIC_WXYZ			= 0x4
};
#endif // !_MULTI_AUDIO_H


typedef int32 media_node_id;
typedef int32 media_buffer_id;
typedef int32 media_addon_id;


#if defined(__cplusplus)
struct media_destination {
								media_destination();
								media_destination(port_id, int32);
								media_destination(
									const media_destination& other);
								~media_destination();

			media_destination&	operator=(const media_destination& other);

			port_id				port;	/* can be different from */
										/* media_node.port */
			int32				id;
	static	media_destination	null;

private:
			uint32				_reserved_media_destination_[2];
};


struct media_source {
								media_source();
								media_source(port_id, int32);
								media_source(const media_source& other);
								~media_source();

			media_source&		operator=(const media_source& other);
			port_id				port;	/* must be the same as */
										/* media_node.port for owner */
			int32				id;
	static	media_source		null;

private:
			uint32				_reserved_media_source_[2];
};


bool operator==(const media_destination& a, const media_destination& b);
bool operator!=(const media_destination& a, const media_destination& b);
bool operator<(const media_destination& a, const media_destination& b);
bool operator==(const media_source& a, const media_source& b);
bool operator!=(const media_source& a, const media_source& b);
bool operator<(const media_source& a, const media_source& b);
bool operator==(const media_node& a, const media_node& b);
bool operator!=(const media_node& a, const media_node& b);
bool operator<(const media_node& a, const media_node& b);



/* Buffers are low-level constructs identified by an ID. */
/* Buffers consist of the actual data area, plus a 64-byte */
/* header area that is different for each type. */
/* Buffers contain typed data. Type is not part of the */
/* buffer header; it's negotiated out-of-bounds by nodes. */

enum {
	B_MEDIA_BIG_ENDIAN			= 1,
	B_MEDIA_LITTLE_ENDIAN		= 2,
#if B_HOST_IS_BENDIAN
	B_MEDIA_HOST_ENDIAN			= B_MEDIA_BIG_ENDIAN
#else
	B_MEDIA_HOST_ENDIAN			= B_MEDIA_LITTLE_ENDIAN
#endif
};


struct media_multi_audio_format;


struct media_raw_audio_format {
	// possible values for "format"
	enum {
		B_AUDIO_FLOAT		= 0x24,
			// 0 == mid, -1.0 == bottom, 1.0 == top
			// (the preferred format for non-game audio)

		B_AUDIO_DOUBLE		= 0x28,
			// 0 == mid, -1.0 == bottom, 1.0 == top
			// (only useful for pro audio)

		B_AUDIO_INT			= 0x4,
			// 0 == mid, 0x80000001 == bottom, 0x7fffffff == top
			// (all >16-bit formats, left-adjusted)

		B_AUDIO_SHORT		= 0x2,
			// 0 == mid, -32767 == bottom, +32767 == top

		B_AUDIO_UCHAR		= 0x11,
			// 128 == mid, 1 == bottom, 255 == top
			// (discouraged but supported format)

		B_AUDIO_CHAR		= 0x1,
			// 0 == mid, -127 == bottom, +127 == top
			// (not officially supported format)

		B_AUDIO_SIZE_MASK	= 0xf
			// This mask can be used to obtain the sample size
			// for raw formats: (format & 0xf) == sizeof(sample)
	};

	float		frame_rate;
	uint32		channel_count;
	uint32		format;			// see possible values above
	uint32		byte_order;		// B_MEDIA_LITTLE_ENDIAN or B_MEDIA_BIG_ENDIAN
	size_t		buffer_size;	// size of each buffer

	static const media_multi_audio_format wildcard;
};


struct media_audio_header {
	// TODO: Refine this structure and put actual data at the end
	int32		_reserved_[14];
	float       frame_rate;
	uint32      channel_count;

};


struct media_multi_audio_info {
	uint32		channel_mask;	// bitmask
	int16		valid_bits;		// if < 32, for B_AUDIO_INT
	uint16		matrix_mask;	// each of these bits may mean more than one
								// channel

	uint32		_reserved_b[3];
};


struct media_multi_audio_format : public media_raw_audio_format,
	public media_multi_audio_info {

	static const media_multi_audio_format wildcard;
};


struct media_encoded_audio_format {
	enum audio_encoding {
		B_ANY
	};

	media_raw_audio_format	output;
	audio_encoding			encoding;

	float					bit_rate;
	size_t					frame_size;

	media_multi_audio_info	multi_info;

	uint32					_reserved_[3];

	static const media_encoded_audio_format wildcard;
};


struct media_encoded_audio_header {
	// NOTE: More data fields need to go to the end
	int32		_reserved_0[14];

	uint32		buffer_flags;
		// B_MEDIA_KEY_FRAME for key frame chunks
	uchar		unused_mask;
		// mask of unused bits for the last byte of data
	uchar		_reserved_2[3];

};

enum media_display_flags {
	B_F1_DOMINANT		= 0x1,	// The first buffer sent (temporally) will
								// be an F1 field.
	B_F2_DOMINANT		= 0x2,	// The first buffer sent (temporally) will
								// be an F2 field.
	B_TOP_SCANLINE_F1	= 0x4,	// The topmost scanline of the output buffer
								// belongs to F1.
	B_TOP_SCANLINE_F2	= 0x8	// The topmost scanline of the output buffer
								// belongs to F2.
};


struct media_video_display_info {
	color_space	format;
	uint32		line_width;
	uint32		line_count;		// sum of all interlace fields lines
	uint32		bytes_per_row;	// bytes_per_row is in format, not header,
								// because it's part of SetBuffers
	uint32		pixel_offset;	// (in pixels) Offset from the start of the
								// buffer (see below).
	uint32		line_offset;	// (in lines) Offset to the start of the field.
								// Think "buffer == framebuffer" when the
								// window displaying the active field moves
								// on screen.
	uint32		flags;
	uint32		_reserved_[3];

	static const media_video_display_info wildcard;
};


struct media_raw_video_format {
	float		field_rate;
	uint32		interlace;		// Number of fields per frame: 1 means
								// progressive (non-interlaced) frames.
	uint32		first_active;	// Index of first active line. 0, typically
								// (wildcard, or "don't care")
	uint32		last_active;	// Index of last active line (typically
								// line_count - 1, if first_active is 0.
	uint32		orientation;	// B_VIDEO_TOP_LEFT_RIGHT is preferred.

	// This is the display aspect ratio (DAR). Usually, you would reduce the
	// width and height of the intended output frame size as far as possible
	// without changing their ratio. Note that you should not put 1 in both
	// fields to mean "undistorted pixels", unless you really intend square
	// video output!
	uint16		pixel_width_aspect;		// 1:1 has 1 here, 4:3 has 4 here
										// 16:9 has 16 here!
	uint16		pixel_height_aspect;	// 1:1 has 1 here, 4:3 has 3 here
										// 16:9 has 9 here!

	media_video_display_info	display;

	static const media_raw_video_format wildcard;
};


struct media_video_header {
	uint32		_reserved_[8];		// NOTE: Keep reserved data at the top!

	uint32      display_line_width; // Number of pixels per display_line
	uint32      display_line_count;	// Sum of all interlace fields lines
	uint32      bytes_per_row;		// Number of bytes in a display_line
									// (padding bytes excluded)
	uint16      pixel_width_aspect;	// 1:1 has 1 here, 4:3 has 4 here
									// 16:9 has 16 here!
	uint16      pixel_height_aspect;// 1:1 has 1 here, 4:3 has 3 here
									// 16:9 has 9 here!
	float		field_gamma;
	uint32		field_sequence;		// Sequence number since start of capture
									// May roll over if machine is on for a
									// LONG time.
	uint16		field_number;		// 0 .. {interlace-1}; F1 == 0 ("odd"),
									// F2 == 1 ("even")
	uint16		pulldown_number;	// 0..2 for pulldown duplicated sequence
	uint16		first_active_line;	// The NTSC/PAL line number (1-based) of
									// the first line in this field
	uint16		line_count;			// The number of active lines in buffer.
};


struct media_encoded_video_format {
	enum video_encoding {
		B_ANY
	};

	media_raw_video_format	output;				// set unknowns to wildcard

	float					avg_bit_rate;
	float					max_bit_rate;

	video_encoding			encoding;
	size_t					frame_size;

	int16					forward_history;	// maximum forward memory
												// required by codec

	int16					backward_history;	// maximum backward memory
												// required by codec

	uint32					_reserved_[3];		// This structure cannot grow
												// more than this (embedded)
												// in media_format union

	static const media_encoded_video_format wildcard;
};


struct media_encoded_video_header {
	// NOTE: More data fields need to go at the end of this structure.
	int32		_reserved_1[9];

	uint32		field_flags;		//	B_MEDIA_KEY_FRAME

	int16		forward_history;	// forward memory required by this buffer
									// (0 for key frames)

	int16		backward_history;	// backward memory required by this buffer
									// (0 for key frames)

	uchar		unused_mask;		// mask of unused bits for the last byte
									// of data
	uchar		_reserved_2[3];
	float		field_gamma;
	uint32		field_sequence;		// sequence since start of capture
	uint16		field_number;		// 0 .. {interlace-1};  F1 == 0, F2 == 1
	uint16		pulldown_number;	// 0..2 for pulldown duplicated sequence
	uint16		first_active_line;	// 0 or 1, typically, but could be 10 or
									// 11 for full-NTSC formats
	uint16		line_count;			// number of actual lines in buffer
};

struct media_multistream_format {
	enum {
		B_ANY					= 0,
		B_VID					= 1,	// raw raw_video/raw_audio buffers
		B_AVI,
		B_MPEG1,
		B_MPEG2,
		B_QUICKTIME,
		B_PRIVATE				= 90000,
		B_FIRST_USER_TYPE		= 100000
	};
	float		avg_bit_rate;			// 8 * byte rate, on average
	float		max_bit_rate;			// 8 * byte rate, tops
	uint32		avg_chunk_size;			// == max_chunk_size for fixed-size
										// chunks
	uint32		max_chunk_size;			// max buffer size
	enum {
		B_HEADER_HAS_FLAGS		= 0x1,	// are flags important?
		B_CLEAN_BUFFERS			= 0x2,	// each buffer represents an integral
										// number of "frames"
		B_HOMOGENOUS_BUFFERS	= 0x4	// a buffer has only one format in it
	};
	uint32		flags;
	int32		format;
	uint32		_reserved_[2];

	struct vid_info {
		float		frame_rate;
		uint16		width;
		uint16		height;
		color_space	space;

		float		sampling_rate;
		uint32		sample_format;
		uint16		byte_order;
		uint16		channel_count;
	};
	struct avi_info {
		uint32		us_per_frame;
		uint16		width;
		uint16		height;
		uint16		_reserved_;
		uint16		type_count;
		media_type	types[5];
	};

	union {
		vid_info	vid;
		avi_info	avi;
	} 			u;

	static const media_multistream_format wildcard;
};


struct media_multistream_header {
	uint32	_reserved_[14];
	uchar	unused_mask;			// mask of unused bits for the last byte
									// of data
	uchar	_reserved_2[3];
	enum {
		B_MASTER_HEADER		= 0x1,	// for master stream header data in buffer
		B_SUBSTREAM_HEADER	= 0x2,	// for sub-stream header data in buffer
		B_COMPLETE_BUFFER	= 0x4	// data is an integral number of "frames"
	};
	uint32 	flags;
};


extern const type_code B_CODEC_TYPE_INFO;


enum media_format_flags {
	B_MEDIA_RETAINED_DATA			= 0x1,
	B_MEDIA_MULTIPLE_BUFFERS		= 0x2,
	B_MEDIA_CONTIGUOUS_BUFFER		= 0x4,
	B_MEDIA_LINEAR_UPDATES			= 0x8,
	B_MEDIA_MAUI_UNDEFINED_FLAGS	= ~0xf	// NOTE: Always deny these flags
											// in new code.
};

// NOTE: A field of 0 typically means "anything" or "wildcard".
// NOTE: This structure should not be bigger than 192 bytes!
struct media_format {
	media_type						type;
	type_code						user_data_type;
	uchar							user_data[48];
	uint32							_reserved_[3];
	uint16							require_flags;	//	media_format_flags
	uint16							deny_flags;		//	media_format_flags

private:
	void*							meta_data;
	int32							meta_data_size;
	area_id							meta_data_area;
	area_id							__unused_was_use_area;
	team_id							__unused_was_team;
	void*							__unused_was_thisPtr;

public:
	union {
		media_multi_audio_format	raw_audio;
		media_raw_video_format		raw_video;
		media_multistream_format	multistream;
		media_encoded_audio_format	encoded_audio;
		media_encoded_video_format	encoded_video;
		char						_reserved_[96];	 // pad to 96 bytes
	} u;

	bool 			IsVideo() const;

	uint32  		Width() const;
	uint32 			Height() const;
	color_space  	ColorSpace() const;

	uint32& 		Width();
	uint32& 		Height();
	color_space& 	ColorSpace();

	bool 			IsAudio() const;
	uint32 			AudioFormat() const;
	uint32& 		AudioFormat();
	uint32 			AudioFrameSize() const;

	uint32			Encoding() const;

	bool			Matches(const media_format* other) const;
	void			SpecializeTo(const media_format* other);

	status_t		SetMetaData(const void* data, size_t size);
	const void*		MetaData() const;
	int32			MetaDataSize() const;

	void			Unflatten(const char *flatBuffer);
	void			Clear();

					media_format();
					media_format(const media_format& other);
					~media_format();

	media_format&	operator=(const media_format& other);
};


bool operator==(const media_raw_audio_format& a,
	const media_raw_audio_format& b);

bool operator==(const media_multi_audio_info& a,
	const media_multi_audio_info& b);

bool operator==(const media_multi_audio_format& a,
	const media_multi_audio_format& b);

bool operator==(const media_encoded_audio_format& a,
	const media_encoded_audio_format& b);

bool operator==(const media_video_display_info& a,
	const media_video_display_info& b);

bool operator==(const media_raw_video_format& a,
	const media_raw_video_format& b);

bool operator==(const media_encoded_video_format& a,
	const media_encoded_video_format& b);

bool operator==(const media_multistream_format::vid_info& a,
	const media_multistream_format::vid_info& b);

bool operator==(const media_multistream_format::avi_info& a,
	const media_multistream_format::avi_info & b);

bool operator==(const media_multistream_format& a,
	const media_multistream_format& b);

bool operator==(const media_format& a, const media_format& b);


bool format_is_compatible(const media_format & a, const media_format & b);
	// Returns true if a and b are compatible (accounting for wildcards)
	// (a is the format you want to feed to something accepting b

bool string_for_format(const media_format & f, char * buf, size_t size);


struct media_seek_tag {
	char	data[16];
};


struct media_header_time_code {
	int8	type;		//	See TimeCode.h; don't use the "DEFAULT" value
	int8	_reserved;
	int8	hours;
	int8	minutes;
	int8	seconds;
	int8	frames;
	int16	subframes;	//	Set to -1 if not available
};


// Broadcast() fills in fields marked with "//+"
struct media_header {
	media_type		type;			// what kind of data (for union)
	media_buffer_id buffer;			//+ what buffer does this header go with?
	int32			destination;	//+ what 'socket' is this intended for?
	media_node_id	time_source;	// node that encoded start_time
	uint32			_deprecated_;	// used to be change_tag
	uint32			size_used;		// size within buffer that is used
	bigtime_t		start_time;		// performance time
	area_id			owner;			//+ buffer owner info area
	enum {
		B_SEEK_TAG	= 'TRST',		// user data type of the codec seek
									// protocol. size of seek tag is 16 bytes
		B_TIME_CODE	= 'TRTC'		// user data is media_header_time_code
	};
	type_code		user_data_type;
	uchar			user_data[64];	// user_data_type indicates what this is
	int32			source;
	port_id			source_port;

	off_t			file_pos;		// where in a file this data came from
	size_t			orig_size;		// and how big it was.  if unused, zero out

	uint32			data_offset;	// offset within buffer (already reflected in Data())

	union {
		media_audio_header			raw_audio;
		media_video_header			raw_video;
		media_multistream_header	multistream;
		media_encoded_audio_header	encoded_audio;
		media_encoded_video_header	encoded_video;
		char						_reserved_[64];	// pad to 64 bytes
	}				u;
};


struct media_file_format_id {
	ino_t	node;
	dev_t	device;
	uint32	internal_id;
};


bool operator==(const media_file_format_id& a, const media_file_format_id& b);
bool operator<(const media_file_format_id& a, const media_file_format_id& b);


typedef enum {
	B_ANY_FORMAT_FAMILY			= 0,
	B_BEOS_FORMAT_FAMILY		= 1,
	B_QUICKTIME_FORMAT_FAMILY	= 2,	// QuickTime is a registered
										// trademark of Apple Computer.
	B_AVI_FORMAT_FAMILY			= 3,
	B_ASF_FORMAT_FAMILY			= 4,
	B_MPEG_FORMAT_FAMILY		= 5,
	B_WAV_FORMAT_FAMILY			= 6,
	B_AIFF_FORMAT_FAMILY		= 7,
	B_AVR_FORMAT_FAMILY			= 8,

	B_MISC_FORMAT_FAMILY		= 99999,
} media_format_family;


struct media_file_format {
	// Possible flags for capabilities bitmask
	enum {
		B_READABLE				= 0x1,
		B_WRITABLE				= 0x2,
		B_PERFECTLY_SEEKABLE	= 0x4,
		B_IMPERFECTLY_SEEKABLE	= 0x8,
		B_KNOWS_RAW_VIDEO		= 0x10,
		B_KNOWS_RAW_AUDIO		= 0x20,
		B_KNOWS_MIDI			= 0x40,
		B_KNOWS_ENCODED_VIDEO	= 0x80,
		B_KNOWS_ENCODED_AUDIO	= 0x100,
		B_KNOWS_OTHER			= 0x1000000, // For example sub-title streams
		B_KNOWS_ANYTHING		= 0x2000000
	};
	uint32					capabilities;	// Bitmask, see flags above
	media_file_format_id	id;				// Opaque id used to construct a
											// BMediaFile
	media_format_family		family;			// One of the family enums
	int32					version;		// 100 for 1.0

	uint32					_reserved_[25];

	char					mime_type[64];
	char					pretty_name[64];	// "QuickTime File Format"
	char					short_name[32];		// "quicktime", "avi", "mpeg"
	char					file_extension[8];	// "mov", "avi", "mpg"

	char					reserved[88];
};


// Initialize the cookie to 0 and keep calling this function to iterate
// over all available media file format writers.
status_t get_next_file_format(int32* cookie, media_file_format* mfi);


// A buffer of this size is guaranteed to be large enough to hold any
// message, which your service thread can read from read_port() and
// passes on to HandleMessage().
const size_t B_MEDIA_MESSAGE_SIZE = 16384;


extern const char* B_MEDIA_SERVER_SIGNATURE;

class media_node;
struct media_input;
struct media_output;
struct live_node_info;
struct dormant_node_info;
struct buffer_clone_info;


// Functions which control the shutdown and launching process of the
// media_server and it's friends. You can provide a call back hook which
// will be called during various steps of the process. This callback should
// currently always return TRUE. A 'stage' value of 100 means the process is
// completely finished. Differently from BeOS the functions automatically
// send notifications to the Deskbar if not differently specified.
// It's also provided a new version of launch_media_server allowing
// to specify a custom callback for notifications.

status_t launch_media_server(bigtime_t timeout = B_INFINITE_TIMEOUT,
	bool (*progress)(int stage, const char* message, void* cookie) = NULL,
	void* cookie = NULL, uint32 flags = 0);

status_t shutdown_media_server(bigtime_t timeout = B_INFINITE_TIMEOUT,
	bool (*progress)(int stage, const char* message, void* cookie) = NULL,
	void* cookie = NULL);


// A teeny bit of legacy preserved for BSoundFile from R3.
// These came from the old MediaDefs.h; don't use them
// unless you get them from BSoundFile.


// values for byte_ordering
enum {
	B_BIG_ENDIAN,
	B_LITTLE_ENDIAN
};


// values for sample_format
enum {
	B_UNDEFINED_SAMPLES,
	B_LINEAR_SAMPLES,
	B_FLOAT_SAMPLES,
	B_MULAW_SAMPLES
};


// #pragma mark - encoders and file writers


struct media_encode_info {
	uint32		flags;					// B_MEDIA_KEY_FRAME, set before every
										// use

	int32		used_data_size;			// data size used by other tracks
										// add output size used by this encoder

	bigtime_t	start_time;				// us from start of file
	bigtime_t	time_to_encode;			// 0 - hurry up, B_INFINITE_TIMEOUT
										// - don't care
	int32		_pad[22];

	void*		file_format_data;		// file format specific info
	size_t		file_format_data_size;

	void*		codec_data;				// codec specific info
	size_t		codec_data_size;

	media_encode_info();
};


struct encode_parameters {
	float		quality;				// 0.0-1.0 , 1.0 is high quality

	int32		avg_field_size;			// in bytes
	int32		max_field_size;			// in bytes

	int32		_pad[27];

	void*		user_data;				// codec specific info
	size_t		user_data_size;
};


struct media_decode_info {
	bigtime_t	time_to_decode;			// 0 - hurry up, B_INFINITE_TIMEOUT
										// - don't care

	int32		_pad[26];

	void*		file_format_data;		// file format specific info
	size_t		file_format_data_size;

	void*		codec_data;				// codec specific info
	size_t		codec_data_size;

	media_decode_info();
};


// #pragma mark - inline implementations


inline bool
media_format::IsVideo() const
{
	return type == B_MEDIA_ENCODED_VIDEO || type == B_MEDIA_RAW_VIDEO;
}


inline uint32
media_format::Width() const
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.line_width
		: u.raw_video.display.line_width;
}


inline uint32
media_format::Height() const
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.line_count
		: u.raw_video.display.line_count;
}


inline color_space
media_format::ColorSpace() const
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.format
		: u.raw_video.display.format;
}


inline uint32&
media_format::Width()
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.line_width
		: u.raw_video.display.line_width;
}


inline uint32&
media_format::Height()
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.line_count
		: u.raw_video.display.line_count;
}


inline color_space&
media_format::ColorSpace()
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.output.display.format
		: u.raw_video.display.format;
}


inline bool
media_format::IsAudio() const
{
	return type == B_MEDIA_ENCODED_AUDIO || type == B_MEDIA_RAW_AUDIO;
}


inline uint32
media_format::AudioFormat() const
{
	return type == B_MEDIA_ENCODED_AUDIO
		? u.encoded_audio.output.format : u.raw_audio.format;
}


inline uint32&
media_format::AudioFormat()
{
	return type == B_MEDIA_ENCODED_AUDIO
		? u.encoded_audio.output.format : u.raw_audio.format;
}


inline uint32
media_format::AudioFrameSize() const
{
	return type == B_MEDIA_ENCODED_AUDIO
		? (u.encoded_audio.output.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* u.encoded_audio.output.channel_count
		: (u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* u.raw_audio.channel_count;
}


inline uint32
media_format::Encoding() const
{
	return type == B_MEDIA_ENCODED_VIDEO
		? u.encoded_video.encoding : type == B_MEDIA_ENCODED_AUDIO
			? u.encoded_audio.encoding : type == B_MEDIA_MULTISTREAM
				? u.multistream.format : 0UL;
}


#endif /* end of __cplusplus section */


#endif /* MEDIA_DEFS_H */
