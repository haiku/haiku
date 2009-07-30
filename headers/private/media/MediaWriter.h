/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_WRITER_H
#define _MEDIA_WRITER_H


#include "EncoderPlugin.h"
#include "TList.h"
#include "WriterPlugin.h"


namespace BPrivate {
namespace media {


class MediaWriter {
public:
								MediaWriter(BDataIO* target,
									const media_file_format& fileFormat);
								~MediaWriter();

			status_t			InitCheck();

			void				GetFileFormatInfo(media_file_format* mfi) const;

			status_t			SetCopyright(const char* copyright);
			status_t			CommitHeader();
			status_t			Flush();
			status_t			Close();

			status_t			AddTrackInfo(void* cookie, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0);

			status_t			CreateEncoder(Encoder** _encoder,
									const media_codec_info* codecInfo,
									uint32 flags = 0);

			status_t			WriteChunk(int32 stream,
									const void* chunkBuffer, size_t chunkSize,
									uint32 flags);

private:
			struct StreamInfo {
				void*			cookie;
			};

private:
			BDataIO*			fTarget;
			Writer*				fWriter;

			List<StreamInfo>	fStreamInfos;

			media_file_format	fFileFormat;
};


}; // namespace media
}; // namespace BPrivate

using namespace BPrivate::media;


#endif // _MEDIA_WRITER_H
