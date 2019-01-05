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
extern const char* kAudioSampleRate;	// float (hz)
extern const char* kVideoFrameRate;		// float (hz)

// RFC2046 and RFC4281
extern const char* kMimeType;			// BString
extern const char* kAudioCodec;			// BString
extern const char* kVideoCodec;			// BString
extern const char* kVideoHeight;		// uint32
extern const char* kVideoWidth;			// uint32
extern const char* kNumTracks;			// uint32
extern const char* kDrmCrippled;		// bool

// Stuff needed to fully describe the BMediaFormat
// TODO: Shouldn't we just use MIME types for it?
extern const char* kMediaType;			// media_type
// Audio stuff
extern const char* kChannelCount;		// uint32
extern const char* kAudioFormat;		// uint32
extern const char* kByteOrder;			// uint32
extern const char* kBufferSize;			// size_t
// Multiaudio
extern const char* kChannelMask;		// size_t

// This is also BMediaFormat stuff, but mostly video
// NOTE: video width/height are defined as per RFC mentioned above
extern const char* kBytesPerRow;		// uint32
extern const char* kPixelOffset;		// uint32
extern const char* kLineOffset;			// uint32
extern const char* kColorSpace;			// color_space
extern const char* kOrientation;		// uint32
extern const char* kVideoFrameSize;		// uint32

extern const char* kEncoding;			// uint32

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
extern const char* kChapterStart;		// uint64
extern const char* kChapterEnd;			// uint64

// Others
extern const char* kProgramData;		// BMetaData

// TODO: Fully honour this:
// https://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata


/**
 * @brief BMetaData stores the media metadata.
 *
 * The metadata model is sparse and each key can occur at most once,
 * except for BMetaData itself.
 * The key is a const char and the value is the actual metadata.
 * The client of this class is required to know in advance the type
 * of a particular metadata key.
 */
class BMetaData {
public:
						BMetaData();
						BMetaData(const BMessage& msg);
	virtual				~BMetaData();

	// Woah. It seems we need BValue there.
	bool				SetString(const char* key, const BString& value);
	bool				SetBool(const char* key, bool value);
	bool				SetUInt32(const char* key, uint32 value);
	bool				SetUInt64(const char* key, uint64 value);
	bool				SetFloat(const char* key, float value);

	bool				GetString(const char* key, BString* value) const;
	bool				GetBool(const char* key, bool* value) const;
	bool				GetUInt32(const char* key, uint32* value) const;
	bool				GetUInt64(const char* key, uint64* value) const;
	bool				GetFloat(const char* key, float* value) const;

	bool				HasString(const char* key) const;
	bool				HasBool(const char* key) const;
	bool				HasUInt32(const char* key) const;
	bool				HasUInt64(const char* key) const;
	bool				HasFloat(const char* key) const;

	// We allow embedding BMetaData into BMetaData. The BMetaData field
	// is the only one allowed to have more occurrences for a single key
	// so an index attribute is given for retrieving it.
	bool				AddMetaData(const char* key, BMetaData* data);
	bool				GetMetaData(const char* key, BMetaData* data,
							uint32 index = 0);
	bool				HasMetaData(const char* key, uint32 index = 0);
	bool				RemoveMetaData(const char* key, uint32 index = 0);

	bool				RemoveValue(const char* key);

	// Clean up all keys
	void 				MakeEmpty();
	bool				IsEmpty();

	// Retain ownership of the object, be careful with that
	// that's why we need to introduce smart pointers!
	const BMessage*		Message();

	BMetaData&			operator=(const BMetaData& other);
						BMetaData(const BMetaData&);

private:
	// TODO: padding
	BMessage*			fMessage;


};


} // namespace BCodecKit


#endif	// _META_DATA_H
