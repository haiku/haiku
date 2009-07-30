#ifndef _ENCODER_PLUGIN_H
#define _ENCODER_PLUGIN_H

#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaPlugin.h"

class AddOnManager;

namespace BPrivate { namespace media {

class PluginManager;

class ChunkWriter {
public:
	virtual						~ChunkWriter() {};
	virtual	status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize, uint32 flags) = 0;
};

class Encoder {
public:
								Encoder();
	virtual						~Encoder();

	virtual	void				GetCodecInfo(media_codec_info* codecInfo) = 0;

	virtual	status_t			SetFormat(const media_file_format& fileFormat,
									const media_format& encodedFormat) = 0;

	virtual	status_t			AddTrackInfo(uint32 code, const void* data,
									size_t size, uint32 flags = 0) = 0;

	virtual status_t			GetEncodeParameters(
									encode_parameters* parameters) const = 0;
	virtual status_t			SetEncodeParameters(
									encode_parameters* parameters) const = 0;
							   
	virtual status_t			Encode(const void* buffer, int64 frameCount,
									media_encode_info* info) = 0;
							   
			status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize, uint32 flags = 0);

			void				SetChunkWriter(ChunkWriter* writer);

	virtual status_t			Perform(perform_code code, void* data);

private:
	virtual void				_ReservedEncoder1();
	virtual void				_ReservedEncoder2();
	virtual void				_ReservedEncoder3();
	virtual void				_ReservedEncoder4();
	virtual void				_ReservedEncoder5();

			ChunkWriter*		fChunkWriter;

	// needed for plug-in reference count management
	friend class PluginManager;
			MediaPlugin*		fMediaPlugin;

			uint32				fReserved[5];
};


class EncoderPlugin : public virtual MediaPlugin {
public:
								EncoderPlugin();

	virtual	Encoder*			NewEncoder(uint index) = 0;
	virtual	status_t			GetSupportedFormats(media_format** formats,
									size_t* count) = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _ENCODER_PLUGIN_H
