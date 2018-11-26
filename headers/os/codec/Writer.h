#ifndef _WRITER_PLUGIN_H
#define _WRITER_PLUGIN_H

#include <MetaData.h>
#include <MediaTrack.h>

#include "MediaPlugin.h"


namespace BCodecKit {

namespace BPrivate {
	class PluginManager;
}


class BWriter {
public:
	virtual	status_t			SetMetaData(BMetaData* data) = 0;
	virtual	status_t			CommitHeader() = 0;
	virtual	status_t			Flush() = 0;
	virtual	status_t			Close() = 0;

	virtual	status_t			AllocateCookie(void** cookie,
									media_format* format,
									const media_codec_info* codecInfo) = 0;
	virtual	status_t			FreeCookie(void* cookie) = 0;

	virtual status_t			SetMetaData(void* cookie,
									BMetaData* data) = 0;

	virtual	status_t			AddTrackInfo(void* cookie, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0) = 0;

	virtual	status_t			WriteChunk(void* cookie,
									const void* chunkBuffer, size_t chunkSize,
									media_encode_info* encodeInfo) = 0;

			BDataIO*			Target() const;

	virtual status_t			Perform(perform_code code, void* data);

protected:
								BWriter();
	virtual						~BWriter();

	virtual	status_t			Init(const media_file_format* fileFormat) = 0;

private:
			void				_Setup(BDataIO* target);

			BDataIO*			fTarget;

			BMediaPlugin*		fMediaPlugin;

	// needed for plug-in reference count management
	friend class BCodecKit::BPrivate::PluginManager;
	friend class BMediaWriter;

	virtual void				_ReservedWriter1();
	virtual void				_ReservedWriter2();
	virtual void				_ReservedWriter3();
	virtual void				_ReservedWriter4();
	virtual void				_ReservedWriter5();

			uint32				fReserved[5];
};


class BWriterPlugin : public virtual BMediaPlugin {
public:
	virtual	BWriter*			NewWriter() = 0;
	virtual	status_t			GetSupportedFileFormats(
									const media_file_format** _fileFormats,
									size_t* _count) = 0;
};


} // namespace BCodecKit


#endif // _WRITER_PLUGIN_H
