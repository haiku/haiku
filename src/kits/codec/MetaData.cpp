/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MetaData.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>


namespace BCodecKit {


const char* kCanPause			= "canpause";
const char* kCanSeekBackward 	= "canseekbackward";
const char* kCanSeekForward	 	= "canseekforward";
const char* kCanSeek			= "canseek";

const char* kAudioBitRate		= "audiobitrate";
const char* kVideoBitRate		= "videobitrate";
const char* kAudioSampleRate	= "audiosamplerate";
const char* kVideoFrameRate		= "videoframerate";

const char* kMimeType			= "mime";
const char* kAudioCodec			= "audiocodec";
const char* kVideoCodec			= "videocodec";
const char* kVideoHeight		= "videoheight";
const char* kVideoWidth			= "videowidth";
const char* kNumTracks			= "numtracks";
const char* kDrmCrippled		= "drmcrippled";

const char* kMediaType			= "mediatype";

const char* kChannelCount		= "channelcount";
const char* kAudioFormat		= "audioformat";
const char* kByteOrder			= "byteorder";
const char* kBufferSize			= "buffersize";
const char* kChannelMask		= "channelmask";

const char* kLineWidth			= "linewidth";
const char* kLineCount			= "linecount";
const char* kBytesPerRow		= "bytesperrow";
const char* kPixelOffset		= "pixeloffset";
const char* kLineOffset			= "lineoffset";
const char* kColorSpace			= "colorspace";
const char* kOrientation		= "orientation";

const char* kVideoFrameSize		= "orientation";

const char* kEncoding			= "encoding";

const char* kTitle				= "title";
const char* kComment			= "comment";
const char* kCopyright			= "copyright";
const char* kAlbum				= "album";
const char* kArtist				= "artist";
const char* kAuthor				= "author";
const char* kComposer			= "composer";
const char* kGenre				= "genre";
const char* kDuration			= "duration";
const char* kRating				= "rating";
const char* kCDTrackNum			= "cdtracknumber";
const char* kCDTrackMax			= "cdtrackmax";
const char* kDate				= "date";
const char* kEncodedBy			= "encoded_by";
const char* kLanguage			= "language";
const char* kAlbumArtist		= "album_artist";
const char* kPerformer			= "performer";
const char* kDisc				= "disc";
const char* kPublisher			= "publisher";
const char* kTrack				= "track";
const char* kEncoder			= "encoder";
const char* kYear				= "year";

const char* kChapter			= "be:chapter";
const char* kChapterStart	 	= "be:chapter:start";
const char* kChapterEnd			= "be:chapter:end";

const char* kProgramData 		= "be:program";


BMetaData::BMetaData()
	:
	fMessage(NULL)
{
	fMessage = new BMessage();
}


BMetaData::BMetaData(const BMetaData& data)
	:
	fMessage(NULL)
{
	fMessage = new BMessage(*data.fMessage);
}


BMetaData::BMetaData(const BMessage& msg)
	:
	fMessage(NULL)
{
	fMessage = new BMessage(msg);
}


BMetaData::~BMetaData()
{
	delete fMessage;
}


bool
BMetaData::SetString(const char* key, const BString& value)
{
	return fMessage->SetString(key, value) == B_OK ? true : false;
}


bool
BMetaData::SetBool(const char* key, bool value)
{
	return fMessage->SetBool(key, value) == B_OK ? true : false;
}


bool
BMetaData::SetUInt32(const char* key, uint32 value)
{
	return fMessage->SetUInt32(key, value) == B_OK ? true : false;
}


bool
BMetaData::SetUInt64(const char* key, uint64 value)
{
	return fMessage->SetUInt64(key, value) == B_OK ? true : false;
}


bool
BMetaData::SetFloat(const char* key, float value)
{
	return fMessage->SetFloat(key, value) == B_OK ? true : false;
}


bool
BMetaData::GetUInt32(const char* key, uint32* value) const
{
	return fMessage->FindUInt32(key, value) == B_OK ? true : false;
}


bool
BMetaData::GetFloat(const char* key, float* value) const
{
	return fMessage->FindFloat(key, value) == B_OK ? true : false;
}


bool
BMetaData::GetString(const char* key, BString* value) const
{
	return fMessage->FindString(key, value) == B_OK ? true : false;
}


bool
BMetaData::GetBool(const char* key, bool* value) const
{
	return fMessage->FindBool(key, value) == B_OK ? true : false;
}


bool
BMetaData::AddMetaData(const char* key, BMetaData* data)
{
	return fMessage->AddMessage(key, data->Message())
		== B_OK ? true : false;
}


bool
BMetaData::GetMetaData(const char* key, BMetaData* data, uint32 index)
{
	BMessage msg;
	status_t err = fMessage->FindMessage(key, index, &msg);
	if (err == B_OK) {
		*data = BMetaData(msg);
		return true;
	}
	return false;
}


bool
BMetaData::RemoveValue(const char* key)
{
	return fMessage->RemoveName(key) == B_OK ? true : false;
}


void
BMetaData::MakeEmpty()
{
	fMessage->MakeEmpty();
}


bool
BMetaData::IsEmpty()
{
	return fMessage->IsEmpty();
}


const BMessage*
BMetaData::Message()
{
	return fMessage;
}


BMetaData&
BMetaData::operator=(const BMetaData& other)
{
	delete fMessage;
	fMessage = new BMessage(*other.fMessage);
	return *this;
}


} // namespace BCodecKit
