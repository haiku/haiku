/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <set>
#include <stdio.h>
#include <string>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <KernelExport.h>
#include <module.h>
#include <List.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Path.h>
#include <String.h>

#ifdef TRACE
#	undef TRACE
#endif
#define TRACE(x)
//#define TRACE(x) printf x


static const char *gModuleDirs[] = {
	"generated/objects/haiku/x86/release/add-ons/userland",
	"generated/objects/haiku/x86/release/tests/add-ons/kernel",
	NULL
};


struct module_name_list {
	set<string>				names;
	set<string>::iterator	it;
};


// ModuleAddOn

class ModuleAddOn {
public:
	ModuleAddOn();
	~ModuleAddOn();

	status_t Load(const char *path, const char *dirPath);
	void Unload();

	const char *Name()	{ return fName.String(); }

	status_t Get();
	bool Put();

	module_info **ModuleInfos() const { return fInfos; }
	module_info *FindModuleInfo(const char *name) const;

private:
	image_id	fAddOn;
	module_info	**fInfos;
	int32		fReferenceCount;
	BString		fName;
};

class Module {
public:
	Module(ModuleAddOn *addon, module_info *info);
	~Module();

	status_t Init();
	status_t Uninit();

	status_t Get();
	bool Put();

	ModuleAddOn *AddOn() const	{ return fAddOn; }
	module_info *Info() const	{ return fInfo; }

private:
	ModuleAddOn	*fAddOn;
	module_info	*fInfo;
	int32		fReferenceCount;
	bool		fInitialized;
};

class ModuleList : public BLocker {
public:
	ModuleList();
	~ModuleList();

	int32 CountModules() const;
	Module *ModuleAt(int32 index) const;

	bool AddModule(Module *module);
	bool RemoveModule(Module *module);
	Module *FindModule(const char *path);

private:
	BList	fModules;
};

class ModuleManager {
public:
	ModuleManager();
	~ModuleManager();

	static ModuleManager *Default() { return &sDefaultManager; }

	status_t GetModule(const char *path, module_info **infop);
	status_t PutModule(const char *path);

	status_t GetNextLoadedModuleName(uint32 *cookie, char *buffer,
		size_t *bufferSize);

	module_name_list *OpenModuleList(const char *prefix,
		const char *suffix = NULL);
	status_t ReadNextModuleName(module_name_list *list, char *buffer,
		size_t *bufferSize);
	status_t CloseModuleList(module_name_list *list);

	status_t AddBuiltInModule(module_info *info);

	status_t GetDependencies(image_id image);
	void PutDependencies(image_id image);

private:
	bool _MatchSuffix(const char *name, const char *suffix);
	void _FindModules(BDirectory &dir, const char *moduleDir,
		const char *suffix, module_name_list *list);
	void _FindBuiltInModules(const char *prefix, const char *suffix,
		module_name_list *list);

	status_t _GetAddOn(const char *path, ModuleAddOn **addon);
	void _PutAddOn(ModuleAddOn *addon);

private:
	static ModuleManager		sDefaultManager;
	ModuleList					fModules;
	BObjectList<ModuleAddOn>	fAddOns;
};


//	#pragma mark - ModuleAddOn


ModuleAddOn::ModuleAddOn()
	: fAddOn(-1),
	  fInfos(NULL),
	  fReferenceCount(0)
{
}


ModuleAddOn::~ModuleAddOn()
{
	Unload();
}

// Load
status_t
ModuleAddOn::Load(const char *path, const char *dirPath)
{
	TRACE(("ModuleAddOn::Load(): searching module `%s'...\n", path));
	Unload();
	status_t error = (path && dirPath ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// get the module dir relative path
		BPath absPath;
		BPath absDirPath;
		if (absPath.SetTo(path, NULL, true) != B_OK
			|| absDirPath.SetTo(dirPath, NULL, true) != B_OK
			|| strlen(absPath.Path()) <= strlen(absDirPath.Path())) {
			return B_ENTRY_NOT_FOUND;
		}
		int32 dirPathLen = strlen(absDirPath.Path());
		if (strncmp(absPath.Path(), absDirPath.Path(), dirPathLen)
			|| absPath.Path()[dirPathLen] != '/') {
			return B_ENTRY_NOT_FOUND;
		}
		const char *name = absPath.Path() + dirPathLen + 1;
		// load the file
		error = B_ENTRY_NOT_FOUND;
		BEntry entry;
		if (entry.SetTo(path) == B_OK && entry.Exists()) {
			image_id image = load_add_on(path);
			module_info **infos = NULL;
			if (image >= 0
				&& get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
									(void**)&infos) == B_OK
				&& infos != NULL) {
				fAddOn = image;
				fInfos = infos;
				fName = name;
				fReferenceCount = 0;
				error = B_OK;
			}
		}
	}
	return error;
}

// Unload
void
ModuleAddOn::Unload()
{
	if (fAddOn >= 0)
		unload_add_on(fAddOn);
	fAddOn = -1;
	fInfos = NULL;
	fReferenceCount = 0;
}

// Get
status_t
ModuleAddOn::Get()
{
	if (fAddOn >= 0) {
		if (fReferenceCount == 0) {
			status_t status = ModuleManager::Default()->GetDependencies(fAddOn);
			if (status < B_OK)
				return status;
		}
		fReferenceCount++;
	}

	return B_OK;
}

// Put
bool
ModuleAddOn::Put()
{
	if (fAddOn >= 0)
		fReferenceCount--;

	if (fReferenceCount == 0) {
		ModuleManager::Default()->PutDependencies(fAddOn);
		return true;
	}
	return false;
}

// FindModuleInfo
module_info *
ModuleAddOn::FindModuleInfo(const char *name) const
{
	if (fInfos && name) {
		for (int32 i = 0; module_info *info = fInfos[i]; i++) {
			if (!strcmp(info->name, name))
				return info;
		}
	}
	return NULL;
}


//	#pragma mark - Module


Module::Module(ModuleAddOn *addon, module_info *info)
	: fAddOn(addon),
	  fInfo(info),
	  fReferenceCount(0),
	  fInitialized(false)
{
}

// destructor
Module::~Module()
{
}

// Init
status_t
Module::Init()
{
	status_t error = (fInfo ? B_OK : B_NO_INIT);
	if (error == B_OK && !fInitialized) {
		if (fInfo->std_ops != NULL)
			error = fInfo->std_ops(B_MODULE_INIT);
		if (error == B_OK)
			fInitialized = true;
	}
	return error;
}

// Uninit
status_t
Module::Uninit()
{
	status_t error = (fInfo ? B_OK : B_NO_INIT);
	if (error == B_OK && fInitialized) {
		if (fInfo->std_ops != NULL)
			error = fInfo->std_ops(B_MODULE_UNINIT);
		fInitialized = false;
	}
	return error;
}


status_t
Module::Get()
{
	if (fReferenceCount == 0) {
		status_t status = Init();
		if (status < B_OK)
			return status;
	}

	fReferenceCount++;
	return B_OK;
}


bool
Module::Put()
{
	if (--fReferenceCount > 0)
		return false;

	Uninit();
	return fAddOn && !(fInfo->flags & B_KEEP_LOADED);
}


//	#pragma mark - ModuleList


ModuleList::ModuleList()
{
}


ModuleList::~ModuleList()
{
}

// CountModules
int32
ModuleList::CountModules() const
{
	return fModules.CountItems();
}

// ModuleAt
Module *
ModuleList::ModuleAt(int32 index) const
{
	return (Module*)fModules.ItemAt(index);
}

// AddModule
bool
ModuleList::AddModule(Module *module)
{
	bool result = false;
	if (module && !FindModule(module->Info()->name))
		result = fModules.AddItem(module);
	return module;
}

// RemoveModule
bool
ModuleList::RemoveModule(Module *module)
{
	return (module && fModules.RemoveItem(module));
}

// FindModule
Module *
ModuleList::FindModule(const char *path)
{
	if (path) {
		for (int32 i = 0; Module *module = ModuleAt(i); i++) {
			if (!strcmp(path, module->Info()->name))
				return module;
		}
	}
	return NULL;
}


//	#pragma mark - ModuleManager


ModuleManager::ModuleManager()
	: fModules()
{
}

// destructor
ModuleManager::~ModuleManager()
{
	for (int32 i = 0; Module *module = fModules.ModuleAt(i); i++)
		delete module;
}


status_t
ModuleManager::GetModule(const char *path, module_info **_info)
{
	if (path == NULL || _info == NULL)
		return B_BAD_VALUE;

	BAutolock _lock(fModules);
	status_t error = B_OK;

	Module *module = fModules.FindModule(path);
	if (module == NULL) {
		// module not yet loaded, try to get it
		// get the responsible add-on
		ModuleAddOn *addon = NULL;
		error = _GetAddOn(path, &addon);
		if (error == B_OK) {
			// add-on found, get the module
			if (module_info *info = addon->FindModuleInfo(path)) {
				module = new Module(addon, info);
				fModules.AddModule(module);
			} else {
				_PutAddOn(addon);
				error = B_ENTRY_NOT_FOUND;
			}
		}
	}

	// "get" the module
	if (error == B_OK)
		error = module->Get();
	if (error == B_OK)
		*_info = module->Info();

	return error;
}

// PutModule
status_t
ModuleManager::PutModule(const char *path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	BAutolock _lock(fModules);

	if (Module *module = fModules.FindModule(path)) {
		if (module->Put()) {
			ModuleAddOn *addon = module->AddOn();
			fModules.RemoveModule(module);
			delete module;
			_PutAddOn(addon);
		}
	} else
		return B_BAD_VALUE;

	return B_OK;
}

// GetNextLoadedModuleName
status_t
ModuleManager::GetNextLoadedModuleName(uint32 *cookie, char *buffer,
	size_t *bufferSize)
{
	status_t error = (cookie && buffer && bufferSize ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BAutolock _lock(fModules);
		if (Module *module = fModules.ModuleAt(*cookie)) {
			module_info *info = module->Info();
			size_t nameLen = strlen(info->name);
			if (nameLen < *bufferSize) {
				strcpy(buffer, info->name);
				*bufferSize = nameLen;
				(*cookie)++;
			} else
				error = B_BAD_VALUE;
		} else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// OpenModuleList
module_name_list *
ModuleManager::OpenModuleList(const char *prefix, const char *suffix)
{
	module_name_list *list = NULL;
	if (prefix) {
		list = new module_name_list;
		_FindBuiltInModules(prefix, suffix, list);

		for (int32 i = 0; gModuleDirs[i]; i++) {
			BPath path;
			BDirectory dir;
			if (path.SetTo(gModuleDirs[i], prefix) == B_OK
				&& dir.SetTo(path.Path()) == B_OK) {
				_FindModules(dir, gModuleDirs[i], suffix, list);
			}
		}

		list->it = list->names.begin();
	}
	return list;
}

// ReadNextModuleName
status_t
ModuleManager::ReadNextModuleName(module_name_list *list, char *buffer,
	size_t *bufferSize)
{
	status_t error = (list && buffer && bufferSize ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (list->it != list->names.end()) {
			const string &name = *list->it;
			size_t nameLen = name.length();
			if (nameLen < *bufferSize) {
				strcpy(buffer, name.c_str());
				*bufferSize = nameLen;
				list->it++;
			} else
				error = B_BAD_VALUE;
		} else
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}


// CloseModuleList
status_t
ModuleManager::CloseModuleList(module_name_list *list)
{
	status_t error = (list ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		delete list;
	return error;
}


status_t
ModuleManager::AddBuiltInModule(module_info *info)
{
	BAutolock _lock(fModules);

	TRACE(("add module %p, \"%s\"\n", info, info->name));
	return fModules.AddModule(new Module(NULL, info)) ? B_OK : B_ERROR;
}


status_t
ModuleManager::GetDependencies(image_id image)
{
	module_dependency *dependencies;
	status_t status = get_image_symbol(image, "module_dependencies",
		B_SYMBOL_TYPE_DATA, (void**)&dependencies);
	if (status < B_OK) {
		// no dependencies means we don't have to do anything
		return B_OK;
	}

	for (uint32 i = 0; dependencies[i].name != NULL; i++) {
		status = GetModule(dependencies[i].name, dependencies[i].info);
		if (status < B_OK)
			return status;
	}
	return B_OK;
}


void
ModuleManager::PutDependencies(image_id image)
{
	module_dependency *dependencies;
	status_t status = get_image_symbol(image, "module_dependencies",
		B_SYMBOL_TYPE_DATA, (void**)&dependencies);
	if (status < B_OK) {
		// no dependencies means we don't have to do anything
		return;
	}

	for (uint32 i = 0; dependencies[i].name != NULL; i++) {
		PutModule(dependencies[i].name);
	}
}


bool
ModuleManager::_MatchSuffix(const char *name, const char *suffix)
{
	if (suffix == NULL || suffix[0] == '\0')
		return true;

	size_t suffixLength = strlen(suffix);
	size_t length = strlen(name);
	if (length <= suffixLength)
		return false;

	return name[length - suffixLength - 1] == '/'
		&& !strcmp(name + length - suffixLength, suffix);
}


void
ModuleManager::_FindModules(BDirectory &dir, const char *moduleDir,
	const char *suffix, module_name_list *list)
{
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (entry.IsFile()) {
			ModuleAddOn addon;
			BPath path;
			if (entry.GetPath(&path) == B_OK
				&& addon.Load(path.Path(), moduleDir) == B_OK) {
				module_info **infos = addon.ModuleInfos();
				for (int32 i = 0; infos[i]; i++) {
					if (infos[i]->name
						&& _MatchSuffix(infos[i]->name, suffix))
						list->names.insert(infos[i]->name);
				}
			}
		} else if (entry.IsDirectory()) {
			BDirectory subdir;
			if (subdir.SetTo(&entry) == B_OK)
				_FindModules(subdir, moduleDir, suffix, list);
		}
	}
}


void
ModuleManager::_FindBuiltInModules(const char *prefix, const char *suffix,
	module_name_list *list)
{
	uint32 count = fModules.CountModules();
	uint32 prefixLength = strlen(prefix);

	for (uint32 i = 0; i < count; i++) {
		Module *module = fModules.ModuleAt(i);
		if (!strncmp(module->Info()->name, prefix, prefixLength)
			&& _MatchSuffix(module->Info()->name, suffix))
			list->names.insert(module->Info()->name);
	}
}


status_t
ModuleManager::_GetAddOn(const char *name, ModuleAddOn **_addon)
{
	// search list first
	for (int32 i = 0; ModuleAddOn *addon = fAddOns.ItemAt(i); i++) {
		BString addonName(addon->Name());
		addonName << "/";
		if (!strcmp(name, addon->Name())
			|| !strncmp(addonName.String(), name, addonName.Length())) {
			addon->Get();
			*_addon = addon;
			return B_OK;
		}
	}
	// not in list yet, load from disk
	// iterate through module dirs
	for (int32 i = 0; gModuleDirs[i]; i++) {
		BPath path;
		if (path.SetTo(gModuleDirs[i]) == B_OK
			&& path.SetTo(path.Path(), name) == B_OK) {
			BEntry entry;
			for (;;) {
				if (entry.SetTo(path.Path()) == B_OK && entry.Exists()) {
					// found an entry: if it is a file, try to load it
					if (entry.IsFile()) {
						ModuleAddOn *addon = new ModuleAddOn;
						if (addon->Load(path.Path(), gModuleDirs[i]) == B_OK) {
							status_t status = addon->Get();
							if (status < B_OK) {
								delete addon;
								return status;
							}

							fAddOns.AddItem(addon);
							*_addon = addon;
							return B_OK;
						}
						delete addon;
					}
					break;
				}
				// chop off last path component
				if (path.GetParent(&path) != B_OK)
					break;
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _PutAddOn
void
ModuleManager::_PutAddOn(ModuleAddOn *addon)
{
	if (addon) {
		if (addon->Put()) {
			fAddOns.RemoveItem(addon);
			delete addon;
		}
	}
}


// singleton instance
ModuleManager ModuleManager::sDefaultManager;


//	#pragma mark - Private emulation functions


extern "C" status_t
_add_builtin_module(module_info *info)
{
	return ModuleManager::Default()->AddBuiltInModule(info);
}


extern "C" status_t
_get_builtin_dependencies(void)
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.type != B_APP_IMAGE)
			continue;

		return ModuleManager::Default()->GetDependencies(info.id);
	}

	return B_OK;
}


//	#pragma mark - Emulated kernel functions


status_t
get_module(const char *path, module_info **_info)
{
	TRACE(("get_module(`%s')\n", path));
	return ModuleManager::Default()->GetModule(path, _info);
}


status_t
put_module(const char *path)
{
	TRACE(("put_module(`%s')\n", path));
	return ModuleManager::Default()->PutModule(path);
}


status_t
get_next_loaded_module_name(uint32 *cookie, char *name, size_t *nameLength)
{
	TRACE(("get_next_loaded_module_name(%lu)\n", *cookie));
	return ModuleManager::Default()->GetNextLoadedModuleName(cookie, name,
		nameLength);
}


void *
open_module_list_etc(const char *prefix, const char *suffix)
{
	TRACE(("open_module_list_etc('%s', '%s')\n", prefix, suffix));
	return (void*)ModuleManager::Default()->OpenModuleList(prefix, suffix);
}


void *
open_module_list(const char *prefix)
{
	TRACE(("open_module_list('%s')\n", prefix));
	return (void*)ModuleManager::Default()->OpenModuleList(prefix);
}


status_t
read_next_module_name(void *cookie, char *buf, size_t *bufsize)
{
	TRACE(("read_next_module_name(%p, %p, %lu)\n", cookie, buf, *bufsize));
	return ModuleManager::Default()->ReadNextModuleName(
		(module_name_list*)cookie, buf, bufsize);
}


status_t
close_module_list(void *cookie)
{
	TRACE(("close_module_list(%p)\n", cookie));
	return ModuleManager::Default()->CloseModuleList(
		(module_name_list*)cookie);
}
