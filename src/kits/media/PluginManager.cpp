/* 
 * Copyright 2004-2010, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */

#include <Path.h>
#include <image.h>
#include <string.h>

#include "PluginManager.h"
#include "DataExchange.h"
#include "debug.h"


PluginManager gPluginManager;


// #pragma mark - Readers/Decoders


status_t
PluginManager::CreateReader(Reader** reader, int32* streamCount,
	media_file_format* mff, BDataIO* source)
{
	TRACE("PluginManager::CreateReader enter\n");

	BPositionIO *seekable_source = dynamic_cast<BPositionIO *>(source);
	if (seekable_source == 0) {
		printf("PluginManager::CreateReader: non-seekable sources not "
			"supported yet\n");
		return B_ERROR;
	}

	// get list of available readers from the server
	server_get_readers_request request;
	server_get_readers_reply reply;
	status_t ret = QueryServer(SERVER_GET_READERS, &request, sizeof(request),
		&reply, sizeof(reply));
	if (ret != B_OK) {
		printf("PluginManager::CreateReader: can't get list of readers: %s\n",
			strerror(ret));
		return ret;
	}

	// try each reader by calling it's Sniff function...
	for (int32 i = 0; i < reply.count; i++) {
		entry_ref ref = reply.ref[i];
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

		seekable_source->Seek(0, SEEK_SET);
		(*reader)->Setup(seekable_source);
		(*reader)->fMediaPlugin = plugin;

		if ((*reader)->Sniff(streamCount) == B_OK) {
			TRACE("PluginManager::CreateReader: Sniff success "
				"(%ld stream(s))\n", *streamCount);
			(*reader)->GetFileFormatInfo(mff);
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

	// get decoder for this format from the server
	server_get_decoder_for_format_request request;
	server_get_decoder_for_format_reply reply;
	request.format = format;
	status_t ret = QueryServer(SERVER_GET_DECODER_FOR_FORMAT, &request,
		sizeof(request), &reply, sizeof(reply));
	if (ret != B_OK) {
		printf("PluginManager::CreateDecoder: can't get decoder for format: "
			"%s\n", strerror(ret));
		return ret;
	}

	MediaPlugin* plugin = GetPlugin(reply.ref);
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
	// TODO
	debugger("not implemented");
	return B_ERROR;
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
	server_get_writer_request request;
	request.internal_id = mff.id.internal_id;
	server_get_writer_reply reply;
	status_t ret = QueryServer(SERVER_GET_WRITER_FOR_FORMAT_FAMILY, &request,
		sizeof(request), &reply, sizeof(reply));
	if (ret != B_OK) {
		printf("PluginManager::CreateWriter: can't get writer for file "
			"family: %s\n", strerror(ret));
		return ret;
	}

	MediaPlugin* plugin = GetPlugin(reply.ref);
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
	server_get_encoder_for_codec_info_request request;
	server_get_encoder_for_codec_info_reply reply;
	request.id = codecInfo->id;
	status_t ret = QueryServer(SERVER_GET_ENCODER_FOR_CODEC_INFO, &request,
		sizeof(request), &reply, sizeof(reply));
	if (ret != B_OK) {
		printf("PluginManager::CreateEncoder: can't get encoder for codec %s: "
			"%s\n", codecInfo->pretty_name, strerror(ret));
		return ret;
	}

	MediaPlugin* plugin = GetPlugin(reply.ref);
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
		printf("PluginManager: Error, unloading PlugIn %s with usecount "
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
				TRACE("  unloading add-on: %ld\n\n", pinfo->image);
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
