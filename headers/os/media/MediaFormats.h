/*	MediaFormats.h	*/
/*	Copyright 1998 Be Incorporated. All rights reserved.	*/

#if !defined(_MEDIA_TYPES_H)
#define _MEDIA_TYPES_H

#if defined(__cplusplus)
#include <MediaDefs.h>

#include <Messenger.h>
#include <List.h>
#include <Locker.h>
#endif


struct media_codec_info {
	char	pretty_name[96];   // eg: "SuperSqueeze Encoder by Foo Inc"
	char	short_name[32];    // eg: "supersqueeze"
	
	int32	id;                // opaque id passed to BMediaFile::CreateTrack
	int32	sub_id;
	
	int32	pad[63];
};

//
// Use this to iterate through the available encoders for a file format.
//
status_t get_next_encoder(int32 *cookie,
						  const media_file_format *mfi,		// this comes from get_next_file_format()
						  const media_format *input_format,	// this is the type of data given to the encoder
						  media_format *output_format,		// this is the type of data encoder will output 
						  media_codec_info *ei);			// information about the encoder
 
status_t get_next_encoder(
	int32 *cookie,
	const media_file_format *mfi,		// this comes from get_next_file_format()
										// pass NULL if you don't care
	const media_format *input_format,	// this is the type of data given to the
										// encoder, wildcards are accepted
	const media_format *output_format,	// this is the type of data encoder
										// you want the encoder to output.
										// Wildcards are accepted
	media_codec_info *ei,				// information about the encoder
	media_format *accepted_input_format,// this is the type of data that the
										// encoder will accept as input.
										// Wildcards in input_format will be
										// specialized here.
	media_format *accepted_output_format// this is the type of data that the
										// encoder will output.
										// Wildcards in output_format will be
										// specialized here.
	);


status_t get_next_encoder(int32 *cookie, media_codec_info *ei);

enum media_file_accept_format_flags {
	B_MEDIA_REJECT_WILDCARDS = 0x1
};

bool does_file_accept_format(const media_file_format *mfi,
                             media_format *format, uint32 flags = 0);

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
	B_MPEG_1_AUDIO_LAYER_3 = 0x103,		//	"MP3"
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

typedef struct {
	char description[48];
} media_meta_description;

typedef struct _media_format_description {
#if defined(__cplusplus)
	_media_format_description();
	~_media_format_description();
	_media_format_description(const _media_format_description & other);
	_media_format_description & operator=(const _media_format_description & other);
#endif
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
		media_meta_description meta;
		uint32 _reserved_[12];
	} u;
} media_format_description;

#if defined(__cplusplus)

// temporary functionality, will go away...
status_t _get_format_for_description(media_format *out_format, const media_format_description &in_desc);
status_t _get_meta_description_for_format(media_format_description *out_desc, const media_format &in_format);

class BMediaFormats {
public:
			BMediaFormats();
virtual		~BMediaFormats();

		status_t InitCheck();

				// Make sure you memset() your descs to 0 before you start filling
				// them in! Else you may register some bogus value.
		enum make_format_flags {
			B_EXCLUSIVE = 0x1,			//	Fail if this format has already been registered
			B_NO_MERGE = 0x2,			//	Don't re-number any formats if there are multiple
										//	clashing previous registrations, but fail instead
			B_SET_DEFAULT = 0x4			//	Set the first format to be the default for the
										//	format family (when registering more than one in
										//	the same family). Only use in Encoder add-ons.
		};
		status_t MakeFormatFor(
				const media_format_description * descs,
				int32 desc_count,
				media_format * io_format,
				uint32 flags = 0,
				void * _reserved = 0);
		status_t GetFormatFor(
				const media_format_description & desc,
				media_format * out_format);

		//	convenience functions
		static status_t GetBeOSFormatFor(
				uint32 fourcc, media_format * out_format,
				media_type type = B_MEDIA_UNKNOWN_TYPE);
		static status_t GetAVIFormatFor(
				uint32 fourcc, media_format * out_format,
				media_type type = B_MEDIA_UNKNOWN_TYPE);
		static status_t GetQuicktimeFormatFor(
				uint32 vendor, uint32 fourcc, media_format * out_format,
				media_type type = B_MEDIA_UNKNOWN_TYPE);

		status_t GetCodeFor(
				const media_format & format,
				media_format_family family,
				media_format_description * out_description);

		status_t RewindFormats();
		status_t GetNextFormat(
				media_format * out_format,
				media_format_description * out_description);
		//	You need to lock/unlock (only) when using RewindFormats()/GetNextFormat()
		bool Lock();
		void Unlock();

	/* --- begin deprecated API --- */
		status_t MakeFormatFor(
				const media_format_description & desc,
				const media_format & in_format,
				media_format * out_format);
private:
		BLocker *	fLocker;
		int32		fIndex;
		uint32		_reserved[21];
};

_IMPEXP_MEDIA bool operator==(const media_format_description & a, const media_format_description & b);
_IMPEXP_MEDIA bool operator<(const media_format_description & a, const media_format_description & b);

_IMPEXP_MEDIA bool operator==(const GUID & a, const GUID & b);
_IMPEXP_MEDIA bool operator<(const GUID & a, const GUID & b);
#endif

#endif	/* _MEDIA_TYPES_H */

