/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MetaData.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>

#define P BPrivate::media::

const char* P kCanPause			= "canpause";
const char* P kCanSeekBackward 	= "canseekbackward";
const char* P kCanSeekForward 	= "canseekforward";
const char* P kCanSeek			= "canseek";

const char* P kAudioBitRate		= "audiobitrate";
const char* P kVideoBitRate		= "videobitrate";
const char* P kAudioSampleRate	= "audiosamplerate";
const char* P kVideoFrameRate	= "videoframerate";

const char* P kMimeType			= "mime";
const char* P kAudioCodec		= "audiocodec";
const char* P kVideoCodec		= "videocodec";
const char* P kVideoHeight		= "videoheight";
const char* P kVideoWidth		= "videowidth";
const char* P kNumTracks		= "numtracks";
const char* P kDrmCrippled		= "drmcrippled";

const char* P kTitle			= "title";
const char* P kComment			= "comment";
const char* P kCopyright		= "copyright";
const char* P kAlbum			= "album";
const char* P kArtist			= "artist";
const char* P kAuthor			= "author";
const char* P kComposer			= "composer";
const char* P kGenre			= "genre";
const char* P kDuration			= "duration";
const char* P kRating			= "rating";
const char* P kCDTrackNum		= "cdtracknumber";
const char* P kCDTrackMax		= "cdtrackmax";


BMetaData::BMetaData()
	:
	fMessage(NULL)
{
	fMessage = new BMessage();
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
BMetaData::GetUInt32(const char* key, uint32* value) const
{
	return fMessage->FindUInt32(key, value) == B_OK ? true : false;
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


BMessage*
BMetaData::Message()
{
	return fMessage;
}
