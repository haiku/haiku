/* 
 * Copyright 2003, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Application.h>
#include <Autolock.h>
#include <string.h>
#include <storage/FindDirectory.h>
#include <storage/Path.h>
#include "MMediaFilesManager.h"
#include "debug.h"


MMediaFilesManager::MMediaFilesManager()
	: fLocker(new BLocker("media files manager locker")), 
	fRegistryMap(new Map<BString, Map<BString, entry_ref> >),
	fRunner(NULL)
{
	CALLED();
	LoadState();
#if DEBUG >=3
	Dump();
#endif
}

MMediaFilesManager::~MMediaFilesManager()
{
	CALLED();
	delete fRunner;
	delete fRegistryMap;
	delete fLocker;
}


int32 
MMediaFilesManager::ReadPascalString(BFile &file, char **str)
{
	uint32 len;
	*str=NULL;
	if (file.Read(&len, 4) < 4)
	        return -1;
	if (len == 0)
	        return 0;
	*str = (char *)malloc(len);
	if (file.Read(*str, len) < (int32)len) {
	        free(*str);
	        *str = NULL;
	        return -1;
	}
	return (int32)len;
}


int32 
MMediaFilesManager::WritePascalString(BFile &file, const char *str)
{
		
	if(str == NULL)
	return -1;
	uint32 len = strlen(str) + 1;
	if (file.Write(&len, 4) < 4)
	        return -1;
	if (len == 0)
	        return 0;
	if (file.Write(str, len) < (int32)len) {
	        return -1;
	}
	return (int32)len;
}


// this is called by the media_server *before* any add-ons have been loaded
status_t
MMediaFilesManager::LoadState()
{
	CALLED();
	status_t err = B_OK;
	BPath path;
	if((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path))!=B_OK)
		return err;
		
	path.Append("Media/MMediaFilesManager");
	
	BFile file(path.Path(), B_READ_ONLY);
		
	uint32 category_count;
    if (file.Read(header, sizeof(uint32)*3) < (int32)sizeof(uint32)*3) {
    	header[0] = 0xac00150c;
		header[1] = 0x18723462;
		header[2] = 0x00000001;
		return B_ERROR;
	}
	TRACE("0x%08lx %ld\n", header[0], header[0]);
	TRACE("0x%08lx %ld\n", header[1], header[1]);
	TRACE("0x%08lx %ld\n", header[2], header[2]);
	if (file.Read(&category_count, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;
	while (category_count--) {
		char *str;
		char *key, *val;
		int32 len;
		len = ReadPascalString(file, &str);
		if (len < 0)
			return B_ERROR;
		if (len == 0)
			break;
		TRACE("%s {\n", str);
		do {
	        int32 vol = 0;
	        len = ReadPascalString(file, &key);
	        if (len < 0)
	                return B_ERROR;
	        if (len == 0)
	                break;
	        len = ReadPascalString(file, &val);
	        if (len == 1) {
	                free(val);
	                val = strdup("(null)");
	        }
	        /*if (file.Read(&vol, sizeof(uint32)) < (int32)sizeof(uint32))
	                return B_ERROR;*/
	        TRACE("        %s: %s, volume: %f\n", key, val, *(float *)&vol);
	        
	        entry_ref ref;
	        if(len>1) {
	        	BEntry entry(val);
	        	if(entry.Exists())
	        		entry.GetRef(&ref);
	        }
	        SetRefFor(str, key, ref, false);
	        
	        free(key);
	        free(val);
		} while (true);
		TRACE("}\n");
		free(str);
	}
	
	return B_OK;
}


status_t
MMediaFilesManager::SaveState()
{
	CALLED();
	status_t err = B_OK;
	BPath path;
	if((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path))!=B_OK)
		return err;
		
	path.Append("Media/MMediaFilesManager");
	
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	uint32 zero = 0;
	
	if (file.Write(header, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
	uint32 category_count = fRegistryMap->CountItems();
    if (file.Write(&category_count, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;
		
	BString *type;
	Map<BString, entry_ref> *map;
	BString *item;
	entry_ref *ref;
	for (fRegistryMap->Rewind(); fRegistryMap->GetNext(&map); ) {
		fRegistryMap->GetCurrentKey(&type);
	
		WritePascalString(file, type->String());
		
		for (map->Rewind(); map->GetNext(&ref);) {
			map->GetCurrentKey(&item);
			BPath path(ref);
			
			WritePascalString(file, item->String());
			WritePascalString(file, path.Path() ? path.Path() : "");
						
		}
		
		file.Write(&zero, sizeof(uint32));
	}
	file.Write(&zero, sizeof(uint32));
	
	printf("save state ok\n");
	
	return B_OK;
}


void
MMediaFilesManager::Dump()
{
	BAutolock lock(fLocker);
	printf("\n");
	
	/* for each type, the registry map contains a map of item/entry_ref
	 */
	printf("MMediaFilesManager: registry map follows\n");
	BString *type;
	Map<BString, entry_ref> *map;
	BString *item;
	entry_ref *ref;
	for (fRegistryMap->Rewind(); fRegistryMap->GetNext(&map); ) {
		fRegistryMap->GetCurrentKey(&type);
		
		for (map->Rewind(); map->GetNext(&ref);) {
			map->GetCurrentKey(&item);
			BPath path(ref);
			printf(" type \"%s\", item \"%s\", path \"%s\"\n", 
				type->String(), item->String(), (path.InitCheck() == B_OK) ? path.Path() : "INVALID");	
		}
	}
	printf("MMediaFilesManager: list end\n");
	printf("\n");

}


status_t
MMediaFilesManager::RewindTypes(BString ***types, int32 *count)
{
	CALLED();
	if(types==NULL || count == NULL)
		return B_BAD_VALUE;
	
	Map<BString, entry_ref> *map;
	BString *type;
	
	*count = fRegistryMap->CountItems();
	*types = new (BString*)[*count];
	int32 i=0; 
	
	for (fRegistryMap->Rewind(); i<*count && fRegistryMap->GetNext(&map); i++) {
		fRegistryMap->GetCurrentKey(&type);
		(*types)[i] = type;		
	}
	
	return B_OK;
}


status_t
MMediaFilesManager::RewindRefs(const char* type, BString ***items, int32 *count)
{
	CALLED();
	if(type == NULL || items==NULL || count == NULL)
		return B_BAD_VALUE;
	
	Map<BString, entry_ref> *map;
	entry_ref *ref;
	BString *item;
	
	fRegistryMap->Get(BString(type), &map);
	
	*count = map->CountItems();
	*items = new (BString*)[*count];
	int32 i=0;
			
	for (map->Rewind(); i<*count && map->GetNext(&ref); i++) {
		map->GetCurrentKey(&item);
		(*items)[i] = item;	
	}
	
	return B_OK;
}


status_t
MMediaFilesManager::GetRefFor(const char *type,
					   const char *item,
					   entry_ref **out_ref)
{
	CALLED();
	Map <BString, entry_ref> *map;
	if(!fRegistryMap->Get(BString(type), &map))
		return B_ENTRY_NOT_FOUND;

	if(!map->Get(BString(item), out_ref))
		return B_ENTRY_NOT_FOUND;
		
	return B_OK;
}


status_t
MMediaFilesManager::SetRefFor(const char *type,
					   const char *item,
					   const entry_ref &ref, bool save)
{
	CALLED();
	TRACE("MMediaFilesManager::SetRefFor %s %s\n", type, item);
	
	BString itemString(item);
	itemString.Truncate(B_MEDIA_NAME_LENGTH);
	BString typeString(type);
	Map <BString, entry_ref> *map;
	if(!fRegistryMap->Get(typeString, &map)) {
		map = new Map<BString, entry_ref>;
		fRegistryMap->Insert(typeString, *map);
		fRegistryMap->Get(typeString, &map);	
	}
	
	if(map->Has(itemString))
		map->Remove(itemString);
	map->Insert(itemString, ref);
	if(save)
		LaunchTimer();
	
	return B_OK;
}


status_t
MMediaFilesManager::RemoveRefFor(const char *type,
						  const char *item,
						  const entry_ref &ref)
{
	CALLED();
	BString itemString(item);
	BString typeString(type);
	Map <BString, entry_ref> *map;
	if(fRegistryMap->Get(typeString, &map)) {
		map->Remove(itemString);
		map->Insert(itemString, *(new entry_ref));
		LaunchTimer();
	}
	return B_OK;
}


status_t
MMediaFilesManager::RemoveItem(const char *type,
						const char *item)
{
	CALLED();
	BString itemString(item);
	BString typeString(type);
	Map <BString, entry_ref> *map;
	if(fRegistryMap->Get(typeString, &map)) {
		map->Remove(itemString);
		LaunchTimer();
	}
	return B_OK;
}

void
MMediaFilesManager::LaunchTimer()
{
	if(!fRunner)
		fRunner = new BMessageRunner(be_app, 
			new BMessage(MMEDIAFILESMANAGER_SAVE_TIMER), 3 * 1000000, 1);	
}

void
MMediaFilesManager::TimerMessage()
{
	SaveState();
	delete fRunner;
	fRunner = NULL;	
}