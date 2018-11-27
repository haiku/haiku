/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <CodecRoster.h>

#include <MediaFormats.h>

#include "AddOnManager.h"
#include "FormatManager.h"
#include "PluginManager.h"


namespace BCodecKit {


status_t
BCodecRoster::InstantiateReader(BReader** reader, int32* streamCount,
	media_file_format* mff, BDataIO* source)
{
	return gPluginManager.CreateReader(reader, streamCount, mff, source);
}


void
BCodecRoster::ReleaseReader(BReader* reader)
{
	return gPluginManager.DestroyReader(reader);
}


status_t
BCodecRoster::InstantiateDecoder(BDecoder** decoder, const media_format& format)
{
	return gPluginManager.CreateDecoder(decoder, format);
}


status_t
BCodecRoster::InstantiateDecoder(BDecoder** decoder, const media_codec_info& mci)
{
	return gPluginManager.CreateDecoder(decoder, mci);
}


void
BCodecRoster::ReleaseDecoder(BDecoder* decoder)
{
	return gPluginManager.DestroyDecoder(decoder);
}


status_t
BCodecRoster::GetDecoderInfo(BDecoder* decoder, media_codec_info* info)
{
	return gPluginManager.GetDecoderInfo(decoder, info);
}


status_t
BCodecRoster::InstantiateWriter(BWriter** writer, const media_file_format& mff,
	BDataIO* target)
{
	return gPluginManager.CreateWriter(writer, mff, target);
}


void
BCodecRoster::ReleaseWriter(BWriter* writer)
{
	return gPluginManager.DestroyWriter(writer);
}


status_t
BCodecRoster::InstantiateEncoder(BEncoder** encoder, const media_format& format)
{
	return gPluginManager.CreateEncoder(encoder, format);
}


status_t
BCodecRoster::InstantiateEncoder(BEncoder** encoder,
	const media_codec_info* codecInfo, uint32 flags)
{
	return gPluginManager.CreateEncoder(encoder, codecInfo, flags);
}


void
BCodecRoster::ReleaseEncoder(BEncoder* encoder)
{
	return gPluginManager.DestroyEncoder(encoder);
}


status_t
BCodecRoster::InstantiateStreamer(BStreamer** streamer, BUrl url,
	BDataIO** source)
{
	return gPluginManager.CreateStreamer(streamer, url, source);
}


void
BCodecRoster::ReleaseStreamer(BStreamer* streamer)
{
	return gPluginManager.DestroyStreamer(streamer);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, media_format* _outputFormat,
	media_codec_info* _codecInfo)
{
	return get_next_encoder(cookie, fileFormat,
		inputFormat, _outputFormat, _codecInfo);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, const media_format* outputFormat,
	media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
	media_format* _acceptedOutputFormat)
{
	return get_next_encoder(cookie, fileFormat, inputFormat, outputFormat,
		_codecInfo, _acceptedInputFormat, _acceptedOutputFormat);
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, media_codec_info* _codecInfo)
{
	return get_next_encoder(cookie, _codecInfo);
}


status_t
BCodecRoster::GetNextFileFormat(int32* cookie, media_file_format* mff)
{
	if (cookie == NULL || mff == NULL)
		return B_BAD_VALUE;

	status_t ret = BPrivate::AddOnManager::GetInstance()->GetFileFormat(mff,
		*cookie);

	if (ret != B_OK)
		return ret;

	*cookie = *cookie + 1;
	return B_OK;
}


status_t
BCodecRoster::GetCodecInfo(media_codec_info* codecInfo,
	media_format_family* formatFamily, media_format* inputFormat,
	media_format* outputFormat, int32 cookie)
{
	return BPrivate::AddOnManager::GetInstance()->GetCodecInfo(codecInfo,
			formatFamily, inputFormat, outputFormat, cookie);
}


status_t
BCodecRoster::MakeFormatFor(const media_format_description* descriptions,
	int32 descriptionCount, media_format& format, uint32 flags,
	void* _reserved)
{
	return FormatManager::GetInstance()->MakeFormatFor(descriptions,
		descriptionCount, format, flags, _reserved);
}


} // namespace BCodecKit
