// kernelland_emu.cpp

#include <lock.h>
#include <fs/devfs.h>

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

#include <set>
#include <stdio.h>
#include <string>

#ifdef TRACE
#undef TRACE
#endif
#define TRACE(x)
//#define TRACE(x) printf x

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

// constructor
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

	static ModuleManager *Default() { return &sDefaultManager; }

	status_t GetModule(const char *path, module_info **infop);
	status_t PutModule(const char *path);

	status_t GetNextLoadedModuleName(uint32 *cookie, char *buffer,
									 size_t *bufferSize);

	module_name_list *OpenModuleList(const char *prefix);
	status_t ReadNextModuleName(module_name_list *list, char *buffer,
								size_t *bufferSize);
	status_t CloseModuleList(module_name_list *list);

	status_t AddBuiltInModule(module_info *info);

private:
	void _FindModules(BDirectory &dir, const char *moduleDir,
					  module_name_list *list);

	status_t _GetAddOn(const char *path, ModuleAddOn **addon);
	void _PutAddOn(ModuleAddOn *addon);

private:
	static ModuleManager		sDefaultManager;
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


status_t 
ModuleManager::AddBuiltInModule(module_info *info)
{
	BAutolock _lock(fModules);

	return fModules.AddModule(new Module(NULL, info)) ? B_OK : B_ERROR;
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
ModuleManager ModuleManager::sDefaultManager;


//	#pragma mark -
//	private emulation functions


extern "C" status_t
_add_builtin_module(module_info *info)
{
	return ModuleManager::Default()->AddBuiltInModule(info);
}


//	#pragma mark -
//	the functions to be emulated follow


// get_module
status_t
get_module(const char *path, module_info **_info)
{
	TRACE(("get_module(`%s')\n", path));
	return ModuleManager::Default()->GetModule(path, _info);
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


thread_id
spawn_kernel_thread(thread_func func, const char *name, int32 priority, void *data)
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


extern "C" void
panic(const char *format, ...)
{
	va_list args;

	puts("*** PANIC ***");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	exit(-1);
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


//	#pragma mark - Private locking functions


int
recursive_lock_get_recursion(recursive_lock *lock)
{
	thread_id thid = find_thread(NULL);

	if (lock->holder == thid)
		return lock->recursion;

	return -1;
}


status_t
recursive_lock_init(recursive_lock *lock, const char *name)
{
	if (lock == NULL)
		return B_BAD_VALUE;

	if (name == NULL)
		name = "recursive lock";

	lock->holder = -1;
	lock->recursion = 0;
	lock->sem = create_sem(1, name);

	if (lock->sem >= B_OK)
		return B_OK;

	return lock->sem;
}


void
recursive_lock_destroy(recursive_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem(lock->sem);
	lock->sem = -1;
}


bool
recursive_lock_lock(recursive_lock *lock)
{
	thread_id thid = find_thread(NULL);
	bool retval = false;

	if (thid != lock->holder) {
		acquire_sem(lock->sem);
		
		lock->holder = thid;
		retval = true;
	}
	lock->recursion++;
	return retval;
}


bool
recursive_lock_unlock(recursive_lock *lock)
{
	thread_id thid = find_thread(NULL);
	bool retval = false;

	if (thid != lock->holder)
		panic("recursive_lock %p unlocked by non-holder thread!\n", lock);

	if (--lock->recursion == 0) {
		lock->holder = -1;
		release_sem(lock->sem);
		retval = true;
	}
	return retval;
}


//	#pragma mark -


status_t
mutex_init(mutex *m, const char *name)
{
	if (m == NULL)
		return EINVAL;

	if (name == NULL)
		name = "mutex_sem";

	m->holder = -1;

	m->sem = create_sem(1, name);
	if (m->sem >= B_OK)
		return B_OK;

	return m->sem;
}


void
mutex_destroy(mutex *mutex)
{
	if (mutex == NULL)
		return;

	if (mutex->sem >= 0) {
		delete_sem(mutex->sem);
		mutex->sem = -1;
	}
	mutex->holder = -1;
}


void
mutex_lock(mutex *mutex)
{
	thread_id me = find_thread(NULL);

	// ToDo: if acquire_sem() fails, we shouldn't panic - but we should definitely
	//	change the mutex API to actually return the status code
	if (acquire_sem(mutex->sem) == B_OK) {
		if (me == mutex->holder)
			panic("mutex_lock failure: mutex %p (sem = 0x%lx) acquired twice by thread 0x%lx\n", mutex, mutex->sem, me);
	}

	mutex->holder = me;
}


void
mutex_unlock(mutex *mutex)
{
	thread_id me = find_thread(NULL);

	if (me != mutex->holder)
		panic("mutex_unlock failure: thread 0x%lx is trying to release mutex %p (current holder 0x%lx)\n",
			me, mutex, mutex->holder);

	mutex->holder = -1;
	release_sem(mutex->sem);
}


//	#pragma mark -


status_t
benaphore_init(benaphore *ben, const char *name)
{
	if (ben == NULL || name == NULL)
		return B_BAD_VALUE;

	ben->count = 1;
	ben->sem = create_sem(0, name);
	if (ben->sem >= B_OK)
		return B_OK;

	return ben->sem;
}


void
benaphore_destroy(benaphore *ben)
{
	delete_sem(ben->sem);
	ben->sem = -1;
}


//	#pragma mark -


status_t
rw_lock_init(rw_lock *lock, const char *name)
{
	if (lock == NULL)
		return B_BAD_VALUE;

	if (name == NULL)
		name = "r/w lock";

	lock->sem = create_sem(RW_MAX_READERS, name);
	if (lock->sem >= B_OK)
		return B_OK;

	return lock->sem;
}


void
rw_lock_destroy(rw_lock *lock)
{
	if (lock == NULL)
		return;

	delete_sem(lock->sem);
}


status_t
rw_lock_read_lock(rw_lock *lock)
{
	return acquire_sem(lock->sem);
}


status_t
rw_lock_read_unlock(rw_lock *lock)
{
	return release_sem(lock->sem);
}


status_t
rw_lock_write_lock(rw_lock *lock)
{
	return acquire_sem_etc(lock->sem, RW_MAX_READERS, 0, 0);
}


status_t
rw_lock_write_unlock(rw_lock *lock)
{
	return release_sem_etc(lock->sem, RW_MAX_READERS, 0);
}

