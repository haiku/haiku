/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*!	Manages kernel add-ons and their exported modules. */


#include <kmodule.h>

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <FindDirectory.h>
#include <NodeMonitor.h>

#include <boot_device.h>
#include <boot/elf.h>
#include <boot/kernel_args.h>
#include <elf.h>
#include <fs/KPath.h>
#include <fs/node_monitor.h>
#include <lock.h>
#include <Notifications.h>
#include <safemode.h>
#include <syscalls.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <util/Stack.h>
#include <vfs.h>


//#define TRACE_MODULE
#ifdef TRACE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif
#define FATAL(x) dprintf x


#define MODULE_HASH_SIZE 16

/*! The modules referenced by this structure are built-in
	modules that can't be loaded from disk.
*/
extern module_info gDeviceManagerModule;
extern module_info gDeviceRootModule;
extern module_info gDeviceGenericModule;
extern module_info gFrameBufferConsoleModule;

// file systems
extern module_info gRootFileSystem;
extern module_info gDeviceFileSystem;

static module_info* sBuiltInModules[] = {
	&gDeviceManagerModule,
	&gDeviceRootModule,
	&gDeviceGenericModule,
	&gFrameBufferConsoleModule,

	&gRootFileSystem,
	&gDeviceFileSystem,
	NULL
};

enum module_state {
	MODULE_QUERIED = 0,
	MODULE_LOADED,
	MODULE_INIT,
	MODULE_READY,
	MODULE_UNINIT,
	MODULE_ERROR
};


/* Each loaded module image (which can export several modules) is put
 * in a hash (gModuleImagesHash) to be easily found when you search
 * for a specific file name.
 * TODO: Could use only the inode number for hashing. Would probably be
 * a little bit slower, but would lower the memory foot print quite a lot.
 */

struct module_image {
	struct module_image* next;
	module_info**		info;		// the module_info we use
	module_dependency*	dependencies;
	char*				path;		// the full path for the module
	image_id			image;
	int32				ref_count;	// how many ref's to this file
};

/* Each known module will have this structure which is put in the
 * gModulesHash, and looked up by name.
 */

struct module {
	struct module*		next;
	::module_image*		module_image;
	char*				name;
	int32				ref_count;
	module_info*		info;		// will only be valid if ref_count > 0
	int32				offset;		// this is the offset in the headers
	module_state		state;
	uint32				flags;
};

#define B_BUILT_IN_MODULE	2

typedef struct module_path {
	const char*			name;
	uint32				base_length;
} module_path;

typedef struct module_iterator {
	module_path*		stack;
	int32				stack_size;
	int32				stack_current;

	char*				prefix;
	size_t				prefix_length;
	const char*			suffix;
	size_t				suffix_length;
	DIR*				current_dir;
	status_t			status;
	int32				module_offset;
		// This is used to keep track of which module_info
		// within a module we're addressing.
	::module_image*		module_image;
	module_info**		current_header;
	const char*			current_path;
	uint32				path_base_length;
	const char*			current_module_path;
	bool				builtin_modules;
	bool				loaded_modules;
} module_iterator;

namespace Module {

struct entry {
	dev_t				device;
	ino_t				node;
};

struct hash_entry : entry {
	~hash_entry()
	{
		free((char*)path);
	}

	hash_entry*			hash_link;
	const char*			path;
};

struct NodeHashDefinition {
	typedef entry* KeyType;
	typedef hash_entry ValueType;

	size_t Hash(ValueType* entry) const
		{ return HashKey(entry); }
	ValueType*& GetLink(ValueType* entry) const
		{ return entry->hash_link; }

	size_t HashKey(KeyType key) const
	{
		return ((uint32)(key->node >> 32) + (uint32)key->node) ^ key->device;
	}

	bool Compare(KeyType key, ValueType* entry) const
	{
		return key->device == entry->device
			&& key->node == entry->node;
	}
};

typedef BOpenHashTable<NodeHashDefinition> NodeHash;

struct module_listener : DoublyLinkedListLinkImpl<module_listener> {
	~module_listener()
	{
		free((char*)prefix);
	}

	NotificationListener* listener;
	const char*			prefix;
};

typedef DoublyLinkedList<module_listener> ModuleListenerList;

struct module_notification : DoublyLinkedListLinkImpl<module_notification> {
	~module_notification()
	{
		free((char*)name);
	}

	int32		opcode;
	dev_t		device;
	ino_t		directory;
	ino_t		node;
	const char*	name;
};

typedef DoublyLinkedList<module_notification> NotificationList;

class DirectoryWatcher : public NotificationListener {
public:
						DirectoryWatcher();
	virtual				~DirectoryWatcher();

	virtual void		EventOccurred(NotificationService& service,
							const KMessage* event);
};

class ModuleWatcher : public NotificationListener {
public:
						ModuleWatcher();
	virtual				~ModuleWatcher();

	virtual void		EventOccurred(NotificationService& service,
							const KMessage* event);
};

class ModuleNotificationService : public NotificationService {
public:
						ModuleNotificationService();
	virtual				~ModuleNotificationService();

			status_t	InitCheck();

			status_t	AddListener(const KMessage* eventSpecifier,
							NotificationListener& listener);
			status_t	UpdateListener(const KMessage* eventSpecifier,
							NotificationListener& listener);
			status_t	RemoveListener(const KMessage* eventSpecifier,
							NotificationListener& listener);

			bool		HasNode(dev_t device, ino_t node);

			void		Notify(int32 opcode, dev_t device, ino_t directory,
							ino_t node, const char* name);

	virtual const char*	Name() { return "modules"; }

	static	void		HandleNotifications(void *data, int iteration);

private:
			status_t	_RemoveNode(dev_t device, ino_t node);
			status_t	_AddNode(dev_t device, ino_t node, const char* path,
							uint32 flags, NotificationListener& listener);
			status_t	_AddDirectoryNode(dev_t device, ino_t node);
			status_t	_AddModuleNode(dev_t device, ino_t node, int fd,
							const char* name);

			status_t	_AddDirectory(const char* prefix);
			status_t	_ScanDirectory(char* directoryPath, const char* prefix,
							size_t& prefixPosition);
			status_t	_ScanDirectory(Stack<DIR*>& stack, DIR* dir,
							const char* prefix, size_t prefixPosition);

			void		_Notify(int32 opcode, dev_t device, ino_t directory,
							ino_t node, const char* name);
			void		_HandleNotifications();

	recursive_lock		fLock;
	ModuleListenerList	fListeners;
	NodeHash			fNodes;
	DirectoryWatcher	fDirectoryWatcher;
	ModuleWatcher		fModuleWatcher;
	NotificationList	fNotifications;
};

}	// namespace Module

using namespace Module;

/* These are the standard base paths where we start to look for modules
 * to load. Order is important, the last entry here will be searched
 * first.
 */
static const directory_which kModulePaths[] = {
	B_BEOS_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_USER_ADDONS_DIRECTORY,
};

static const uint32 kNumModulePaths = sizeof(kModulePaths)
	/ sizeof(kModulePaths[0]);
static const uint32 kFirstNonSystemModulePath = 1;


static ModuleNotificationService sModuleNotificationService;
static bool sDisableUserAddOns = false;

/*	Locking scheme: There is a global lock only; having several locks
	makes trouble if dependent modules get loaded concurrently ->
	they have to wait for each other, i.e. we need one lock per module;
	also we must detect circular references during init and not dead-lock.

	Reference counting: get_module() increments the ref count of a module,
	put_module() decrements it. When a B_KEEP_LOADED module is initialized
	the ref count is incremented once more, so it never gets
	uninitialized/unloaded. A referenced module, unless it's built-in, has a
	non-null module_image and owns a reference to the image. When the last
	module reference is put, the image's reference is released and module_image
	zeroed (as long as the boot volume has not been mounted, it is not zeroed).
	An unreferenced module image is unloaded (when the boot volume is mounted).
*/
static recursive_lock sModulesLock;

/* We store the loaded modules by directory path, and all known modules
 * by module name in a hash table for quick access
 */
static hash_table* sModuleImagesHash;
static hash_table* sModulesHash;


/*!	Calculates hash for a module using its name */
static uint32
module_hash(void* _module, const void* _key, uint32 range)
{
	module* module = (struct module*)_module;
	const char* name = (const char*)_key;

	if (module != NULL)
		return hash_hash_string(module->name) % range;

	if (name != NULL)
		return hash_hash_string(name) % range;

	return 0;
}


/*!	Compares a module to a given name */
static int
module_compare(void* _module, const void* _key)
{
	module* module = (struct module*)_module;
	const char* name = (const char*)_key;
	if (name == NULL)
		return -1;

	return strcmp(module->name, name);
}


/*!	Calculates the hash of a module image using its path */
static uint32
module_image_hash(void* _module, const void* _key, uint32 range)
{
	module_image* image = (module_image*)_module;
	const char* path = (const char*)_key;

	if (image != NULL)
		return hash_hash_string(image->path) % range;

	if (path != NULL)
		return hash_hash_string(path) % range;

	return 0;
}


/*!	Compares a module image to a path */
static int
module_image_compare(void* _module, const void* _key)
{
	module_image* image = (module_image*)_module;
	const char* path = (const char*)_key;
	if (path == NULL)
		return -1;

	return strcmp(image->path, path);
}


/*!	Try to load the module image at the specified \a path.
	If it could be loaded, it returns \c B_OK, and stores a pointer
	to the module_image object in \a _moduleImage.
	Needs to be called with the sModulesLock held.
*/
static status_t
load_module_image(const char* path, module_image** _moduleImage)
{
	module_image* moduleImage;
	status_t status;
	image_id image;

	TRACE(("load_module_image(path = \"%s\", _image = %p)\n", path,
		_moduleImage));
	ASSERT_LOCKED_RECURSIVE(&sModulesLock);
	ASSERT(_moduleImage != NULL);

	image = load_kernel_add_on(path);
	if (image < 0) {
		dprintf("load_module_image(%s) failed: %s\n", path, strerror(image));
		return image;
	}

	moduleImage = (module_image*)malloc(sizeof(module_image));
	if (moduleImage == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	if (get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
			(void**)&moduleImage->info) != B_OK) {
		TRACE(("load_module_image: Failed to load \"%s\" due to lack of "
			"'modules' symbol\n", path));
		status = B_BAD_TYPE;
		goto err1;
	}

	moduleImage->dependencies = NULL;
	get_image_symbol(image, "module_dependencies", B_SYMBOL_TYPE_DATA,
		(void**)&moduleImage->dependencies);
		// this is allowed to be NULL

	moduleImage->path = strdup(path);
	if (!moduleImage->path) {
		status = B_NO_MEMORY;
		goto err1;
	}

	moduleImage->image = image;
	moduleImage->ref_count = 0;

	hash_insert(sModuleImagesHash, moduleImage);

	TRACE(("load_module_image(\"%s\"): image loaded: %p\n", path, moduleImage));

	*_moduleImage = moduleImage;
	return B_OK;

err1:
	free(moduleImage);
err:
	unload_kernel_add_on(image);

	return status;
}


/*!	Unloads the module's kernel add-on. The \a image will be freed.
	Needs to be called with the sModulesLock held.
*/
static status_t
unload_module_image(module_image* moduleImage, bool remove)
{
	TRACE(("unload_module_image(image %p, remove %d)\n", moduleImage, remove));
	ASSERT_LOCKED_RECURSIVE(&sModulesLock);

	if (moduleImage->ref_count != 0) {
		FATAL(("Can't unload %s due to ref_cnt = %" B_PRId32 "\n", moduleImage->path,
			moduleImage->ref_count));
		return B_ERROR;
	}

	if (remove)
		hash_remove(sModuleImagesHash, moduleImage);

	unload_kernel_add_on(moduleImage->image);
	free(moduleImage->path);
	free(moduleImage);

	return B_OK;
}


static void
put_module_image(module_image* image)
{
	RecursiveLocker locker(sModulesLock);

	int32 refCount = atomic_add(&image->ref_count, -1);
	ASSERT(refCount > 0);

	// Don't unload anything when there is no boot device yet
	// (because chances are that we will never be able to access it again)

	if (refCount == 1 && gBootDevice > 0)
		unload_module_image(image, true);
}


static status_t
get_module_image(const char* path, module_image** _image)
{
	struct module_image* image;

	TRACE(("get_module_image(path = \"%s\")\n", path));

	RecursiveLocker _(sModulesLock);

	image = (module_image*)hash_lookup(sModuleImagesHash, path);
	if (image == NULL) {
		status_t status = load_module_image(path, &image);
		if (status < B_OK)
			return status;
	}

	atomic_add(&image->ref_count, 1);
	*_image = image;

	return B_OK;
}


/*!	Extract the information from the module_info structure pointed at
	by "info" and create the entries required for access to it's details.
*/
static status_t
create_module(module_info* info, int offset, module** _module)
{
	module* module;

	TRACE(("create_module(info = %p, offset = %d, _module = %p)\n",
		info, offset, _module));

	if (!info->name)
		return B_BAD_VALUE;

	module = (struct module*)hash_lookup(sModulesHash, info->name);
	if (module) {
		FATAL(("Duplicate module name (%s) detected... ignoring new one\n",
			info->name));
		return B_FILE_EXISTS;
	}

	if ((module = (struct module*)malloc(sizeof(struct module))) == NULL)
		return B_NO_MEMORY;

	TRACE(("create_module: name = \"%s\"\n", info->name));

	module->module_image = NULL;
	module->name = strdup(info->name);
	if (module->name == NULL) {
		free(module);
		return B_NO_MEMORY;
	}

	module->state = MODULE_QUERIED;
	module->info = info;
	module->offset = offset;
		// record where the module_info can be found in the module_info array
	module->ref_count = 0;
	module->flags = info->flags;

	recursive_lock_lock(&sModulesLock);
	hash_insert(sModulesHash, module);
	recursive_lock_unlock(&sModulesLock);

	if (_module)
		*_module = module;

	return B_OK;
}


/*!	Loads the file at \a path and scans all modules contained therein.
	Returns \c B_OK if \a searchedName could be found under those modules,
	and will return the referenced image in \a _moduleImage.
	Returns \c B_ENTRY_NOT_FOUND if the module could not be found.

	Must only be called for files that haven't been scanned yet.
	\a searchedName is allowed to be \c NULL (if all modules should be scanned)
*/
static status_t
check_module_image(const char* path, const char* searchedName,
	module_image** _moduleImage)
{
	status_t status = B_ENTRY_NOT_FOUND;
	module_image* image;
	module_info** info;
	int index = 0;

	TRACE(("check_module_image(path = \"%s\", searchedName = \"%s\")\n", path,
		searchedName));

	if (get_module_image(path, &image) < B_OK)
		return B_ENTRY_NOT_FOUND;

	for (info = image->info; *info; info++) {
		// try to create a module for every module_info, check if the
		// name matches if it was a new entry
		bool freshModule = false;
		struct module* module = (struct module*)hash_lookup(sModulesHash,
			(*info)->name);
		if (module != NULL) {
			// Module does already exist
			if (module->module_image == NULL && module->ref_count == 0) {
				module->info = *info;
				module->offset = index;
				module->flags = (*info)->flags;
				module->state = MODULE_QUERIED;
				freshModule = true;
			}
		} else if (create_module(*info, index, NULL) == B_OK)
			freshModule = true;

		if (freshModule && searchedName != NULL
			&& strcmp((*info)->name, searchedName) == 0) {
			status = B_OK;
		}

		index++;
	}

	if (status != B_OK) {
		// decrement the ref we got in get_module_image
		put_module_image(image);
		return status;
	}

	*_moduleImage = image;
	return B_OK;
}


static module*
search_module(const char* name, module_image** _moduleImage)
{
	status_t status = B_ENTRY_NOT_FOUND;
	uint32 i;

	TRACE(("search_module(%s)\n", name));

	for (i = kNumModulePaths; i-- > 0;) {
		if (sDisableUserAddOns && i >= kFirstNonSystemModulePath)
			continue;

		// let the VFS find that module for us

		KPath basePath;
		if (find_directory(kModulePaths[i], gBootDevice, true,
				basePath.LockBuffer(), basePath.BufferSize()) != B_OK)
			continue;

		basePath.UnlockBuffer();
		basePath.Append("kernel");

		KPath path;
		status = vfs_get_module_path(basePath.Path(), name, path.LockBuffer(),
			path.BufferSize());
		if (status == B_OK) {
			path.UnlockBuffer();
			status = check_module_image(path.Path(), name, _moduleImage);
			if (status == B_OK)
				break;
		}
	}

	if (status != B_OK)
		return NULL;

	return (module*)hash_lookup(sModulesHash, name);
}


static status_t
put_dependent_modules(struct module* module)
{
	module_image* image = module->module_image;
	module_dependency* dependencies;

	// built-in modules don't have a module_image structure
	if (image == NULL
		|| (dependencies = image->dependencies) == NULL)
		return B_OK;

	for (int32 i = 0; dependencies[i].name != NULL; i++) {
		status_t status = put_module(dependencies[i].name);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


static status_t
get_dependent_modules(struct module* module)
{
	module_image* image = module->module_image;
	module_dependency* dependencies;

	// built-in modules don't have a module_image structure
	if (image == NULL
		|| (dependencies = image->dependencies) == NULL)
		return B_OK;

	TRACE(("resolving module dependencies...\n"));

	for (int32 i = 0; dependencies[i].name != NULL; i++) {
		status_t status = get_module(dependencies[i].name,
			dependencies[i].info);
		if (status < B_OK) {
			dprintf("loading dependent module %s of %s failed!\n",
				dependencies[i].name, module->name);
			return status;
		}
	}

	return B_OK;
}


/*!	Initializes a loaded module depending on its state */
static inline status_t
init_module(module* module)
{
	switch (module->state) {
		case MODULE_QUERIED:
		case MODULE_LOADED:
		{
			status_t status;
			module->state = MODULE_INIT;

			// resolve dependencies

			status = get_dependent_modules(module);
			if (status < B_OK) {
				module->state = MODULE_LOADED;
				return status;
			}

			// init module

			TRACE(("initializing module %s (at %p)... \n", module->name,
				module->info->std_ops));

			if (module->info->std_ops != NULL)
				status = module->info->std_ops(B_MODULE_INIT);

			TRACE(("...done (%s)\n", strerror(status)));

			if (status >= B_OK)
				module->state = MODULE_READY;
			else {
				put_dependent_modules(module);
				module->state = MODULE_LOADED;
			}

			return status;
		}

		case MODULE_READY:
			return B_OK;

		case MODULE_INIT:
			FATAL(("circular reference to %s\n", module->name));
			return B_ERROR;

		case MODULE_UNINIT:
			FATAL(("tried to load module %s which is currently unloading\n",
				module->name));
			return B_ERROR;

		case MODULE_ERROR:
			FATAL(("cannot load module %s because its earlier unloading "
				"failed\n", module->name));
			return B_ERROR;

		default:
			return B_ERROR;
	}
	// never trespasses here
}


/*!	Uninitializes a module depeding on its state */
static inline int
uninit_module(module* module)
{
	TRACE(("uninit_module(%s)\n", module->name));

	switch (module->state) {
		case MODULE_QUERIED:
		case MODULE_LOADED:
			return B_NO_ERROR;

		case MODULE_INIT:
			panic("Trying to unload module %s which is initializing\n",
				module->name);
			return B_ERROR;

		case MODULE_UNINIT:
			panic("Trying to unload module %s which is un-initializing\n",
				module->name);
			return B_ERROR;

		case MODULE_READY:
		{
			status_t status = B_OK;
			module->state = MODULE_UNINIT;

			TRACE(("uninitializing module %s...\n", module->name));

			if (module->info->std_ops != NULL)
				status = module->info->std_ops(B_MODULE_UNINIT);

			TRACE(("...done (%s)\n", strerror(status)));

			if (status == B_OK) {
				module->state = MODULE_LOADED;
				put_dependent_modules(module);
				return B_OK;
			}

			FATAL(("Error unloading module %s (%s)\n", module->name,
				strerror(status)));

			module->state = MODULE_ERROR;
			module->flags |= B_KEEP_LOADED;
			module->ref_count++;

			return status;
		}
		default:
			return B_ERROR;
	}
	// never trespasses here
}


static const char*
iterator_pop_path_from_stack(module_iterator* iterator, uint32* _baseLength)
{
	if (iterator->stack_current <= 0)
		return NULL;

	if (_baseLength)
		*_baseLength = iterator->stack[iterator->stack_current - 1].base_length;

	return iterator->stack[--iterator->stack_current].name;
}


static status_t
iterator_push_path_on_stack(module_iterator* iterator, const char* path,
	uint32 baseLength)
{
	if (iterator->stack_current + 1 > iterator->stack_size) {
		// allocate new space on the stack
		module_path* stack = (module_path*)realloc(iterator->stack,
			(iterator->stack_size + 8) * sizeof(module_path));
		if (stack == NULL)
			return B_NO_MEMORY;

		iterator->stack = stack;
		iterator->stack_size += 8;
	}

	iterator->stack[iterator->stack_current].name = path;
	iterator->stack[iterator->stack_current++].base_length = baseLength;
	return B_OK;
}


static bool
match_iterator_suffix(module_iterator* iterator, const char* name)
{
	if (iterator->suffix == NULL || iterator->suffix_length == 0)
		return true;

	size_t length = strlen(name);
	if (length <= iterator->suffix_length)
		return false;

	return name[length - iterator->suffix_length - 1] == '/'
		&& !strcmp(name + length - iterator->suffix_length, iterator->suffix);
}


static status_t
iterator_get_next_module(module_iterator* iterator, char* buffer,
	size_t* _bufferSize)
{
	status_t status;

	TRACE(("iterator_get_next_module() -- start\n"));

	if (iterator->builtin_modules) {
		for (int32 i = iterator->module_offset; sBuiltInModules[i] != NULL;
				i++) {
			// the module name must fit the prefix
			if (strncmp(sBuiltInModules[i]->name, iterator->prefix,
					iterator->prefix_length)
				|| !match_iterator_suffix(iterator, sBuiltInModules[i]->name))
				continue;

			*_bufferSize = strlcpy(buffer, sBuiltInModules[i]->name,
				*_bufferSize);
			iterator->module_offset = i + 1;
			return B_OK;
		}
		iterator->builtin_modules = false;
	}

	if (iterator->loaded_modules) {
		recursive_lock_lock(&sModulesLock);
		hash_iterator hashIterator;
		hash_open(sModulesHash, &hashIterator);

		struct module* module = (struct module*)hash_next(sModulesHash,
			&hashIterator);
		for (int32 i = 0; module != NULL; i++) {
			if (i >= iterator->module_offset) {
				if (!strncmp(module->name, iterator->prefix,
						iterator->prefix_length)
					&& match_iterator_suffix(iterator, module->name)) {
					*_bufferSize = strlcpy(buffer, module->name, *_bufferSize);
					iterator->module_offset = i + 1;

					hash_close(sModulesHash, &hashIterator, false);
					recursive_lock_unlock(&sModulesLock);
					return B_OK;
				}
			}
			module = (struct module*)hash_next(sModulesHash, &hashIterator);
		}

		hash_close(sModulesHash, &hashIterator, false);
		recursive_lock_unlock(&sModulesLock);

		// prevent from falling into modules hash iteration again
		iterator->loaded_modules = false;
	}

nextPath:
	if (iterator->current_dir == NULL) {
		// get next directory path from the stack
		const char* path = iterator_pop_path_from_stack(iterator,
			&iterator->path_base_length);
		if (path == NULL) {
			// we are finished, there are no more entries on the stack
			return B_ENTRY_NOT_FOUND;
		}

		free((char*)iterator->current_path);
		iterator->current_path = path;
		iterator->current_dir = opendir(path);
		TRACE(("open directory at %s -> %p\n", path, iterator->current_dir));

		if (iterator->current_dir == NULL) {
			// we don't throw an error here, but silently go to
			// the next directory on the stack
			goto nextPath;
		}
	}

nextModuleImage:
	// TODO: remember which directories were already scanned, and don't search
	// through them again, unless they change (use DirectoryWatcher)

	if (iterator->current_header == NULL) {
		// get next entry from the current directory

		errno = 0;

		struct dirent* dirent;
		if ((dirent = readdir(iterator->current_dir)) == NULL) {
			closedir(iterator->current_dir);
			iterator->current_dir = NULL;

			if (errno < B_OK)
				return errno;

			goto nextPath;
		}

		// check if the prefix matches
		int32 passedOffset, commonLength;
		passedOffset = strlen(iterator->current_path) + 1;
		commonLength = iterator->path_base_length + iterator->prefix_length
			- passedOffset;

		if (commonLength > 0) {
			// the prefix still reaches into the new path part
			int32 length = strlen(dirent->d_name);
			if (commonLength > length)
				commonLength = length;

			if (strncmp(dirent->d_name, iterator->prefix + passedOffset
					- iterator->path_base_length, commonLength))
				goto nextModuleImage;
		}

		// we're not interested in traversing these (again)
		if (!strcmp(dirent->d_name, ".")
			|| !strcmp(dirent->d_name, "..")
			// TODO: this is a bit unclean, as we actually only want to prevent
			// drivers/bin and drivers/dev to be scanned
			|| !strcmp(dirent->d_name, "bin")
			|| !strcmp(dirent->d_name, "dev"))
			goto nextModuleImage;

		// build absolute path to current file
		KPath path(iterator->current_path);
		if (path.InitCheck() != B_OK)
			return B_NO_MEMORY;

		if (path.Append(dirent->d_name) != B_OK)
			return B_BUFFER_OVERFLOW;

		// find out if it's a directory or a file
		struct stat stat;
		if (::stat(path.Path(), &stat) < 0)
			return errno;

		iterator->current_module_path = strdup(path.Path());
		if (iterator->current_module_path == NULL)
			return B_NO_MEMORY;

		if (S_ISDIR(stat.st_mode)) {
			status = iterator_push_path_on_stack(iterator,
				iterator->current_module_path, iterator->path_base_length);
			if (status != B_OK)
				return status;

			iterator->current_module_path = NULL;
			goto nextModuleImage;
		}

		if (!S_ISREG(stat.st_mode))
			return B_BAD_TYPE;

		TRACE(("open module at %s\n", path.Path()));

		status = get_module_image(path.Path(), &iterator->module_image);
		if (status < B_OK) {
			free((char*)iterator->current_module_path);
			iterator->current_module_path = NULL;
			goto nextModuleImage;
		}

		iterator->current_header = iterator->module_image->info;
		iterator->module_offset = 0;
	}

	// search the current module image until we've got a match
	while (*iterator->current_header != NULL) {
		module_info* info = *iterator->current_header;

		// TODO: we might want to create a module here and cache it in the
		// hash table

		iterator->current_header++;
		iterator->module_offset++;

		if (strncmp(info->name, iterator->prefix, iterator->prefix_length)
			|| !match_iterator_suffix(iterator, info->name))
			continue;

		*_bufferSize = strlcpy(buffer, info->name, *_bufferSize);
		return B_OK;
	}

	// leave this module and get the next one

	iterator->current_header = NULL;
	free((char*)iterator->current_module_path);
	iterator->current_module_path = NULL;

	put_module_image(iterator->module_image);
	iterator->module_image = NULL;

	goto nextModuleImage;
}


static void
register_builtin_modules(struct module_info** info)
{
	for (; *info; info++) {
		(*info)->flags |= B_BUILT_IN_MODULE;
			// this is an internal flag, it doesn't have to be set by modules
			// itself

		if (create_module(*info, -1, NULL) != B_OK) {
			dprintf("creation of built-in module \"%s\" failed!\n",
				(*info)->name);
		}
	}
}


static status_t
register_preloaded_module_image(struct preloaded_image* image)
{
	module_image* moduleImage;
	struct module_info** info;
	status_t status;
	int32 index = 0;

	TRACE(("register_preloaded_module_image(image = %p, name = \"%s\")\n",
		image, image->name));

	image->is_module = false;

	if (image->id < 0)
		return B_BAD_VALUE;

	moduleImage = (module_image*)malloc(sizeof(module_image));
	if (moduleImage == NULL)
		return B_NO_MEMORY;

	if (get_image_symbol(image->id, "modules", B_SYMBOL_TYPE_DATA,
			(void**)&moduleImage->info) != B_OK) {
		status = B_BAD_TYPE;
		goto error;
	}

	image->is_module = true;

	if (moduleImage->info[0] == NULL) {
		status = B_BAD_DATA;
		goto error;
	}

	moduleImage->dependencies = NULL;
	get_image_symbol(image->id, "module_dependencies", B_SYMBOL_TYPE_DATA,
		(void**)&moduleImage->dependencies);
		// this is allowed to be NULL

	moduleImage->path = strdup(image->name);
	if (moduleImage->path == NULL) {
		status = B_NO_MEMORY;
		goto error;
	}

	moduleImage->image = image->id;
	moduleImage->ref_count = 0;

	hash_insert(sModuleImagesHash, moduleImage);

	for (info = moduleImage->info; *info; info++) {
		struct module* module = NULL;
		if (create_module(*info, index++, &module) == B_OK)
			module->module_image = moduleImage;
	}

	return B_OK;

error:
	free(moduleImage);

	// We don't need this image anymore. We keep it, if it doesn't look like
	// a module at all. It might be an old-style driver.
	if (image->is_module)
		unload_kernel_add_on(image->id);

	return status;
}


static int
dump_modules(int argc, char** argv)
{
	hash_iterator iterator;
	struct module_image* image;
	struct module* module;

	hash_rewind(sModulesHash, &iterator);
	kprintf("-- known modules:\n");

	while ((module = (struct module*)hash_next(sModulesHash, &iterator))
			!= NULL) {
		kprintf("%p: \"%s\", \"%s\" (%" B_PRId32 "), refcount = %" B_PRId32 ", "
			"state = %d, mimage = %p\n", module, module->name,
			module->module_image ? module->module_image->path : "",
			module->offset, module->ref_count, module->state,
			module->module_image);
	}

	hash_rewind(sModuleImagesHash, &iterator);
	kprintf("\n-- loaded module images:\n");

	while ((image = (struct module_image*)hash_next(sModuleImagesHash,
				&iterator)) != NULL) {
		kprintf("%p: \"%s\" (image_id = %" B_PRId32 "), info = %p, refcount = "
			"%" B_PRId32 "\n", image, image->path, image->image, image->info,
			image->ref_count);
	}
	return 0;
}


//	#pragma mark - DirectoryWatcher


DirectoryWatcher::DirectoryWatcher()
{
}


DirectoryWatcher::~DirectoryWatcher()
{
}


void
DirectoryWatcher::EventOccurred(NotificationService& service,
	const KMessage* event)
{
	int32 opcode = event->GetInt32("opcode", -1);
	dev_t device = event->GetInt32("device", -1);
	ino_t directory = event->GetInt64("directory", -1);
	ino_t node = event->GetInt64("node", -1);
	const char *name = event->GetString("name", NULL);

	if (opcode == B_ENTRY_MOVED) {
		// Determine whether it's a move within, out of, or into one
		// of our watched directories.
		directory = event->GetInt64("to directory", -1);
		if (!sModuleNotificationService.HasNode(device, directory)) {
			directory = event->GetInt64("from directory", -1);
			opcode = B_ENTRY_REMOVED;
		} else {
			// Move within, doesn't sound like a good idea for modules
			opcode = B_ENTRY_CREATED;
		}
	}

	sModuleNotificationService.Notify(opcode, device, directory, node, name);
}


//	#pragma mark - ModuleWatcher


ModuleWatcher::ModuleWatcher()
{
}


ModuleWatcher::~ModuleWatcher()
{
}


void
ModuleWatcher::EventOccurred(NotificationService& service, const KMessage* event)
{
	if (event->GetInt32("opcode", -1) != B_STAT_CHANGED
		|| (event->GetInt32("fields", 0) & B_STAT_MODIFICATION_TIME) == 0)
		return;

	dev_t device = event->GetInt32("device", -1);
	ino_t node = event->GetInt64("node", -1);

	sModuleNotificationService.Notify(B_STAT_CHANGED, device, -1, node, NULL);
}


//	#pragma mark - ModuleNotificationService


ModuleNotificationService::ModuleNotificationService()
{
	recursive_lock_init(&fLock, "module notifications");
}


ModuleNotificationService::~ModuleNotificationService()
{
	recursive_lock_destroy(&fLock);
}


status_t
ModuleNotificationService::AddListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	const char* prefix = eventSpecifier->GetString("prefix", NULL);
	if (prefix == NULL)
		return B_BAD_VALUE;

	module_listener* moduleListener = new(std::nothrow) module_listener;
	if (moduleListener == NULL)
		return B_NO_MEMORY;

	moduleListener->prefix = strdup(prefix);
	if (moduleListener->prefix == NULL) {
		delete moduleListener;
		return B_NO_MEMORY;
	}

	status_t status = _AddDirectory(prefix);
	if (status != B_OK) {
		delete moduleListener;
		return status;
	}

	moduleListener->listener = &listener;
	fListeners.Add(moduleListener);

	return B_OK;
}


status_t
ModuleNotificationService::UpdateListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	return B_ERROR;
}


status_t
ModuleNotificationService::RemoveListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	return B_ERROR;
}


bool
ModuleNotificationService::HasNode(dev_t device, ino_t node)
{
	RecursiveLocker _(fLock);

	struct entry entry = {device, node};
	return fNodes.Lookup(&entry) != NULL;
}


status_t
ModuleNotificationService::_RemoveNode(dev_t device, ino_t node)
{
	RecursiveLocker _(fLock);

	struct entry key = {device, node};
	hash_entry* entry = fNodes.Lookup(&key);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	remove_node_listener(device, node, entry->path != NULL
		? (NotificationListener&)fModuleWatcher
		: (NotificationListener&)fDirectoryWatcher);

	fNodes.Remove(entry);
	delete entry;

	return B_OK;
}


status_t
ModuleNotificationService::_AddNode(dev_t device, ino_t node, const char* path,
	uint32 flags, NotificationListener& listener)
{
	RecursiveLocker locker(fLock);

	if (HasNode(device, node))
		return B_OK;

	struct hash_entry* entry = new(std::nothrow) hash_entry;
	if (entry == NULL)
		return B_NO_MEMORY;

	if (path != NULL) {
		entry->path = strdup(path);
		if (entry->path == NULL) {
			delete entry;
			return B_NO_MEMORY;
		}
	} else
		entry->path = NULL;

	status_t status = add_node_listener(device, node, flags, listener);
	if (status != B_OK) {
		delete entry;
		return status;
	}

	//dprintf("  add %s %ld:%Ld (%s)\n", flags == B_WATCH_DIRECTORY
	//	? "dir" : "file", device, node, path);

	entry->device = device;
	entry->node = node;
	fNodes.Insert(entry);

	return B_OK;
}


status_t
ModuleNotificationService::_AddDirectoryNode(dev_t device, ino_t node)
{
	return _AddNode(device, node, NULL, B_WATCH_DIRECTORY, fDirectoryWatcher);
}


status_t
ModuleNotificationService::_AddModuleNode(dev_t device, ino_t node, int fd,
	const char* name)
{
	struct vnode* vnode;
	status_t status = vfs_get_vnode_from_fd(fd, true, &vnode);
	if (status != B_OK)
		return status;

	ino_t directory;
	vfs_vnode_to_node_ref(vnode, &device, &directory);

	KPath path;
	status = path.InitCheck();
	if (status == B_OK) {
		status = vfs_entry_ref_to_path(device, directory, name,
			path.LockBuffer(), path.BufferSize());
	}
	if (status != B_OK)
		return status;

	path.UnlockBuffer();

	return _AddNode(device, node, path.Path(), B_WATCH_STAT, fModuleWatcher);
}


status_t
ModuleNotificationService::_AddDirectory(const char* prefix)
{
	status_t status = B_ERROR;

	for (uint32 i = 0; i < kNumModulePaths; i++) {
		if (sDisableUserAddOns && i >= kFirstNonSystemModulePath)
			break;

		KPath pathBuffer;
		if (find_directory(kModulePaths[i], gBootDevice, true,
				pathBuffer.LockBuffer(), pathBuffer.BufferSize()) != B_OK)
			continue;

		pathBuffer.UnlockBuffer();
		pathBuffer.Append("kernel");
		pathBuffer.Append(prefix);

		size_t prefixPosition = strlen(prefix);
		status_t scanStatus = _ScanDirectory(pathBuffer.LockBuffer(), prefix,
			prefixPosition);

		pathBuffer.UnlockBuffer();

		// It's enough if we succeed for one directory
		if (status != B_OK)
			status = scanStatus;
	}

	return status;
}


status_t
ModuleNotificationService::_ScanDirectory(char* directoryPath,
	const char* prefix, size_t& prefixPosition)
{
	DIR* dir = NULL;
	while (true) {
		dir = opendir(directoryPath);
		if (dir != NULL || prefixPosition == 0)
			break;

		// the full prefix is not accessible, remove path components
		const char* parentPrefix = prefix + prefixPosition - 1;
		while (parentPrefix != prefix && parentPrefix[0] != '/')
			parentPrefix--;

		size_t cutPosition = parentPrefix - prefix;
		size_t length = strlen(directoryPath);
		directoryPath[length - prefixPosition + cutPosition] = '\0';
		prefixPosition = cutPosition;
	}

	if (dir == NULL)
		return B_ERROR;

	Stack<DIR*> stack;
	stack.Push(dir);

	while (stack.Pop(&dir)) {
		status_t status = _ScanDirectory(stack, dir, prefix, prefixPosition);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
ModuleNotificationService::_ScanDirectory(Stack<DIR*>& stack, DIR* dir,
	const char* prefix, size_t prefixPosition)
{
	bool directMatchAdded = false;
	struct dirent* dirent;

	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] == '.')
			continue;

		bool directMatch = false;

		if (prefix[prefixPosition] != '\0') {
			// the start must match
			const char* startPrefix = prefix + prefixPosition;
			if (startPrefix[0] == '/')
				startPrefix++;

			const char* endPrefix = strchr(startPrefix, '/');
			size_t length;

			if (endPrefix != NULL)
				length = endPrefix - startPrefix;
			else
				length = strlen(startPrefix);

			if (strncmp(dirent->d_name, startPrefix, length))
				continue;

			if (dirent->d_name[length] == '\0')
				directMatch = true;
		}

		struct stat stat;
		status_t status = vfs_read_stat(dirfd(dir), dirent->d_name, true, &stat,
			true);
		if (status != B_OK)
			continue;

		if (S_ISDIR(stat.st_mode)) {
			int fd = _kern_open_dir(dirfd(dir), dirent->d_name);
			if (fd < 0)
				continue;

			DIR* subDir = fdopendir(fd);
			if (subDir == NULL) {
				close(fd);
				continue;
			}

			stack.Push(subDir);

			if (_AddDirectoryNode(stat.st_dev, stat.st_ino) == B_OK
				&& directMatch)
				directMatchAdded = true;
		} else if (S_ISREG(stat.st_mode)) {
			if (_AddModuleNode(stat.st_dev, stat.st_ino, dirfd(dir),
					dirent->d_name) == B_OK && directMatch)
				directMatchAdded = true;
		}
	}

	if (!directMatchAdded) {
		// We need to monitor this directory to see if a matching file
		// is added.
		struct stat stat;
		status_t status = vfs_read_stat(dirfd(dir), NULL, true, &stat, true);
		if (status == B_OK)
			_AddDirectoryNode(stat.st_dev, stat.st_ino);
	}

	closedir(dir);
	return B_OK;
}


void
ModuleNotificationService::_Notify(int32 opcode, dev_t device, ino_t directory,
	ino_t node, const char* name)
{
	// construct path

	KPath pathBuffer;
	const char* path;

	if (name != NULL) {
		// we have an entry ref
		if (pathBuffer.InitCheck() != B_OK
			|| vfs_entry_ref_to_path(device, directory, name,
				pathBuffer.LockBuffer(), pathBuffer.BufferSize()) != B_OK)
			return;

		pathBuffer.UnlockBuffer();
		path = pathBuffer.Path();
	} else {
		// we only have a node ref
		RecursiveLocker _(fLock);

		struct entry key = {device, node};
		hash_entry* entry = fNodes.Lookup(&key);
		if (entry == NULL || entry->path == NULL)
			return;

		path = entry->path;
	}

	// remove kModulePaths from path

	for (uint32 i = 0; i < kNumModulePaths; i++) {
		KPath modulePath;
		if (find_directory(kModulePaths[i], gBootDevice, true,
				modulePath.LockBuffer(), modulePath.BufferSize()) != B_OK)
			continue;

		modulePath.UnlockBuffer();
		modulePath.Append("kernel");

		if (strncmp(path, modulePath.Path(), modulePath.Length()))
			continue;

		path += modulePath.Length();
		if (path[i] == '/')
			path++;

		break;
	}

	KMessage event;

	// find listeners by prefix/path

	ModuleListenerList::Iterator iterator = fListeners.GetIterator();
	while (iterator.HasNext()) {
		module_listener* listener = iterator.Next();

		if (strncmp(path, listener->prefix, strlen(listener->prefix)))
			continue;

		if (event.IsEmpty()) {
			// construct message only when needed
			event.AddInt32("opcode", opcode);
			event.AddString("path", path);
		}

		// notify them!
		listener->listener->EventOccurred(*this, &event);

		// we might need to watch new files now
		if (opcode == B_ENTRY_CREATED)
			_AddDirectory(listener->prefix);

	}

	// remove notification listeners, if needed

	if (opcode == B_ENTRY_REMOVED)
		_RemoveNode(device, node);
}


void
ModuleNotificationService::_HandleNotifications()
{
	RecursiveLocker _(fLock);

	NotificationList::Iterator iterator = fNotifications.GetIterator();
	while (iterator.HasNext()) {
		module_notification* notification = iterator.Next();

		_Notify(notification->opcode, notification->device,
			notification->directory, notification->node, notification->name);

		iterator.Remove();
		delete notification;
	}
}


void
ModuleNotificationService::Notify(int32 opcode, dev_t device, ino_t directory,
	ino_t node, const char* name)
{
	module_notification* notification = new(std::nothrow) module_notification;
	if (notification == NULL)
		return;

	if (name != NULL) {
		notification->name = strdup(name);
		if (notification->name == NULL) {
			delete notification;
			return;
		}
	} else
		notification->name = NULL;

	notification->opcode = opcode;
	notification->device = device;
	notification->directory = directory;
	notification->node = node;

	RecursiveLocker _(fLock);
	fNotifications.Add(notification);
}


/*static*/ void
ModuleNotificationService::HandleNotifications(void */*data*/,
	int /*iteration*/)
{
	sModuleNotificationService._HandleNotifications();
}


//	#pragma mark - Exported Kernel API (private part)


/*!	Unloads a module in case it's not in use. This is the counterpart
	to load_module().
*/
status_t
unload_module(const char* path)
{
	struct module_image* moduleImage;

	recursive_lock_lock(&sModulesLock);
	moduleImage = (module_image*)hash_lookup(sModuleImagesHash, path);
	recursive_lock_unlock(&sModulesLock);

	if (moduleImage == NULL)
		return B_ENTRY_NOT_FOUND;

	put_module_image(moduleImage);
	return B_OK;
}


/*!	Unlike get_module(), this function lets you specify the add-on to
	be loaded by path.
	However, you must not use the exported modules without having called
	get_module() on them. When you're done with the NULL terminated
	\a modules array, you have to call unload_module(), no matter if
	you're actually using any of the modules or not - of course, the
	add-on won't be unloaded until the last put_module().
*/
status_t
load_module(const char* path, module_info*** _modules)
{
	module_image* moduleImage;
	status_t status = get_module_image(path, &moduleImage);
	if (status != B_OK)
		return status;

	*_modules = moduleImage->info;
	return B_OK;
}


status_t
start_watching_modules(const char* prefix, NotificationListener& listener)
{
	KMessage specifier;
	status_t status = specifier.AddString("prefix", prefix);
	if (status != B_OK)
		return status;

	return sModuleNotificationService.AddListener(&specifier, listener);
}


status_t
stop_watching_modules(const char* prefix, NotificationListener& listener)
{
	KMessage specifier;
	status_t status = specifier.AddString("prefix", prefix);
	if (status != B_OK)
		return status;

	return sModuleNotificationService.RemoveListener(&specifier, listener);
}


/*! Setup the module structures and data for use - must be called
	before any other module call.
*/
status_t
module_init(kernel_args* args)
{
	struct preloaded_image* image;

	recursive_lock_init(&sModulesLock, "modules rlock");

	sModulesHash = hash_init(MODULE_HASH_SIZE, 0, module_compare, module_hash);
	if (sModulesHash == NULL)
		return B_NO_MEMORY;

	sModuleImagesHash = hash_init(MODULE_HASH_SIZE, 0, module_image_compare,
		module_image_hash);
	if (sModuleImagesHash == NULL)
		return B_NO_MEMORY;

	// register built-in modules

	register_builtin_modules(sBuiltInModules);

	// register preloaded images

	for (image = args->preloaded_images; image != NULL; image = image->next) {
		status_t status = register_preloaded_module_image(image);
		if (status != B_OK && image->is_module) {
			dprintf("Could not register image \"%s\": %s\n", image->name,
				strerror(status));
		}
	}

	new(&sModuleNotificationService) ModuleNotificationService();

	sDisableUserAddOns = get_safemode_boolean(B_SAFEMODE_DISABLE_USER_ADD_ONS,
		false);

	add_debugger_command("modules", &dump_modules,
		"list all known & loaded modules");

	return B_OK;
}


status_t
module_init_post_threads(void)
{
	return register_kernel_daemon(
		&ModuleNotificationService::HandleNotifications, NULL, 10);
		// once every second

	return B_OK;
}


status_t
module_init_post_boot_device(bool bootingFromBootLoaderVolume)
{
	// Remove all unused pre-loaded module images. Now that the boot device is
	// available, we can load an image when we need it.
	// When the boot volume is also where the boot loader pre-loaded the images
	// from, we get the actual paths for those images.
	TRACE(("module_init_post_boot_device(%d)\n", bootingFromBootLoaderVolume));

	RecursiveLocker _(sModulesLock);

	// First of all, clear all pre-loaded module's module_image, if the module
	// isn't in use.
	hash_iterator iterator;
	hash_open(sModulesHash, &iterator);
	struct module* module;
	while ((module = (struct module*)hash_next(sModulesHash, &iterator))
			!= NULL) {
		if (module->ref_count == 0
			&& (module->flags & B_BUILT_IN_MODULE) == 0) {
			TRACE(("  module %p, \"%s\" unused, clearing image\n", module,
				module->name));
			module->module_image = NULL;
		}
	}

	// Now iterate through the images and drop them respectively normalize their
	// paths.
	hash_open(sModuleImagesHash, &iterator);

	module_image* imagesToReinsert = NULL;
		// When renamed, an image is added to this list to be re-entered in the
		// hash at the end. We can't do that during the iteration.

	while (true) {
		struct module_image* image
			= (struct module_image*)hash_next(sModuleImagesHash, &iterator);
		if (image == NULL)
			break;

		if (image->ref_count == 0) {
			// not in use -- unload it
			TRACE(("  module image %p, \"%s\" unused, removing\n", image,
				image->path));
			hash_remove_current(sModuleImagesHash, &iterator);
			unload_module_image(image, false);
		} else if (bootingFromBootLoaderVolume) {
			bool pathNormalized = false;
			KPath pathBuffer;
			if (image->path[0] != '/') {
				// relative path
				for (uint32 i = kNumModulePaths; i-- > 0;) {
					if (sDisableUserAddOns && i >= kFirstNonSystemModulePath)
						continue;

					if (find_directory(kModulePaths[i], gBootDevice, true,
							pathBuffer.LockBuffer(), pathBuffer.BufferSize())
								!= B_OK) {
						pathBuffer.UnlockBuffer();
						continue;
					}

					pathBuffer.UnlockBuffer();

					// Append the relative boot module directory and the
					// relative image path, normalize the path, and check
					// whether it exists.
					struct stat st;
					if (pathBuffer.Append("kernel/boot") != B_OK
						|| pathBuffer.Append(image->path) != B_OK
						|| pathBuffer.Normalize(true) != B_OK
						|| lstat(pathBuffer.Path(), &st) != 0) {
						continue;
					}

					pathNormalized = true;
					break;
				}
			} else {
				// absolute path -- try to normalize it anyway
				struct stat st;
				if (pathBuffer.SetPath(image->path) == B_OK
					&& pathBuffer.Normalize(true) == B_OK
					&& lstat(pathBuffer.Path(), &st) == 0) {
					pathNormalized = true;
				}
			}

			if (pathNormalized) {
				TRACE(("  normalized path of module image %p, \"%s\" -> "
					"\"%s\"\n", image, image->path, pathBuffer.Path()));

				// set the new path
				free(image->path);
				size_t pathLen = pathBuffer.Length();
				image->path = (char*)realloc(pathBuffer.DetachBuffer(),
					pathLen + 1);

				// remove the image -- its hash value has probably changed,
				// so we need to re-insert it later
				hash_remove_current(sModuleImagesHash, &iterator);
				image->next = imagesToReinsert;
				imagesToReinsert = image;
			} else {
				dprintf("module_init_post_boot_device() failed to normalize "
					"path of module image %p, \"%s\"\n", image, image->path);
			}
		}
	}

	// re-insert the images that have got a new path
	while (module_image* image = imagesToReinsert) {
		imagesToReinsert = image->next;
		hash_insert(sModuleImagesHash, image);
	}

	TRACE(("module_init_post_boot_device() done\n"));

	return B_OK;
}


//	#pragma mark - Exported Kernel API (public part)


/*! This returns a pointer to a structure that can be used to
	iterate through a list of all modules available under
	a given prefix that adhere to the specified suffix.
	All paths will be searched and the returned list will
	contain all modules available under the prefix.
	The structure is then used by read_next_module_name(), and
	must be freed by calling close_module_list().
*/
void*
open_module_list_etc(const char* prefix, const char* suffix)
{
	TRACE(("open_module_list(prefix = %s)\n", prefix));

	if (sModulesHash == NULL) {
		dprintf("open_module_list() called too early!\n");
		return NULL;
	}

	module_iterator* iterator = (module_iterator*)malloc(
		sizeof(module_iterator));
	if (iterator == NULL)
		return NULL;

	memset(iterator, 0, sizeof(module_iterator));

	iterator->prefix = strdup(prefix != NULL ? prefix : "");
	if (iterator->prefix == NULL) {
		free(iterator);
		return NULL;
	}
	iterator->prefix_length = strlen(iterator->prefix);

	iterator->suffix = suffix;
	if (suffix != NULL)
		iterator->suffix_length = strlen(iterator->suffix);

	if (gBootDevice > 0) {
		// We do have a boot device to scan

		// first, we'll traverse over the built-in modules
		iterator->builtin_modules = true;
		iterator->loaded_modules = false;

		// put all search paths on the stack
		for (uint32 i = 0; i < kNumModulePaths; i++) {
			if (sDisableUserAddOns && i >= kFirstNonSystemModulePath)
				break;

			KPath pathBuffer;
			if (find_directory(kModulePaths[i], gBootDevice, true,
					pathBuffer.LockBuffer(), pathBuffer.BufferSize()) != B_OK)
				continue;

			pathBuffer.UnlockBuffer();
			pathBuffer.Append("kernel");

			// Copy base path onto the iterator stack
			char* path = strdup(pathBuffer.Path());
			if (path == NULL)
				continue;

			size_t length = strlen(path);

			// TODO: it would currently be nicer to use the commented
			// version below, but the iterator won't work if the prefix
			// is inside a module then.
			// It works this way, but should be done better.
#if 0
			// Build path component: base path + '/' + prefix
			size_t length = strlen(sModulePaths[i]);
			char* path = (char*)malloc(length + iterator->prefix_length + 2);
			if (path == NULL) {
				// ToDo: should we abort the whole operation here?
				//	if we do, don't forget to empty the stack
				continue;
			}

			memcpy(path, sModulePaths[i], length);
			path[length] = '/';
			memcpy(path + length + 1, iterator->prefix,
				iterator->prefix_length + 1);
#endif

			iterator_push_path_on_stack(iterator, path, length + 1);
		}
	} else {
		// include loaded modules in case there is no boot device yet
		iterator->builtin_modules = false;
		iterator->loaded_modules = true;
	}

	return (void*)iterator;
}


void*
open_module_list(const char* prefix)
{
	return open_module_list_etc(prefix, NULL);
}


/*!	Frees the cookie allocated by open_module_list() */
status_t
close_module_list(void* cookie)
{
	module_iterator* iterator = (module_iterator*)cookie;
	const char* path;

	TRACE(("close_module_list()\n"));

	if (iterator == NULL)
		return B_BAD_VALUE;

	// free stack
	while ((path = iterator_pop_path_from_stack(iterator, NULL)) != NULL)
		free((char*)path);

	// close what have been left open
	if (iterator->module_image != NULL)
		put_module_image(iterator->module_image);

	if (iterator->current_dir != NULL)
		closedir(iterator->current_dir);

	free(iterator->stack);
	free((char*)iterator->current_path);
	free((char*)iterator->current_module_path);

	free(iterator->prefix);
	free(iterator);

	return B_OK;
}


/*!	Return the next module name from the available list, using
	a structure previously created by a call to open_module_list().
	Returns B_OK as long as it found another module, B_ENTRY_NOT_FOUND
	when done.
*/
status_t
read_next_module_name(void* cookie, char* buffer, size_t* _bufferSize)
{
	module_iterator* iterator = (module_iterator*)cookie;
	status_t status;

	TRACE(("read_next_module_name: looking for next module\n"));

	if (iterator == NULL || buffer == NULL || _bufferSize == NULL)
		return B_BAD_VALUE;

	if (iterator->status < B_OK)
		return iterator->status;

	status = iterator->status;
	recursive_lock_lock(&sModulesLock);

	status = iterator_get_next_module(iterator, buffer, _bufferSize);

	iterator->status = status;
	recursive_lock_unlock(&sModulesLock);

	TRACE(("read_next_module_name: finished with status %s\n",
		strerror(status)));
	return status;
}


/*!	Iterates through all loaded modules, and stores its path in "buffer".
	TODO: check if the function in BeOS really does that (could also mean:
		iterate through all modules that are currently loaded; have a valid
		module_image pointer)
*/
status_t
get_next_loaded_module_name(uint32* _cookie, char* buffer, size_t* _bufferSize)
{
	if (sModulesHash == NULL) {
		dprintf("get_next_loaded_module_name() called too early!\n");
		return B_ERROR;
	}

	//TRACE(("get_next_loaded_module_name(\"%s\")\n", buffer));

	if (_cookie == NULL || buffer == NULL || _bufferSize == NULL)
		return B_BAD_VALUE;

	status_t status = B_ENTRY_NOT_FOUND;
	uint32 offset = *_cookie;

	RecursiveLocker _(sModulesLock);

	hash_iterator iterator;
	hash_open(sModulesHash, &iterator);
	struct module* module = (struct module*)hash_next(sModulesHash, &iterator);

	for (uint32 i = 0; module != NULL; i++) {
		if (i >= offset) {
			*_bufferSize = strlcpy(buffer, module->name, *_bufferSize);
			*_cookie = i + 1;
			status = B_OK;
			break;
		}
		module = (struct module*)hash_next(sModulesHash, &iterator);
	}

	hash_close(sModulesHash, &iterator, false);

	return status;
}


status_t
get_module(const char* path, module_info** _info)
{
	module_image* moduleImage = NULL;
	module* module;
	status_t status;

	TRACE(("get_module(%s)\n", path));

	if (path == NULL)
		return B_BAD_VALUE;

	RecursiveLocker _(sModulesLock);

	module = (struct module*)hash_lookup(sModulesHash, path);

	// if we don't have it cached yet, search for it
	if (module == NULL || ((module->flags & B_BUILT_IN_MODULE) == 0
			&& module->module_image == NULL)) {
		module = search_module(path, &moduleImage);
		if (module == NULL) {
			FATAL(("module: Search for %s failed.\n", path));
			return B_ENTRY_NOT_FOUND;
		}

		module->info = moduleImage->info[module->offset];
		module->module_image = moduleImage;
	} else if ((module->flags & B_BUILT_IN_MODULE) == 0 && gBootDevice < 0
		&& module->ref_count == 0) {
		// The boot volume isn't available yet. I.e. instead of searching the
		// right module image, we already know it and just increment the ref
		// count.
		atomic_add(&module->module_image->ref_count, 1);
	}

	// The state will be adjusted by the call to init_module
	// if we have just loaded the file
	if (module->ref_count == 0) {
		status = init_module(module);
		// For "keep loaded" modules we increment the ref count here. That will
		// cause them never to get unloaded.
		if (status == B_OK && (module->flags & B_KEEP_LOADED) != 0)
			module->ref_count++;
	} else
		status = B_OK;

	if (status == B_OK) {
		ASSERT(module->ref_count >= 0);
		module->ref_count++;
		*_info = module->info;
	} else if ((module->flags & B_BUILT_IN_MODULE) == 0
		&& module->ref_count == 0) {
		// initialization failed -- release the image reference
		put_module_image(module->module_image);
		if (gBootDevice >= 0)
			module->module_image = NULL;
	}

	return status;
}


status_t
put_module(const char* path)
{
	module* module;

	TRACE(("put_module(path = %s)\n", path));

	RecursiveLocker _(sModulesLock);

	module = (struct module*)hash_lookup(sModulesHash, path);
	if (module == NULL) {
		FATAL(("module: We don't seem to have a reference to module %s\n",
			path));
		return B_BAD_VALUE;
	}

	if (module->ref_count == 0) {
		panic("module %s has no references.\n", path);
		return B_BAD_VALUE;
	}

	if (--module->ref_count == 0) {
		if ((module->flags & B_KEEP_LOADED) != 0) {
			panic("ref count of B_KEEP_LOADED module %s dropped to 0!",
				module->name);
			module->ref_count++;
			return B_BAD_VALUE;
		}

		uninit_module(module);

		if ((module->flags & B_BUILT_IN_MODULE) == 0
			&& module->ref_count == 0) {
				// uninit_module() increments the ref count on failure
			put_module_image(module->module_image);
			// Unless we don't have a boot device yet, we clear the module's
			// image pointer if the ref count dropped to 0. get_module() will
			// have to reload the image.
			if (gBootDevice >= 0)
				module->module_image = NULL;
		}
	}

	return B_OK;
}
