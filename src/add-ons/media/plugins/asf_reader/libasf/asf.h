/*  libasf - An Advanced Systems Format media file parser
 *  Copyright (C) 2006-2010 Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ASF_H
#define ASF_H

/* used int types for different platforms */
#if defined(WIN32) && !defined(__MINGW_H)
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;

typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#if defined(WIN32) && defined(DLL_EXPORT)
# define LIBASF_API __declspec(dllexport)
#else
# define LIBASF_API
#endif




typedef enum {
	/* fatal errors related to library function */
	ASF_ERROR_INTERNAL            = -100,  /* incorrect input to API calls */
	ASF_ERROR_OUTOFMEM            = -101,  /* some malloc inside program failed */

	/* errors related to reading data from stream */
	ASF_ERROR_NEEDS_BYTES         = -200,  /* not enough data from read */
	ASF_ERROR_EOF                 = -201,  /* end of file reached */
	ASF_ERROR_IO                  = -202,  /* error reading or writing to file */

	/* errors related to file being corrupted */
	ASF_ERROR_INVALID_LENGTH      = -300,  /* length value conflict in input data */
	ASF_ERROR_INVALID_VALUE       = -301,  /* other value conflict in input data */
	ASF_ERROR_INVALID_OBJECT      = -302,  /* ASF object missing or in wrong place */
	ASF_ERROR_INVALID_OBJECT_SIZE = -303,  /* invalid ASF object size (too small) */

	/* errors related to seeking, usually non-fatal */
	ASF_ERROR_SEEKABLE            = -400,  /* file not seekable */
	ASF_ERROR_SEEK                = -401   /* file is seekable but seeking failed */
} asf_error_t;

typedef enum {
	ASF_STREAM_TYPE_NONE     = 0x00,
	ASF_STREAM_TYPE_AUDIO    = 0x01,
	ASF_STREAM_TYPE_VIDEO    = 0x02,
	ASF_STREAM_TYPE_COMMAND  = 0x03,
	ASF_STREAM_TYPE_UNKNOWN  = 0xff
} asf_stream_type_t;

#define ASF_STREAM_FLAG_NONE       0x0000
#define ASF_STREAM_FLAG_AVAILABLE  0x0001
#define ASF_STREAM_FLAG_HIDDEN     0x0002
#define ASF_STREAM_FLAG_EXTENDED   0x0004


/* waveformatex fields specified in Microsoft documentation:
   http://msdn2.microsoft.com/en-us/library/ms713497.aspx */
struct asf_waveformatex_s {
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
	uint8_t *data;
};
typedef struct asf_waveformatex_s asf_waveformatex_t;

#define ASF_BITMAPINFOHEADER_SIZE 40
/* bitmapinfoheader fields specified in Microsoft documentation:
   http://msdn2.microsoft.com/en-us/library/ms532290.aspx */
struct asf_bitmapinfoheader_s {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	uint8_t *data;
};
typedef struct asf_bitmapinfoheader_s asf_bitmapinfoheader_t;


struct asf_iostream_s {
	/* read function, returns -1 on error, 0 on EOF and read bytes
	 * otherwise */
	int32_t (*read)(void *opaque, void *buffer, int32_t size);

	/* write function, returns -1 on error, 0 on EOF and written
	 * bytes otherwise */
	int32_t (*write)(void *opaque, void *buffer, int32_t size);

	/* seek function, seeks to offset from beginning of the file,
	 * returns -1 on error, 0 on EOF */
	int64_t (*seek)(void *opaque, int64_t offset);

	/* opaque data pointer passed to each of the stream handling
	 * callbacks */
	void *opaque;
};
typedef struct asf_iostream_s asf_iostream_t;

struct asf_metadata_entry_s {
	char *key;	/* key of extended metadata entry */
	char *value;	/* value of extended metadata entry */
};
typedef struct asf_metadata_entry_s asf_metadata_entry_t;

/* all metadata entries are presented in UTF-8 character encoding */
struct asf_metadata_s {
	char *title;		/* title of the stream */
	char *artist;		/* artist of the stream */
	char *copyright;	/* copyright holder */
	char *description;	/* description of the stream */
	char *rating;		/* rating of the stream */
	uint16_t extended_count;	/* number of extended entries */
	asf_metadata_entry_t *extended; /* array of extended entries */
};
typedef struct asf_metadata_s asf_metadata_t;

struct asf_payload_s {
	uint8_t stream_number;	/* the stream number this payload belongs to */
	uint8_t key_frame;	/* a flag indicating if this payload contains a key frame  */

	uint32_t media_object_number;	/* number of media object this payload is part of */
	uint32_t media_object_offset;	/* byte offset from beginning of media object */

	uint32_t replicated_length;	/* length of some replicated data of a media object... */
	uint8_t *replicated_data;	/* the replicated data mentioned */

	uint32_t datalen;	/* length of the actual payload data */
	uint8_t *data;		/* the actual payload data to decode */

	uint32_t pts;		/* presentation time of this payload */
};
typedef struct asf_payload_s asf_payload_t;

struct asf_packet_s {
	uint8_t ec_length;	/* error correction data length */
	uint8_t *ec_data;	/* error correction data array */

	uint32_t length;		/* length of this packet, usually constant per stream */
	uint32_t sequence;              /* sequence value reserved for future use, should be ignored */
	uint32_t padding_length;	/* length of the padding after the data in this packet */
	uint32_t send_time;		/* send time of this packet in milliseconds */
	uint16_t duration;		/* duration of this packet in milliseconds */

	uint16_t payload_count;		/* number of payloads contained in this packet */
	asf_payload_t *payloads;	/* an array of payloads in this packet */
	uint16_t payloads_size;		/* for internal library use, not to be modified by applications! */

	uint32_t payload_data_len;	/* length of the raw payload data of this packet */
	uint8_t *payload_data;		/* the raw payload data of this packet, usually not useful */

	uint8_t *data;		/* for internal library use, not to be modified by applications! */
	uint32_t data_size;	/* for internal library use, not to be modified by applications! */
};
typedef struct asf_packet_s asf_packet_t;

struct asf_stream_extended_properties_s {
	uint64_t start_time;
	uint64_t end_time;
	uint32_t data_bitrate;
	uint32_t buffer_size;
	uint32_t initial_buf_fullness;
	uint32_t data_bitrate2;
	uint32_t buffer_size2;
	uint32_t initial_buf_fullness2;
	uint32_t max_obj_size;
	uint32_t flags;
	uint16_t stream_num;
	uint16_t lang_idx;
	uint64_t avg_time_per_frame;
	uint16_t stream_name_count;
	uint16_t num_payload_ext;
};
typedef struct asf_stream_extended_properties_s asf_stream_extended_properties_t;

struct asf_stream_s {
	asf_stream_type_t type;	/* type of this current stream */
	uint16_t flags;         /* possible flags related to this stream */

	/* pointer to type specific data (ie. waveformatex or bitmapinfoheader)
	 * only available if ASF_STREAM_FLAG_AVAILABLE flag is set, otherwise NULL */
	void *properties;

	/* pointer to extended properties of the stream if they specified
	 * only available if ASF_STREAM_FLAG_EXTENDED flag is set, otherwise NULL */
	asf_stream_extended_properties_t *extended_properties;
};
typedef struct asf_stream_s asf_stream_t;

typedef struct asf_file_s asf_file_t;


/* initialize the library using file on a local filesystem */
LIBASF_API asf_file_t *asf_open_file(const char *filename);

/* initialize the library using callbacks defined on a stream structure,
   the stream structure can be freed after calling this function */
LIBASF_API asf_file_t *asf_open_cb(asf_iostream_t *iostream);

/* close the library handle and free all allocated memory */
LIBASF_API void asf_close(asf_file_t *file);


/* initialize the library and read all header information of the ASF file */
LIBASF_API int asf_init(asf_file_t *file);


/* create a packet structure for reading data packets */
LIBASF_API asf_packet_t *asf_packet_create();

/* free the packet structure allocated earlier, need to be called only once */
LIBASF_API void asf_packet_destroy(asf_packet_t *packet);

/* get next packet from the stream to the specified packet structure */
LIBASF_API int asf_get_packet(asf_file_t *file, asf_packet_t *packet);

/* seek to the closest (key frame) packet specified by milliseconds position */
LIBASF_API int64_t asf_seek_to_msec(asf_file_t *file, int64_t msec);


/* get metadata information of the ASF file handle */
LIBASF_API asf_metadata_t *asf_header_get_metadata(asf_file_t *file);

/* free metadata structure received from the library */
LIBASF_API void asf_metadata_destroy(asf_metadata_t *metadata);

/* free all header information from the ASF file structure
 * WARNING: after calling this function all asf_header_*
 *          functions will return NULL or failure!!! */
LIBASF_API void asf_header_destroy(asf_file_t *file);


/* calculate how many streams are available in current ASF file */
LIBASF_API uint8_t asf_get_stream_count(asf_file_t *file);

/* get info of a stream, the resulting pointer and its contents should NOT be freed */
LIBASF_API asf_stream_t *asf_get_stream(asf_file_t *file, uint8_t track);


/* return non-zero if the file is broadcasted, 0 otherwise */
LIBASF_API int asf_is_broadcast(asf_file_t *file);

/* return non-zero if the file is seekable, 0 otherwise */
LIBASF_API int asf_is_seekable(asf_file_t *file);

/* get size of the ASF file in bytes */
LIBASF_API uint64_t asf_get_file_size(asf_file_t *file);

/* get creation date in 100-nanosecond units since Jan 1, 1601 GMT
   this value should be ignored for broadcasts */
LIBASF_API uint64_t asf_get_creation_date(asf_file_t *file);

/* get number of data packets available in this file
   this value should be ignored for broadcasts */
LIBASF_API uint64_t asf_get_data_packets(asf_file_t *file);

/* get play duration of the file in 100-nanosecond units,
   this value should be ignored for broadcasts */
LIBASF_API uint64_t asf_get_duration(asf_file_t *file);

/* maximum bitrate as bits per second in the entire file */
LIBASF_API uint32_t asf_get_max_bitrate(asf_file_t *file);

#endif
