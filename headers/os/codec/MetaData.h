/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _META_DATA_H
#define _META_DATA_H

#include <Message.h>


namespace BPrivate {
namespace media {


enum MetaDataKeys {
	// Playback capabilities
	CanPause = 0x1000,	// bool
	CanSeekBackward,	// bool
	CanSeekForward,		// bool
	CanSeek,			// bool

	// Bitrates
	AudioBitRate,		// uint32 (bps)
	VideoBitRate,		// uint32 (bps)
	AudioSampleRate,	// uint32 (hz)
	VideoFrameRate,		// uint32 (hz)

	// RFC2046 and RFC4281
	MimeType,			// BString
	AudioCodec,			// BString
	VideoCodec,			// BString
	VideoHeight,		// uint32
	VideoWidth,			// uint32
	NumTracks,			// uint32
	DrmCrippled,		// bool

	// General use attributes
	Title,				// BString
	Comment,			// BString
	Copyright,			// BString
	Album,				// BString
	Artist,				// BString
	Author,				// BString
	Composer,			// BString
	Genre,				// BString
	// TODO: what we would use for encoded dates?
	// Date,			// date
	Duration,			// uint32 (ms)
	Rating,				// BString
	// TODO: BBitmap? uint8 array?
	//AlbumArt,			// 
	CDTrackNum,			// uint32
	CDTrackMax			// uint32
};


class BMetaData {
public:
						BMetaData();
						~BMetaData();

	// Woah. It seems we need BValue there.
	status_t			SetString(uint32 key, const BString& value);
	status_t			SetBool(uint32 key, bool value);
	status_t			SetUInt32(uint32 key, uint32 value);

	status_t			FindString(uint32 key, BString* value) const;
	status_t			FindBool(uint32 key, bool* value) const;
    status_t			FindUInt32(uint32 key, uint32* value) const;

	status_t			RemoveValue(uint32 key);

	// Clean up all keys
	void 				Reset();

	status_t			FromMessage(const BMessage& msg);
	const BMessage&		ToMessage();

private:
	// TODO: padding
	BMessage*			fMessage;

						BMetaData(const BMetaData&);
						BMetaData& operator=(const BMetaData&);
};

} // namespace media
} // namespace BPrivate

using namespace BPrivate::media;

#endif	// _META_DATA_H
