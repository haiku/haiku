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
#include <Path.h>

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


// Module

class Module {
public:
	Module();
	~Module();

	status_t Init(const char *name, bool isPath = false);
	void Unset();

	void Get();
	bool Put();

	module_info *Info() const { return fInfo; }

private:
	bool _Load(const char *path, const char *name, bool isPath);

private:
	image_id	fAddOn;
	module_info	*fInfo;
	int32		fReferenceCount;
	bool		fInitialized;
};

// constructor
Module::Module()
	: fAddOn(-1),
	  fInfo(NULL),
	  fReferenceCount(0),
	  fInitialized(false)
{
}

// constructor
Module::~Module()
{
	Unset();
}

// Init
status_t
Module::Init(const char *name, bool isPath)
{
TRACE(("Module::Init(): searching module `%s'...\n", name));
	Unset();
	status_t error = (name ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = B_ENTRY_NOT_FOUND;
		if (isPath) {
			// name is a path name: try to load it
			if (_Load(name, name, isPath))
				error = B_OK;
		} else {
			// name is a relative module name: search in the module dirs
			for (int32 i = 0; gModuleDirs[i]; i++) {
TRACE(("Module::Init(): ...in `%s'\n", gModuleDirs[i]));
				BPath path;
				if (path.SetTo(gModuleDirs[i]) == B_OK
					&& path.SetTo(path.Path(), name) == B_OK) {
					if (_Load(path.Path(), name, isPath)) {
						error = B_OK;
						break;
					}
				}
			}
		}
	}
	return error;
}

// Unset
void
Module::Unset()
{
	if (fAddOn >= 0) {
		if (fInfo)
			fInfo->std_ops(B_MODULE_UNINIT);
		unload_add_on(fAddOn);
	}
	fAddOn = -1;
	fInfo = NULL;
	fReferenceCount = 0;
	fInitialized = false;
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

// _Load
bool
Module::_Load(const char *path, const char *name, bool isPath)
{
	TRACE(("Module::_Load(): trying to load `%s'\n", path));
	BEntry entry;
	if (entry.SetTo(path) == B_OK && entry.Exists()) {
		image_id image = load_add_on(path);
		module_info **infos = NULL;
		if (image >= 0
			&& get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
								(void**)&infos) == B_OK
			&& infos != NULL) {
			for (int32 i = 0; module_info *info = infos[i]; i++) {
				if ((isPath || !strcmp(name, info->name))
					&& info->std_ops(B_MODULE_INIT) == B_OK) {
					fAddOn = image;
					fInfo = info;
					fReferenceCount = 0;
					return true;
				}
			}
		}
	} else if (!isPath) {
		// entry does not exist -- try loading the parent path
		BPath parentPath;
		if (BPath(path).GetParent(&parentPath) == B_OK)
			return _Load(parentPath.Path(), name, isPath);
	}
	return false;
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
	void _FindModules(BDirectory &dir, module_name_list *list);

private:
	static ModuleManager	fDefaultManager;
	ModuleList				fModules;
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
			module = new Module;
			error = module->Init(path);
			if (error == B_OK && !fModules.AddModule(module))
				error = B_NO_MEMORY;
			if (error == B_OK) {
				module->Get();
				*infop = module->Info();
			} else
				delete module;
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
				fModules.RemoveModule(module);
				delete module;
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
				_FindModules(dir, list);
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
ModuleManager::_FindModules(BDirectory &dir, module_name_list *list)
{
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (entry.IsFile()) {
			Module module;
			BPath path;
			if (entry.GetPath(&path) == B_OK
				&& module.Init(path.Path(), true) == B_OK) {
				list->names.insert(module.Info()->name);
			}
		} else if (entry.IsDirectory()) {
			BDirectory subdir;
			if (subdir.SetTo(&entry) == B_OK)
				_FindModules(subdir, list);
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

