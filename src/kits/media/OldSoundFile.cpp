//OldSoundFile.h is replaced by SoundFile.h
//OldSoundFile.cpp is replaced by SoundFile.cpp

#if 0

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: OldSoundFile.cpp
 *  DESCR: 
 ***********************************************************************/
#include <OldSoundFile.h>
#include "debug.h"

/*************************************************************
 * public BSoundFile
 *************************************************************/

BSoundFile::BSoundFile()
{
	UNIMPLEMENTED();
}


BSoundFile::BSoundFile(const entry_ref *ref,
					   uint32 open_mode)
{
	UNIMPLEMENTED();
}


BSoundFile::~BSoundFile()
{
	UNIMPLEMENTED();
}


status_t
BSoundFile::InitCheck() const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t
BSoundFile::SetTo(const entry_ref *ref,
				  uint32 open_mode)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


int32
BSoundFile::FileFormat() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SamplingRate() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::CountChannels() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SampleSize() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::ByteOrder() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SampleFormat() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::FrameSize() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


off_t
BSoundFile::CountFrames() const
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}


bool
BSoundFile::IsCompressed() const
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


int32
BSoundFile::CompressionType() const
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


char *
BSoundFile::CompressionName() const
{
	UNIMPLEMENTED();
	return NULL;
}


int32
BSoundFile::SetFileFormat(int32 format)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetSamplingRate(int32 fps)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetChannelCount(int32 spf)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetSampleSize(int32 bps)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetByteOrder(int32 bord)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetSampleFormat(int32 fmt)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


int32
BSoundFile::SetCompressionType(int32 type)
{
	UNIMPLEMENTED();
	int32 dummy;

	return dummy;
}


char *
BSoundFile::SetCompressionName(char *name)
{
	UNIMPLEMENTED();
	return NULL;
}


bool
BSoundFile::SetIsCompressed(bool tf)
{
	UNIMPLEMENTED();
	bool dummy;

	return dummy;
}


off_t
BSoundFile::SetDataLocation(off_t offset)
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}


off_t
BSoundFile::SetFrameCount(off_t count)
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}


size_t
BSoundFile::ReadFrames(char *buf,
					   size_t count)
{
	UNIMPLEMENTED();
	size_t dummy;

	return dummy;
}


size_t
BSoundFile::WriteFrames(char *buf,
						size_t count)
{
	UNIMPLEMENTED();
	size_t dummy;

	return dummy;
}


off_t
BSoundFile::SeekToFrame(off_t n)
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}


off_t
BSoundFile::FrameIndex() const
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}


off_t
BSoundFile::FramesRemaining() const
{
	UNIMPLEMENTED();
	off_t dummy;

	return dummy;
}

/*************************************************************
 * private BSoundFile
 *************************************************************/


void
BSoundFile::_ReservedSoundFile1()
{
	UNIMPLEMENTED();
}


void
BSoundFile::_ReservedSoundFile2()
{
	UNIMPLEMENTED();
}


void
BSoundFile::_ReservedSoundFile3()
{
	UNIMPLEMENTED();
}


void
BSoundFile::_init_raw_stats()
{
	UNIMPLEMENTED();
}


status_t
BSoundFile::_ref_to_file(const entry_ref *ref)
{
	UNIMPLEMENTED();
	status_t dummy;

	return dummy;
}


#endif