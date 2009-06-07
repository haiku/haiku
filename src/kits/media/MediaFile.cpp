/*
 * Copyright (c) 2002-2004, Marcus Overhagen <marcus@overhagen.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <MediaFile.h>

#include <new>
#include <string.h>

#include <MediaTrack.h>
#include <File.h>

#include "MediaExtractor.h"
#include "debug.h"


BMediaFile::BMediaFile(const entry_ref* ref)
{
	CALLED();
	_Init();
	fDeleteSource = true;
	_InitReader(new (std::nothrow) BFile(ref, O_RDONLY));
}


BMediaFile::BMediaFile(BDataIO* source)
{
	CALLED();
	_Init();
	_InitReader(source);
}


BMediaFile::BMediaFile(const entry_ref* ref, int32 flags)
{
	CALLED();
	_Init();
	fDeleteSource = true;
	_InitReader(new (std::nothrow) BFile(ref, O_RDONLY), flags);
}


BMediaFile::BMediaFile(BDataIO* source, int32 flags)
{
	CALLED();
	_Init();
	_InitReader(source, flags);
}


BMediaFile::BMediaFile(const entry_ref* ref, const media_file_format* mfi,
	int32 flags)
{
	CALLED();
	_Init();
	fDeleteSource = true;
	_InitWriter(new (std::nothrow) BFile(ref, O_WRONLY), mfi, flags);
}


BMediaFile::BMediaFile(BDataIO* destination, const media_file_format* mfi,
	int32 flags)
{
	CALLED();
	_Init();
	_InitWriter(destination, mfi, flags);
}


// File will be set later by SetTo()
BMediaFile::BMediaFile(const media_file_format* mfi, int32 flags)
{
	debugger("BMediaFile::BMediaFile not implemented");
}


status_t
BMediaFile::SetTo(const entry_ref* ref)
{
	CALLED();

	if (ref == NULL)
		return B_BAD_VALUE;

	_UnInit();
	fDeleteSource = true;
	_InitReader(new (std::nothrow) BFile(ref, O_RDONLY));

	return fErr;
}


status_t
BMediaFile::SetTo(BDataIO* destination)
{
	CALLED();

	if (destination == NULL)
		return B_BAD_VALUE;

	_UnInit();
	_InitReader(destination);

	return fErr;
}


BMediaFile::~BMediaFile()
{
	CALLED();

	_UnInit();
}


status_t
BMediaFile::InitCheck() const
{
	CALLED();
	return fErr;
}


// Get info about the underlying file format.
status_t
BMediaFile::GetFileFormatInfo(media_file_format* mfi) const
{
	CALLED();
	if (mfi == NULL)
		return B_BAD_VALUE;
	if (fErr)
		return B_ERROR;
	*mfi = fMFI;
	return B_OK;
}


const char*
BMediaFile::Copyright() const
{
	return fExtractor->Copyright();
}


int32
BMediaFile::CountTracks() const
{
	return fTrackNum;
}


// Can be called multiple times with the same index.  You must call
// ReleaseTrack() when you're done with a track.
BMediaTrack*
BMediaFile::TrackAt(int32 index)
{
	CALLED();
	if (fTrackList == NULL || fExtractor == NULL
		|| index < 0 || index >= fTrackNum) {
		return NULL;
	}
	if (fTrackList[index] == NULL) {
		TRACE("BMediaFile::TrackAt, creating new track for index %ld\n", index);
		fTrackList[index] = new (std::nothrow) BMediaTrack(fExtractor, index);
		TRACE("BMediaFile::TrackAt, new track is %p\n", fTrackList[index]);
	}
	return fTrackList[index];
}


// Release the resource used by a given BMediaTrack object, to reduce
// the memory usage of your application. The specific 'track' object
// can no longer be used, but you can create another one by calling
// TrackAt() with the same track index.
status_t
BMediaFile::ReleaseTrack(BMediaTrack* track)
{
	CALLED();
	if (!fTrackList || !track)
		return B_ERROR;
	for (int32 i = 0; i < fTrackNum; i++) {
		if (fTrackList[i] == track) {
			TRACE("BMediaFile::ReleaseTrack, releasing track %p with index "
				"%ld\n", track, i);
			delete track;
			fTrackList[i] = NULL;
			return B_OK;
		}
	}
	fprintf(stderr, "BMediaFile::ReleaseTrack track %p not found\n", track);
	return B_ERROR;
}


status_t
BMediaFile::ReleaseAllTracks()
{
	CALLED();
	if (!fTrackList)
		return B_ERROR;
	for (int32 i = 0; i < fTrackNum; i++) {
		if (fTrackList[i]) {
			TRACE("BMediaFile::ReleaseAllTracks, releasing track %p with "
				"index %ld\n", fTrackList[i], i);
			delete fTrackList[i];
			fTrackList[i] = NULL;
		}
	}
	return B_OK;
}


// Create and add a track to the media file
BMediaTrack*
BMediaFile::CreateTrack(media_format* mf, const media_codec_info* mci,
	uint32 flags)
{
	UNIMPLEMENTED();
	return 0;
}


// Create and add a raw track to the media file (it has no encoder)
BMediaTrack*
BMediaFile::CreateTrack(media_format* mf, uint32 flags)
{
	return CreateTrack(mf, NULL, flags);
}


// For BeOS R5 compatibility
extern "C" BMediaTrack*
CreateTrack__10BMediaFileP12media_formatPC16media_codec_info(
	BMediaFile* self, media_format* mf, const media_codec_info* mci);
BMediaTrack*
CreateTrack__10BMediaFileP12media_formatPC16media_codec_info(BMediaFile* self,
	media_format* mf, const media_codec_info* mci)
{
	return self->CreateTrack(mf, mci, 0);
}


// For BeOS R5 compatibility
extern "C" BMediaTrack* CreateTrack__10BMediaFileP12media_format(
	BMediaFile* self, media_format* mf);
BMediaTrack*
CreateTrack__10BMediaFileP12media_format(BMediaFile* self, media_format* mf)
{
	return self->CreateTrack(mf, NULL, 0);
}


// Lets you set the copyright info for the entire file
status_t
BMediaFile::AddCopyright(const char* data)
{
	UNIMPLEMENTED();
	return B_OK;
}


// Call this to add user-defined chunks to a file (if they're supported)
status_t
BMediaFile::AddChunk(int32 type, const void* data, size_t size)
{
	UNIMPLEMENTED();
	return B_OK;
}


// After you have added all the tracks you want, call this
status_t
BMediaFile::CommitHeader()
{
	UNIMPLEMENTED();
	return B_OK;
}


// After you have written all the data to the track objects, call this
status_t
BMediaFile::CloseFile()
{
	UNIMPLEMENTED();
	return B_OK;
}

// This is for controlling file format parameters

// returns a copy of the parameter web
status_t
BMediaFile::GetParameterWeb(BParameterWeb** outWeb)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


// deprecated BeOS R5 API
BParameterWeb*
BMediaFile::Web()
{
	UNIMPLEMENTED();
	return 0;
}


status_t
BMediaFile::GetParameterValue(int32 id,	void* value, size_t* size)
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t
BMediaFile::SetParameterValue(int32 id,	const void* value, size_t size)
{
	UNIMPLEMENTED();
	return B_OK;
}


BView*
BMediaFile::GetParameterView()
{
	UNIMPLEMENTED();
	return 0;
}


status_t
BMediaFile::Perform(int32 selector, void* data)
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t
BMediaFile::ControlFile(int32 selector, void* ioData, size_t size)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


// #pragma mark - private


void
BMediaFile::_Init()
{
	CALLED();

	fSource = NULL;
	fTrackNum = 0;
	fTrackList = NULL;
	fExtractor = NULL;
	fErr = B_OK;
	fDeleteSource = false;

	// not used so far:
	fEncoderMgr = NULL;
	fWriterMgr = NULL;
	fWriter = NULL;
	fWriterID = 0;
	fFileClosed = false;
}


void
BMediaFile::_UnInit()
{
	ReleaseAllTracks();
	delete[] fTrackList;
	fTrackList = NULL;
	fTrackNum = 0;
	delete fExtractor;
	fExtractor = NULL;
	if (fDeleteSource) {
		delete fSource;
		fSource = NULL;
		fDeleteSource = false;
	}
}


void
BMediaFile::_InitReader(BDataIO* source, int32 flags)
{
	CALLED();

	if (source == NULL)
		return;

	fSource = source;

	fExtractor = new (std::nothrow) MediaExtractor(source, flags);
	if (fExtractor == NULL)
		fErr = B_NO_MEMORY;
	else
		fErr = fExtractor->InitCheck();
	if (fErr != B_OK)
		return;

	fExtractor->GetFileFormatInfo(&fMFI);
	fTrackNum = fExtractor->StreamCount();
	fTrackList = new (std::nothrow) BMediaTrack*[fTrackNum];
	if (fTrackList == NULL) {
		fErr = B_NO_MEMORY;
		return;
	}
	memset(fTrackList, 0, fTrackNum * sizeof(BMediaTrack*));
}


void
BMediaFile::_InitWriter(BDataIO* source, const media_file_format* mfi,
	int32 flags)
{
	UNIMPLEMENTED();
	fSource = source;
	fErr = B_NOT_ALLOWED;
}


/*
//unimplemented
BMediaFile::BMediaFile();
BMediaFile::BMediaFile(const BMediaFile&);
 BMediaFile::BMediaFile& operator=(const BMediaFile&);
*/

status_t BMediaFile::_Reserved_BMediaFile_0(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_1(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_2(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_3(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_4(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_5(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_6(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_7(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_8(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_9(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_10(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_11(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_12(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_13(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_14(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_15(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_16(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_17(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_18(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_19(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_20(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_21(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_22(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_23(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_24(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_25(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_26(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_27(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_28(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_29(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_30(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_31(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_32(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_33(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_34(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_35(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_36(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_37(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_38(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_39(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_40(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_41(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_42(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_43(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_44(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_45(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_46(int32 arg, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_47(int32 arg, ...) { return B_ERROR; }

