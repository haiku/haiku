/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEDIA_TYPES_H
#define _MEDIA_TYPES_H


#include <MediaDefs.h>

#include <Messenger.h>
#include <List.h>
#include <Locker.h>


struct media_codec_info {
	char	pretty_name[96];   /* eg: "SuperSqueeze Encoder by Foo Inc" */
	char	short_name[32];    /* eg: "SuperSqueeze" */

	int32	id;                /* opaque id passed to
								  BMediaFile::CreateTrack() */
	int32	sub_id;

	int32	pad[63];
};

/*!	\brief	Use this to iterate through the available encoders for a given file
			format.
	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param fileFormat	A pointer to a valid media_file_format structure
						as can be obtained through get_next_file_format().
	\param inputFormat	This is the type of data given to the encoder.
	\param _outputFormat This is the type of data the encoder will output.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
 */
status_t get_next_encoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, media_format* _outputFormat,
	media_codec_info* _codecInfo);

/*!	\brief	Use this to iterate through the available encoders with
			restrictions to the input and output media_format while the
			encoders can specialize wildcards in the media_formats.

	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param fileFormat	A pointer to a valid media_file_format structure
						as can be obtained through get_next_file_format().
						You can pass \c NULL if you don't care.
	\param inputFormat	This is the type of data given to the encoder,
						wildcards are accepted.
	\param outputFormat	This is the type of data you want the encoder to
						output. Wildcards are accepted.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.
	\param _acceptedInputFormat This is the type of data that the encoder will
						accept as input. Wildcards in \a inFormat will be
						specialized here.
	\param _acceptedOutputFormat This is the type of data that the encoder will
						output. Wildcards in \a _outFormat will be specialized
						here.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
*/
status_t get_next_encoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, const media_format* outputFormat,
	media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
	media_format* _acceptedOutputFormat);


/*!	\brief	Iterate over the all the available encoders without media_format
			restrictions.

	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
*/
status_t get_next_encoder(int32* cookie, media_codec_info* _codecInfo);


enum media_file_accept_format_flags {
	B_MEDIA_REJECT_WILDCARDS = 0x1
};

bool does_file_accept_format(const media_file_format* fileFormat,
	media_format* format, uint32 flags = 0);


typedef struct {
	uint8 data[16];
} GUID;


enum beos_format {
	B_BEOS_FORMAT_RAW_AUDIO = 'rawa',
	B_BEOS_FORMAT_RAW_VIDEO = 'rawv'
};


typedef struct {
	int32 format;
} media_beos_description;


typedef struct {
	uint32 codec;
	uint32 vendor;
} media_quicktime_description;


typedef struct {
	uint32 codec;
} media_avi_description;


typedef struct {
	uint32 id;
} media_avr_description;


typedef struct {
	GUID guid;
} media_asf_description;


enum mpeg_id {
	B_MPEG_ANY = 0,
	B_MPEG_1_AUDIO_LAYER_1 = 0x101,
	B_MPEG_1_AUDIO_LAYER_2 = 0x102,
	B_MPEG_1_AUDIO_LAYER_3 = 0x103,		/* "MP3" */
	B_MPEG_1_VIDEO = 0x111,
	B_MPEG_2_AUDIO_LAYER_1 = 0x201,
	B_MPEG_2_AUDIO_LAYER_2 = 0x202,
	B_MPEG_2_AUDIO_LAYER_3 = 0x203,
	B_MPEG_2_VIDEO = 0x211,
	B_MPEG_2_5_AUDIO_LAYER_1 = 0x301,
	B_MPEG_2_5_AUDIO_LAYER_2 = 0x302,
	B_MPEG_2_5_AUDIO_LAYER_3 = 0x303,
};


typedef struct {
	uint32 id;
} media_mpeg_description;


typedef struct {
	uint32 codec;
} media_wav_description;


typedef struct {
	uint32 codec;
} media_aiff_description;


typedef struct {
	uint32 file_format;
	uint32 codec;
} media_misc_description;


typedef struct _media_format_description {
								_media_format_description();
								~_media_format_description();
								_media_format_description(
									const _media_format_description& other);
	_media_format_description&	operator=(
									const _media_format_description& other);

	media_format_family family;
	uint32 _reserved_[3];
	union {
		media_beos_description beos;
		media_quicktime_description quicktime;
		media_avi_description avi;
		media_asf_description asf;
		media_mpeg_description mpeg;
		media_wav_description wav;
		media_aiff_description aiff;
		media_misc_description misc;
		media_avr_description avr;
		uint32 _reserved_[12];
	} u;
} media_format_description;


class BMediaFormats {
public:
								BMediaFormats();
	virtual						~BMediaFormats();

			status_t			InitCheck();

	// Make sure you memset() your descs to 0 before you start filling
	// them in! Else you may register some bogus value.
	enum make_format_flags {
		B_EXCLUSIVE = 0x1,		// Fail if this format has already been
								// registered.

		B_NO_MERGE = 0x2,		// Don't re-number any formats if there are
								// multiple clashing previous registrations,
								// but fail instead.

		B_SET_DEFAULT = 0x4		// Set the first format to be the default for
								// the format family (when registering more
								// than one in the same family). Only use in
								// Encoder add-ons.
	};

			status_t			MakeFormatFor(const media_format_description*
									descriptions, int32 descriptionCount,
									media_format* _inOutFormat,
									uint32 flags = 0, void* _reserved = 0);

			status_t			GetFormatFor(const media_format_description&
									description, media_format* _outFormat);

			status_t			GetCodeFor(const media_format& format,
									media_format_family family,
									media_format_description* _outDescription);

			status_t			RewindFormats();
			status_t			GetNextFormat(media_format* _outFormat,
									media_format_description* _outDescription);

	// You need to lock/unlock (only) when using
	// RewindFormats()/GetNextFormat().
			bool				Lock();
			void				Unlock();

	//	convenience functions
	static	status_t			GetBeOSFormatFor(uint32 fourcc,
									media_format* _outFormat,
									media_type type = B_MEDIA_UNKNOWN_TYPE);
	static	status_t			GetAVIFormatFor(uint32 fourcc,
									media_format* _outFormat,
									media_type type = B_MEDIA_UNKNOWN_TYPE);
	static	status_t			GetQuicktimeFormatFor(uint32 vendor,
									uint32 fourcc, media_format* _outFormat,
									media_type type = B_MEDIA_UNKNOWN_TYPE);

	// Deprecated:
			status_t			MakeFormatFor(const media_format_description&
									description, const media_format& inFormat,
									media_format* _outFormat);

private:
			int32				fIteratorIndex;

			uint32				_reserved[30];
};


bool operator==(const media_format_description& a,
	const media_format_description& b);

bool operator<(const media_format_description& a,
	const media_format_description& b);


bool operator==(const GUID& a, const GUID& b);

bool operator<(const GUID& a, const GUID& b);


#endif	// _MEDIA_TYPES_H
