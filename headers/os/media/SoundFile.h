/******************************************************************************

	File: SoundFile.h

	Description:	Interface for a format-insensitive sound file object.

	Copyright 1995-97, Be Incorporated

******************************************************************************/


#ifndef _SOUND_FILE_H
#define _SOUND_FILE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <MediaDefs.h>
#include <Entry.h>
#include <File.h>

enum  /* sound_format*/
{ B_UNKNOWN_FILE, 
  B_AIFF_FILE, 
  B_WAVE_FILE, 
  B_UNIX_FILE };

class BSoundFile  {

public:
					BSoundFile();
					BSoundFile(const entry_ref *ref,
							   uint32 open_mode);
	virtual			~BSoundFile();

	status_t		InitCheck() const;

	status_t		SetTo(const entry_ref *ref, uint32 open_mode);

	int32			FileFormat() const;
	int32			SamplingRate() const;
	int32			CountChannels() const;
	int32			SampleSize() const;
	int32			ByteOrder() const;
	int32			SampleFormat() const;
	int32			FrameSize() const;
	off_t			CountFrames() const;

	bool			IsCompressed() const;
	int32			CompressionType() const;
	char	 		*CompressionName() const;

	virtual int32 	SetFileFormat(int32 format);
	virtual int32	SetSamplingRate(int32 fps);
	virtual int32	SetChannelCount(int32 spf);
	virtual int32	SetSampleSize(int32 bps);
	virtual int32	SetByteOrder(int32 bord);
	virtual int32	SetSampleFormat(int32 fmt);
	virtual int32	SetCompressionType(int32 type);
	virtual char   	*SetCompressionName(char *name);
	virtual bool	SetIsCompressed(bool tf);
	virtual	off_t	SetDataLocation(off_t offset);
	virtual off_t	SetFrameCount(off_t count);
	
	size_t			ReadFrames(char *buf,  size_t count);
	size_t			WriteFrames(char *buf, size_t count);
	virtual off_t	SeekToFrame(off_t n);
	off_t 			FrameIndex() const;
	off_t			FramesRemaining() const;

	BFile			*fSoundFile;

private:

virtual	void		_ReservedSoundFile1();
virtual	void		_ReservedSoundFile2();
virtual	void		_ReservedSoundFile3();

	int32			fFileFormat;
	int32			fSamplingRate;
	int32 			fChannelCount;
	int32			fSampleSize;
	int32			fByteOrder;
	int32			fSampleFormat;
	
	off_t			fByteOffset;	/* offset to first sample */

	off_t			fFrameCount;
	off_t			fFrameIndex;

	bool			fIsCompressed;
	int32			fCompressionType;
	char			*fCompressionName;
	status_t		fCStatus;
	void _init_raw_stats();
	status_t _ref_to_file(const entry_ref *ref);	  
	uint32			_reserved[4];
};


#endif			/* #ifndef _SOUND_FILE_H*/
