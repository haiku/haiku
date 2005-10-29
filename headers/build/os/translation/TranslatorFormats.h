/********************************************************************************
/
/      File:           TranslatorFormats.h
/
/      Description:    Defines for common "lowest common denominator" formats 
/                      used in the Translation Kit.
/
/      Copyright 1998, Be Incorporated, All Rights Reserved.
/      Copyright 1995-1997, Jon Watte
/
********************************************************************************/


#ifndef _TRANSLATOR_FORMATS_H
#define _TRANSLATOR_FORMATS_H


#include <GraphicsDefs.h>
#include <Rect.h>

/*	extensions used in the extension BMessage. Use of these 	*/
/*	is described in the documentation.	*/

_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_HEADER_ONLY[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_DATA_ONLY[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_COMMENT[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_TIME[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_FRAME[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_BITMAP_RECT[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_BITMAP_PALETTE[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_SOUND_CHANNEL[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_SOUND_MONO[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_SOUND_MARKER[];
_IMPEXP_TRANSLATION extern char B_TRANSLATOR_EXT_SOUND_LOOP[];

#if defined(_DATATYPES_COMPATIBLE_)
#define kCommentExtension		"/comment"
#define kTimeExtension			"/time"
#define kFrameExtension			"/frame"
#define kBitsRectExtension		"bits/Rect"
#define kBitsSpaceExtension		"bits/space"
#define kHeaderExtension		"/headerOnly"
#define kDataExtension			"/dataOnly"
#define kNoisChannelExtension	"nois/channel"
#define kNoisMonoExtension		"nois/mono"
#define kNoisMarkerExtension	"nois/marker"
#define kNoisLoopExtension		"nois/loop"
#endif


/*	For each data group, there is a "standard" format that	*/
/*	all translators should support.	*/
/*      These type codes are lower-case because they are already	*/
/*	established standards in the Be developer community.	*/
enum TranslatorGroups {
	B_TRANSLATOR_BITMAP			=	'bits',		/*	TranslatorBitmap	*/
	B_TRANSLATOR_PICTURE		=	'pict',		/*	BPicture data	*/
	B_TRANSLATOR_TEXT			=	'TEXT',		/*	B_ASCII_TYPE	*/
	B_TRANSLATOR_SOUND			=	'nois',		/*	TranslatorSound	*/
	B_TRANSLATOR_MIDI			=	'midi',		/*	standard MIDI	*/
	B_TRANSLATOR_MEDIA			=	'mhi!',		/*	a stream of stuff	*/
	B_TRANSLATOR_NONE			=	'none',
	B_TRANSLATOR_ANY_TYPE		=	0
};
#if defined(_DATATYPES_COMPATIBLE_)
#define kAnyType 0
#endif


/* some pre-defined data format type codes */

enum {
/* bitmap formats (B_TRANSLATOR_BITMAP) */
	B_GIF_FORMAT = 'GIF ',
	B_JPEG_FORMAT = 'JPEG',
	B_PNG_FORMAT = 'PNG ',
	B_PPM_FORMAT = 'PPM ',
	B_TGA_FORMAT = 'TGA ',
	B_BMP_FORMAT = 'BMP ',
	B_TIFF_FORMAT = 'TIFF',

/* line drawing formats (B_TRANSLATOR_PICTURE) */
	B_DXF_FORMAT = 'DXF ',
	B_EPS_FORMAT = 'EPS ',
	B_PICT_FORMAT = 'PICT',	/* MacOS PICT file */

/* sound file formats (B_TRANSLATOR_SOUND) */
	B_WAV_FORMAT = 'WAV ',
	B_AIFF_FORMAT = 'AIFF',
	B_CD_FORMAT = 'CD  ',	/* 44 kHz, stereo, 16 bit, linear, big-endian */
	B_AU_FORMAT = 'AU  ',	/* Sun ulaw */

/* text file formats (B_TRANSLATOR_TEXT) */
	B_STYLED_TEXT_FORMAT = 'STXT'
};



/*	This is the standard bitmap format	*/
/*	Note that data is always in big-endian format in the stream!	*/
struct TranslatorBitmap {
	uint32		magic;		/*	B_TRANSLATOR_BITMAP	*/
	BRect		bounds;
	uint32		rowBytes;
	color_space	colors;
	uint32		dataSize;
};	/*	data follows, padded to rowBytes	*/

/*	This is the standard sound format	*/
/*	Note that data is always in big-endian format in the stream!	*/
struct TranslatorSound {
	uint32		magic;		/*	B_TRANSLATOR_SOUND	*/
	uint32		channels;	/*	Left should always be first	*/
	float		sampleFreq;	/*	16 bit linear is assumed	*/
	uint32		numFrames;
};	/*	data follows	*/

/*	This is the standard styled text format	*/
/*	Note that it is of group type text!	*/
/*	Skip any records types you don't know about.	*/
/*	Always big-endian.	*/
struct TranslatorStyledTextRecordHeader {
	uint32		magic;
	uint32		header_size;
	uint32		data_size;
};	/*	any number of records	*/
struct TranslatorStyledTextStreamHeader {
#if defined(__cplusplus)
	enum {
		STREAM_HEADER_MAGIC = 'STXT'
	};
#endif
	TranslatorStyledTextRecordHeader	header;
	int32		version;	/* 100 */
};	/*	no data for this header	*/
struct TranslatorStyledTextTextHeader {
#if defined(__cplusplus)
	enum {
		TEXT_HEADER_MAGIC = 'TEXT'
	};
#endif
	TranslatorStyledTextRecordHeader	header;
	int32		charset;	/*	Always write B_UTF8 for now!	*/
};	/*	text data follows	*/
struct TranslatorStyledTextStyleHeader {
#if defined(__cplusplus)
	enum {
		STYLE_HEADER_MAGIC = 'STYL'
	};
#endif
	TranslatorStyledTextRecordHeader	header;
	uint32		apply_offset;
	uint32		apply_length;
};	/*	flattened R3 BTextView style records for preceding text	*/
#if 0
/*	format of the STYL record data (always big-endian) FYI:	*/
struct {
	uchar	magic[4];	/* 41 6c 69 21 */
	uchar	version[4];	/* 00 00 00 00 */
	int32	count;
	struct {
		int32	offset;
		char	family[64];
		char	style[64];
		float	size;
		float	shear;	/* typically 90.0 */
		uint16	face;	/* typically 0 */
		uint8	red;
		uint8	green;
		uint8	blue;
		uint8	alpha;	/* 255 == opaque */
		uint16	_reserved_; /* 0 */
	}	styles[];
};
#endif

#endif /* _TRANSLATOR_FORMATS_H	*/

