#include <MediaFormats.h>
#include <Locker.h>
#include <Path.h>
#include <Entry.h>
#include <Directory.h>
#include <image.h>
#include <stdio.h>
#include <string.h>
#include "AddOnManager.h"
#include "FormatManager.h"
#include "debug.h"

extern FormatManager *gFormatManager;
extern AddOnManager *gAddOnManager;

status_t
_PublishDecoder(DecoderPlugin *decoderplugin,
				const char *meta_description,
				const char *short_name,
				const char *pretty_name,
				const char *default_mapping /* = 0 */);

bool operator==(const media_meta_description & a, const media_meta_description & b);

AddOnManager::AddOnManager()
 :	fLocker(new BLocker),
 	fReaderList(new List<reader_info>),
 	fWriterList(new List<writer_info>),
 	fDecoderList(new List<decoder_info>),
 	fEncoderList(new List<encoder_info>),
	fReaderInfo(0),
	fWriterInfo(0),
	fDecoderInfo(0),
	fEncoderInfo(0)
{
}

AddOnManager::~AddOnManager()
{
	delete fLocker;
 	delete fReaderList;
 	delete fWriterList;
 	delete fDecoderList;
 	delete fEncoderList;
}

void
AddOnManager::LoadState()
{
	RegisterAddOns();
}

void
AddOnManager::SaveState()
{
}
	
status_t
AddOnManager::GetDecoderForFormat(xfer_entry_ref *out_ref,
								  const media_format &in_format)
{
	media_format_description desc;
	
	if (B_OK != gFormatManager->GetDescriptionForFormat(&desc, in_format, B_META_FORMAT_FAMILY)) {
		printf("AddOnManager::GetDecoderForFormat: Error, meta format description unknown\n");
		return B_ERROR;	
	}

	fLocker->Lock();
	decoder_info *info;
	for (fDecoderList->Rewind(); fDecoderList->GetNext(&info); ) {
		media_meta_description *meta_desc;
		for (info->meta_desc.Rewind(); info->meta_desc.GetNext(&meta_desc); ) {
			if (desc.u.meta == *meta_desc) {
				printf("AddOnManager::GetDecoderForFormat: found decoder %s for format %s\n", info->ref.name, meta_desc->description);
				*out_ref = info->ref;
				fLocker->Unlock();
				return B_OK;
			}
		}
	}
	fLocker->Unlock();
	return B_ERROR;	
}
									
status_t
AddOnManager::GetReaders(xfer_entry_ref *out_res, int32 *out_count, int32 max_count)
{
	fLocker->Lock();
	
	*out_count = 0;
	
	reader_info *info;
	for (*out_count = 0, fReaderList->Rewind(); fReaderList->GetNext(&info) && *out_count <= max_count; *out_count += 1)
		out_res[*out_count] = info->ref;

	fLocker->Unlock();
	return B_OK;
}

void
AddOnManager::RegisterAddOns()
{
	const char *searchdir[] = {
		 "/boot/home/config/add-ons/media/plugins",
		 "/boot/beos/system/add-ons/media/plugins",
		 "/boot/home/develop/openbeos/current/distro/x86.R1/beos/system/add-ons/media/plugins",
		 NULL
	};
	
	for (int i = 0; searchdir[i]; i++) {
		BDirectory dir(searchdir[i]);
		BEntry e;
		while (B_OK == dir.GetNextEntry(&e)) {
			BPath p(&e);
			entry_ref ref;
			e.GetRef(&ref);
		
			printf("AddOnManager: RegisterAddOns trying to load %s\n", p.Path());

			image_id id;
			id = load_add_on(p.Path());
			if (id < 0)
				continue;
			
			MediaPlugin *(*instantiate_plugin_func)();
		
			if (get_image_symbol(id, "instantiate_plugin", B_SYMBOL_TYPE_TEXT, (void**)&instantiate_plugin_func) < B_OK) {
				printf("AddOnManager: Error, RegisterAddOns can't find instantiate_plugin in %s\n", p.Path());
				unload_add_on(id);
				continue;
			}
		
			MediaPlugin *pl;
		
			pl = (*instantiate_plugin_func)();
			if (pl == 0) {
				printf("AddOnManager: Error, RegisterAddOns instantiate_plugin in %s returned 0\n", p.Path());
				unload_add_on(id);
				continue;
			}

			ReaderPlugin *rp = dynamic_cast<ReaderPlugin *>(pl);
			if (rp)
				RegisterReader(rp, ref);
	
			DecoderPlugin *dp = dynamic_cast<DecoderPlugin *>(pl);
			if (dp)
				RegisterDecoder(dp, ref);
			
			delete pl;
			unload_add_on(id);
		}
	}
}

void
AddOnManager::RegisterReader(ReaderPlugin *reader, const entry_ref &ref)
{
	bool already_found;
	reader_info *pinfo;
	
	fLocker->Lock();
	for (already_found = false, fReaderList->Rewind(); fReaderList->GetNext(&pinfo); )
		if (0 == strcmp(pinfo->ref.name, ref.name)) {
			already_found = true;
			break;
		}
	fLocker->Unlock();
	if (already_found)
		return;

	printf("AddOnManager::RegisterReader, name %s\n", ref.name);

	reader_info info;
	info.ref = ref;

	if (B_OK != reader->RegisterPlugin())
		return;

	fLocker->Lock();
	fReaderList->Insert(info);
	fLocker->Unlock();
}

void
AddOnManager::RegisterDecoder(DecoderPlugin *decoder, const entry_ref &ref)
{
	bool already_found;
	decoder_info *pinfo;
	
	fLocker->Lock();
	for (already_found = false, fDecoderList->Rewind(); fDecoderList->GetNext(&pinfo); )
		if (0 == strcmp(pinfo->ref.name, ref.name)) {
			already_found = true;
			break;
		}
	fLocker->Unlock();
	if (already_found)
		return;

	printf("AddOnManager::RegisterDecoder, name %s\n", ref.name);
	
	decoder->Setup((void*)&_PublishDecoder);
	
	decoder_info info;
	info.ref = ref;
	fDecoderInfo = &info;

	if (B_OK != decoder->RegisterPlugin())
		return;

	fLocker->Lock();
	fDecoderList->Insert(info);
	fLocker->Unlock();
	fDecoderInfo = 0;
}

status_t
AddOnManager::PublishDecoderCallback(DecoderPlugin *decoderplugin,
									 const char *meta_description,
									 const char *short_name,
									 const char *pretty_name,
									 const char *default_mapping /* = 0 */)
{
												 
	printf("AddOnManager::PublishDecoderCallback: meta_description \"%s\",  short_name \"%s\", pretty_name \"%s\", default_mapping \"%s\"\n",
		meta_description, short_name, pretty_name, default_mapping);
		
	media_meta_description meta_desc;
	snprintf(meta_desc.description, sizeof(meta_desc), "%s", meta_description);
		
	fDecoderInfo->meta_desc.Insert(meta_desc);

	return B_OK;
}


//typedef status_t *(*publish_func)(DecoderPlugin *, const char *, const char *, const char *, const char *);
status_t
_PublishDecoder(DecoderPlugin *decoderplugin,
				const char *meta_description,
				const char *short_name,
				const char *pretty_name,
				const char *default_mapping /* = 0 */)
{
	return gAddOnManager->PublishDecoderCallback(decoderplugin,
												 meta_description,
												 short_name,
												 pretty_name,
												 default_mapping);
}

bool
operator==(const media_meta_description & a, const media_meta_description & b)
{
	return 0 == strcmp(a.description, b.description);
}
