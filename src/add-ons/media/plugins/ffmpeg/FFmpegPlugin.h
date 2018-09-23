/*
 * Copyright (C) 2001 Carlos Hasan. All Rights Reserved.
 * Copyright (C) 2001 François Revol. All Rights Reserved.
 * Copyright (C) 2001 Axel Dörfler. All Rights Reserved.
 * Copyright (C) 2004 Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef FFMPEG_PLUGIN_H
#define FFMPEG_PLUGIN_H


#include <MediaFormats.h>

#include "DecoderPlugin.h"
#include "EncoderPlugin.h"
#include "ReaderPlugin.h"
#include "WriterPlugin.h"


class FFmpegPlugin : public ReaderPlugin, public DecoderPlugin,
	public WriterPlugin, public EncoderPlugin {
public:
	virtual	Reader*				NewReader();

	virtual	Decoder*			NewDecoder(uint index);
	virtual	status_t			GetSupportedFormats(media_format** _formats,
									size_t* _count);

	virtual	Writer*				NewWriter();
	virtual	status_t			GetSupportedFileFormats(
									const media_file_format** _fileFormats,
									size_t* _count);

	virtual	Encoder*			NewEncoder(
									const media_codec_info& codecInfo);

	virtual	Encoder*			NewEncoder(const media_format& format);

	virtual	status_t			RegisterNextEncoder(int32* cookie,
									media_codec_info* codecInfo,
									media_format_family* formatFamily,
									media_format* inputFormat,
									media_format* outputFormat);
};


#endif // FFMPEG_PLUGIN_H
