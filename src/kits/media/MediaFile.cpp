/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaFile.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFile.h>
#include <MediaTrack.h>
#include <File.h>
#include <string.h>
#include "MediaExtractor.h"
#include "debug.h"


/*************************************************************
 * public BMediaFile
 *************************************************************/

BMediaFile::BMediaFile(const entry_ref *ref)
{
	CALLED();
	Init();
	fDeleteSource = true;
	InitReader(new BFile(ref, O_RDONLY));
}

BMediaFile::BMediaFile(BDataIO * source)
{
	CALLED();
	Init();
	InitReader(source);
}

BMediaFile::BMediaFile(const entry_ref * ref,
					   int32 flags)
{
	CALLED();
	Init();
	fDeleteSource = true;
	InitReader(new BFile(ref, O_RDONLY), flags);
}

BMediaFile::BMediaFile(BDataIO * source,
					   int32 flags)
{
	CALLED();
	Init();
	InitReader(source, flags);
}

BMediaFile::BMediaFile(const entry_ref *ref,
					   const media_file_format * mfi,
					   int32 flags)
{
	CALLED();
	Init();
	fDeleteSource = true;
	InitWriter(new BFile(ref, O_WRONLY), mfi, flags);
}
					   
BMediaFile::BMediaFile(BDataIO	*destination,
					   const media_file_format * mfi,
					   int32 flags)
{
	CALLED();
	Init();
	InitWriter(destination, mfi, flags);
}

/* virtual */
BMediaFile::~BMediaFile()
{
	CALLED();
	
	ReleaseAllTracks();
	delete fTrackList;
	delete fExtractor;
	if (fDeleteSource)
		delete fSource;
}

status_t 
BMediaFile::InitCheck() const
{
	CALLED();
	return fErr;
}

// Get info about the underlying file format.
status_t 
BMediaFile::GetFileFormatInfo(media_file_format *mfi) const
{
	CALLED();
	if (fErr)
		return B_ERROR;
	*mfi = fMFI;
	return B_OK;
}

const char *
BMediaFile::Copyright(void) const
{
	UNIMPLEMENTED();
	return "";
}

int32
BMediaFile::CountTracks() const
{
	return fTrackNum;
}


// Can be called multiple times with the same index.  You must call
// ReleaseTrack() when you're done with a track.
BMediaTrack	*
BMediaFile::TrackAt(int32 index)
{
	CALLED();
	if (!fTrackList || !fExtractor || index < 0 || index >= fTrackNum)
		return 0;
	if (!fTrackList[index]) {
		TRACE("BMediaFile::TrackAt, creating new track for index %ld\n", index);
		fTrackList[index] = new BMediaTrack(fExtractor, index);
		TRACE("BMediaFile::TrackAt, new track is %p\n", fTrackList[index]);
	}
	return fTrackList[index];
}


// Release the resource used by a given BMediaTrack object, to reduce
// the memory usage of your application. The specific 'track' object
// can no longer be used, but you can create another one by calling
// TrackAt() with the same track index.
status_t
BMediaFile::ReleaseTrack(BMediaTrack *track)
{
	CALLED();
	if (!fTrackList || !track)
		return B_ERROR;
	for (int32 i = 0; i < fTrackNum; i++) {
		if (fTrackList[i] == track) {
			TRACE("BMediaFile::ReleaseTrack, releasing track %p with index %ld\n", track, i);
			delete track;
			fTrackList[i] = 0;
			return B_OK;
		}
	}
	printf("BMediaFile::ReleaseTrack track %p not found\n", track);
	return B_ERROR;
}

status_t 
BMediaFile::ReleaseAllTracks(void)
{
	CALLED();
	if (!fTrackList)
		return B_ERROR;
	for (int32 i = 0; i < fTrackNum; i++) {
		if (fTrackList[i]) {
			TRACE("BMediaFile::ReleaseAllTracks, releasing track %p with index %ld\n", fTrackList[i], i);
			delete fTrackList[i];
			fTrackList[i] = 0;
		}
	}
	return B_OK;
}

// Create and add a track to the media file
BMediaTrack	*
BMediaFile::CreateTrack(media_format *mf, 
						const media_codec_info *mci)
{
	UNIMPLEMENTED();
	return 0;
}

// Create and add a raw track to the media file (it has no encoder)
BMediaTrack *
BMediaFile::CreateTrack(media_format *mf)
{
	UNIMPLEMENTED();
	return 0;
}


// Lets you set the copyright info for the entire file
status_t
BMediaFile::AddCopyright(const char *data)
{
	UNIMPLEMENTED();
	return B_OK;
}


// Call this to add user-defined chunks to a file (if they're supported)
status_t
BMediaFile::AddChunk(int32 type, const void *data, size_t size)
{
	UNIMPLEMENTED();
	return B_OK;
}


// After you have added all the tracks you want, call this
status_t
BMediaFile::CommitHeader(void)
{
	UNIMPLEMENTED();
	return B_OK;
}


// After you have written all the data to the track objects, call this
status_t
BMediaFile::CloseFile(void)
{
	UNIMPLEMENTED();
	return B_OK;
}


// This is for controlling file format parameters
BParameterWeb *
BMediaFile::Web()
{
	UNIMPLEMENTED();
	return 0;
}

status_t
BMediaFile::GetParameterValue(int32 id,	void *valu, size_t *size)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t
BMediaFile::SetParameterValue(int32 id,	const void *valu, size_t size)
{
	UNIMPLEMENTED();
	return B_OK;
}


BView *
BMediaFile::GetParameterView()
{
	UNIMPLEMENTED();
	return 0;
}

/* virtual */ status_t 
BMediaFile::Perform(int32 selector, void * data)
{
	UNIMPLEMENTED();
	return B_OK;
}


/*************************************************************
 * private BMediaFile
 *************************************************************/

void 
BMediaFile::Init()
{
	CALLED();
	fSource = 0;
	fTrackNum = 0;
	fTrackList = 0;
	fExtractor = 0;
	fErr = B_OK;
	fDeleteSource = false;

	// not used so far:
	fEncoderMgr = 0;
	fWriterMgr = 0;
	fWriter = 0;
	fWriterID = 0;
	fFileClosed = 0;
}

void 
BMediaFile::InitReader(BDataIO *source, int32 flags)
{
	CALLED();
	
	fSource = source;
	
	fExtractor = new MediaExtractor(source, flags);
	fErr = fExtractor->InitCheck();
	if (fErr)
		return;
		
	fExtractor->GetFileFormatInfo(&fMFI);
	fTrackNum = fExtractor->StreamCount();
	fTrackList = new BMediaTrack *[fTrackNum];
	memset(fTrackList, 0, fTrackNum * sizeof(BMediaTrack *));
}

void
BMediaFile::InitWriter(BDataIO *source, const media_file_format * mfi,  int32 flags)
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

