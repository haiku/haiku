#ifndef _MEDIA_FILE_H
#define	_MEDIA_FILE_H

#include <kernel/image.h>
#include <SupportDefs.h>
#include <StorageDefs.h>
#include <List.h>
#include <MediaDefs.h>
#include <MediaFormats.h>


namespace BPrivate {
	class MediaExtractor;
	class _IMPEXP_MEDIA MediaWriter;
	class _AddonManager;
}


// forward declarations 
class BMediaTrack;
class BParameterWeb;
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
					BMediaFile(	const entry_ref *ref); 
					BMediaFile(	BDataIO * source);     // BFile is a BDataIO
					BMediaFile(	const entry_ref * ref,
								int32 flags);
					BMediaFile(	BDataIO * source,
								int32 flags);     // BFile is a BDataIO

					//	these two constructors are for read-write access
					BMediaFile(const entry_ref *ref,   // these two are write-only
							   const media_file_format * mfi,
							   int32 flags=0);
					BMediaFile(BDataIO	*destination,  // BFile is a BDataIO
							   const media_file_format * mfi,
							   int32 flags=0);

/* modified by Marcus Overhagen */
BMediaFile(const media_file_format * mfi,
					   int32 flags);
/* modified by Marcus Overhagen */
					   
					   
	virtual			~BMediaFile();

	status_t		InitCheck() const;

	// Get info about the underlying file format.
	status_t		GetFileFormatInfo(media_file_format *mfi) const;

	//
	// These functions are for read-only access to a media file.  
	// The data is read using the BMediaTrack object.
	//
	const char 		*Copyright(void) const;
	int32			CountTracks() const;

	// Can be called multiple times with the same index.  You must call
	// ReleaseTrack() when you're done with a track.
	BMediaTrack 	*TrackAt(int32 index);

	// Release the resource used by a given BMediaTrack object, to reduce
	// the memory usage of your application. The specific 'track' object
	// can no longer be used, but you can create another one by calling
	// TrackAt() with the same track index.
	status_t		ReleaseTrack(BMediaTrack *track);

	// A convenience.
	status_t		ReleaseAllTracks(void);


	// Create and add a track to the media file
	BMediaTrack 	*CreateTrack(media_format *mf, const media_codec_info *mci);
	// Create and add a raw track to the media file (it has no encoder)
	BMediaTrack 	*CreateTrack(media_format *mf);

	// Lets you set the copyright info for the entire file
	status_t		AddCopyright(const char *data);

	// Call this to add user-defined chunks to a file (if they're supported)
	status_t		AddChunk(int32 type, const void *data, size_t size);

	// After you have added all the tracks you want, call this
	status_t		CommitHeader(void);

	// After you have written all the data to the track objects, call this
	status_t        CloseFile(void);

	// This is for controlling file format parameters
	BParameterWeb	*Web();
	status_t 		GetParameterValue(int32 id,	void *valu, size_t *size);
	status_t		SetParameterValue(int32 id,	const void *valu, size_t size);
	BView			*GetParameterView();


	// For the future...
	virtual	status_t Perform(int32 selector, void * data);

private:
	BPrivate::MediaExtractor *fExtractor;
	int32					_reserved_BMediaFile_was_fExtractorID;
	int32					fTrackNum;
	status_t				fErr;

	BPrivate::_AddonManager *fEncoderMgr;
	BPrivate::_AddonManager *fWriterMgr;
	BPrivate::MediaWriter	*fWriter;
	int32					fWriterID;
	media_file_format		fMFI;

	bool					fFileClosed;
	bool					_reserved_was_fUnused[3];
	BList					*fTrackList;

	void					Init();
	void					InitReader(BDataIO *source, int32 flags = 0);
	void					InitWriter(BDataIO *source, const media_file_format * mfi,
									   int32 flags);

	BMediaFile();
	BMediaFile(const BMediaFile&);
	BMediaFile& operator=(const BMediaFile&);

	BFile					*fFile;


	/* fbc data and virtuals */

	uint32 _reserved_BMediaFile_[32];

virtual	status_t _Reserved_BMediaFile_0(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_1(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_2(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_3(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_4(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_5(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_6(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_7(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_8(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_9(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_10(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_11(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_12(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_13(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_14(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_15(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_16(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_17(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_18(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_19(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_20(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_21(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_22(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_23(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_24(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_25(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_26(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_27(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_28(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_29(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_30(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_31(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_32(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_33(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_34(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_35(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_36(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_37(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_38(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_39(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_40(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_41(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_42(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_43(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_44(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_45(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_46(int32 arg, ...);
virtual	status_t _Reserved_BMediaFile_47(int32 arg, ...);
};

#endif
