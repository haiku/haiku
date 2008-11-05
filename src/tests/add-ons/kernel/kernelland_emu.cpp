/*
 * Copyright 2002-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <set>
#include <signal.h>
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

#ifdef ASSERT
#	undef ASSERT
#endif

#include <condition_variable.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <fs/devfs.h>
#include <kscheduler.h>
#include <slab/Slab.h>

#ifdef TRACE
#	undef TRACE
#endif
#define TRACE(x)
//#define TRACE(x) printf x

#define RW_MAX_READERS 10000

static const char *gModuleDirs[] = {
	"distro/x86.R1/beos/system/add-ons/userland",
	NULL
};

bool gDebugOutputEnabled = true;

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


thread_id
spawn_kernel_thread(thread_func func, const char *name, int32 priority,
	void *data)
{
	return spawn_thread(func, name, priority, data);
}


extern "C" status_t
devfs_unpublish_partition(const char *path)
{
	printf("unpublish partition: %s\n", path);
	return B_OK;
}


extern "C" status_t
devfs_publish_partition(const char *path, const partition_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	printf("publish partition: %s (device \"%s\", size %Ld)\n", path, info->device, info->size);
	return B_OK;
}


extern "C" int32
atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst)
{
#if __INTEL__
	int32 oldValue;
	asm volatile("lock; cmpxchg %%ecx, (%%edx)"
		: "=a" (oldValue) : "a" (testAgainst), "c" (newValue), "d" (value));
	return oldValue;
#else
#warn "atomic_test_and_set() won't work correctly!"
	int32 oldValue = *value;
	if (oldValue == testAgainst)
		*value = newValue;

	return oldValue;
#endif
}


extern "C" int
send_signal_etc(pid_t thread, uint signal, uint32 flags)
{
	return send_signal(thread, signal);
}


extern "C" int32
low_resource_state(uint32 resources)
{
	return B_NO_LOW_RESOURCE;
}


extern "C" void
low_resource(uint32 resource, uint64 requirements, uint32 flags,
	uint32 timeout)
{
}


extern "C" status_t
register_low_resource_handler(low_resource_func function, void *data,
	uint32 resources, int32 priority)
{
	return B_OK;
}


extern "C" status_t
unregister_low_resource_handler(low_resource_func function, void *data)
{
	return B_OK;
}


extern "C" void
panic(const char *format, ...)
{
	va_list args;

	puts("*** PANIC ***");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	putchar('\n');
	debugger("Kernel Panic");
}


extern "C" void
dprintf(const char *format,...)
{
	if (!gDebugOutputEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("\33[34m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
kprintf(const char *format,...)
{
	va_list args;
	va_start(args, format);
	printf("\33[35m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
dump_block(const char *buffer, int size, const char *prefix)
{
	const int DUMPED_BLOCK_SIZE = 16;
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		dprintf(prefix);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				dprintf(" ");

			if (i >= size)
				dprintf("  ");
			else
				dprintf("%02x", *(unsigned char *)(buffer + i));
		}
		dprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					dprintf(".");
				else
					dprintf("%c", c);
			} else
				break;
		}
		dprintf("\n");
	}
}


extern "C" status_t
user_memcpy(void *to, const void *from, size_t size)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	while (size--)
		*tmp++ = *s++;

	return 0;
}


extern "C" int
user_strcpy(char *to, const char *from)
{
	while ((*to++ = *from++) != '\0')
		;
	return 0;
}


/*!	\brief Copies at most (\a size - 1) characters from the string in \a from to
	the string in \a to, NULL-terminating the result.

	\param to Pointer to the destination C-string.
	\param from Pointer to the source C-string.
	\param size Size in bytes of the string buffer pointed to by \a to.
	
	\return strlen(\a from).
*/
extern "C" ssize_t 
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


//	#pragma mark - Debugger


extern "C" uint64
parse_expression(const char* arg)
{
	return strtoull(arg, NULL, 0);
}


extern "C" bool
set_debug_variable(const char* variableName, uint64 value)
{
	return true;
}


extern "C" bool
print_debugger_command_usage(const char* command)
{
	return true;
}


extern "C" status_t
add_debugger_command_etc(const char* name, debugger_command_hook func,
	const char* description, const char* usage, uint32 flags)
{
	return B_OK;
}


extern "C" int
add_debugger_command(char *name, int (*func)(int, char **), char *desc)
{
	return B_OK;
}


extern "C" int
remove_debugger_command(char * name, int (*func)(int, char **))
{
	return B_OK;
}


//	#pragma mark - Slab allocator


extern "C" void *
object_cache_alloc(object_cache *cache, uint32 flags)
{
	return malloc((size_t)cache);
}


extern "C" void
object_cache_free(object_cache *cache, void *object)
{
	free(object);
}


extern "C" object_cache *
create_object_cache_etc(const char *name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, uint32 flags, void *cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	return (object_cache*)objectSize;
}


extern "C" object_cache *
create_object_cache(const char *name, size_t objectSize,
	size_t alignment, void *cookie, object_cache_constructor constructor,
	object_cache_destructor)
{
	return (object_cache*)objectSize;
}


extern "C" void
delete_object_cache(object_cache *cache)
{
}


extern "C" void
object_cache_get_usage(object_cache *cache, size_t *_allocatedMemory)
{
	*_allocatedMemory = 100;
}


//	#pragma mark - Thread/scheduler functions


struct scheduler_ops kScheduler = {
	NULL,
	NULL,
	NULL,
	NULL,
};
struct scheduler_ops* gScheduler = &kScheduler;


//	#pragma mark - Private locking functions


int32
recursive_lock_get_recursion(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

#if !KDEBUG
	if (lock->holder == thread)
		return lock->recursion;
#else
	if (lock->lock.holder == thread)
		return lock->recursion;
#endif

	return -1;
}


void
recursive_lock_init_etc(recursive_lock *lock, const char *name, uint32 flags)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "recursive lock";

#if !KDEBUG
	lock->holder = -1;
#else
	lock->lock.holder = -1;
#endif
	lock->recursion = 0;
	lock->lock.waiters = (mutex_waiter*)create_sem(1, name);

	if ((sem_id)lock->lock.waiters < B_OK)
		panic("recursive lock creation failed: %s\n", name);
}


void
recursive_lock_init(recursive_lock *lock, const char *name)
{
	recursive_lock_init_etc(lock, name, 0);
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem((sem_id)lock->lock.waiters);
	lock->lock.waiters = (mutex_waiter*)-1;
}


status_t
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thread = find_thread(NULL);

#if !KDEBUG
	if (thread != lock->holder) {
		status_t status;
		do {
			status = acquire_sem((sem_id)lock->lock.waiters);
		} while (status == B_INTERRUPTED);
		if (status < B_OK)
			return status;

		lock->holder = thread;
	}
#else
	if (thread != lock->lock.holder) {
		status_t status;
		do {
			status = acquire_sem((sem_id)lock->lock.waiters);
		} while (status == B_INTERRUPTED);
		if (status < B_OK)
			return status;

		lock->lock.holder = thread;
	}
#endif
	lock->recursion++;
	return B_OK;
}


void
recursive_lock_unlock(recursive_lock *lock)
{
#if !KDEBUG
	if (find_thread(NULL) != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);
#else
	if (find_thread(NULL) != lock->lock.holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);
#endif

	if (--lock->recursion == 0) {
#if !KDEBUG
		lock->holder = -1;
#else
		lock->lock.holder = -1;
#endif
		release_sem_etc((sem_id)lock->lock.waiters, 1, 0);
	}
}


//	#pragma mark -


void
mutex_init(mutex *m, const char *name)
{
	if (m == NULL)
		return;

	if (name == NULL)
		name = "mutex_sem";

	// We need to store the semaphore in "waiters", as it is no sem anymore
	// Also, kernel mutex creation cannot fail anymore, but we could...
	m->waiters = (struct mutex_waiter *)create_sem(1, name);
	if ((sem_id)m->waiters < B_OK)
		debugger("semaphore creation failed");
}


void
mutex_init_etc(mutex *m, const char *name, uint32 flags)
{
	if (m == NULL)
		return;

	if (name == NULL)
		name = "mutex_sem";

	m->waiters = (struct mutex_waiter *)create_sem(1, name);
	if ((sem_id)m->waiters < B_OK)
		debugger("semaphore creation failed");
}


void
mutex_destroy(mutex *mutex)
{
	if (mutex == NULL)
		return;

	if ((sem_id)mutex->waiters >= 0) {
		delete_sem((sem_id)mutex->waiters);
		mutex->waiters = (struct mutex_waiter *)-1;
	}
}


status_t
_mutex_trylock(mutex *mutex)
{
	status_t status;
	do {
		status = acquire_sem_etc((sem_id)mutex->waiters, 1, B_RELATIVE_TIMEOUT,
			0);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
_mutex_lock(mutex *mutex, bool threadsLocked)
{
	if (mutex->waiters == NULL) {
		// MUTEX_INITIALIZER has been used; this is not thread-safe!
		mutex_init(mutex, mutex->name);
	}

	status_t status;
	do {
		status = acquire_sem((sem_id)mutex->waiters);
	} while (status == B_INTERRUPTED);

#if KDEBUG
	if (status == B_OK)
		mutex->holder = find_thread(NULL);
#endif
	return status;
}


void
_mutex_unlock(mutex *mutex, bool threadsLocked)
{
#if KDEBUG
	mutex->holder = -1;
#endif
	release_sem((sem_id)mutex->waiters);
}


//	#pragma mark -


void
rw_lock_init_etc(rw_lock *lock, const char *name, uint32 flags)
{
	if (lock == NULL)
		return;

	if (name == NULL)
		name = "r/w lock";

	lock->waiters = (rw_lock_waiter*)create_sem(RW_MAX_READERS, name);
	if ((sem_id)lock->waiters < B_OK)
		panic("r/w lock \"%s\" creation failed.", name);
}


void
rw_lock_init(rw_lock *lock, const char *name)
{
	rw_lock_init_etc(lock, name, 0);
}


void
rw_lock_destroy(rw_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem((sem_id)lock->waiters);
}


status_t
rw_lock_read_lock(rw_lock *lock)
{
	status_t status;
	do {
		status = acquire_sem((sem_id)lock->waiters);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
rw_lock_read_unlock(rw_lock *lock)
{
	return release_sem((sem_id)lock->waiters);
}


status_t
rw_lock_write_lock(rw_lock *lock)
{
	status_t status;
	do {
		status = acquire_sem_etc((sem_id)lock->waiters, RW_MAX_READERS, 0, 0);
	} while (status == B_INTERRUPTED);
	return status;
}


status_t
rw_lock_write_unlock(rw_lock *lock)
{
	return release_sem_etc((sem_id)lock->waiters, RW_MAX_READERS, 0);
}


//	#pragma mark - Condition variables (anonymous only)


#define STATUS_ADDED	1
#define STATUS_WAITING	2


struct condition_private {
	mutex		lock;
	sem_id		wait_sem;
	const void*	object;
};


status_t
ConditionVariableEntry::Wait(uint32 flags, bigtime_t timeout)
{
	if (fVariable == NULL)
		return fWaitStatus;

	condition_private* condition = (condition_private*)fVariable->fObject;
	fWaitStatus = STATUS_WAITING;
	
	status_t status;
	do {
		status = acquire_sem_etc(condition->wait_sem, 1, flags, timeout);
	} while (status == B_INTERRUPTED);

	mutex_lock(&condition->lock);

	// remove entry from variable, if not done yet
	if (fVariable != NULL) {
		fVariable->fEntries.Remove(this);
		fVariable = NULL;
	}

	mutex_unlock(&condition->lock);

	return status;
}


inline void
ConditionVariableEntry::AddToVariable(ConditionVariable* variable)
{
	fVariable = variable;
	fWaitStatus = STATUS_ADDED;
	fVariable->fEntries.Add(this);
}


//	#pragma mark -


void
ConditionVariable::Init(const void* object, const char* objectType)
{
	fObjectType = objectType;
	new(&fEntries) EntryList;
	
	condition_private* condition = new condition_private;
	mutex_init(&condition->lock, objectType);
	condition->wait_sem = create_sem(0, "condition variable wait");
	if (condition->wait_sem < B_OK)
		panic("cannot create condition variable.");

	condition->object = object;

	fObject = condition;
}


void
ConditionVariable::Add(ConditionVariableEntry* entry)
{
	entry->AddToVariable(this);
}


void
ConditionVariable::_Notify(bool all, bool threadsLocked)
{
	condition_private* condition = (condition_private*)fObject;
	mutex_lock(&condition->lock);

	uint32 count = 0;

	while (ConditionVariableEntry* entry = fEntries.RemoveHead()) {
		entry->fVariable = NULL;
		if (entry->fWaitStatus <= 0)
			continue;

		entry->fWaitStatus = B_OK;
		count++;

		if (!all)
			break;
	}

	release_sem_etc(condition->wait_sem, count, 0);
	mutex_unlock(&condition->lock);
}

