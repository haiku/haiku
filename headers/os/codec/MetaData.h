/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _META_DATA_H
#define _META_DATA_H


#include <Message.h>


namespace BCodecKit {


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
extern const char* kAlbumArtist;		// BString
extern const char* kAuthor;				// BString
extern const char* kPerformer;			// BString
extern const char* kComposer;			// BString
extern const char* kGenre;				// BString
extern const char* kYear;				// BString
extern const char* kEncodedBy;			// BString
extern const char* kLanguage;			// BString
extern const char* kDisc;				// BString
extern const char* kPublisher;			// BString
extern const char* kEncoder;			// BString
extern const char* kTrack;
extern const char* kDate;				// BString
extern const char* kDuration;			// uint32 (ms)
extern const char* kRating;				// BString
// TODO: BBitmap? uint8 array? url?
//extern const char* kAlbumArt
extern const char* kCDTrackNum;			// uint32
extern const char* kCDTrackMax;			// uint32

extern const char* kChapter;			// BMetaData
extern const char* kChapterStart;		// int64
extern const char* kChapterEnd;			// int64

// Others
extern const char* kProgramData;		// BMetaData


class BMetaData {
public:
						BMetaData();
						BMetaData(const BMessage& msg);
	virtual				~BMetaData();

	// Woah. It seems we need BValue there.
	bool				SetString(const char* key, const BString& value);
	bool				SetBool(const char* key, bool value);
	bool				SetUInt32(const char* key, uint32 value);
	bool				SetInt32(const char* key, int32 value);
	bool				SetInt64(const char* key, int64 value);

	bool				GetString(const char* key, BString* value) const;
	bool				GetBool(const char* key, bool* value) const;
    bool				GetUInt32(const char* key, uint32* value) const;
    bool				GetInt32(const char* key, int32* value) const;
	bool				GetInt64(const char* key, int64* value) const;

	// We allow embedding BMetaData into BMetaData. The BMetaData field
	// is the only one allowed to have more occurrences for a single key
	// so an index attribute is given for retrieving it.
	bool				AddMetaData(const char* key, BMetaData* data);
	bool				GetMetaData(const char* key, BMetaData* data,
							uint32 index = 0);

	bool				RemoveValue(const char* key);

	// Clean up all keys
	void 				MakeEmpty();
	bool				IsEmpty();

	// Retain ownership of the object, be careful with that
	// that's why we need to introduce smart pointers!
	const BMessage*		Message();

	BMetaData&			operator=(const BMetaData& other);

private:
	// TODO: padding
	BMessage*			fMessage;

						BMetaData(const BMetaData&);
};


} // namespace BCodecKit


#endif	// _META_DATA_H
