/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SoundFile.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaFile.h>
#include <MediaTrack.h>
#include <SoundFile.h>

#include <string.h>

#include "MediaDebug.h"

/*************************************************************
 * public BSoundFile
 *************************************************************/

BSoundFile::BSoundFile()
{
	_init_raw_stats();
}


BSoundFile::BSoundFile(const entry_ref *ref,
					   uint32 open_mode)
{
	_init_raw_stats();
	SetTo(ref,open_mode);
}

/* virtual */ 
BSoundFile::~BSoundFile()
{
	delete fSoundFile;
	delete fMediaFile;
		// fMediaTrack will be deleted by the BMediaFile destructor
}


status_t
BSoundFile::InitCheck() const
{
	if (!fSoundFile) {
		return B_NO_INIT;
	}
	return fSoundFile->InitCheck();
}


status_t
BSoundFile::SetTo(const entry_ref *ref,
				  uint32 open_mode)
{
	if (fMediaTrack) {
		BMediaTrack * track = fMediaTrack;
		fMediaTrack = 0;
		fMediaFile->ReleaseTrack(track);
	}
	if (fMediaFile) {
		BMediaFile * file = fMediaFile;
		fMediaFile = 0;
		delete file;
	}		
	if (fSoundFile) {
		BFile * file = fSoundFile;
		fSoundFile = 0;
		delete file;
	}
	if (open_mode == B_READ_ONLY) {
		return _ref_to_file(ref);
	} else {
		UNIMPLEMENTED();
		return B_ERROR;
	}
}


int32
BSoundFile::FileFormat() const
{
	return fFileFormat;
}


int32
BSoundFile::SamplingRate() const
{
	return fSamplingRate;
}


int32
BSoundFile::CountChannels() const
{
	return fChannelCount;
}


int32
BSoundFile::SampleSize() const
{
	return fSampleSize;
}


int32
BSoundFile::ByteOrder() const
{
	return fByteOrder;
}


int32
BSoundFile::SampleFormat() const
{
	return fSampleFormat;
}


int32
BSoundFile::FrameSize() const
{
	return fSampleSize * fChannelCount;
}


off_t
BSoundFile::CountFrames() const
{
	return fFrameCount;
}


bool
BSoundFile::IsCompressed() const
{
	return fIsCompressed;
}


int32
BSoundFile::CompressionType() const
{
	return fCompressionType;
}


char *
BSoundFile::CompressionName() const
{
	return fCompressionName;
}


/* virtual */ int32
BSoundFile::SetFileFormat(int32 format)
{
	fFileFormat = format;
	return fFileFormat;
}


/* virtual */ int32
BSoundFile::SetSamplingRate(int32 fps)
{
	fSamplingRate = fps;
	return fSamplingRate;
}


/* virtual */ int32
BSoundFile::SetChannelCount(int32 spf)
{
	fChannelCount = spf;
	return fChannelCount;
}


/* virtual */ int32
BSoundFile::SetSampleSize(int32 bps)
{
	fSampleSize = bps;
	return fSampleSize;
}


/* virtual */ int32
BSoundFile::SetByteOrder(int32 bord)
{
	fByteOrder = bord;
	return fByteOrder;
}


/* virtual */ int32
BSoundFile::SetSampleFormat(int32 fmt)
{
	fSampleFormat = fmt;
	return fSampleFormat;
}


/* virtual */ int32
BSoundFile::SetCompressionType(int32 type)
{
	return 0;
}


/* virtual */ char *
BSoundFile::SetCompressionName(char *name)
{
	return NULL;
}


/* virtual */ bool
BSoundFile::SetIsCompressed(bool tf)
{
	return false;
}


/* virtual */ off_t
BSoundFile::SetDataLocation(off_t offset)
{
	UNIMPLEMENTED();

	return 0;
}


/* virtual */ off_t
BSoundFile::SetFrameCount(off_t count)
{
	fFrameCount = count;
	return fFrameCount;
}


size_t
BSoundFile::ReadFrames(char *buf,
					   size_t count)
{
	size_t frameRead = 0;
	int64 frames = count;
	while (count > 0) {
		status_t status = fMediaTrack->ReadFrames(
				reinterpret_cast<void *>(buf), &frames);
		count -= frames;
		frameRead += frames;
		buf += fSampleSize * fChannelCount * frames;
		if (status != B_OK) {
			if (frameRead > 0)
				break;
			return status;
		}
	}
	return frameRead;
}


size_t
BSoundFile::WriteFrames(char *buf,
						size_t count)
{
	return fMediaTrack->WriteFrames(
			reinterpret_cast<void *>(buf), count);
}


/* virtual */ off_t
BSoundFile::SeekToFrame(off_t n)
{
	int64 frames = n;
	status_t status = fMediaTrack->SeekToFrame(&frames);

	if (status != B_OK)
		return status;

	return frames;
}


off_t
BSoundFile::FrameIndex() const
{
	return fFrameIndex;
}


off_t
BSoundFile::FramesRemaining() const
{
	return fFrameCount - FrameIndex();
}

/*************************************************************
 * private BSoundFile
 *************************************************************/


void BSoundFile::_ReservedSoundFile1() {}
void BSoundFile::_ReservedSoundFile2() {}
void BSoundFile::_ReservedSoundFile3() {}

void
BSoundFile::_init_raw_stats()
{
	fSoundFile = 0;
	fMediaFile = 0;
	fMediaTrack = 0;	
	fFileFormat = B_UNKNOWN_FILE;
	fSamplingRate = 44100;
	fChannelCount = 2;
	fSampleSize = 2;
	fByteOrder = B_BIG_ENDIAN;
	fSampleFormat = B_LINEAR_SAMPLES;
	fFrameCount = 0;
	fFrameIndex = 0;
	fIsCompressed = false;
	fCompressionType = -1;
	fCompressionName = NULL;
}


static int32
_ParseMimeType(char *mime_type)
{
	if (strcmp(mime_type, "audio/x-aiff") == 0)
		return B_AIFF_FILE;
	if (strcmp(mime_type, "audio/x-wav") == 0)
		return B_WAVE_FILE;
	return B_UNKNOWN_FILE;
}


status_t
BSoundFile::_ref_to_file(const entry_ref *ref)
{
	status_t status;
	BFile * file = new BFile(ref, B_READ_ONLY);
	status = file->InitCheck();
	if (status != B_OK) {
		fSoundFile = file;
		return status;
	}	
	BMediaFile * media = new BMediaFile(file);
	status = media->InitCheck();
	if (status != B_OK) {
		delete media;
		delete file;
		return status;
	}
	media_file_format mfi;
	media->GetFileFormatInfo(&mfi);
	switch (mfi.family) {
		case B_AIFF_FORMAT_FAMILY: fFileFormat = B_AIFF_FILE; break;
		case B_WAV_FORMAT_FAMILY:  fFileFormat = B_WAVE_FILE; break;
		default: fFileFormat = _ParseMimeType(mfi.mime_type); break;
	}
	int trackNum = 0;
	BMediaTrack * track = 0;
	media_format mf;
	while (trackNum < media->CountTracks()) {
		track = media->TrackAt(trackNum);
		status = track->DecodedFormat(&mf);
		if (status != B_OK) {
			media->ReleaseTrack(track);
			delete media;
			delete file;
			return status;
		}		
		if (mf.IsAudio()) {
			break;
		}
		media->ReleaseTrack(track);
		track = 0;
	}
	if (track == 0) {
		delete media;
		delete file;
		return B_ERROR;
	}
	media_raw_audio_format * raw = 0;
	if (mf.type == B_MEDIA_ENCODED_AUDIO) {
		raw = &mf.u.encoded_audio.output;
	}
	if (mf.type == B_MEDIA_RAW_AUDIO) {
		raw = &mf.u.raw_audio;
	}

	if (raw == NULL) {
		delete media;
		delete file;
		return B_ERROR;
	}

	fSamplingRate = (int)raw->frame_rate;
	fChannelCount = raw->channel_count;
	fSampleSize = raw->format & 0xf;
	fByteOrder = raw->byte_order;
	switch (raw->format) {
		case media_raw_audio_format::B_AUDIO_FLOAT: 
			fSampleFormat = B_FLOAT_SAMPLES;
			break;
		case media_raw_audio_format::B_AUDIO_INT:
		case media_raw_audio_format::B_AUDIO_SHORT: 
		case media_raw_audio_format::B_AUDIO_UCHAR:
		case media_raw_audio_format::B_AUDIO_CHAR:
			fSampleFormat = B_LINEAR_SAMPLES;
			break;
		default:
			fSampleFormat = B_UNDEFINED_SAMPLES;
	}
	fByteOffset = 0;
	fFrameCount = track->CountFrames();
	fFrameIndex = 0;
	if (mf.type == B_MEDIA_ENCODED_AUDIO) {
		fIsCompressed = true;
		fCompressionType = mf.u.encoded_audio.encoding;
	}
	fMediaFile = media;
	fMediaTrack = track;
	fSoundFile = file;
	return B_OK;
}


