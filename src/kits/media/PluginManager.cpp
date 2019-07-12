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


PluginManager gPluginManager;

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
		fDataIOAdapter(NULL),
		fErr(B_NO_ERROR)
	{
		CALLED();

		fPosition = dynamic_cast<BPositionIO*>(source);
		fMedia = dynamic_cast<BMediaIO*>(source);
		fData = source;

		if (!IsPosition()) {
			// In this case we have to supply our own form
			// of pseudo-seekable object from a non-seekable
			// BDataIO.
			fDataIOAdapter = new DataIOAdapter(source);
			fMedia = dynamic_cast<BMediaIO*>(fDataIOAdapter);
			fPosition = dynamic_cast<BPositionIO*>(fDataIOAdapter);
			fData = dynamic_cast<BDataIO*>(fDataIOAdapter);
			TRACE("Unable to improve performance with a BufferIO\n");
		}

		if (IsMedia())
			fMedia->GetFlags(&fFlags);
		else if (IsPosition())
			fFlags = B_MEDIA_SEEKABLE;
	}

	virtual	~BMediaIOWrapper()
	{
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
	DataIOAdapter*		fDataIOAdapter;

	int32				fFlags;

	status_t			fErr;
};


// #pragma mark - Readers/Decoders


status_t
PluginManager::CreateReader(Reader** reader, int32* streamCount,
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
		MediaPlugin* plugin = GetPlugin(ref);
		if (plugin == NULL) {
			printf("PluginManager::CreateReader: GetPlugin failed\n");
			return B_ERROR;
		}

		ReaderPlugin* readerPlugin = dynamic_cast<ReaderPlugin*>(plugin);
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
		(*reader)->Setup(buffered_source);
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
PluginManager::DestroyReader(Reader* reader)
{
	if (reader != NULL) {
		TRACE("PluginManager::DestroyReader(%p (plugin: %p))\n", reader,
			reader->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the reader,
		// since otherwise we may actually unload the code for the
		// destructor...
		MediaPlugin* plugin = reader->fMediaPlugin;
		delete reader;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateDecoder(Decoder** _decoder, const media_format& format)
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

	MediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		printf("PluginManager::CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	DecoderPlugin* decoderPlugin = dynamic_cast<DecoderPlugin*>(plugin);
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
PluginManager::CreateDecoder(Decoder** decoder, const media_codec_info& mci)
{
	TRACE("PluginManager::CreateDecoder enter\n");
	entry_ref ref;
	status_t status = AddOnManager::GetInstance()->GetEncoder(&ref, mci.id);
	if (status != B_OK)
		return status;

	MediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		ERROR("PluginManager::CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}

	DecoderPlugin* decoderPlugin = dynamic_cast<DecoderPlugin*>(plugin);
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
PluginManager::GetDecoderInfo(Decoder* decoder, media_codec_info* _info) const
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
PluginManager::DestroyDecoder(Decoder* decoder)
{
	if (decoder != NULL) {
		TRACE("PluginManager::DestroyDecoder(%p, plugin: %p)\n", decoder,
			decoder->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the decoder,
		// since otherwise we may actually unload the code for the
		// destructor...
		MediaPlugin* plugin = decoder->fMediaPlugin;
		delete decoder;
		PutPlugin(plugin);
	}
}


// #pragma mark - Writers/Encoders


status_t
PluginManager::CreateWriter(Writer** writer, const media_file_format& mff,
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

	MediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		printf("PluginManager::CreateWriter: GetPlugin failed\n");
		return B_ERROR;
	}

	WriterPlugin* writerPlugin = dynamic_cast<WriterPlugin*>(plugin);
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

	(*writer)->Setup(target);
	(*writer)->fMediaPlugin = plugin;

	TRACE("PluginManager::CreateWriter leave\n");
	return B_OK;
}


void
PluginManager::DestroyWriter(Writer* writer)
{
	if (writer != NULL) {
		TRACE("PluginManager::DestroyWriter(%p (plugin: %p))\n", writer,
			writer->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the writer,
		// since otherwise we may actually unload the code for the
		// destructor...
		MediaPlugin* plugin = writer->fMediaPlugin;
		delete writer;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateEncoder(Encoder** _encoder,
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

	MediaPlugin* plugin = GetPlugin(ref);
	if (!plugin) {
		printf("PluginManager::CreateEncoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	EncoderPlugin* encoderPlugin = dynamic_cast<EncoderPlugin*>(plugin);
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
PluginManager::CreateEncoder(Encoder** encoder, const media_format& format)
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

	MediaPlugin* plugin = GetPlugin(ref);
	if (plugin == NULL) {
		ERROR("PluginManager::CreateEncoder: GetPlugin failed\n");
		return B_ERROR;
	}

	EncoderPlugin* encoderPlugin = dynamic_cast<EncoderPlugin*>(plugin);
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
PluginManager::DestroyEncoder(Encoder* encoder)
{
	if (encoder != NULL) {
		TRACE("PluginManager::DestroyEncoder(%p, plugin: %p)\n", encoder,
			encoder->fMediaPlugin);
		// NOTE: We have to put the plug-in after deleting the encoder,
		// since otherwise we may actually unload the code for the
		// destructor...
		MediaPlugin* plugin = encoder->fMediaPlugin;
		delete encoder;
		PutPlugin(plugin);
	}
}


status_t
PluginManager::CreateStreamer(Streamer** streamer, BUrl url, BDataIO** source)
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
		MediaPlugin* plugin = GetPlugin(ref);
		if (plugin == NULL) {
			printf("PluginManager::CreateStreamer: GetPlugin failed\n");
			return B_ERROR;
		}

		StreamerPlugin* streamerPlugin = dynamic_cast<StreamerPlugin*>(plugin);
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

		BDataIO* streamSource = NULL;
		if ((*streamer)->Sniff(url, &streamSource) == B_OK) {
			TRACE("PluginManager::CreateStreamer: Sniff success\n");
			*source = streamSource;
			return B_OK;
		}

		DestroyStreamer(*streamer);
		*streamer = NULL;
	}

	TRACE("PluginManager::CreateStreamer leave\n");
	return B_MEDIA_NO_HANDLER;
}


void
PluginManager::DestroyStreamer(Streamer* streamer)
{
	BAutolock _(fLocker);

	if (streamer != NULL) {
		TRACE("PluginManager::DestroyStreamer(%p, plugin: %p)\n", streamer,
			streamer->fMediaPlugin);

		// NOTE: We have to put the plug-in after deleting the streamer,
		// since otherwise we may actually unload the code for the
		// destructor...
		MediaPlugin* plugin = streamer->fMediaPlugin;
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

	
MediaPlugin*
PluginManager::GetPlugin(const entry_ref& ref)
{
	TRACE("PluginManager::GetPlugin(%s)\n", ref.name);
	fLocker.Lock();
	
	MediaPlugin* plugin;
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
PluginManager::PutPlugin(MediaPlugin* plugin)
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
PluginManager::_LoadPlugin(const entry_ref& ref, MediaPlugin** plugin,
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
		
	MediaPlugin* (*instantiate_plugin_func)();
	
	if (get_image_symbol(id, "instantiate_plugin", B_SYMBOL_TYPE_TEXT,
			(void**)&instantiate_plugin_func) < B_OK) {
		printf("PluginManager: Error, _LoadPlugin can't find "
			"instantiate_plugin in %s\n", p.Path());
		unload_add_on(id);
		return B_ERROR;
	}
	
	MediaPlugin *pl;
	
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
