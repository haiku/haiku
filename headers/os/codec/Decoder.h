#ifndef _DECODER_PLUGIN_H
#define _DECODER_PLUGIN_H


#include <MediaTrack.h>
#include <MediaFormats.h>

#include "MediaPlugin.h"


namespace BCodecKit {

namespace BPrivate {
	class PluginManager;
}


class BChunkProvider {
public:
								BChunkProvider();
	virtual						~BChunkProvider();

protected:
	friend class BDecoder;

	virtual	status_t			GetNextChunk(const void** chunkBuffer,
									size_t* chunkSize,
									media_header* mediaHeader) = 0;
};


class BDecoder {
public:
	virtual	void				GetCodecInfo(media_codec_info* codecInfo) = 0;

	// Setup get's called with the info data from Reader::GetStreamInfo
	virtual	status_t			Setup(media_format* ioEncodedFormat,
									const void* infoBuffer,
									size_t infoSize) = 0;

	virtual	status_t			NegotiateOutputFormat(
									media_format* ioDecodedFormat) = 0;
	
	virtual	status_t			SeekedTo(int64 frame, bigtime_t time) = 0;
							 
	virtual status_t			Decode(void* buffer, int64* frameCount,
									media_header* mediaHeader,
									media_decode_info* info = 0) = 0;
							   
			status_t			GetNextChunk(const void** chunkBuffer,
									size_t* chunkSize,
									media_header* mediaHeader);

			void				SetChunkProvider(BChunkProvider* provider);

	virtual status_t			Perform(perform_code code, void* data);

protected:
								BDecoder();
	virtual						~BDecoder();


private:
			BChunkProvider*		fChunkProvider;

			BMediaPlugin*		fMediaPlugin;

	// needed for plug-in reference count management
	friend class BCodecKit::BPrivate::PluginManager;

	virtual void				_ReservedDecoder1();
	virtual void				_ReservedDecoder2();
	virtual void				_ReservedDecoder3();
	virtual void				_ReservedDecoder4();
	virtual void				_ReservedDecoder5();

			uint32				fReserved[5];
};


class BDecoderPlugin : public virtual BMediaPlugin {
public:
								BDecoderPlugin();

	virtual	BDecoder*			NewDecoder(uint index) = 0;
	virtual	status_t			GetSupportedFormats(media_format** formats,
									size_t* count) = 0;
};


} // namespace BCodecKit


#endif // _DECODER_PLUGIN_H
