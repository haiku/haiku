/* 
 * Copyright 2004-2010, Marcus Overhagen. All rights reserved.
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <AdapterIO.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <BufferIO.h>
#include <DataIO.h>
#include <image.h>
#include <Path.h>

#include <string.h>

#include "AddOnManager.h"
#include "PluginManager.h"
#include "DataExchange.h"
#include "MediaDebug.h"


// Need to stay outside namespace
BCodecKit::BPrivate::PluginManager gPluginManager;


namespace BCodecKit {
namespace BPrivate {


#define BLOCK_SIZE 4096
#define MAX_STREAMERS 40


class DataIOAdapter : public BAdapterIO {
public:
	DataIOAdapter(BDataIO* dataIO)
		:
		BAdapterIO(B_MEDIA_SEEK_BACKWARD | B_MEDIA_MUTABLE_SIZE,
			B_INFINITE_TIMEOUT),
		fDataIO(dataIO)
	{
		fDataInputAdapter = BuildInputAdapter();
	}

	virtual	~DataIOAdapter()
	{
	}

	virtual	ssize_t	ReadAt(off_t position, void* buffer,
		size_t size)
	{
		if (position == Position()) {
			ssize_t ret = fDataIO->Read(buffer, size);
			fDataInputAdapter->Write(buffer, ret);
			return ret;
		}

		off_t totalSize = 0;
		if (GetSize(&totalSize) != B_OK)
			return B_UNSUPPORTED;

		if (position+size < (size_t)totalSize)
			return ReadAt(position, buffer, size);

		return B_NOT_SUPPORTED;
	}

	virtual	ssize_t	WriteAt(off_t position, const void* buffer,
		size_t size)
	{
		if (position == Position()) {
			ssize_t ret = fDataIO->Write(buffer, size);
			fDataInputAdapter->Write(buffer, ret);
			return ret;
		}

		return B_NOT_SUPPORTED;
	}

private:
	BDataIO*		fDataIO;
	BInputAdapter*	fDataInputAdapter;
};


class BMediaIOWrapper : public BMediaIO {
public:
	BMediaIOWrapper(BDataIO* source)
		:
		fData(NULL),
		fPosition(NULL),
		fMedia(NULL),
		fBufferIO(NULL),
		fDataIOAdapter(NULL),
		fErr(B_NO_ERROR)
	{
		CALLED();

		fPosition = dynamic_cast<BPositionIO*>(source);
		fMedia = dynamic_cast<BMediaIO*>(source);
		fBufferIO = dynamic_cast<BBufferIO *>(source);
		fData = source;

		// No need to do additional buffering if we have
		// a BBufferIO or a BMediaIO.
		if (!IsMedia() && fBufferIO == NULL) {
			// Source needs to be at least a BPositionIO to wrap with a BBufferIO
			if (IsPosition()) {
				fBufferIO = new(std::nothrow) BBufferIO(fPosition, 65536, false);
				if (fBufferIO == NULL) {
					fErr = B_NO_MEMORY;
					return;
				}
				// We have to reset our parents reference too
				fPosition = dynamic_cast<BPositionIO*>(fBufferIO);
				fData = dynamic_cast<BDataIO*>(fPosition);
			} else {
				// In this case we have to supply our own form
				// of pseudo-seekable object from a non-seekable
				// BDataIO.
				fDataIOAdapter = new DataIOAdapter(source);
				fMedia = dynamic_cast<BMediaIO*>(fDataIOAdapter);
				fPosition = dynamic_cast<BPositionIO*>(fDataIOAdapter);
				fData = dynamic_cast<BDataIO*>(fDataIOAdapter);
				TRACE("Unable to improve performance with a BufferIO\n");
			}
		}

		if (IsMedia())
			fMedia->GetFlags(&fFlags);
		else if (IsPosition())
			fFlags = B_MEDIA_SEEKABLE;
	}

	virtual	~BMediaIOWrapper()
	{
		if (fBufferIO != NULL)
			delete fBufferIO;

		if (fDataIOAdapter != NULL)
			delete fDataIOAdapter;
	}

	status_t InitCheck() const
	{
		return fErr;
	}

	// BMediaIO interface

	virtual void GetFlags(int32* flags) const
	{
		*flags = fFlags;
	}

	// BPositionIO interface

	virtual	ssize_t ReadAt(off_t position, void* buffer,
		size_t size)
	{
		CALLED();

		return fPosition->ReadAt(position, buffer, size);
	}

	virtual	ssize_t WriteAt(off_t position, const void* buffer,
		size_t size)
	{
		CALLED();

		return fPosition->WriteAt(position, buffer, size);
	}

	virtual	off_t Seek(off_t position, uint32 seekMode)
	{
		CALLED();

		return fPosition->Seek(position, seekMode);

	}

	virtual off_t Position() const
	{
		CALLED();

		return fPosition->Position();
	}

	virtual	status_t SetSize(off_t size)
	{
		CALLED();

		return fPosition->SetSize(size);
	}

	virtual	status_t GetSize(off_t* size) const
	{
		CALLED();

		return fPosition->GetSize(size);
	}

protected:

	bool IsMedia() const
	{
		return fMedia != NULL;
	}

	bool IsPosition() const
	{
		return fPosition != NULL;
	}

private:
	BDataIO*			fData;
	BPositionIO*		fPosition;
	BMediaIO*			fMedia;
	BBufferIO*			fBufferIO;
	DataIOAdapter*		fDataIOAdapter;

	int32				fFlags;

	status_t			fErr;
};


// #pragma mark - Readers/Decoders


status_t
PluginManager::CreateReader(BReader** reader, int32* streamCount,
	media_file_format* mff, BDataIO* source)
{
	TRACE("PluginManager::CreateReader enter\n");

	// The wrapper class will present our source in a more useful
	// way, we create an instance which is buffering our reads and
	// writes.
	BMediaIOWrapper* buffered_source = new BMediaIOWrapper(source);
	ObjectDeleter<BMediaIOWrapper> ioDeleter(buffered_source);

	status_t ret = buffered_source->InitCheck();
	if (ret != B_OK)
		return ret;

	// get list of available readers from the server
	entry_ref refs[MAX_READERS];
	int32 count;

	ret = AddOnManager::GetInstance()->GetReaders(refs, &count,
		MAX_READERS);
	if (ret != B_OK) {
		printf("PluginManager::CreateReader: can't get list of readers: %s\n",
			strerror(ret));
		return ret;
	}

	// try each reader by calling it's Sniff function...
	for (int32 i = 0; i < count; i++) {
		entry_ref ref = refs[i];
		BMediaPlugin* plugin = GetPlugin(ref);
		if (plugin == NULL) {
			printf("PluginManager::CreateReader: GetPlugin failed\n");
			return B_ERROR;
		}

		BReaderPlugin* readerPlugin = dynamic_cast<BReaderPlugin*>(plugin);
		if (readerPlugin == NULL) {
			printf("PluginManager::CreateReader: dynamic_cast failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		*reader = readerPlugin->NewReader();
		if (*reader == NULL) {
			printf("PluginManager::CreateReader: NewReader failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		buffered_source->Seek(0, SEEK_SET);
		(*reader)->_Setup(buffered_source);
		(*reader)->fMediaPlugin = plugin;

		if ((*reader)->Sniff(streamCount) == B_OK) {
			TRACE("PluginManager::CreateReader: Sniff success "
				"(%" B_PRId32 " stream(s))\n", *streamCount);
			(*reader)->GetFileFormatInfo(mff);
			ioDeleter.Detach();
			return B_OK;
		}

		DestroyReader(*reader);
		*reader = NULL;
	}

	TRACE("PluginManager::CreateReader leave\n");
	return B_MEDIA_NO_HANDLER;
}


void
PluginManager::DestroyReader(BReader* reader)
{
	if (reader != NULL) {
		TRACE("PluginManager::DestroyReader(%p (plugin: %p))\n", reader,
			reader->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the reader,
		// since otherwise we may actually unload the code for the
		// destructor...
		BMediaPlugin* plugin = reader->fMediaPlugin;
		delete reader;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateDecoder(BDecoder** _decoder, const media_format& format)
{
	TRACE("PluginManager::CreateDecoder enter\n");

	// get decoder for this format
	entry_ref ref;
	status_t ret = AddOnManager::GetInstance()->GetDecoderForFormat(
		&ref, format);
	if (ret != B_OK) {
		printf("PluginManager::CreateDecoder: can't get decoder for format: "
			"%s\n", strerror(ret));
		return ret;
	}

	BMediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		printf("PluginManager::CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	BDecoderPlugin* decoderPlugin = dynamic_cast<BDecoderPlugin*>(plugin);
	if (decoderPlugin == NULL) {
		printf("PluginManager::CreateDecoder: dynamic_cast failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}
	
	// TODO: In theory, one DecoderPlugin could support multiple Decoders,
	// but this is not yet handled (passing "0" as index/ID).
	*_decoder = decoderPlugin->NewDecoder(0);
	if (*_decoder == NULL) {
		printf("PluginManager::CreateDecoder: NewDecoder() failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}
	TRACE("  created decoder: %p\n", *_decoder);
	(*_decoder)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateDecoder leave\n");

	return B_OK;
}


status_t
PluginManager::CreateDecoder(BDecoder** decoder, const media_codec_info& mci)
{
	TRACE("PluginManager::CreateDecoder enter\n");
	entry_ref ref;
	status_t status = AddOnManager::GetInstance()->GetEncoder(&ref, mci.id);
	if (status != B_OK)
		return status;

	BMediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		ERROR("PluginManager::CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}

	BDecoderPlugin* decoderPlugin = dynamic_cast<BDecoderPlugin*>(plugin);
	if (decoderPlugin == NULL) {
		ERROR("PluginManager::CreateDecoder: dynamic_cast failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}

	// TODO: In theory, one DecoderPlugin could support multiple Decoders,
	// but this is not yet handled (passing "0" as index/ID).
	*decoder = decoderPlugin->NewDecoder(0);
	if (*decoder == NULL) {
		ERROR("PluginManager::CreateDecoder: NewDecoder() failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}
	TRACE("  created decoder: %p\n", *decoder);
	(*decoder)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateDecoder leave\n");

	return B_OK;

}


status_t
PluginManager::GetDecoderInfo(BDecoder* decoder, media_codec_info* _info) const
{
	if (decoder == NULL)
		return B_BAD_VALUE;

	decoder->GetCodecInfo(_info);
	// TODO:
	// out_info->id = 
	// out_info->sub_id = 
	return B_OK;
}


void
PluginManager::DestroyDecoder(BDecoder* decoder)
{
	if (decoder != NULL) {
		TRACE("PluginManager::DestroyDecoder(%p, plugin: %p)\n", decoder,
			decoder->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the decoder,
		// since otherwise we may actually unload the code for the
		// destructor...
		BMediaPlugin* plugin = decoder->fMediaPlugin;
		delete decoder;
		PutPlugin(plugin);
	}
}


// #pragma mark - Writers/Encoders


status_t
PluginManager::CreateWriter(BWriter** writer, const media_file_format& mff,
	BDataIO* target)
{
	TRACE("PluginManager::CreateWriter enter\n");

	// Get the Writer responsible for this media_file_format from the server.
	entry_ref ref;
	status_t ret = AddOnManager::GetInstance()->GetWriter(&ref,
		mff.id.internal_id);
	if (ret != B_OK) {
		printf("PluginManager::CreateWriter: can't get writer for file "
			"family: %s\n", strerror(ret));
		return ret;
	}

	BMediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		printf("PluginManager::CreateWriter: GetPlugin failed\n");
		return B_ERROR;
	}

	BWriterPlugin* writerPlugin = dynamic_cast<BWriterPlugin*>(plugin);
	if (writerPlugin == NULL) {
		printf("PluginManager::CreateWriter: dynamic_cast failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}

	*writer = writerPlugin->NewWriter();
	if (*writer == NULL) {
		printf("PluginManager::CreateWriter: NewWriter failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}

	(*writer)->_Setup(target);
	(*writer)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateWriter leave\n");
	return B_OK;
}


void
PluginManager::DestroyWriter(BWriter* writer)
{
	if (writer != NULL) {
		TRACE("PluginManager::DestroyWriter(%p (plugin: %p))\n", writer,
			writer->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the writer,
		// since otherwise we may actually unload the code for the
		// destructor...
		BMediaPlugin* plugin = writer->fMediaPlugin;
		delete writer;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateEncoder(BEncoder** _encoder,
	const media_codec_info* codecInfo, uint32 flags)
{
	TRACE("PluginManager::CreateEncoder enter\n");

	// Get encoder for this codec info from the server
	entry_ref ref;
	status_t ret = AddOnManager::GetInstance()->GetEncoder(&ref,
		codecInfo->id);
	if (ret != B_OK) {
		printf("PluginManager::CreateEncoder: can't get encoder for codec %s: "
			"%s\n", codecInfo->pretty_name, strerror(ret));
		return ret;
	}

	BMediaPlugin* plugin = GetPlugin(ref);
	if (!plugin) {
		printf("PluginManager::CreateEncoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	BEncoderPlugin* encoderPlugin = dynamic_cast<BEncoderPlugin*>(plugin);
	if (encoderPlugin == NULL) {
		printf("PluginManager::CreateEncoder: dynamic_cast failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}

	*_encoder = encoderPlugin->NewEncoder(*codecInfo);
	if (*_encoder == NULL) {
		printf("PluginManager::CreateEncoder: NewEncoder() failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}
	TRACE("  created encoder: %p\n", *_encoder);
	(*_encoder)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateEncoder leave\n");

	return B_OK;
}


status_t
PluginManager::CreateEncoder(BEncoder** encoder, const media_format& format)
{
	TRACE("PluginManager::CreateEncoder enter nr2\n");

	entry_ref ref;

	status_t ret = AddOnManager::GetInstance()->GetEncoderForFormat(
		&ref, format);

	if (ret != B_OK) {
		ERROR("PluginManager::CreateEncoder: can't get decoder for format: "
			"%s\n", strerror(ret));
		return ret;
	}

	BMediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		ERROR("PluginManager::CreateEncoder: GetPlugin failed\n");
		return B_ERROR;
	}

	BEncoderPlugin* encoderPlugin = dynamic_cast<BEncoderPlugin*>(plugin);
	if (encoderPlugin == NULL) {
		ERROR("PluginManager::CreateEncoder: dynamic_cast failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}


	*encoder = encoderPlugin->NewEncoder(format);
	if (*encoder == NULL) {
		ERROR("PluginManager::CreateEncoder: NewEncoder() failed\n");
		PutPlugin(plugin);
		return B_ERROR;
	}
	TRACE("  created encoder: %p\n", *encoder);
	(*encoder)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateEncoder leave nr2\n");

	return B_OK;
}


void
PluginManager::DestroyEncoder(BEncoder* encoder)
{
	if (encoder != NULL) {
		TRACE("PluginManager::DestroyEncoder(%p, plugin: %p)\n", encoder,
			encoder->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the encoder,
		// since otherwise we may actually unload the code for the
		// destructor...
		BMediaPlugin* plugin = encoder->fMediaPlugin;
		delete encoder;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateStreamer(BStreamer** streamer, BUrl url)
{
	BAutolock _(fLocker);

	TRACE("PluginManager::CreateStreamer enter\n");

	entry_ref refs[MAX_STREAMERS];
	int32 count;

	status_t ret = AddOnManager::GetInstance()->GetStreamers(refs, &count,
		MAX_STREAMERS);
	if (ret != B_OK) {
		printf("PluginManager::CreateStreamer: can't get list of streamers:"
			" %s\n", strerror(ret));
		return ret;
	}

	// try each reader by calling it's Sniff function...
	for (int32 i = 0; i < count; i++) {
		entry_ref ref = refs[i];
		BMediaPlugin* plugin = GetPlugin(ref);
		if (plugin == NULL) {
			printf("PluginManager::CreateStreamer: GetPlugin failed\n");
			return B_ERROR;
		}

		BStreamerPlugin* streamerPlugin = dynamic_cast<BStreamerPlugin*>(plugin);
		if (streamerPlugin == NULL) {
			printf("PluginManager::CreateStreamer: dynamic_cast failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		*streamer = streamerPlugin->NewStreamer();
		if (*streamer == NULL) {
			printf("PluginManager::CreateStreamer: NewReader failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		(*streamer)->fMediaPlugin = plugin;
		plugin->fRefCount++;

		if ((*streamer)->Sniff(url) == B_OK) {
			TRACE("PluginManager::CreateStreamer: Sniff success\n");
			return B_OK;
		}

		DestroyStreamer(*streamer);
		*streamer = NULL;
	}

	TRACE("PluginManager::CreateStreamer leave\n");
	return B_MEDIA_NO_HANDLER;
}


void
PluginManager::DestroyStreamer(BStreamer* streamer)
{
	BAutolock _(fLocker);

	if (streamer != NULL) {
		TRACE("PluginManager::DestroyStreamer(%p, plugin: %p)\n", streamer,
			streamer->fMediaPlugin);

		// NOTE: We have to put the plug-in after deleting the streamer,
		// since otherwise we may actually unload the code for the
		// destructor...
		BMediaPlugin* plugin = streamer->fMediaPlugin;
		delete streamer;

		// Delete the plugin only when every reference is released
		if (plugin->fRefCount == 1) {
			plugin->fRefCount = 0;
			PutPlugin(plugin);
		} else
			plugin->fRefCount--;
	}
}


// #pragma mark -


PluginManager::PluginManager()
	:
	fPluginList(),
	fLocker("media plugin manager")
{
	CALLED();
}


PluginManager::~PluginManager()
{
	CALLED();
	for (int i = fPluginList.CountItems() - 1; i >= 0; i--) {
		plugin_info* info = NULL;
		fPluginList.Get(i, &info);
		TRACE("PluginManager: Error, unloading PlugIn %s with usecount "
			"%d\n", info->name, info->usecount);
		delete info->plugin;
		unload_add_on(info->image);
	}
}

	
BMediaPlugin*
PluginManager::GetPlugin(const entry_ref& ref)
{
	TRACE("PluginManager::GetPlugin(%s)\n", ref.name);
	fLocker.Lock();
	
	BMediaPlugin* plugin;
	plugin_info* pinfo;
	plugin_info info;
	
	for (fPluginList.Rewind(); fPluginList.GetNext(&pinfo); ) {
		if (0 == strcmp(ref.name, pinfo->name)) {
			plugin = pinfo->plugin;
			pinfo->usecount++;
			TRACE("  found existing plugin: %p\n", pinfo->plugin);
			fLocker.Unlock();
			return plugin;
		}
	}

	if (_LoadPlugin(ref, &info.plugin, &info.image) < B_OK) {
		printf("PluginManager: Error, loading PlugIn %s failed\n", ref.name);
		fLocker.Unlock();
		return NULL;
	}

	strcpy(info.name, ref.name);
	info.usecount = 1;
	fPluginList.Insert(info);
	
	TRACE("PluginManager: PlugIn %s loaded\n", ref.name);

	plugin = info.plugin;
	TRACE("  loaded plugin: %p\n", plugin);
	
	fLocker.Unlock();
	return plugin;
}


void
PluginManager::PutPlugin(BMediaPlugin* plugin)
{
	TRACE("PluginManager::PutPlugin()\n");
	fLocker.Lock();
	
	plugin_info* pinfo;
	
	for (fPluginList.Rewind(); fPluginList.GetNext(&pinfo); ) {
		if (plugin == pinfo->plugin) {
			pinfo->usecount--;
			if (pinfo->usecount == 0) {
				TRACE("  deleting %p\n", pinfo->plugin);
				delete pinfo->plugin;
				TRACE("  unloading add-on: %" B_PRId32 "\n\n", pinfo->image);
				unload_add_on(pinfo->image);
				fPluginList.RemoveCurrent();
			}
			fLocker.Unlock();
			return;
		}
	}
	
	printf("PluginManager: Error, can't put PlugIn %p\n", plugin);
	
	fLocker.Unlock();
}


status_t
PluginManager::_LoadPlugin(const entry_ref& ref, BMediaPlugin** plugin,
	image_id* image)
{
	BPath p(&ref);

	TRACE("PluginManager: _LoadPlugin trying to load %s\n", p.Path());

	image_id id;
	id = load_add_on(p.Path());
	if (id < 0) {
		printf("PluginManager: Error, load_add_on(): %s\n", strerror(id));
		return B_ERROR;
	}
		
	BMediaPlugin* (*instantiate_plugin_func)();
	
	if (get_image_symbol(id, "instantiate_plugin", B_SYMBOL_TYPE_TEXT,
			(void**)&instantiate_plugin_func) < B_OK) {
		printf("PluginManager: Error, _LoadPlugin can't find "
			"instantiate_plugin in %s\n", p.Path());
		unload_add_on(id);
		return B_ERROR;
	}
	
	BMediaPlugin *pl;
	
	pl = (*instantiate_plugin_func)();
	if (pl == NULL) {
		printf("PluginManager: Error, _LoadPlugin instantiate_plugin in %s "
			"returned NULL\n", p.Path());
		unload_add_on(id);
		return B_ERROR;
	}
	
	*plugin = pl;
	*image = id;
	return B_OK;
}


} // namespace BPrivate
} // namespace BCodecKit
