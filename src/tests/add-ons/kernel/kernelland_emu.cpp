// kernelland_emu.cpp

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
#undef TRACE
#endif
#define TRACE(x)
//#define TRACE(x) printf x

static const char *gModuleDirs[] = {
	"distro/x86.R1/beos/system/add-ons/userland",
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

	void Get();
	bool Put();

	module_info **ModuleInfos() const { return fInfos; }
	module_info *FindModuleInfo(const char *name) const;

private:
	image_id	fAddOn;
	module_info	**fInfos;
	int32		fReferenceCount;
	BString		fName;
};

// constructor
ModuleAddOn::ModuleAddOn()
	: fAddOn(-1),
	  fInfos(NULL),
	  fReferenceCount(0)
{
}

// destructor
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
void
ModuleAddOn::Get()
{
	if (fAddOn >= 0)
		fReferenceCount++;
}

// Put
bool
ModuleAddOn::Put()
{
	if (fAddOn >= 0)
		fReferenceCount--;
	return (fReferenceCount == 0);
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


// Module

class Module {
public:
	Module(ModuleAddOn *addon, module_info *info);
	~Module();

	status_t Init();
	status_t Uninit();

	void Get();
	bool Put();

	ModuleAddOn *AddOn() const	{ return fAddOn; }
	module_info *Info() const	{ return fInfo; }

private:
	ModuleAddOn	*fAddOn;
	module_info	*fInfo;
	int32		fReferenceCount;
	bool		fInitialized;
};

// constructor
Module::Module(ModuleAddOn *addon, module_info *info)
	: fAddOn(addon),
	  fInfo(info),
	  fReferenceCount(0),
	  fInitialized(false)
{
	Init();
}

// destructor
Module::~Module()
{
	Uninit();
}

// Init
status_t
Module::Init()
{
	status_t error = (fInfo ? B_OK : B_NO_INIT);
	if (error == B_OK && !fInitialized) {
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
		error = fInfo->std_ops(B_MODULE_UNINIT);
		fInitialized = false;
	}
	return error;
}

// Get
void
Module::Get()
{
	if (fAddOn >= 0)
		fReferenceCount++;
}

// Put
bool
Module::Put()
{
	if (fAddOn >= 0)
		fReferenceCount--;
	return (fReferenceCount == 0 && !(fInfo->flags & B_KEEP_LOADED));
}


// ModuleList

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

// constructor
ModuleList::ModuleList()
{
}

// destructor
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


// ModuleManager

class ModuleManager {
public:
	ModuleManager();
	~ModuleManager();

	static ModuleManager *Default() { return &fDefaultManager; }

	status_t GetModule(const char *path, module_info **infop);
	status_t PutModule(const char *path);

	status_t GetNextLoadedModuleName(uint32 *cookie, char *buffer,
									 size_t *bufferSize);

	module_name_list *OpenModuleList(const char *prefix);
	status_t ReadNextModuleName(module_name_list *list, char *buffer,
								size_t *bufferSize);
	status_t CloseModuleList(module_name_list *list);

private:
	void _FindModules(BDirectory &dir, const char *moduleDir,
					  module_name_list *list);

	status_t _GetAddOn(const char *path, ModuleAddOn **addon);
	void _PutAddOn(ModuleAddOn *addon);

private:
	static ModuleManager		fDefaultManager;
	ModuleList					fModules;
	BObjectList<ModuleAddOn>	fAddOns;
};

// constructor
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

// GetModule
status_t
ModuleManager::GetModule(const char *path, module_info **infop)
{
	status_t error = (path && infop ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BAutolock _lock(fModules);
		Module *module = fModules.FindModule(path);
		if (!module) {
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
		if (error == B_OK) {
			module->Get();
			*infop = module->Info();
		}
	}
	return error;
}

// PutModule
status_t
ModuleManager::PutModule(const char *path)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BAutolock _lock(fModules);
		if (Module *module = fModules.FindModule(path)) {
			if (module->Put()) {
				ModuleAddOn *addon = module->AddOn();
				fModules.RemoveModule(module);
				delete module;
				_PutAddOn(addon);
			}
		} else
			error = B_BAD_VALUE;
	}
	return error;
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
ModuleManager::OpenModuleList(const char *prefix)
{
	module_name_list *list = NULL;
	if (prefix) {
		list = new module_name_list;
		for (int32 i = 0; gModuleDirs[i]; i++) {
			BPath path;
			BDirectory dir;
			if (path.SetTo(gModuleDirs[i], prefix) == B_OK
				&& dir.SetTo(path.Path()) == B_OK) {
				_FindModules(dir, gModuleDirs[i], list);
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

// _FindModules
void
ModuleManager::_FindModules(BDirectory &dir, const char *moduleDir,
							module_name_list *list)
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
					if (infos[i]->name)
						list->names.insert(infos[i]->name);
				}
			}
		} else if (entry.IsDirectory()) {
			BDirectory subdir;
			if (subdir.SetTo(&entry) == B_OK)
				_FindModules(subdir, moduleDir, list);
		}
	}
}

// _GetAddOn
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
							fAddOns.AddItem(addon);
							addon->Get();
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
ModuleManager ModuleManager::fDefaultManager;


// the functions to be emulated follow

// get_module
status_t
get_module(const char *path, module_info **vec)
{
	TRACE(("get_module(`%s')\n", path));
	return ModuleManager::Default()->GetModule(path, vec);
}

// put_module
status_t
put_module(const char *path)
{
	TRACE(("put_module(`%s')\n", path));
	return ModuleManager::Default()->PutModule(path);
}

// get_next_loaded_module_name
status_t
get_next_loaded_module_name(uint32 *cookie, char *buf, size_t *bufsize)
{
	TRACE(("get_next_loaded_module_name(%lu)\n", *cookie));
	return ModuleManager::Default()->GetNextLoadedModuleName(cookie, buf,
															 bufsize);
}

// open_module_list
void *
open_module_list(const char *prefix)
{
	TRACE(("open_module_list(`%s')\n", prefix));
	return (void*)ModuleManager::Default()->OpenModuleList(prefix);
}

// read_next_module_name
status_t
read_next_module_name(void *cookie, char *buf, size_t *bufsize)
{
	TRACE(("read_next_module_name(%p, %p, %lu)\n", cookie, buf, *bufsize));
	return ModuleManager::Default()->ReadNextModuleName(
		(module_name_list*)cookie, buf, bufsize);
}

// close_module_list
status_t
close_module_list(void *cookie)
{
	TRACE(("close_module_list(%p)\n", cookie));
	return ModuleManager::Default()->CloseModuleList(
		(module_name_list*)cookie);
}

// dprintf
void
dprintf(const char *format,...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

extern "C" int user_memcpy(void *to, const void *from, size_t size);
extern "C" int user_strlcpy(char *to, const char *from, size_t size);

// user_memcpy
int
user_memcpy(void *to, const void *from, size_t size)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	while (size--)
		*tmp++ = *s++;

	return 0;
}

// user_strlcpy
/*!	\brief Copies at most (\a size - 1) characters from the string in \a from to
	the string in \a to, NULL-terminating the result.

	\param to Pointer to the destination C-string.
	\param from Pointer to the source C-string.
	\param size Size in bytes of the string buffer pointed to by \a to.
	
	\return strlen(\a from).
*/
int
user_strlcpy(char *to, const char *from, size_t size)
{
	int from_length = 0;

	if (size > 0) {
		to[--size] = '\0';
		// copy 
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	return from_length;
}

