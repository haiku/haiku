/* Module manager */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kmodule.h>
#include <lock.h>
#include <khash.h>
#include <elf.h>
#include <boot/elf.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MODULE_HASH_SIZE 16

/** The modules referenced by this structure are built-in
 *	modules that can't be loaded from disk.
 */

extern module_info gDeviceManagerModule;
extern module_info gDeviceRootModule;

static module_info *sBuiltInModules[] = {
	&gDeviceManagerModule,
	&gDeviceRootModule,
	NULL
};

#define TRACE_MODULE
#ifdef TRACE_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif
#define FATAL(x) dprintf x


typedef enum {
	MODULE_QUERIED = 0,
	MODULE_LOADED,
	MODULE_INIT,
	MODULE_READY,
	MODULE_UNINIT,
	MODULE_ERROR
} module_state;


/* Each loaded module image (which can export several modules) is put
 * in a hash (gModuleImagesHash) to be easily found when you search
 * for a specific file name.
 * ToDo: Could use only the inode number for hashing. Would probably be
 * a little bit slower, but would lower the memory foot print quite a lot.
 */

typedef struct module_image {
	struct module_image	*next;
	module_info			**info;		/* the module_info we use */
	module_dependency	*dependencies;
	char				*path;		/* the full path for the module */
	image_id			image;
	int32				ref_count;	/* how many ref's to this file */
	bool				keep_loaded;
} module_image;

/* Each known module will have this structure which is put in the
 * gModulesHash, and looked up by name.
 */

typedef struct module {
	struct module		*next;
	module_image		*module_image;
	char				*name;
	char				*file;
	int32				ref_count;
	module_info			*info;		/* will only be valid if ref_count > 0 */
	int32				offset;		/* this is the offset in the headers */
	module_state		state;		/* state of module */
	uint32				flags;
} module;

#define B_BUILT_IN_MODULE	2

typedef struct module_path {
	const char			*name;
	uint32				base_length;
} module_path;

typedef struct module_iterator {
	module_path			*stack;
	int32				stack_size;
	int32				stack_current;

	char				*prefix;
	size_t				prefix_length;
	DIR					*current_dir;
	status_t			status;
	int32				module_offset;
		/* This is used to keep track of which module_info
		 * within a module we're addressing. */
	module_image		*module_image;
	module_info			**current_header;
	const char			*current_path;
	int32				path_base_length;
	const char			*current_module_path;
	bool				builtin_modules;
} module_iterator;


static bool sDisableUserAddOns = false;

/* locking scheme: there is a global lock only; having several locks
 * makes trouble if dependent modules get loaded concurrently ->
 * they have to wait for each other, i.e. we need one lock per module;
 * also we must detect circular references during init and not dead-lock
 */
static recursive_lock sModulesLock;		

/* These are the standard base paths where we start to look for modules
 * to load. Order is important, the last entry here will be searched
 * first.
 * ToDo: these are not yet BeOS compatible (because the current bootfs is very limited)
 * ToDo: these should probably be retrieved by using find_directory().
 */
static const char * const sModulePaths[] = {
	"/boot/addons/kernel",
	"/boot/home/config/add-ons/kernel",
};

#define NUM_MODULE_PATHS (sizeof(sModulePaths) / sizeof(sModulePaths[0]))
#define USER_MODULE_PATHS 1		/* first user path */

/* we store the loaded modules by directory path, and all known modules by module name
 * in a hash table for quick access
 */
static hash_table *sModuleImagesHash;
static hash_table *sModulesHash;


/** calculates hash for a module using its name */

static uint32
module_hash(void *_module, const void *_key, uint32 range)
{
	module *module = (struct module *)_module;
	const char *name = (const char *)_key;

	if (module != NULL)
		return hash_hash_string(module->name) % range;
	
	if (name != NULL)
		return hash_hash_string(name) % range;

	return 0;
}


/** compares a module to a given name */

static int
module_compare(void *_module, const void *_key)
{
	module *module = (struct module *)_module;
	const char *name = (const char *)_key;
	if (name == NULL)
		return -1;

	return strcmp(module->name, name);
}


/** calculates the hash of a module image using its path */

static uint32
module_image_hash(void *_module, const void *_key, uint32 range)
{
	module_image *image = (module_image *)_module;
	const char *path = (const char *)_key;

	if (image != NULL)
		return hash_hash_string(image->path) % range;

	if (path != NULL)
		return hash_hash_string(path) % range;

	return 0;
}


/** compares a module image to a path */

static int
module_image_compare(void *_module, const void *_key)
{
	module_image *image = (module_image *)_module;
	const char *path = (const char *)_key;
	if (path == NULL)
		return -1;

	return strcmp(image->path, path);
}


static inline void
inc_module_ref_count(struct module *module)
{
	module->ref_count++;
}


static inline void
dec_module_ref_count(struct module *module)
{
	module->ref_count--;
}


/** Try to load the module image at the specified location.
 *	If it could be loaded, it returns B_OK, and stores a pointer
 *	to the module_image object in "_moduleImage".
 */

static status_t
load_module_image(const char *path, module_image **_moduleImage)
{
	module_image *moduleImage;
	status_t status;
	image_id image;

	TRACE(("load_module_image(path = \"%s\", _image = %p)\n", path, _moduleImage));
	ASSERT(_moduleImage != NULL);

	image = load_kernel_add_on(path);
	if (image < 0) {
		dprintf("load_module_image failed: %s\n", strerror(image));
		return image;
	}

	moduleImage = (module_image *)malloc(sizeof(module_image));
	if (!moduleImage) {
		status = B_NO_MEMORY;
		goto err;
	}

	if (get_image_symbol(image, "modules", B_SYMBOL_TYPE_DATA,
			(void **)&moduleImage->info) != B_OK) {
		FATAL(("load_module_image: Failed to load %s due to lack of 'modules' symbol\n", path));
		status = B_BAD_TYPE;
		goto err1;
	}

	moduleImage->dependencies = NULL;
	get_image_symbol(image, "module_dependencies", B_SYMBOL_TYPE_DATA,
		(void **)&moduleImage->dependencies);
		// this is allowed to be NULL

	moduleImage->path = strdup(path);
	if (!moduleImage->path) {
		status = B_NO_MEMORY;
		goto err1;
	}

	moduleImage->image = image;
	moduleImage->ref_count = 0;
	moduleImage->keep_loaded = false;

	recursive_lock_lock(&sModulesLock);
	hash_insert(sModuleImagesHash, moduleImage);
	recursive_lock_unlock(&sModulesLock);

	*_moduleImage = moduleImage;
	return B_OK;

err1:
	free(moduleImage);
err:
	unload_kernel_add_on(image);

	return status;
}


static status_t
unload_module_image(module_image *moduleImage, const char *path)
{
	TRACE(("unload_module_image(image = %p, path = %s)\n", moduleImage, path));

	if (moduleImage == NULL) {
		// if no image was specified, lookup it up in the hash table
		moduleImage = (module_image *)hash_lookup(sModuleImagesHash, path);
		if (moduleImage == NULL)
			return B_ENTRY_NOT_FOUND;
	}

	if (moduleImage->ref_count != 0) {
		FATAL(("Can't unload %s due to ref_cnt = %ld\n", moduleImage->path, moduleImage->ref_count));
		return B_ERROR;
	}

	recursive_lock_lock(&sModulesLock);
	hash_remove(sModuleImagesHash, moduleImage);
	recursive_lock_unlock(&sModulesLock);

	unload_kernel_add_on(moduleImage->image);
	free(moduleImage->path);
	free(moduleImage);

	return B_OK;
}


static void
put_module_image(module_image *image)
{
	int32 refCount = atomic_add(&image->ref_count, -1);
	ASSERT(refCount > 0);

	if (refCount == 1 && !image->keep_loaded)
		unload_module_image(image, NULL);
}


static status_t
get_module_image(const char *path, module_image **_image)
{
	struct module_image *image;

	TRACE(("get_module_image(path = \"%s\", _image = %p)\n", path, _image));

	image = (module_image *)hash_lookup(sModuleImagesHash, path);
	if (image == NULL) {
		status_t status = load_module_image(path, &image);
		if (status < B_OK)
			return status;
	}

	atomic_add(&image->ref_count, 1);
	*_image = image;

	return B_OK;
}


/** Extract the information from the module_info structure pointed at
 *	by "info" and create the entries required for access to it's details.
 */

static status_t
create_module(module_info *info, const char *file, int offset, module **_module)
{
	module *module;

	TRACE(("create_module(info = %p, file = \"%s\", offset = %d, _module = %p)\n",
		info, file, offset, _module));

	if (!info->name)
		return B_BAD_VALUE;

	module = (struct module *)hash_lookup(sModulesHash, info->name);
	if (module) {
		FATAL(("Duplicate module name (%s) detected... ignoring new one\n", info->name));
		return B_FILE_EXISTS;
	}

	if ((module = (struct module *)malloc(sizeof(struct module))) == NULL)
		return B_NO_MEMORY;

	TRACE(("create_module: name = \"%s\", file = \"%s\"\n", info->name, file));

	module->module_image = NULL;
	module->name = strdup(info->name);
	if (module->name == NULL) {
		free(module);
		return B_NO_MEMORY;
	}

	module->file = strdup(file);
	if (module->file == NULL) {
		free(module->name);
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


/** Loads the file at "path" and scans all modules contained therein.
 *	Returns B_OK if "searchedName" could be found under those modules,
 *	B_ENTRY_NOT_FOUND if not.
 *	Must only be called for files that haven't been scanned yet.
 *	"searchedName" is allowed to be NULL (if all modules should be scanned)
 */

static status_t
check_module_image(const char *path, const char *searchedName)
{
	module_image *image;
	module_info **info;
	int index = 0, match = B_ENTRY_NOT_FOUND;

	TRACE(("check_module_image(path = \"%s\", searchedName = \"%s\")\n", path, searchedName));
	ASSERT(hash_lookup(sModuleImagesHash, path) == NULL);

	if (load_module_image(path, &image) < B_OK)
		return B_ENTRY_NOT_FOUND;

	for (info = image->info; *info; info++) {
		// try to create a module for every module_info, check if the
		// name matches if it was a new entry
		if (create_module(*info, path, index++, NULL) == B_OK) {
			if (searchedName && !strcmp((*info)->name, searchedName))
				match = B_OK;
		}
	}

	// The module we looked for couldn't be found, so we can unload the
	// loaded module at this point
	if (match != B_OK) {
		TRACE(("check_module_file: unloading module file \"%s\" (not used yet)\n", path));
		unload_module_image(image, path);
	}

	return match;
}

	
/** Recursively scans through the provided path for the specified module
 *	named "searchedName".
 *	If "searchedName" is NULL, all modules will be scanned.
 *	Returns B_OK if the module could be found, B_ENTRY_NOT_FOUND if not,
 *	or some other error occured during scanning.
 */

static status_t
recurse_directory(const char *path, const char *searchedName)
{
	status_t status;

	DIR *dir = opendir(path);
	if (dir == NULL)
		return errno;

	errno = 0;

	// loop until we have a match or we run out of entries
	while (true) {
		struct dirent *dirent;
		struct stat st;
		char *newPath;
		size_t size = 0;

		TRACE(("scanning %s\n", path));

		dirent = readdir(dir);
		if (dirent == NULL) {
			// we tell the upper layer we couldn't find anything in here
			status = errno == 0 ? B_ENTRY_NOT_FOUND : errno;
			goto exit;
		}

		size = strlen(path) + strlen(dirent->d_name) + 2;	
		newPath = (char *)malloc(size);
		if (newPath == NULL) {
			status = B_NO_MEMORY;
			goto exit;
		}

		strlcpy(newPath, path, size);
		strlcat(newPath, "/", size);
			// two slashes wouldn't hurt
		strlcat(newPath, dirent->d_name, size);

		if (stat(newPath, &st) != 0) {
			free(newPath);
			errno = 0;

			// If we couldn't stat the current file, we will just ignore it;
			// it's a problem of the file system, not ours.
			continue;
		}

		if (S_ISREG(st.st_mode)) {
			// if it's a file, check if we already have it in the hash table,
			// because then we know it doesn't contain the module we are
			// searching for (we are here because it couldn't be found in
			// the first place)
			if (hash_lookup(sModuleImagesHash, newPath) != NULL)
				continue;

			status = check_module_image(newPath, searchedName);
		} else if (S_ISDIR(st.st_mode))
			status = recurse_directory(newPath, searchedName);
		else
			status = B_ERROR;

		if (status == B_OK)
			goto exit;

		free(newPath);
	}

exit:
	closedir(dir);
	return status;
}


/** This is only called if we fail to find a module already in our cache...
 *	saves us some extra checking here :)
 */

static module *
search_module(const char *name)
{
	status_t status = B_ENTRY_NOT_FOUND;
	uint32 i;

	TRACE(("search_module(%s)\n", name));

	// ToDo: As Ingo found out, BeOS uses the module name to locate the module
	//		on disk. We now have the vfs_get_module_path() call to achieve this.
	//		As soon as we boot from a file system other than bootfs, we should
	//		change the loading behaviour to only use that function (bootfs has
	//		a very low maximum path length, which makes it unable to contain
	//		the standard module directories).
	//		The call to vfs_get_module_path() is only for testing purposes

	for (i = 0; i < NUM_MODULE_PATHS; i++) {
		if (sDisableUserAddOns && i >= USER_MODULE_PATHS)
			return NULL;

		{
			char path[B_FILE_NAME_LENGTH];
			if (vfs_get_module_path(sModulePaths[i], name, path, sizeof(path)) < B_OK) {
				TRACE(("vfs_get_module_path() failed for \"%s\"\n", name));
			} else {
				TRACE(("vfs_get_module_path(): found \"%s\" (for \"%s\")\n", path, name));
			}
		}
		if ((status = recurse_directory(sModulePaths[i], name)) == B_OK)
			break;
	}

	if (status != B_OK)
		return NULL;

	return (module *)hash_lookup(sModulesHash, name);
}


static status_t
put_dependent_modules(struct module *module)
{
	module_dependency *dependencies;
	int32 i = 0;

	if (module->module_image == NULL
		|| (dependencies = module->module_image->dependencies) == NULL)
		return B_OK;

	for (; dependencies[i].name != NULL; i++) {
		status_t status = put_module(dependencies[i].name);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


static status_t
get_dependent_modules(struct module *module)
{
	module_dependency *dependencies;
	int32 i = 0;

	// built-in modules don't have a module_image structure
	if (module->module_image == NULL
		|| (dependencies = module->module_image->dependencies) == NULL)
		return B_OK;

	TRACE(("resolving module dependencies...\n"));

	for (; dependencies[i].name != NULL; i++) {
		status_t status = get_module(dependencies[i].name, dependencies[i].info);
		if (status < B_OK) {
			TRACE(("loading dependent module \"%s\" failed!\n", dependencies[i].name));
			return status;
		}
	}

	return B_OK;
}


/** Initializes a loaded module depending on its state */

static inline status_t
init_module(module *module)
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

dprintf("module = %p, info = %p\n", module, module->info);
			TRACE(("initializing module %s (at %p)... \n", module->name, module->info->std_ops));
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
			FATAL(("tried to load module %s which is currently unloading\n", module->name));
			return B_ERROR;

		case MODULE_ERROR:
			FATAL(("cannot load module %s because its earlier unloading failed\n", module->name));
			return B_ERROR;

		default:
			return B_ERROR;
	}
	// never trespasses here
}


/** Uninitializes a module depeding on its state */

static inline int
uninit_module(module *module)
{
	switch (module->state) {
		case MODULE_QUERIED:
		case MODULE_LOADED:
			return B_NO_ERROR;

		case MODULE_INIT:
			panic("Trying to unload module %s which is initializing\n", module->name);
			return B_ERROR;

		case MODULE_UNINIT:
			panic("Trying to unload module %s which is un-initializing\n", module->name);
			return B_ERROR;

		case MODULE_READY:
		{
			status_t status;

			module->state = MODULE_UNINIT;

			TRACE(("uninitializing module %s...\n", module->name));
			status = module->info->std_ops(B_MODULE_UNINIT);
			TRACE(("...done (%s)\n", strerror(status)));

			if (status == B_NO_ERROR) {
				module->state = MODULE_LOADED;

				put_dependent_modules(module);
				return 0;
			}

			FATAL(("Error unloading module %s (%s)\n", module->name, strerror(status)));

			module->state = MODULE_ERROR;
			module->flags |= B_KEEP_LOADED;

			return status;
		}
		default:	
			return B_ERROR;		
	}
	// never trespasses here
}


static const char *
iterator_pop_path_from_stack(module_iterator *iterator, uint32 *_baseLength)
{
	if (iterator->stack_current <= 0)
		return NULL;

	if (_baseLength)
		*_baseLength = iterator->stack[iterator->stack_current - 1].base_length;

	return iterator->stack[--iterator->stack_current].name;
}


static status_t
iterator_push_path_on_stack(module_iterator *iterator, const char *path, uint32 baseLength)
{
	if (iterator->stack_current + 1 > iterator->stack_size) {
		// allocate new space on the stack
		module_path *stack = (module_path *)realloc(iterator->stack,
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


static status_t
iterator_get_next_module(module_iterator *iterator, char *buffer, size_t *_bufferSize)
{
	status_t status;

	TRACE(("iterator_get_next_module() -- start\n"));

	if (iterator->builtin_modules) {
		int32 i;

		for (i = iterator->module_offset; sBuiltInModules[i] != NULL; i++) {
			// the module name must fit the prefix
			if (strncmp(sBuiltInModules[i]->name, iterator->prefix, iterator->prefix_length))
				continue;

			*_bufferSize = strlcpy(buffer, sBuiltInModules[i]->name, *_bufferSize);
			iterator->module_offset = i + 1;
			return B_OK;
		}
		iterator->builtin_modules = false;
	}

nextPath:
	if (iterator->current_dir == NULL) {
		// get next directory path from the stack
		const char *path = iterator_pop_path_from_stack(iterator, &iterator->path_base_length);
		if (path == NULL) {
			// we are finished, there are no more entries on the stack
			return B_ENTRY_NOT_FOUND;
		}

		free((void *)iterator->current_path);
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
	if (iterator->current_header == NULL) {
		// get next entry from the current directory
		char path[B_PATH_NAME_LENGTH];
		struct dirent *dirent;
		struct stat st;
		int32 passedOffset, commonLength;

		errno = 0;

		if ((dirent = readdir(iterator->current_dir)) == NULL) {
			closedir(iterator->current_dir);
			iterator->current_dir = NULL;

			if (errno < B_OK)
				return errno;

			goto nextPath;
		}

		// check if the prefix matches
		passedOffset = strlen(iterator->current_path) + 1;
		commonLength = iterator->path_base_length + iterator->prefix_length - passedOffset;

		if (commonLength > 0) {
			// the prefix still reaches into the new path part
			int32 length = strlen(dirent->d_name);
			if (commonLength > length)
				commonLength = length;

			if (strncmp(dirent->d_name,
					iterator->prefix + passedOffset - iterator->path_base_length, commonLength))
				goto nextModuleImage;
		}

		// we're not interested in traversing these again
		if (!strcmp(dirent->d_name, ".")
			|| !strcmp(dirent->d_name, ".."))
			goto nextModuleImage;

		// build absolute path to current file
		strlcpy(path, iterator->current_path, sizeof(path));
		strlcat(path, "/", sizeof(path));
		strlcat(path, dirent->d_name, sizeof(path));

		// find out if it's a directory or a file
		if (stat(path, &st) < 0)
			return errno;

		iterator->current_module_path = strdup(path);
		if (iterator->current_module_path == NULL)
			return B_NO_MEMORY;

		if (S_ISDIR(st.st_mode)) {
			status = iterator_push_path_on_stack(iterator, iterator->current_module_path,
							iterator->path_base_length);
			if (status < B_OK)
				return status;

			iterator->current_module_path = NULL;
			goto nextModuleImage;
		}

		if (!S_ISREG(st.st_mode))
			return B_BAD_TYPE;

		TRACE(("open module at %s\n", path));

		status = get_module_image(path, &iterator->module_image);
		if (status < B_OK) {
			free((void *)iterator->current_module_path);
			iterator->current_module_path = NULL;
			goto nextModuleImage;
		}

		iterator->current_header = iterator->module_image->info;
		iterator->module_offset = 0;
	}

	// search the current module image until we've got a match
	while (*iterator->current_header != NULL) {
		module_info *info = *iterator->current_header;

		// ToDo: we might want to create a module here and cache it in the hash table

		iterator->current_header++;
		iterator->module_offset++;

		if (strncmp(info->name, iterator->prefix, iterator->prefix_length))
			continue;

		*_bufferSize = strlcpy(buffer, info->name, *_bufferSize);
		return B_OK;
	}

	// leave this module and get the next one

	iterator->current_header = NULL;
	free((void *)iterator->current_module_path);
	iterator->current_module_path = NULL;

	put_module_image(iterator->module_image);
	iterator->module_image = NULL;

	goto nextModuleImage;
}


static void
register_builtin_modules(struct module_info **info)
{
	for (; *info; info++) {
		(*info)->flags |= B_BUILT_IN_MODULE;
			// this is an internal flag, it doesn't have to be set by modules itself

		if (create_module(*info, "", -1, NULL) != B_OK)
			dprintf("creation of built-in module \"%s\" failed!\n", (*info)->name);
	}
}


static status_t
register_preloaded_module_image(struct preloaded_image *image)
{
	module_image *moduleImage;
	struct module_info **info;
	status_t status;
	int32 index = 0;

	if (image->id < 0)
		return B_BAD_VALUE;

	moduleImage = (module_image *)malloc(sizeof(module_image));
	if (moduleImage == NULL)
		return B_NO_MEMORY;

	if (get_image_symbol(image->id, "modules", B_SYMBOL_TYPE_DATA,
			(void **)&moduleImage->info) != B_OK) {
		status = B_BAD_TYPE;
		goto error;
	}

	moduleImage->dependencies = NULL;
	get_image_symbol(image->id, "module_dependencies", B_SYMBOL_TYPE_DATA,
		(void **)&moduleImage->dependencies);
		// this is allowed to be NULL

	moduleImage->path = strdup(image->name);
	if (!moduleImage->path) {
		status = B_NO_MEMORY;
		goto error;
	}

	moduleImage->image = image->id;
	moduleImage->ref_count = 0;
	moduleImage->keep_loaded = false;

	hash_insert(sModuleImagesHash, moduleImage);

	for (info = moduleImage->info; *info; info++) {
		create_module(*info, image->name, index++, NULL);
	}

	return B_OK;

error:
	free(moduleImage);
	
	// we don't need this image anymore
	unload_kernel_add_on(image->id);

	return status;
}


static int
dump_modules(int argc, char **argv)
{
	hash_iterator iterator;
	struct module_image *image;
	struct module *module;

	hash_rewind(sModulesHash, &iterator);
	dprintf("-- known modules:\n");

	while ((module = (struct module *)hash_next(sModulesHash, &iterator)) != NULL) {
		dprintf("%p: \"%s\", \"%s\" (%ld), refcount = %ld, state = %d, mimage = %p\n",
			module, module->name, module->file, module->offset, module->ref_count,
			module->state, module->module_image);
	}

	hash_rewind(sModuleImagesHash, &iterator);
	dprintf("\n-- loaded module images:\n");

	while ((image = (struct module_image *)hash_next(sModuleImagesHash, &iterator)) != NULL) {
		dprintf("%p: \"%s\" (image_id = %ld), info = %p, refcount = %ld, %s\n", image,
			image->path, image->image, image->info, image->ref_count,
			image->keep_loaded ? "keep loaded" : "can be unloaded");
	}
	return 0;
}


//	#pragma mark -
//	Exported Kernel API (private part)


/** Setup the module structures and data for use - must be called
 *	before any other module call.
 */

status_t
module_init(kernel_args *args)
{
	struct preloaded_image *image;

	if (recursive_lock_init(&sModulesLock, "modules rlock") < B_OK)
		return B_ERROR;

	sModulesHash = hash_init(MODULE_HASH_SIZE, 0, module_compare, module_hash);
	if (sModulesHash == NULL)
		return B_NO_MEMORY;

	sModuleImagesHash = hash_init(MODULE_HASH_SIZE, 0, module_image_compare, module_image_hash);
	if (sModuleImagesHash == NULL)
		return B_NO_MEMORY;

	// register built-in modules

	register_builtin_modules(sBuiltInModules);

	// register preloaded images

	for (image = args->preloaded_images; image != NULL; image = image->next) {
		register_preloaded_module_image(image);
	}

	// ToDo: set sDisableUserAddOns from kernel_args!

	add_debugger_command("modules", &dump_modules, "list all known & loaded modules");

	return B_OK;
}


//	#pragma mark -
//	Exported Kernel API (public part)


/** This returns a pointer to a structure that can be used to
 *	iterate through a list of all modules available under
 *	a given prefix.
 *	All paths will be searched and the returned list will
 *	contain all modules available under the prefix.
 *	The structure is then used by read_next_module_name(), and
 *	must be freed by calling close_module_list().
 */

void *
open_module_list(const char *prefix)
{
	module_iterator *iterator;
	uint32 i;

	TRACE(("open_module_list(prefix = %s)\n", prefix));

	iterator = (module_iterator *)malloc(sizeof(module_iterator));
	if (!iterator)
		return NULL;

	memset(iterator, 0, sizeof(module_iterator));

	// ToDo: possibly, the prefix don't have to be copied, just referenced
	iterator->prefix = strdup(prefix ? prefix : "");
	if (iterator->prefix == NULL) {
		free(iterator);
		return NULL;
	}
	iterator->prefix_length = strlen(prefix);

	// first, we'll traverse over the built-in modules
	iterator->builtin_modules = true;

	// put all search paths on the stack
	for (i = 0; i < NUM_MODULE_PATHS; i++) {
		const char *path;

		if (sDisableUserAddOns && i >= USER_MODULE_PATHS)
			break;

		path = strdup(sModulePaths[i]);
		if (path == NULL) {
			// ToDo: should we abort the whole operation here?
			//	if we do, don't forget to empty the stack
			continue;
		}
		iterator_push_path_on_stack(iterator, path, strlen(path) + 1);
	}

	return (void *)iterator;
}


/** Frees the cookie allocated by open_module_list()
 */

status_t
close_module_list(void *cookie)
{
	module_iterator *iterator = (module_iterator *)cookie;
	const char *path;

	TRACE(("close_module_list()\n"));

	if (iterator == NULL)
		return B_BAD_VALUE;

	// free stack
	while ((path = iterator_pop_path_from_stack(iterator, NULL)) != NULL)
		free((void *)path);

	// close what have been left open
	if (iterator->module_image != NULL)
		put_module_image(iterator->module_image);

	if (iterator->current_dir != NULL)
		closedir(iterator->current_dir);

	free(iterator->stack);
	free((void *)iterator->current_path);
	free((void *)iterator->current_module_path);

	free(iterator->prefix);
	free(iterator);

	return 0;
}


/** Return the next module name from the available list, using
 *	a structure previously created by a call to open_module_list.
 *	Returns B_OK as long as it found another module, B_ENTRY_NOT_FOUND
 *	when done.
 */

status_t
read_next_module_name(void *cookie, char *buffer, size_t *_bufferSize)
{
	module_iterator *iterator = (module_iterator *)cookie;
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

	TRACE(("read_next_module_name: finished with status %s\n", strerror(status)));
	return status;
}


/** Iterates through all loaded modules, and stores its path in "buffer".
 *	ToDo: check if the function in BeOS really does that (could also mean:
 *		iterate through all modules that are currently loaded; have a valid
 *		module_image pointer, which would be hard to test for)
 */

status_t 
get_next_loaded_module_name(uint32 *_cookie, char *buffer, size_t *_bufferSize)
{
	hash_iterator *iterator = (hash_iterator *)*_cookie;
	struct module *module;
	status_t status;

	TRACE(("get_next_loaded_module_name()\n"));

	if (_cookie == NULL || buffer == NULL || _bufferSize == NULL)
		return B_BAD_VALUE;

	if (iterator == NULL) {
		iterator = hash_open(sModulesHash, NULL);
		if (iterator == NULL)
			return B_NO_MEMORY;

		*(hash_iterator **)_cookie = iterator;
	}

	recursive_lock_lock(&sModulesLock);

	module = hash_next(sModulesHash, iterator);
	if (module != NULL) {
		*_bufferSize = strlcpy(buffer, module->name, *_bufferSize);
		status = B_OK;
	} else {
		hash_close(sModulesHash, iterator, true);
		status = B_ENTRY_NOT_FOUND;
	}

	recursive_lock_unlock(&sModulesLock);

	return status;
}


status_t
get_module(const char *path, module_info **_info)
{
	module_image *moduleImage;
	module *module;
	status_t status;

	TRACE(("get_module(%s)\n", path));

	if (path == NULL)
		return B_BAD_VALUE;

	recursive_lock_lock(&sModulesLock);

	module = (struct module *)hash_lookup(sModulesHash, path);

	// if we don't have it cached yet, search for it
	if (module == NULL) {
		module = search_module(path);
		if (module == NULL) {
			FATAL(("module: Search for %s failed.\n", path));
			goto err;
		}
	}

	if ((module->flags & B_BUILT_IN_MODULE) == 0) {
		/* We now need to find the module_image for the module. This should
		 * be in memory if we have just run search_modules, but may not be
		 * if we are using cached information.
		 * We can't use the module->module_image pointer, because it is not
		 * reliable at this point (it won't be set to NULL when the module_image
		 * is unloaded).
		 */
		if (get_module_image(module->file, &moduleImage) < B_OK)
			goto err;
	
		// (re)set in-memory data for the loaded module
		module->info = moduleImage->info[module->offset];
		module->module_image = moduleImage;
	
		// the module image must not be unloaded anymore
		if (module->flags & B_KEEP_LOADED)
			module->module_image->keep_loaded = true;
	}

	inc_module_ref_count(module);

	// The state will be adjusted by the call to init_module
	// if we have just loaded the file
	if (module->ref_count == 1)
		status = init_module(module);
	else
		status = B_OK;

	recursive_lock_unlock(&sModulesLock);

	if (status == B_OK)
		*_info = module->info;

	return status;

err:
	recursive_lock_unlock(&sModulesLock);
	return B_ENTRY_NOT_FOUND;
}


status_t
put_module(const char *path)
{
	module *module;

	TRACE(("put_module(path = %s)\n", path));

	recursive_lock_lock(&sModulesLock);

	module = (struct module *)hash_lookup(sModulesHash, path);
	if (module == NULL) {
		FATAL(("module: We don't seem to have a reference to module %s\n", path));
		recursive_lock_unlock(&sModulesLock);
		return B_BAD_VALUE;
	}
	dec_module_ref_count(module);

	// ToDo: not sure if this should ever be called for keep_loaded modules...
	if (module->ref_count == 0)
		uninit_module(module);

	if ((module->flags & B_BUILT_IN_MODULE) == 0)
		put_module_image(module->module_image);

	recursive_lock_unlock(&sModulesLock);
	return B_OK;
}
