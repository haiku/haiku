/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _META_DATA_H
#define _META_DATA_H

#include <Message.h>


namespace BPrivate {
namespace media {


// Playback capabilities
extern const char* kCanPause;			// bool
extern const char* kCanSeekBackward;	// bool
extern const char* kCanSeekForward;		// bool
extern const char* kCanSeek;			// bool

// Bitrates
extern const char* kAudioBitRate;		// uint32 (bps)
extern const char* kVideoBitRate;		// uint32 (bps)
extern const char* kAudioSampleRate;	// uint32 (hz)
extern const char* kVideoFrameRate;		// uint32 (hz)

// RFC2046 and RFC4281
extern const char* kMimeType;			// BString
extern const char* kAudioCodec;			// BString
extern const char* kVideoCodec;			// BString
extern const char* kVideoHeight;		// uint32
extern const char* kVideoWidth;			// uint32
extern const char* kNumTracks;			// uint32
extern const char* kDrmCrippled;		// bool

// General use attributes
extern const char* kTitle;				// BString
extern const char* kComment;			// BString
extern const char* kCopyright;			// BString
extern const char* kAlbum;				// BString
extern const char* kArtist;				// BString
extern const char* kAuthor;				// BString
extern const char* kComposer;			// BString
extern const char* kGenre;				// BString
// TODO: what we would use for encoded dates?
// Date,			// date
extern const char* kDuration;			// uint32 (ms)
extern const char* kRating;				// BString
// TODO: BBitmap? uint8 array?
//AlbumArt,
extern const char* kCDTrackNum;			// uint32
extern const char* kCDTrackMax;			// uint32


class BMetaData {
public:
						BMetaData();
						BMetaData(const BMessage& msg);

						~BMetaData();

	// Woah. It seems we need BValue there.
	bool				SetString(const char* key, const BString& value);
	bool				SetBool(const char* key, bool value);
	bool				SetUInt32(const char* key, uint32 value);

	bool				GetString(const char* key, BString* value) const;
	bool				GetBool(const char* key, bool* value) const;
    bool				GetUInt32(const char* key, uint32* value) const;

	bool				RemoveValue(const char* key);

	// Clean up all keys
	void 				MakeEmpty();
	bool				IsEmpty();

	// Retain ownership of the object, be careful with that
	// that's why we need to introduce smart pointers!
	BMessage*			Message();

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
