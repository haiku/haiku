/* 
** Copyright 2004-2007, Marcus Overhagen. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Path.h>
#include <image.h>
#include <string.h>

#include "PluginManager.h"
#include "DataExchange.h"
#include "debug.h"


PluginManager _plugin_manager;


status_t
PluginManager::CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source)
{
	TRACE("PluginManager::CreateReader enter\n");

	BPositionIO *seekable_source = dynamic_cast<BPositionIO *>(source);
	if (seekable_source == 0) {
		printf("PluginManager::CreateReader: non-seekable sources not supported yet\n");
		return B_ERROR;
	}

	// get list of available readers from the server
	server_get_readers_request request;
	server_get_readers_reply reply;
	if (B_OK != QueryServer(SERVER_GET_READERS, &request, sizeof(request), &reply, sizeof(reply))) {
		printf("PluginManager::CreateReader: can't get list of readers\n");
		return B_ERROR;
	}

	// try each reader by calling it's Sniff function...
	for (int32 i = 0; i < reply.count; i++) {
		entry_ref ref = reply.ref[i];
		MediaPlugin *plugin = GetPlugin(ref);
		if (!plugin) {
			printf("PluginManager::CreateReader: GetPlugin failed\n");
			return B_ERROR;
		}

		ReaderPlugin *readerPlugin = dynamic_cast<ReaderPlugin *>(plugin);
		if (!readerPlugin) {
			printf("PluginManager::CreateReader: dynamic_cast failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		*reader = readerPlugin->NewReader();
		if (! *reader) {
			printf("PluginManager::CreateReader: NewReader failed\n");
			PutPlugin(plugin);
			return B_ERROR;
		}

		seekable_source->Seek(0, SEEK_SET);
		(*reader)->Setup(seekable_source);

		if (B_OK == (*reader)->Sniff(streamCount)) {
			TRACE("PluginManager::CreateReader: Sniff success (%ld stream(s))\n", *streamCount);
			(*reader)->GetFileFormatInfo(mff);
			return B_OK;
		}

		// _DestroyReader(*reader);
		delete *reader;
		PutPlugin(plugin);
	}

	TRACE("PluginManager::CreateReader leave\n");
	return B_MEDIA_NO_HANDLER;
}


void
PluginManager::DestroyReader(Reader *reader)
{
	// ToDo: must call put plugin
	delete reader;
}


status_t
PluginManager::CreateDecoder(Decoder **_decoder, const media_format &format)
{
	TRACE("PluginManager::CreateDecoder enter\n");

	// get decoder for this format from the server
	server_get_decoder_for_format_request request;
	server_get_decoder_for_format_reply reply;
	request.format = format;
	if (B_OK != QueryServer(SERVER_GET_DECODER_FOR_FORMAT, &request, sizeof(request), &reply, sizeof(reply))) {
		printf("PluginManager::CreateDecoder: can't get decoder for format\n");
		return B_ERROR;
	}

	MediaPlugin *plugin = GetPlugin(reply.ref);
	if (!plugin) {
		printf("PluginManager::CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	DecoderPlugin *decoderPlugin = dynamic_cast<DecoderPlugin *>(plugin);
	if (!decoderPlugin) {
		printf("PluginManager::CreateDecoder: dynamic_cast failed\n");
		_plugin_manager.PutPlugin(plugin);
		return B_ERROR;
	}
	
	*_decoder = decoderPlugin->NewDecoder(0);
	if (*_decoder == NULL) {
		printf("PluginManager::CreateDecoder: NewDecoder() failed\n");
		_plugin_manager.PutPlugin(plugin);
		return B_ERROR;
	}

	TRACE("PluginManager::CreateDecoder leave\n");

	return B_OK;
}


status_t
PluginManager::CreateDecoder(Decoder **decoder, const media_codec_info &mci)
{
	// TODO
	debugger("not implemented");
	return B_ERROR;
}


status_t
PluginManager::GetDecoderInfo(Decoder *decoder, media_codec_info *out_info) const
{
	if (!decoder)
		return B_BAD_VALUE;

	decoder->GetCodecInfo(out_info);
	// TODO:
	// out_info->id = 
	// out_info->sub_id = 
	return B_OK;
}


void
PluginManager::DestroyDecoder(Decoder *decoder)
{
	// ToDo: must call put plugin
	delete decoder;
}


//	#pragma mark -


PluginManager::PluginManager()
{
	CALLED();
	fLocker = new BLocker;
	fPluginList = new List<plugin_info>;
}


PluginManager::~PluginManager()
{
	CALLED();
	while (!fPluginList->IsEmpty()) {
		plugin_info *info = NULL;
		fPluginList->Get(fPluginList->CountItems() - 1, &info);
		printf("PluginManager: Error, unloading PlugIn %s with usecount %d\n", info->name, info->usecount);
		delete info->plugin;
		unload_add_on(info->image);
		fPluginList->Remove(fPluginList->CountItems() - 1);
	}
	delete fLocker;
}

	
MediaPlugin *
PluginManager::GetPlugin(const entry_ref &ref)
{
	fLocker->Lock();
	
	MediaPlugin *plugin;
	plugin_info *pinfo;
	plugin_info info;
	
	for (fPluginList->Rewind(); fPluginList->GetNext(&pinfo); ) {
		if (0 == strcmp(ref.name, pinfo->name)) {
			plugin = pinfo->plugin;
			pinfo->usecount++;
			fLocker->Unlock();
			return plugin;
		}
	}

	if (!LoadPlugin(ref, &info.plugin, &info.image)) {
		printf("PluginManager: Error, loading PlugIn %s failed\n", ref.name);
		fLocker->Unlock();
		return 0;
	}

	strcpy(info.name, ref.name);
	info.usecount = 1;
	fPluginList->Insert(info);
	
	TRACE("PluginManager: PlugIn %s loaded\n", ref.name);

	plugin = info.plugin;
	
	fLocker->Unlock();
	return plugin;
}


void
PluginManager::PutPlugin(MediaPlugin *plugin)
{
	fLocker->Lock();
	
	plugin_info *pinfo;
	
	for (fPluginList->Rewind(); fPluginList->GetNext(&pinfo); ) {
		if (plugin == pinfo->plugin) {
			pinfo->usecount--;
			if (pinfo->usecount == 0) {
				delete pinfo->plugin;
				unload_add_on(pinfo->image);
			}
			fPluginList->RemoveCurrent();
			fLocker->Unlock();
			return;
		}
	}
	
	printf("PluginManager: Error, can't put PlugIn %p\n", plugin);
	
	fLocker->Unlock();
}


bool
PluginManager::LoadPlugin(const entry_ref &ref, MediaPlugin **plugin, image_id *image)
{
	BPath p(&ref);

	TRACE("PluginManager: LoadPlugin trying to load %s\n", p.Path());

	image_id id;
	id = load_add_on(p.Path());
	if (id < 0)
		return false;
		
	MediaPlugin *(*instantiate_plugin_func)();
	
	if (get_image_symbol(id, "instantiate_plugin", B_SYMBOL_TYPE_TEXT, (void**)&instantiate_plugin_func) < B_OK) {
		printf("PluginManager: Error, LoadPlugin can't find instantiate_plugin in %s\n", p.Path());
		unload_add_on(id);
		return false;
	}
	
	MediaPlugin *pl;
	
	pl = (*instantiate_plugin_func)();
	if (pl == NULL) {
		printf("PluginManager: Error, LoadPlugin instantiate_plugin in %s returned NULL\n", p.Path());
		unload_add_on(id);
		return false;
	}
	
	*plugin = pl;
	*image = id;
	return true;
}
