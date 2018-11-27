/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MEDIA_FILE_H
#define	_MEDIA_FILE_H


#include <image.h>
#include <List.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <StorageDefs.h>


namespace BCodecKit {
	class BMediaExtractor;
	class BMediaStreamer;
	class BMediaWriter;
}

namespace BPrivate {
	class _AddonManager;
}


// forward declarations
class BMediaTrack;
class BMessage;
class BParameterWeb;
class BUrl;
class BView;


// flags for the BMediaFile constructor
enum {
	B_MEDIA_FILE_REPLACE_MODE    = 0x00000001,
	B_MEDIA_FILE_NO_READ_AHEAD   = 0x00000002,
	B_MEDIA_FILE_UNBUFFERED      = 0x00000006,
	B_MEDIA_FILE_BIG_BUFFERS     = 0x00000008
};

// BMediaFile represents a media file (AVI, Quicktime, MPEG, AIFF, WAV, etc)
//
// To read a file you construct a BMediaFile with an entry_ref, get the
// BMediaTracks out of it and use those to read the data.
//
// To write a file you construct a BMediaFile with an entry ref and an id as
// returned by get_next_file_format().   You then CreateTrack() to create
// various audio & video tracks.  Once you're done creating tracks, call
// CommitHeader(), then write data to each track and call CloseFile() when
// you're done.
//

class BMediaFile {
public:
	//	these four constructors are used for read-only access
								BMediaFile(const entry_ref* ref);
								BMediaFile(BDataIO* source);
									// BFile is a BDataIO
								BMediaFile(const entry_ref* ref, int32 flags);
								BMediaFile(BDataIO* source, int32 flags);

	//	these three constructors are for read-write access
								BMediaFile(const entry_ref* ref,
									const media_file_format* mfi,
									int32 flags = 0);
								BMediaFile(BDataIO* destination,
								   const media_file_format* mfi,
								   int32 flags = 0);
								BMediaFile(const media_file_format* mfi,
								   	int32 flags = 0);
									// set file later using SetTo()

	// Additional constructors used to stream data from protocols
	// supported by the Streamer API
								BMediaFile(const BUrl& url);
								BMediaFile(const BUrl& url, int32 flags);
	// Read-Write streaming constructor
								BMediaFile(const BUrl& destination,
								   const media_file_format* mfi,
								   int32 flags = 0);

	virtual						~BMediaFile();

			status_t			SetTo(const entry_ref* ref);
			status_t			SetTo(BDataIO* destination);
	// The streaming equivalent of SetTo
			status_t			SetTo(const BUrl& url);

			status_t			InitCheck() const;

	// Get info about the underlying file format.
			status_t			GetFileFormatInfo(
									media_file_format* mfi) const;

	// Returns in _data hierarchical meta-data about the stream.
	// The fields in the message shall follow a defined naming-scheme,
	// such that applications can find the same information in different
	// types of files.
			status_t			GetMetaData(BMessage* _data) const;

	//
	// These functions are for read-only access to a media file.
	// The data is read using the BMediaTrack object.
	//
			const char*			Copyright() const;
			int32				CountTracks() const;

	// Can be called multiple times with the same index.  You must call
	// ReleaseTrack() when you're done with a track.
			BMediaTrack*		TrackAt(int32 index);

	// Release the resource used by a given BMediaTrack object, to reduce
	// the memory usage of your application. The specific 'track' object
	// can no longer be used, but you can create another one by calling
	// TrackAt() with the same track index.
			status_t			ReleaseTrack(BMediaTrack* track);

	// A convenience. Deleting a BMediaFile will also call this.
			status_t			ReleaseAllTracks();


	// Create and add a track to the media file
			BMediaTrack*		CreateTrack(media_format* mf,
									const media_codec_info* mci,
									uint32 flags = 0);
	// Create and add a raw track to the media file (it has no encoder)
			BMediaTrack*		CreateTrack(media_format* mf,
									uint32 flags = 0);

	// Lets you set the copyright info for the entire file
			status_t			AddCopyright(const char* data);

	// Call this to add user-defined chunks to a file (if they're supported)
			status_t			AddChunk(int32 type, const void* data,
									size_t size);

	// After you have added all the tracks you want, call this
			status_t			CommitHeader();

	// After you have written all the data to the track objects, call this
			status_t			CloseFile();

	// This is for controlling file format parameters

	// returns a copy of the parameter web
			status_t			GetParameterWeb(BParameterWeb** outWeb);
			status_t 			GetParameterValue(int32 id,	void* value,
									size_t* size);
			status_t			SetParameterValue(int32 id,	const void* value,
									size_t size);
			BView*				GetParameterView();

	// For the future...
	virtual	status_t			Perform(int32 selector, void* data);

private:
	// deprecated, but for R5 compatibility
			BParameterWeb*		Web();

	// Does nothing, returns B_ERROR, for Zeta compatiblity only
			status_t			ControlFile(int32 selector, void* ioData,
									size_t size);

			BCodecKit::BMediaExtractor* fExtractor;
			int32				_reserved_BMediaFile_was_fExtractorID;
			int32				fTrackNum;
			status_t			fErr;

			BPrivate::_AddonManager* fEncoderMgr;
			BPrivate::_AddonManager* fWriterMgr;
			BCodecKit::BMediaWriter* fWriter;
			int32				fWriterID;
			media_file_format	fMFI;

			BCodecKit::BMediaStreamer* fStreamer;

			bool				fFileClosed;
			bool				fDeleteSource;
			bool				_reserved_was_fUnused[2];
			BMediaTrack**		fTrackList;

			void				_Init();
			void				_UnInit();
			void				_InitReader(BDataIO* source,
									const BUrl* url = NULL,
									int32 flags = 0);
			void				_InitWriter(BDataIO* target,
									const BUrl* url,
									const media_file_format* fileFormat,
									int32 flags);
			void				_InitStreamer(const BUrl& url,
									BDataIO** adapter);

								BMediaFile();
								BMediaFile(const BMediaFile&);
			BMediaFile&			operator=(const BMediaFile&);

			BDataIO*			fSource;

	// FBC data and virtuals

			uint32				_reserved_BMediaFile_[31];

	virtual	status_t			_Reserved_BMediaFile_0(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_1(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_2(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_3(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_4(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_5(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_6(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_7(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_8(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_9(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_10(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_11(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_12(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_13(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_14(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_15(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_16(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_17(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_18(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_19(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_20(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_21(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_22(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_23(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_24(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_25(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_26(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_27(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_28(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_29(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_30(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_31(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_32(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_33(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_34(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_35(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_36(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_37(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_38(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_39(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_40(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_41(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_42(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_43(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_44(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_45(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_46(int32 arg, ...);
	virtual	status_t			_Reserved_BMediaFile_47(int32 arg, ...);
};

#endif
