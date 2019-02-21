/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Copyright 2004-2009, The Haiku Project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * 	Authors:
 *		Axel Dörfler
 *		Marcus Overhagen
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
BCodecRoster::InstantiateStreamer(BStreamer** streamer, BUrl url)
{
	return gPluginManager.CreateStreamer(streamer, url);
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
	// TODO: If fileFormat is provided (existing apps also pass NULL),
	// we could at least check fileFormat->capabilities against
	// outputFormat->type without even contacting the server.

	if (cookie == NULL || inputFormat == NULL || _codecInfo == NULL)
		return B_BAD_VALUE;

	while (true) {
		media_codec_info candidateCodecInfo;
		media_format_family candidateFormatFamily;
		media_format candidateInputFormat;
		media_format candidateOutputFormat;

		status_t ret = BCodecRoster::GetCodecInfo(&candidateCodecInfo,
			&candidateFormatFamily, &candidateInputFormat,
			&candidateOutputFormat, *cookie);

		if (ret != B_OK)
			return ret;

		*cookie = *cookie + 1;

		if (fileFormat != NULL && candidateFormatFamily != B_ANY_FORMAT_FAMILY
			&& fileFormat->family != B_ANY_FORMAT_FAMILY
			&& fileFormat->family != candidateFormatFamily) {
			continue;
		}

		if (!candidateInputFormat.Matches(inputFormat))
			continue;

		if (_outputFormat != NULL)
			*_outputFormat = candidateOutputFormat;

		*_codecInfo = candidateCodecInfo;
		break;
	}

	return B_OK;
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
	const media_format* inputFormat, const media_format* outputFormat,
	media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
	media_format* _acceptedOutputFormat)
{
	// TODO: If fileFormat is provided (existing apps also pass NULL),
	// we could at least check fileFormat->capabilities against
	// outputFormat->type without even contacting the server.

	if (cookie == NULL || inputFormat == NULL || outputFormat == NULL
		|| _codecInfo == NULL) {
		return B_BAD_VALUE;
	}

	while (true) {
		media_codec_info candidateCodecInfo;
		media_format_family candidateFormatFamily;
		media_format candidateInputFormat;
		media_format candidateOutputFormat;

		status_t ret = BCodecRoster::GetCodecInfo(&candidateCodecInfo,
			&candidateFormatFamily, &candidateInputFormat,
			&candidateOutputFormat, *cookie);

		if (ret != B_OK)
			return ret;

		*cookie = *cookie + 1;

		if (fileFormat != NULL && candidateFormatFamily != B_ANY_FORMAT_FAMILY
			&& fileFormat->family != B_ANY_FORMAT_FAMILY
			&& fileFormat->family != candidateFormatFamily) {
			continue;
		}

		if (!candidateInputFormat.Matches(inputFormat)
			|| !candidateOutputFormat.Matches(outputFormat)) {
			continue;
		}

		// TODO: These formats are currently way too generic. For example,
		// an encoder may want to adjust video width to a multiple of 16,
		// or overwrite the intput and or output color space. To make this
		// possible, we actually have to instantiate an Encoder here and
		// ask it to specifiy the format.
		if (_acceptedInputFormat != NULL)
			*_acceptedInputFormat = candidateInputFormat;
		if (_acceptedOutputFormat != NULL)
			*_acceptedOutputFormat = candidateOutputFormat;

		*_codecInfo = candidateCodecInfo;
		break;
	}

	return B_OK;
}


status_t
BCodecRoster::GetNextEncoder(int32* cookie, media_codec_info* _codecInfo)
{
	if (cookie == NULL || _codecInfo == NULL)
		return B_BAD_VALUE;

	media_format_family formatFamily;
	media_format inputFormat;
	media_format outputFormat;

	status_t ret = BCodecRoster::GetCodecInfo(_codecInfo,
		&formatFamily, &inputFormat, &outputFormat, *cookie);
	if (ret != B_OK)
		return ret;

	*cookie = *cookie + 1;

	return B_OK;
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


} // namespace BCodecKit
