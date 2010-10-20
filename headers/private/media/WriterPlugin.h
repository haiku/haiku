#ifndef _WRITER_PLUGIN_H
#define _WRITER_PLUGIN_H

#include <MediaTrack.h>
#include "MediaPlugin.h"

namespace BPrivate { namespace media {

class PluginManager;

class Writer {
public:
								Writer();
	virtual						~Writer();

	virtual	status_t			Init(const media_file_format* fileFormat) = 0;

	virtual	status_t			SetCopyright(const char* copyright) = 0;
	virtual	status_t			CommitHeader() = 0;
	virtual	status_t			Flush() = 0;
	virtual	status_t			Close() = 0;

	virtual	status_t			AllocateCookie(void** cookie,
									media_format* format,
									const media_codec_info* codecInfo) = 0;
	virtual	status_t			FreeCookie(void* cookie) = 0;

	virtual	status_t			SetCopyright(void* cookie,
									const char* copyright) = 0;

	virtual	status_t			AddTrackInfo(void* cookie, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0) = 0;

	virtual	status_t			WriteChunk(void* cookie,
									const void* chunkBuffer, size_t chunkSize,
									media_encode_info* encodeInfo) = 0;

			BDataIO*			Target() const;

	virtual status_t			Perform(perform_code code, void* data);

private:
	virtual void				_ReservedWriter1();
	virtual void				_ReservedWriter2();
	virtual void				_ReservedWriter3();
	virtual void				_ReservedWriter4();
	virtual void				_ReservedWriter5();

public: // XXX for test programs only
			void				Setup(BDataIO* target);

			BDataIO*			fTarget;

	// needed for plug-in reference count management
	friend class PluginManager;
			MediaPlugin*		fMediaPlugin;

			uint32				fReserved[5];
};


class WriterPlugin : public virtual MediaPlugin {
public:
	virtual	Writer*				NewWriter() = 0;
	virtual	status_t			GetSupportedFileFormats(
									const media_file_format** _fileFormats,
									size_t* _count) = 0;

};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _WRITER_PLUGIN_H
