/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_WRITER_H
#define _MEDIA_WRITER_H


#include <Encoder.h>
#include <MetaData.h>
#include <Writer.h>

#include "TList.h"


namespace BCodecKit {


class BMediaWriter {
public:
								BMediaWriter(BDataIO* target,
									const media_file_format& fileFormat);
								~BMediaWriter();

			status_t			InitCheck() const;

			BDataIO*			Target() const;

			void				GetFileFormatInfo(media_file_format* mfi) const;

			status_t			CreateEncoder(BEncoder** _encoder,
									const media_codec_info* codecInfo,
									media_format* format, uint32 flags = 0);

			// TODO: Why pointers? Just copy it
			status_t			SetMetaData(BMetaData* data);
			status_t			SetMetaData(int32 streamIndex,
									BMetaData* data);

			status_t			CommitHeader();
			status_t			Flush();
			status_t			Close();

			status_t			AddTrackInfo(int32 streamIndex, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0);

			status_t			WriteChunk(int32 streamIndex,
									const void* chunkBuffer, size_t chunkSize,
									media_encode_info* encodeInfo);

private:
			struct StreamInfo {
				void*			cookie;
			};

			status_t 			fInitCheck;
			BDataIO*			fTarget;
			BWriter*			fWriter;
			List<StreamInfo>	fStreamInfos;
			media_file_format	fFileFormat;

			uint32				fReserved[5];
};


} // namespace BCodecKit


#endif // _MEDIA_WRITER_H
