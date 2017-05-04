/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MediaWriter.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include "debug.h"

#include "PluginManager.h"



class MediaExtractorChunkWriter : public ChunkWriter {
public:
	MediaExtractorChunkWriter(MediaWriter* writer, int32 streamIndex)
		:
		fWriter(writer),
		fStreamIndex(streamIndex)
	{
	}

	virtual status_t WriteChunk(const void* chunkBuffer, size_t chunkSize,
		media_encode_info* encodeInfo)
	{
		return fWriter->WriteChunk(fStreamIndex, chunkBuffer, chunkSize,
			encodeInfo);
	}

private:
	MediaWriter*	fWriter;
	int32			fStreamIndex;
};


// #pragma mark -


MediaWriter::MediaWriter(BDataIO* target, const media_file_format& fileFormat)
	:
	fTarget(target),
	fWriter(NULL),
	fStreamInfos(),
	fFileFormat(fileFormat)
{
	CALLED();

	gPluginManager.CreateWriter(&fWriter, fFileFormat, fTarget);
}


MediaWriter::~MediaWriter()
{
	CALLED();

	if (fWriter != NULL) {
		// free all stream cookies
		// and chunk caches
		StreamInfo* info;
		for (fStreamInfos.Rewind(); fStreamInfos.GetNext(&info);)
			fWriter->FreeCookie(info->cookie);

		gPluginManager.DestroyWriter(fWriter);
	}

	// fTarget is owned by the BMediaFile
}


status_t
MediaWriter::InitCheck()
{
	CALLED();

	return fWriter != NULL ? fWriter->Init(&fFileFormat) : B_NO_INIT;
}


BDataIO*
MediaWriter::Target() const
{
	return fTarget;
}


void
MediaWriter::GetFileFormatInfo(media_file_format* _fileFormat) const
{
	CALLED();

	if (_fileFormat != NULL)
		*_fileFormat = fFileFormat;
}


status_t
MediaWriter::CreateEncoder(Encoder** _encoder,
	const media_codec_info* codecInfo, media_format* format, uint32 flags)
{
	CALLED();

	if (fWriter == NULL)
		return B_NO_INIT;

	// TODO: Here we should work out a way so that if there is a setup
	// failure we can try the next encoder.
	Encoder* encoder;
	status_t ret = gPluginManager.CreateEncoder(&encoder, codecInfo, flags);
	if (ret != B_OK) {
		ERROR("MediaWriter::CreateEncoder gPluginManager.CreateEncoder "
			"failed, codec: %s\n", codecInfo->pretty_name);
		return ret;
	}

	StreamInfo info;
	ret = fWriter->AllocateCookie(&info.cookie, format, codecInfo);
	if (ret != B_OK) {
		gPluginManager.DestroyEncoder(encoder);
		return ret;
	}

	int32 streamIndex = fStreamInfos.CountItems();

	if (!fStreamInfos.Insert(info)) {
		gPluginManager.DestroyEncoder(encoder);
		ERROR("MediaWriter::CreateEncoder can't create StreamInfo "
			"for stream %" B_PRId32 "\n", streamIndex);
		return B_NO_MEMORY;
	}

	ChunkWriter* chunkWriter = new(std::nothrow) MediaExtractorChunkWriter(
		this, streamIndex);
	if (chunkWriter == NULL) {
		gPluginManager.DestroyEncoder(encoder);
		ERROR("MediaWriter::CreateEncoder can't create ChunkWriter "
			"for stream %" B_PRId32 "\n", streamIndex);
		return B_NO_MEMORY;
	}

	encoder->SetChunkWriter(chunkWriter);
	*_encoder = encoder;

	return B_OK;
}


status_t
MediaWriter::SetCopyright(const char* copyright)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->SetCopyright(copyright);
}


status_t
MediaWriter::SetCopyright(int32 streamIndex, const char* copyright)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	StreamInfo* info;
	if (!fStreamInfos.Get(streamIndex, &info))
		return B_BAD_INDEX;

	return fWriter->SetCopyright(info->cookie, copyright);
}


status_t
MediaWriter::CommitHeader()
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->CommitHeader();
}


status_t
MediaWriter::Flush()
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->Flush();
}


status_t
MediaWriter::Close()
{
	if (fWriter == NULL)
		return B_NO_INIT;

	return fWriter->Close();
}


status_t
MediaWriter::AddTrackInfo(int32 streamIndex, uint32 code,
	const void* data, size_t size, uint32 flags)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	StreamInfo* info;
	if (!fStreamInfos.Get(streamIndex, &info))
		return B_BAD_INDEX;

	return fWriter->AddTrackInfo(info->cookie, code, data, size, flags);
}


status_t
MediaWriter::WriteChunk(int32 streamIndex, const void* chunkBuffer,
	size_t chunkSize, media_encode_info* encodeInfo)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	StreamInfo* info;
	if (!fStreamInfos.Get(streamIndex, &info))
		return B_BAD_INDEX;

	return fWriter->WriteChunk(info->cookie, chunkBuffer, chunkSize,
		encodeInfo);
}

