/* Module manager */

/*
** Copyright 2001, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kmodule.h>
#include <lock.h>
#include <Errors.h>
#include <khash.h>
#include <malloc.h>
#include <elf.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define MODULE_HASH_SIZE 16

static bool modules_disable_user_addons = false;

#define TRACE_MODULE 0
#if TRACE_MODULE
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

/* This represents the actual loaded module. The module is loaded and
 * may have more than one exported image, i.e. the module foo may actually have
 * module_info structures for foo and bar.
 * To allow for this each module_info structure within the module loaded is
 * represented by a loaded_module_info structure.
 */
typedef struct loaded_module {
	struct loaded_module	*next;
	module_info				**info;		/* the module_info we use */
	char					*path;		/* the full path for the module */
	image_id				image;
	int32					ref_count;	/* how many ref's to this file */
} loaded_module;

/* This is used to keep a list of module and the file it's found
 * in. It's used when we do searches to record that a module_info for 
 * a particular module is found in a particular file which covers us for
 * the case where we have a single file exporting a number of modules.
 */
typedef struct module {
	struct module			*next;
	struct loaded_module	*loaded_module;
	char					*name;
	char					*file;
	int32					ref_count;
	module_info				*info;		/* will only be valid if ref_cnt > 0 */
	int						offset;		/* this is the offset in the headers */
	module_state			state;		/* state of module */
	bool					keep_loaded;
} module;


#define INC_MOD_REF_COUNT(x) \
	x->ref_count++; \
	x->loaded_module->ref_count++;

#define DEC_MOD_REF_COUNT(x) \
	x->ref_count--; \
	x->loaded_module->ref_count--;


typedef struct module_iterator {
	char						*prefix;
	int							base_path_id;
	struct module_dir_iterator	*base_dir;
	struct module_dir_iterator	*cur_dir;
	int							err;
	int							module_pos;	/* This is used to keep track of which module_info
											 * within a module we're addressing. */
	module_info					**current_header;
	char						*current_path;
} module_iterator;

typedef struct module_dir_iterator {
	struct module_dir_iterator	*parent_dir;
	struct module_dir_iterator	*sub_dir;
	char						*name;
	int							file;
	int							hdr_prefix;
} module_dir_iterator;


/* locking scheme: there is a global lock only; having several locks
 * makes trouble if dependent modules get loaded concurrently ->
 * they have to wait for each other, i.e. we need one lock per module;
 * also we must detect circular references during init and not dead-lock
 */
static recursive_lock gModulesLock;		

/* These are the standard paths that we look on for mdoules to load.
 * By default we only look on these plus the prefix, though we do search
 * below the prefix.
 * i.e. using media as the prefix will match
 *      /boot/user-addons/media
 *      /boot/addons/media
 *      /boot/addons/media/encoders
 * but will NOT match
 *      /boot/addons/kernel/media
 */
const char *const gModulePaths[] = {
	"/boot/user-addons", 
	"/boot/addons"
};

#define NUM_MODULE_PATHS (sizeof(gModulePaths) / sizeof(gModulePaths[0]))

/* we store the loaded modules by directory path, and all known modules by module name
 * in a hash table for quick access
 */
hash_table *gLoadedModulesHash;
hash_table *gModulesHash;


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


/** calculates the hash of a loaded module using its path */

static uint32
loaded_module_hash(void *_module, const void *_key, uint32 range)
{
	loaded_module *loadedModule = (loaded_module *)_module;
	const char *path = (const char *)_key;

	if (loadedModule != NULL)
		return hash_hash_string(loadedModule->path) % range;

	if (path != NULL)
		return hash_hash_string(path) % range;

	return 0;
}


/** compares a loaded module to a path */

static int
loaded_module_compare(void *_module, const void *_key)
{
	loaded_module *loadedModule = (loaded_module *)_module;
	const char *path = (const char *)_key;
	if (path == NULL)
		return -1;

	return strcmp(loadedModule->path, path);
}


/** Try to load the module file we've found into memory.
 *	This may fail if all the symbols can't be resolved.
 *	Returns 0 on success, -1 on failure.
 *
 *	NB hdrs can be passed as a NULL if the modules ** header
 *	pointer isn't required.
 *
 *	Returns
 *		NULL on failure
 *		pointer to modules symbol on success
 */

static module_info **
load_module_file(const char *path)
{
	loaded_module *loadedModule;

	image_id image = elf_load_kspace(path, "");
	if (image < 0) {
		dprintf("load_module_file failed: %s\n", strerror(image));
		return NULL;
	}

	loadedModule = (loaded_module *)malloc(sizeof(loaded_module));
	if (!loadedModule) {
		elf_unload_kspace(path);
		return NULL;
	}

	loadedModule->info = (module_info **)elf_lookup_symbol(image, "modules");
	if (!loadedModule->info) {
		FATAL(("load_module_file: Failed to load %s due to lack of 'modules' symbol\n", path));
		elf_unload_kspace(path);
		free(loadedModule);
		return NULL;
	}

	loadedModule->path = strdup(path);
	if (!loadedModule->path) {
		elf_unload_kspace(path);
		free(loadedModule);
		return NULL;
	}

	loadedModule->image = image;
	loadedModule->ref_count = 0;

	recursive_lock_lock(&gModulesLock);
	hash_insert(gLoadedModulesHash, loadedModule);
	recursive_lock_unlock(&gModulesLock);

	return loadedModule->info;
}


static inline void
unload_module_file(const char *path)
{
	loaded_module *loadedModule;

	TRACE(("unload_mdoule_file: %s\n", path));

	loadedModule = (loaded_module *)hash_lookup(gLoadedModulesHash, path);
	if (loadedModule == NULL)
		return;

	if (loadedModule->ref_count != 0) {
		FATAL(("Can't unload %s due to ref_cnt = %ld\n", loadedModule->path, loadedModule->ref_count));
		return;
	}

	recursive_lock_lock(&gModulesLock);
	hash_remove(gLoadedModulesHash, loadedModule);
	recursive_lock_unlock(&gModulesLock);

	elf_unload_kspace(loadedModule->path);
	free(loadedModule->path);
	free(loadedModule);
}


/** Extract the information from the module_info structure pointed at
 *	by "info" and create the entries required for access to it's details.
 */

static int
create_module(module_info *info, const char *file, int offset, module **_module)
{
	module *module;

	if (!info->name)
		return B_BAD_VALUE;

	module = (struct module *)hash_lookup(gModulesHash, info->name);
	if (module) {
		FATAL(("Duplicate module name (%s) detected... ignoring new one\n", info->name));
		return B_FILE_EXISTS;
	}

	if ((module = (struct module *)malloc(sizeof(module))) == NULL)
		return B_NO_MEMORY;

	TRACE(("create_module(%s, %s)\n", info->name, file));

	module->loaded_module = NULL;
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
	module->offset = offset;
		// record where the module_info can be found in the module_info array
	module->ref_count = 0;

	if (info->flags & B_KEEP_LOADED) {
		TRACE(("module %s wants to be kept loaded\n", module->name));
		module->keep_loaded = true;
	}

	recursive_lock_lock(&gModulesLock);
	hash_insert(gModulesHash, module);
	recursive_lock_unlock(&gModulesLock);

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
check_module_file(const char *path, const char *searchedName)
{
	module_info **header = NULL, **info;
	int index = 0, match = B_ENTRY_NOT_FOUND;

	ASSERT(hash_lookup(gLoadedModulesHash, path) == NULL);

	if ((header = load_module_file(path)) == NULL)
		return -1;

	for (info = header; *info; info++) {
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
		TRACE(("check_module_file: unloading module file %s\n", path));
		unload_module_file(path);
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
	if (dir == NULL);
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
			if (hash_lookup(gLoadedModulesHash, newPath) != NULL)
				continue;

			status = check_module_file(newPath, searchedName);
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
	int i, res = 0;
	TRACE(("search_module(%s)\n", name));
	
	for (i = 0; i < (int)NUM_MODULE_PATHS; ++i) {
		if ((res = recurse_directory(gModulePaths[i], name)) == 1)
			break;
	}

	if (res != 1)
		return NULL;

	return (module *)hash_lookup(gModulesHash, name);
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

			TRACE(("initing module %s... \n", module->name));
			status = module->info->std_ops(B_MODULE_INIT);
			TRACE(("...done (%s)\n", strerror(status)));

			if (!status) 
				module->state = MODULE_READY;
			else
				module->state = MODULE_LOADED;

			return status;
		}

		case MODULE_READY:
			return B_NO_ERROR;

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

			TRACE(("uniniting module %s...\n", module->name));
			status = module->info->std_ops(B_MODULE_UNINIT);
			TRACE(("...done (%s)\n", strerror(status)));

			if (status == B_NO_ERROR) {
				module->state = MODULE_LOADED;
				return 0;
			}

			FATAL(("Error unloading module %s (%s)\n", module->name, strerror(status)));

			module->state = MODULE_ERROR;
			module->keep_loaded = true;

			return status;
		}
		default:	
			return B_ERROR;		
	}
	// never trespasses here
}


static status_t
process_module_info(module_iterator *iterator, char *buffer, size_t *_bufferSize)
{
	module *module = NULL;
	module_info **info = iterator->current_header;
	status_t status = B_NO_ERROR;

	if (!info || !(*info)) {
		status = B_BAD_VALUE;
	} else {
		status = create_module(*info, iterator->current_path, iterator->module_pos++, &module);
		if (status == B_OK) {
			strlcpy(buffer, module->name, *_bufferSize);
			*_bufferSize = strlen(module->name);
		}
	}

	// If we have a valid "info" pointer, traverse to the next, or mark the end
	if (info && *(++info) != NULL)
		iterator->current_header++;
	else
		iterator->current_header = NULL;

	return status;
}


static inline int
module_create_dir_iterator(module_iterator *iter, int file, const char *name)
{
	module_dir_iterator *dir;

	/* if we're creating a dir_iterator, there is no way that the
	 * current_header value can be valid, so make sure and reset it
	 * here.
	 */
	iter->current_header = NULL;

	dir = (struct module_dir_iterator *)malloc(sizeof(*dir));
	if (dir == NULL )
		return ENOMEM;

	dir->name = strdup(name);
	if (dir->name == NULL) {
		free(dir);
		return ENOMEM;
	}

	dir->file = file;
	dir->sub_dir = NULL;		
	dir->parent_dir = iter->cur_dir;

	if (iter->cur_dir)
		iter->cur_dir->sub_dir = dir;
	else
		iter->base_dir = dir;
		
	iter->cur_dir = dir;

	TRACE(("created dir iterator for %s\n", name));
	return B_NO_ERROR;
}


static inline int
module_enter_dir(module_iterator *iter, const char *path)
{
	int dir;
	int res;

	// ToDo: use opendir() instead
	dir = sys_open_dir(path);
	if (dir < 0) {
		TRACE(("couldn't open directory %s (%s)\n", path, strerror(dir)));

		// there are so many errors for "not found" that we don't bother
		// and always assume that the directory suddenly disappeared
		return B_NO_ERROR;
	}

	res = module_create_dir_iterator(iter, dir, path);
	if (res != B_NO_ERROR) {
		sys_close(dir);
		return ENOMEM;
	}

	TRACE(("entered directory %s\n", path));
	return B_NO_ERROR;
}


static inline void
destroy_dir_iterator(module_iterator *iter)
{
	module_dir_iterator *dir = iter->cur_dir;

	TRACE(("destroying directory iterator for sub-dir %s\n", dir->name));

	if (dir->parent_dir)
		dir->parent_dir->sub_dir = NULL;

	iter->cur_dir = dir->parent_dir;

	free(dir->name);
	free(dir);
}


static inline void
module_leave_dir(module_iterator *iter)
{
	module_dir_iterator *parent_dir;
	
	TRACE(("leaving directory %s\n", iter->cur_dir->name));

	parent_dir = iter->cur_dir->parent_dir;
	iter->current_header = NULL;	
	sys_close(iter->cur_dir->file);
	destroy_dir_iterator(iter);

	iter->cur_dir = parent_dir;
}


static void
compose_path(char *path, module_iterator *iter, const char *name, bool full_path)
{
	module_dir_iterator *dir;
	
	if (full_path) {
		strlcpy(path, iter->base_dir->name, SYS_MAX_PATH_LEN);
		strlcat(path, "/", SYS_MAX_PATH_LEN);
	} else {
		strlcpy(path, iter->prefix, SYS_MAX_PATH_LEN);
		if (*iter->prefix)
			strlcat(path, "/", SYS_MAX_PATH_LEN);
	}

	for (dir = iter->base_dir->sub_dir; dir; dir = dir->sub_dir) {
		strlcat(path, dir->name, SYS_MAX_PATH_LEN);
		strlcat(path, "/", SYS_MAX_PATH_LEN);
	}

	strlcat(path, name, SYS_MAX_PATH_LEN);

	TRACE(("name: %s, %s -> %s\n", name, full_path ? "full path" : "relative path", path));
}


/** Logic as follows...
 *	If we have a headers pointer,
 *	 - check if the next structure is NULL, if not process that module_info structure
 *	 - if it's null, close the file, NULL the headers pointer and fall through
 *
 *	This function tries to find the next module filename and then set the headers
 *	pointer in the cur_dir structure.
 */

static inline int
module_traverse_dir(module_iterator *iter)
{
	struct stat st;
	char buffer[SYS_MAX_NAME_LEN + sizeof(struct dirent)];
	struct dirent *dirent = (struct dirent *)buffer;
	char path[SYS_MAX_PATH_LEN];
	int res;

	/* If (*iter->current_header) != NULL we have another module within
	 * the existing file to return, so just return.
	 * Otherwise, actually find the next file to read.
	 */ 
	if (iter->current_header) {
		if (*iter->current_header != NULL)
			return B_OK;

		unload_module_file(iter->current_path);
	}

	TRACE(("scanning %s\n", iter->cur_dir->name));
	if ((res = sys_read_dir(iter->cur_dir->file, dirent, sizeof(buffer), 1)) <= 0) {
		TRACE(("got error: %s\n", strerror(res)));
		module_leave_dir(iter);
		return B_NO_ERROR;
	}

	TRACE(("got %s\n", dirent->d_name));

	if (strcmp(dirent->d_name, ".") == 0
		|| strcmp(dirent->d_name, "..") == 0 )
		return B_NO_ERROR;

	compose_path(path, iter, dirent->d_name, true);

	/* As we're doing a new file, reset the pointers that might get
	 * screwed up...
	 */
	iter->current_header = NULL;
	iter->module_pos = 0;

	if ((res = stat(path, &st)) != B_NO_ERROR)
		return res;

	if (S_ISREG(st.st_mode)) {
		module_info **hdrs = NULL;
		if ((hdrs = load_module_file(path)) != NULL) {
			iter->current_header = hdrs;
			iter->current_path = strdup(path);			
			return B_NO_ERROR;
		}
		return EINVAL; /* not sure what we should return here */
	}

	if (S_ISDIR(st.st_mode))
		return module_enter_dir(iter, path);

	TRACE(("entry %s not a file nor a directory - ignored\n", dirent->d_name));
	return B_NO_ERROR;
}


/** Basically try each of the directories we have listed as module paths,
 *	trying each with the prefix we've been allocated.
 */

static inline int
module_enter_base_path(module_iterator *iter)
{
	char path[SYS_MAX_PATH_LEN];

	++iter->base_path_id;

	if (iter->base_path_id >= NUM_MODULE_PATHS) {
		TRACE(("no locations left\n"));
		return B_ENTRY_NOT_FOUND;
	}

	TRACE(("trying base path (%s)\n", gModulePaths[iter->base_path_id]));

	if (iter->base_path_id == 0 && modules_disable_user_addons) {
		TRACE(("ignoring user add-ons (they are disabled)\n"));
		return B_NO_ERROR;
	}
	strcpy(path, gModulePaths[iter->base_path_id]);
	if (*iter->prefix) {
		strcat(path, "/");
		strlcat(path, iter->prefix, sizeof(path));
	}

	return module_enter_dir(iter, path);
}


//	#pragma mark -
//	Exported Kernel API (private part)


/** Setup the module structures and data for use - must be called
 *	before any other module call.
 */

status_t
module_init(kernel_args *ka, module_info **sys_module_headers)
{
	recursive_lock_create(&gModulesLock);

	gModulesHash = hash_init(MODULE_HASH_SIZE, 0, module_compare, module_hash);
	if (gModulesHash == NULL)
		return B_NO_MEMORY;

	gLoadedModulesHash = hash_init(MODULE_HASH_SIZE, 0, loaded_module_compare, loaded_module_hash);
	if (gLoadedModulesHash == NULL)
		return B_NO_MEMORY;

/*
	if (sys_module_headers) { 
		if (register_module_image("", "(built-in)", 0, sys_module_headers) == NULL)
			return ENOMEM;
	}
*/	

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

	TRACE(("open_module_list(prefix = %s)\n", prefix));

	iterator = (module_iterator *)malloc(sizeof( module_iterator));
	if (!iterator)
		return NULL;

	// ToDo: possibly, the prefix don't have to be copied, just referenced
	iterator->prefix = strdup(prefix);
	if (iterator->prefix == NULL) {
		free(iterator);
		return NULL;
	}

	iterator->base_path_id = -1;
	iterator->base_dir = iterator->cur_dir = NULL;
	iterator->err = B_NO_ERROR;
	iterator->module_pos = 0;

	return (void *)iterator;
}


/** Frees the cookie allocated by open_module_list()
 */

status_t
close_module_list(void *cookie)
{
	module_iterator *iterator = (module_iterator *)cookie;

	TRACE(("close_module_list()\n"));

	if (iterator == NULL)
		return B_BAD_VALUE;

	while (iterator->cur_dir)
		module_leave_dir(iterator);

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

	if (iterator == NULL || buffer == NULL || _bufferSize == NULL)
		return B_BAD_VALUE;

	TRACE(("read_next_module_name: looking for next module\n"));
	
	status = iterator->err;
	recursive_lock_lock(&gModulesLock);

	while (status == B_OK) {
		TRACE(("searching for module\n"));

		if (iterator->cur_dir != NULL) {
			if ((status = module_traverse_dir(iterator)) == B_NO_ERROR) {
				// By this point we should have a valid pointer to a 
				// module_info structure in iterator->current_header
				status = process_module_info(iterator, buffer, _bufferSize);
				if (status == B_OK)
					break;
			}
		} else
			status = module_enter_base_path(iterator);
	}

	iterator->err = status;
	recursive_lock_unlock(&gModulesLock);

	TRACE(("finished with status %s\n", strerror(status)));
	return status;
}


/** Iterates through all loaded modules, and stores its path in "buffer".
 *	ToDo: check if the function in BeOS really does that (could also mean:
 *		iterate through all modules that are currently loaded; have a valid
 *		loaded_module pointer, which would be hard to test for)
 */

status_t 
get_next_loaded_module_name(uint32 *cookie, char *buffer, size_t *_bufferSize)
{
	hash_iterator *iterator = (hash_iterator *)*cookie;
	loaded_module *loadedModule;
	status_t status;

	TRACE(("get_next_loaded_module_name()\n"));

	if (cookie == NULL || buffer == NULL || _bufferSize == NULL)
		return B_BAD_VALUE;

	if (iterator == NULL) {
		iterator = hash_open(gLoadedModulesHash, NULL);
		if (iterator == NULL)
			return B_NO_MEMORY;

		*(hash_iterator **)cookie = iterator;
	}

	recursive_lock_lock(&gModulesLock);

	loadedModule = hash_next(gLoadedModulesHash, iterator);
	if (loadedModule != NULL) {
		strlcpy(buffer, loadedModule->path, *_bufferSize);
		*_bufferSize = strlen(loadedModule->path);
		status = B_OK;
	} else {
		hash_close(gLoadedModulesHash, iterator, true);
		status = B_ENTRY_NOT_FOUND;
	}

	recursive_lock_unlock(&gModulesLock);

	return status;
}


status_t
get_module(const char *path, module_info **_info)
{
	loaded_module *loadedModule;
	module *module;
	status_t status;

	TRACE(("get_module(%s)\n", path));
	if (path == NULL)
		return B_BAD_VALUE;

	recursive_lock_lock(&gModulesLock);

	module = (struct module *)hash_lookup(gModulesHash, path);

	// if we don't have it cached yet, search for it
	if (module == NULL) {
		module = search_module(path);
		if (module == NULL) {
			FATAL(("module: Search for %s failed.\n", path));
			goto err;
		}
	}

	/* We now need to find the loaded_module for the module. This should
	 * be in memory if we have just run search_modules, but may not be
	 * if we are used cached information.
	 * We can't use the module->loaded_module pointer, because it is not
	 * reliable at this point (it won't be set to NULL when the loaded_module
	 * is unloaded).
	 */
	loadedModule = (loaded_module *)hash_lookup(gLoadedModulesHash, module->file);
	if (loadedModule == NULL) {
		if (load_module_file(module->file) == NULL)
			goto err;

		loadedModule = (loaded_module *)hash_lookup(gLoadedModulesHash, module->file);
		if (loadedModule == NULL)
			goto err;
	}

	// (re)set in-memory data for the loaded module
	module->info = loadedModule->info[module->offset];
	module->loaded_module = loadedModule;
	INC_MOD_REF_COUNT(module);

	// The state will be adjusted by the call to init_module
	// if we have just loaded the file
	if (module->ref_count == 1)
		status = init_module(module);
	else
		status = B_OK;

	recursive_lock_unlock(&gModulesLock);

	if (status == B_OK)
		*_info = module->info;

	return status;

err:
	recursive_lock_unlock(&gModulesLock);
	return B_ENTRY_NOT_FOUND;
}


status_t
put_module(const char *path)
{
	module *module;

	TRACE(("put_module(path = %s)\n", path));

	recursive_lock_lock(&gModulesLock);

	module = (struct module *)hash_lookup(gModulesHash, path);
	if (module == NULL) {
		FATAL(("module: We don't seem to have a reference to module %s\n", path));
		recursive_lock_unlock(&gModulesLock);
		return B_BAD_VALUE;
	}
	DEC_MOD_REF_COUNT(module);

	// If there are nomore references to this module - must we keep it loaded?
	if (module->ref_count == 0 && !module->keep_loaded) {
		// destruct the module, and check if we can also unload the
		// loaded module it refers to
		uninit_module(module);
		if (module->loaded_module->ref_count == 0)
			unload_module_file(module->file);
	}

	recursive_lock_unlock(&gModulesLock);
	return B_OK;
}
