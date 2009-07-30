/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include "MediaWriter.h"

#include <new>

#include <stdio.h>
#include <string.h>

#include <Autolock.h>

#include "debug.h"

#include "PluginManager.h"


MediaWriter::MediaWriter(BDataIO* target, const media_file_format& fileFormat)
	:
	fTarget(target),
	fWriter(NULL),
	fStreamInfos(),
	fFileFormat(fileFormat)
{
	CALLED();

	_plugin_manager.CreateWriter(&fWriter, fFileFormat, fTarget);
}


MediaWriter::~MediaWriter()
{
	CALLED();

	// free all stream cookies
	// and chunk caches
	StreamInfo* info;
	for (fStreamInfos.Rewind(); fStreamInfos.GetNext(&info);)
		fWriter->FreeCookie(info->cookie);

	_plugin_manager.DestroyWriter(fWriter);

	// fTarget is owned by the BMediaFile
}


status_t
MediaWriter::InitCheck()
{
	CALLED();

	return fWriter != NULL ? B_OK : B_NO_INIT;
}


void
MediaWriter::GetFileFormatInfo(media_file_format* _fileFormat) const
{
	CALLED();

	if (_fileFormat != NULL)
		*_fileFormat = fFileFormat;
}


status_t
MediaWriter::WriteChunk(int32 streamIndex, const void* chunkBuffer,
	size_t chunkSize, uint32 flags)
{
	if (fWriter == NULL)
		return B_NO_INIT;

	StreamInfo* info;
	if (!fStreamInfos.Get(streamIndex, &info))
		return B_BAD_INDEX;

	return fWriter->WriteChunk(info->cookie, chunkBuffer, chunkSize, flags);
}


class MediaExtractorChunkWriter : public ChunkWriter {
public:
	MediaExtractorChunkWriter(MediaWriter* writer, int32 streamIndex)
		:
		fWriter(writer),
		fStreamIndex(streamIndex)
	{
	}

	virtual status_t WriteChunk(const void* chunkBuffer, size_t chunkSize,
		uint32 flags)
	{
		return fWriter->WriteChunk(fStreamIndex, chunkBuffer, chunkSize,
			flags);
	}

private:
	MediaWriter*	fWriter;
	int32			fStreamIndex;
};


status_t
MediaWriter::CreateEncoder(Encoder** _encoder,
	const media_codec_info* codecInfo, uint32 flags)
{
	CALLED();

	// TODO: Here we should work out a way so that if there is a setup
	// failure we can try the next encoder.
	Encoder* encoder;
	status_t ret = _plugin_manager.CreateEncoder(&encoder, codecInfo, flags);
	if (ret != B_OK) {
		ERROR("MediaWriter::CreateEncoder _plugin_manager.CreateEncoder "
			"failed, codec: %s\n", codecInfo->pretty_name);
		return ret;
	}

	StreamInfo info;
	ret = fWriter->AllocateCookie(&info.cookie);
	if (ret != B_OK) {
		_plugin_manager.DestroyEncoder(encoder);
		return ret;
	}

	int32 streamIndex = fStreamInfos.CountItems();

	if (!fStreamInfos.Insert(info)) {
		_plugin_manager.DestroyEncoder(encoder);
		ERROR("MediaWriter::CreateEncoder can't create StreamInfo "
			"for stream %ld\n", streamIndex);
		return B_NO_MEMORY;
	}

	ChunkWriter* chunkWriter = new(std::nothrow) MediaExtractorChunkWriter(
		this, streamIndex);
	if (chunkWriter == NULL) {
		_plugin_manager.DestroyEncoder(encoder);
		ERROR("MediaWriter::CreateEncoder can't create ChunkWriter "
			"for stream %ld\n", streamIndex);
		return B_NO_MEMORY;
	}

	encoder->SetChunkWriter(chunkWriter);
	*_encoder = encoder;

	return B_OK;
}
