/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaFile.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFile.h>
#include <MediaTrack.h>
#include "debug.h"


/*************************************************************
 * public BMediaFile
 *************************************************************/

BMediaFile::BMediaFile(const entry_ref *ref)
{
	UNIMPLEMENTED();
}

BMediaFile::BMediaFile(BDataIO * source)
{
	UNIMPLEMENTED();
}

BMediaFile::BMediaFile(const entry_ref * ref,
					   int32 flags)
{
	UNIMPLEMENTED();
}

BMediaFile::BMediaFile(BDataIO * source,
					   int32 flags)
{
	UNIMPLEMENTED();
}

BMediaFile::BMediaFile(const entry_ref *ref,
					   const media_file_format * mfi,
					   int32 flags)
{
	UNIMPLEMENTED();
}
					   
BMediaFile::BMediaFile(BDataIO	*destination,
					   const media_file_format * mfi,
					   int32 flags)
{
	UNIMPLEMENTED();
}


BMediaFile::BMediaFile(const media_file_format * mfi,
					   int32 flags)
{
	UNIMPLEMENTED();
}

/* virtual */
BMediaFile::~BMediaFile()
{
	UNIMPLEMENTED();
}

status_t 
BMediaFile::InitCheck() const
{
	UNIMPLEMENTED();

	return B_OK;
}

// Get info about the underlying file format.
status_t 
BMediaFile::GetFileFormatInfo(media_file_format *mfi) const
{
	UNIMPLEMENTED();
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
	UNIMPLEMENTED();
	return 1;
}


// Can be called multiple times with the same index.  You must call
// ReleaseTrack() when you're done with a track.
BMediaTrack	*
BMediaFile::TrackAt(int32 index)
{
	UNIMPLEMENTED();
	return new BMediaTrack(0, 0);
}


// Release the resource used by a given BMediaTrack object, to reduce
// the memory usage of your application. The specific 'track' object
// can no longer be used, but you can create another one by calling
// TrackAt() with the same track index.
status_t
BMediaFile::ReleaseTrack(BMediaTrack *track)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaFile::ReleaseAllTracks(void)
{
	UNIMPLEMENTED();
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
	UNIMPLEMENTED();
}

void 
BMediaFile::InitReader(BDataIO *source, int32 flags /* = 0 */)
{
	UNIMPLEMENTED();
}

void
BMediaFile::InitWriter(BDataIO *source, const media_file_format * mfi,  int32 flags)
{
	UNIMPLEMENTED();
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

