#ifndef _MOV_FILE_READER_H
#define _MOV_FILE_READER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include "MOVParser.h"

// QT MOV file reader (and maybe MP4 as well)
class MOVFileReader {
private:
	// Atom List
	AtomArray atomChildren;
	uint32	TotalChildren;
	off_t	StreamSize;
	BPositionIO	*theStream;

	AtomBase *GetChildAtom(uint32 patomType, uint32 offset=0);
	uint32	CountChildAtoms(uint32 patomType);
	
	uint32		getMovieTimeScale();

	// Add a atom to the children array (return false on failure)
	bool	AddChild(AtomBase *pChildAtom);

	AudioMetaData	theAudio;
	VideoMetaData	theVideo;
	mov_stream_header theStreamHeader;
	mov_main_header theMainHeader;	
	
	MVHDAtom	*theMVHDAtom;

public:
			MOVFileReader(BPositionIO *pStream);
	virtual	~MOVFileReader();

	// getter for MVHDAtom
	MVHDAtom	*getMVHDAtom();

	bool	IsEndOfFile(off_t pPosition);
	bool	IsEndOfFile();
	
	// Is this a quicktime file
	static bool	IsSupported(BPositionIO *source);
	
	// How many tracks in file
	uint32		getStreamCount();
	
	// Duration defined 
	bigtime_t	getMovieDuration();
	
	// The first video track duration
	bigtime_t	getVideoDuration();
	// the first audio track duration
	bigtime_t	getAudioDuration();
	// the max of the audio or video durations
	bigtime_t	getMaxDuration();

	// The no of frames in the first video track
	uint32		getVideoFrameCount();
	// The no of frames in the first audio track
	uint32		getAudioFrameCount();

	// Is stream (track) a video track
	bool		IsVideo(uint32 stream_index);
	// Is stream (track) a audio track
	bool		IsAudio(uint32 stream_index);

	uint32	getNoFramesInChunk(uint32 stream_index, uint32 pFrameNo);

	uint64	getOffsetForFrame(uint32 stream_index, uint32 pFrameNo);
	uint32	getChunkSize(uint32 stream_index, uint32 pFrameNo);
	bool	IsKeyFrame(uint32 stream_index, uint32 pFrameNo);

	status_t	ParseFile();
	BPositionIO *Source() {return theStream;};

	bool	GetNextChunkInfo(uint32 stream_index, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe);
	
	// Return all Audio Meta Data
	const 	AudioMetaData 		*AudioFormat(uint32 stream_index, size_t *size = 0);
	// Return all Video Meta Data
	const 	VideoMetaData		*VideoFormat(uint32 stream_index);
	
	// XXX these need work
	const 	mov_main_header		*MovMainHeader();
	const 	mov_stream_header	*StreamFormat(uint32 stream_index);
};

#endif
