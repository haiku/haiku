#include <Path.h>
#include <image.h>
#include <string.h>

#include "PluginManager.h"
#include "debug.h"

PluginManager _plugin_manager;

status_t
_CreateReader(Reader **reader, int32 *streamCount, media_file_format *mff, BDataIO *source)
{
	printf("_CreateReader enter\n");
	
	MediaPlugin *plugin;
	ReaderPlugin *readerplugin;
	
	plugin = _plugin_manager.GetPlugin("mp3_reader");
	if (!plugin) {
		printf("_CreateReader: GetPlugin failed\n");
		return B_ERROR;
	}
	
	readerplugin = dynamic_cast<ReaderPlugin *>(plugin);
	if (!readerplugin) {
		printf("_CreateReader: dynamic_cast failed\n");
		return B_ERROR;
	}
	
	*reader = readerplugin->NewReader();
	if (! *reader) {
		printf("_CreateReader: NewReader failed\n");
		return B_ERROR;
	}
	
	(*reader)->Setup(source);
	
	if (B_OK != (*reader)->Sniff(streamCount)) {
		printf("_CreateReader: Sniff failed\n");
		return B_ERROR;
	}
	
	(*reader)->GetFileFormatInfo(mff);
	
	printf("_CreateReader leave\n");

	return B_OK;
}

status_t
_CreateDecoder(Decoder **decoder, media_codec_info *mci, const media_format *format)
{
	printf("_CreateDecoder enter\n");

	MediaPlugin *plugin;
	DecoderPlugin *decoderplugin;
	
	plugin = _plugin_manager.GetPlugin("mp3_decoder");
	if (!plugin) {
		printf("_CreateDecoder: GetPlugin failed\n");
		return B_ERROR;
	}
	
	decoderplugin = dynamic_cast<DecoderPlugin *>(plugin);
	if (!decoderplugin) {
		printf("_CreateDecoder: dynamic_cast failed\n");
		return B_ERROR;
	}
	
	*decoder = decoderplugin->NewDecoder();
	if (! *decoder) {
		printf("_CreateDecoder: NewReader failed\n");
		return B_ERROR;
	}
	
	strcpy(mci->short_name,  " mci short_name");
	strcpy(mci->pretty_name, " mci pretty_name");
	
	printf("_CreateDecoder leave\n");
	
	return B_OK;
}


void
_DestroyReader(Reader *reader)
{
	delete reader;
}

void
_DestroyDecoder(Decoder *decoder)
{
	delete decoder;
}

status_t
_PublishDecoder(DecoderPlugin *decoderplugin,
				const char *meta_description,
				const char *short_name,
				const char *pretty_name,
				const char *default_mapping /* = 0 */)
{
	printf("_PublishDecoder: meta_description \"%s\",  short_name \"%s\", pretty_name \"%s\", default_mapping \"%s\"\n",
		meta_description, short_name, pretty_name, default_mapping);

	return B_OK;
}
							  


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
		plugin_info *info;
		fPluginList->Get(fPluginList->CountItems() - 1, &info);
		printf("PluginManager: Error, unloading PlugIn %s with usecount %d\n", info->name, info->usecount);
		delete info->plugin;
		unload_add_on(info->image);
		fPluginList->Remove(fPluginList->CountItems() - 1);
	}
	delete fLocker;
}

	
MediaPlugin *
PluginManager::GetPlugin(const char *name)
{
	fLocker->Lock();
	
	MediaPlugin *plugin;
	plugin_info *pinfo;
	plugin_info info;
	
	for (fPluginList->Rewind(); fPluginList->GetNext(&pinfo); ) {
		if (0 == strcmp(name, pinfo->name)) {
			plugin = pinfo->plugin;
			pinfo->usecount++;
			fLocker->Unlock();
			return plugin;
		}
	}

	if (!LoadPlugin(name, &info.plugin, &info.image)) {
		printf("PluginManager: Error, loading PlugIn %s failed\n", name);
		fLocker->Unlock();
		return 0;
	}

	strcpy(info.name, name);
	info.usecount = 1;
	fPluginList->Insert(info);
	
	printf("PluginManager: PlugIn %s loaded\n", name);

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
PluginManager::LoadPlugin(const char *name, MediaPlugin **plugin, image_id *image)
{
	const char *searchdir[] = {
		 "/boot/home/config/add-ons/media/plugins",
		 "/boot/beos/system/add-ons/media/plugins",
		 "/boot/home/develop/openbeos/current/distro/x86.R1/beos/system/add-ons/media/plugins",
		 NULL
	};
	
	for (int i = 0; searchdir[i]; i++) {
		BPath p(searchdir[i], name);

		printf("PluginManager: LoadPlugin trying to load %s\n", p.Path());

		image_id id;
		id = load_add_on(p.Path());
		if (id < 0)
			continue;
			
		MediaPlugin *(*instantiate_plugin_func)();
		
		if (get_image_symbol(id, "instantiate_plugin", B_SYMBOL_TYPE_TEXT, (void**)&instantiate_plugin_func) < B_OK) {
			printf("PluginManager: Error, LoadPlugin can't find instantiate_plugin in %s\n", p.Path());
			unload_add_on(id);
			continue;
		}
		
		MediaPlugin *pl;
		
		pl = (*instantiate_plugin_func)();
		if (pl == 0) {
			printf("PluginManager: Error, LoadPlugin instantiate_plugin in %s returned 0\n", p.Path());
			unload_add_on(id);
			continue;
		}
		
		*plugin = pl;
		*image = id;
		return true;
	}

	return false;
}
