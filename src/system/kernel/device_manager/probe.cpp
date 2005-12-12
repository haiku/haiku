/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager
	Probing for consumers.

	Here is all the core logic how consumers are found for one node.
*/


#include "device_manager_private.h"

#include <boot_device.h>
#include <elf.h>
#include <kmodule.h>
#include <fs/KPath.h>
#include <fs/devfs.h>
#include <util/Stack.h>
#include <util/kernel_cpp.h>

#include <image.h>
#include <KernelExport.h>

#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


//#define TRACE_PROBE
#ifdef TRACE_PROBE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct path_entry {
	struct list_link	link;
	char				*path;
	dev_t				device;
	ino_t				node;
	int32				busses;
};

struct module_entry {
	struct list_link	link;
	char				*name;
	driver_module_info	*driver;
	float				support;
	bool				no_connection;
};

// list of driver registration directories
const char *pnp_registration_dirs[2] = {
	COMMON_DRIVER_REGISTRATION,
	SYSTEM_DRIVER_REGISTRATION
};


// list of module directories
static const char *kModulePaths[] = {
	COMMON_MODULES_DIR,
	"/boot/beos/system/add-ons/kernel",//SYSTEM_MODULES_DIR,
	"/boot",	// ToDo: this is for the bootfs boot - to be removed
	NULL
};


class DirectoryIterator {
	public:
		DirectoryIterator(const char *path, const char *subPath = NULL, bool recursive = false);
		DirectoryIterator(const char **paths, const char *subPath = NULL, bool recursive = false);
		~DirectoryIterator();

		void SetTo(const char *path, const char *subPath = NULL, bool recursive = false);
		void SetTo(const char **paths, const char *subPath = NULL, bool recursive = false);

		status_t GetNext(KPath &path, struct stat &stat);
		const char *CurrentName() const { return fCurrentName; }

		void Unset();
		void AddPath(const char *path, const char *subPath = NULL);

	private:
		Stack<KPath *>	fPaths;
		bool			fRecursive;
		DIR				*fDirectory;
		KPath			*fBasePath;
		const char		*fCurrentName;
};


DirectoryIterator::DirectoryIterator(const char *path, const char *subPath, bool recursive)
	:
	fDirectory(NULL),
	fBasePath(NULL),
	fCurrentName(NULL)
{
	SetTo(path, subPath, recursive);
}


DirectoryIterator::DirectoryIterator(const char **paths, const char *subPath, bool recursive)
	:
	fDirectory(NULL),
	fBasePath(NULL)
{
	SetTo(paths, subPath, recursive);
}


DirectoryIterator::~DirectoryIterator()
{
	Unset();
}


void 
DirectoryIterator::SetTo(const char *path, const char *subPath, bool recursive)
{
	Unset();
	fRecursive = recursive;

	AddPath(path, subPath);
}


void 
DirectoryIterator::SetTo(const char **paths, const char *subPath, bool recursive)
{
	Unset();
	fRecursive = recursive;

	for (int32 i = 0; paths[i] != NULL; i++) {
		AddPath(paths[i], subPath);
	}
}


status_t 
DirectoryIterator::GetNext(KPath &path, struct stat &stat)
{
next_directory:
	while (fDirectory == NULL) {
		delete fBasePath;
		fBasePath = NULL;

		if (!fPaths.Pop(&fBasePath))
			return B_ENTRY_NOT_FOUND;

		fDirectory = opendir(fBasePath->Path());
	}

next_entry:
	struct dirent *dirent = readdir(fDirectory);
	if (dirent == NULL) {
		// get over to next directory on the stack
		closedir(fDirectory);
		fDirectory = NULL;

		goto next_directory;
	}

	if (!strcmp(dirent->d_name, "..") || !strcmp(dirent->d_name, "."))
		goto next_entry;

	fCurrentName = dirent->d_name;

	path.SetTo(fBasePath->Path());
	path.Append(fCurrentName);

	if (::stat(path.Path(), &stat) != 0)
		goto next_entry;

	if (S_ISDIR(stat.st_mode) && fRecursive) {
		KPath *nextPath = new KPath(path);
		if (fPaths.Push(nextPath) != B_OK)
			return B_NO_MEMORY;

		goto next_entry;
	}

	return B_OK;
}


void 
DirectoryIterator::Unset()
{
	if (fDirectory != NULL) {
		closedir(fDirectory);
		fDirectory = NULL;
	}

	delete fBasePath;
	fBasePath = NULL;

	KPath *path;
	while (fPaths.Pop(&path))
		delete path;
}


void 
DirectoryIterator::AddPath(const char *basePath, const char *subPath)
{
	KPath *path = new KPath(basePath);
	if (subPath != NULL)
		path->Append(subPath);

	fPaths.Push(path);
}


//	#pragma mark -


struct path_entry *
new_path_entry(const char *path, dev_t device, ino_t node)
{
	path_entry *entry = (path_entry *)malloc(sizeof(path_entry));
	if (entry == NULL)
		return NULL;

	entry->path = strdup(path);
	if (entry->path == NULL) {
		free(entry);
		return NULL;
	}

	entry->device = device;
	entry->node = node;
	entry->busses = 0;
	return entry;
}


struct path_entry *
copy_path_entry(path_entry *entry)
{
	path_entry *newEntry = (path_entry *)malloc(sizeof(struct path_entry));
	if (newEntry == NULL)
		return NULL;

	*newEntry = *entry;
	newEntry->path = strdup(entry->path);
	if (newEntry->path == NULL) {
		free(newEntry);
		return NULL;
	}

	return newEntry;
}


void
free_module_entry(module_entry *entry)
{
	if (entry->name != NULL && entry->driver != NULL)
		put_module(entry->name);

	free(entry->name);
	free(entry);
}


struct module_entry *
new_module_entry(const char *name, driver_module_info *driver)
{
	module_entry *entry = (module_entry *)malloc(sizeof(module_entry));
	if (entry == NULL)
		return NULL;

	entry->name = strdup(name);
	if (entry->name == NULL) {
		free(entry);
		return NULL;
	}

	entry->driver = driver;
	entry->support = -1.0f;
	entry->no_connection = false;
	return entry;
}


static module_entry *
find_module_entry(struct list *list, const char *name)
{
	module_entry *entry = NULL;
	
	while ((entry = (module_entry *)list_get_next_item(list, entry)) != NULL) {
		if (!strcmp(entry->name, name))
			return entry;
	}

	return NULL;
}


static status_t
add_device_node(struct list *list, device_node_info *node)
{
	node_entry *entry = (node_entry *)malloc(sizeof(node_entry));
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->node = node;

	uint8 exploreLast = false;
	pnp_get_attr_uint8(node, B_DRIVER_EXPLORE_LAST, &exploreLast, false);

	if (exploreLast)
		list_add_link_to_tail(list, &entry->link);
	else
		list_add_link_to_head(list, &entry->link);

	return B_OK;
}


static status_t
get_next_device_node(struct list *list, uint32 *_cookie, device_node_info **_node)
{
	node_entry *entry = (node_entry *)list_get_next_item(list, (void *)*_cookie);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	*_node = entry->node;
	*_cookie = (uint32)entry;

	return B_OK;
}


static void
remove_device_nodes(struct list *list)
{
	struct node_entry *entry;

	while ((entry = (node_entry *)list_remove_head_item(list)) != NULL) {
		free(entry);
	}
}


#if 0
/** notify a consumer that a device he might handle is added
 *	fileName - file name of consumer
 *	(moved to end of file to avoid inlining)
 */

static status_t
notify_probe_by_file(device_node_info *node, const char *fileName)
{
	char *type;
	char *resolved_path;
	char *module_name;
	int i;
	bool valid_module_name;
	status_t res;

	TRACE(("notify_probe_by_file(%s)\n", fileName));

	res = pnp_get_attr_string(node, PNP_DRIVER_TYPE, &type, false);
	if (res != B_OK)
		return res;

	// resolve link to actual driver file
	resolved_path = (char *)malloc(B_PATH_NAME_LENGTH + 1);
	if (resolved_path == NULL) {
		res = B_NO_MEMORY;
		goto err;
	}

	// ToDo: do something about this; realpath() doesn't exist in the kernel!
	//module_name = pnp_boot_safe_realpath(consumer_name, resolved_path);
	module_name = NULL;
	if (module_name == NULL) {
		// broken link or something
		dprintf("Cannot resolve driver file name: %s\n", fileName);
		res = errno;
		goto err2;
	}

	// make sure both consumer and module file are either in 
	// system or user modules directory
	valid_module_name = false;

	for (i = 0; i < (disable_useraddons ? 1 : 2); ++i) {
		int len = strlen(kModulePaths[i]);

		if (!strncmp(fileName, kModulePaths[i], len)
			&& !strncmp(module_name, kModulePaths[i], len)) {
			valid_module_name = true;
			module_name = module_name + len;
			break;
		}
	}

	if (!valid_module_name) {
		TRACE(("Module file %s of consumer %s is in wrong path\n",
			fileName, module_name));
		res = B_NAME_NOT_FOUND;
		goto err2;
	}

	// append driver type to get specific module name
	strlcat(module_name, "/", B_PATH_NAME_LENGTH);
	strlcat(module_name, type, B_PATH_NAME_LENGTH);	

	res = dm_register_child_device(node, module_name);

err2:
	free(resolved_path);
err:
	free(type);
	return res;
}


/**	compose all possible names of Specific drivers; as there are
 *	multiple names which only differ in length, the most specific
 *	driver name gets stored in <path>, whereas <term_array> is a
 *	list of lengths of individual driver names with index 0
 *	containing the length of the shortest and num_parts-1 the
 *	length of the longest name; <buffer> is a supplied buffer of
 *	MAX_PATH+1 size
 */

static status_t
compose_driver_names(device_node_info *node, const char *dir,
	const char *filename_pattern, int num_parts, 
	char *path, char *buffer, size_t **res_term_array)
{
	size_t *term_array;
	int id;
	status_t res;

	term_array = (size_t *)malloc(num_parts * sizeof(size_t));
	if (term_array == NULL)
		return B_NO_MEMORY;

	strlcpy(path, dir, B_PATH_NAME_LENGTH);
	strlcat(path, "/", B_PATH_NAME_LENGTH);

	TRACE(("compose_drive_names(%s)\n", path));

	benaphore_lock(&gNodeLock);

	res = pnp_expand_pattern(node, filename_pattern, path, buffer, 
		term_array, &id);

	benaphore_unlock(&gNodeLock);

	if (res != B_OK)
		goto err;

	if (id != num_parts) {
		panic("compose_driver_names: number of pattern parts in %s is inconsistent (%d!=%d)",
			filename_pattern, num_parts, id);
	}

	TRACE(("driver=%s, parts=%d\n", buffer, id));

	*res_term_array	= term_array;
	return B_OK;

err:
	free(term_array);
	return res;
}


/**	notify all drivers under <directory>. If <tell_all> is true, notify all,
 *	if false, stop once a drivers notification function returned B_OK 
 *	buffer - scratch buffer of size B_PATH_NAME_LENGTH + 1 ; destroyed on exit
 *	return: B_NAME_NOT_FOUND, if tell_all is false and no driver returned B_OK
 */

static status_t
try_drivers(device_node_info *node, char *directory, 
	bool tell_all, char *buffer)
{
	DIR *dir;
	size_t dir_len;
	struct dirent *entry;
	int i;

	TRACE(("try_drivers(dir: %s)\n", directory));

	// first try user drivers, then system drivers
	for (i = 0; i < (disable_useraddons ? 1 : 2); ++i) {
		strcpy(buffer, pnp_registration_dirs[i]);
		strlcat(buffer, directory, B_PATH_NAME_LENGTH);

		TRACE(("cur_dir: %s\n", buffer));

		dir_len = strlen(buffer);

		dir = opendir(buffer);
		if (dir == NULL) {
			//SHOW_ERROR(3, "Directory %s doesn't exists", buffer);
			// directory doesn't exist
			return tell_all ? B_OK : B_NAME_NOT_FOUND;
		}

		while ((entry = readdir(dir)) != NULL) {
			buffer[dir_len] = 0;
			strlcat(buffer, "/", B_PATH_NAME_LENGTH);
			strlcat(buffer, entry->d_name, B_PATH_NAME_LENGTH);

			// skip default directory entries
			if (!strcmp( entry->d_name, ".")
				|| !strcmp( entry->d_name, ".."))
				continue;

			if (notify_probe_by_file(node, buffer) == B_OK && !tell_all) {
				// tell_all is false, return on first hit
				closedir(dir);
				return B_OK;
			}
		}

		closedir(dir);
	}

	return tell_all ? B_OK : B_NAME_NOT_FOUND;
}


/**	find normal child of node that are stored under <dir>; first, we
 *	look for a specific driver; if none could be found, find a generic
 *	one path, buffer - used as scratch buffer (all of size B_PATH_NAME_LENGTH + 1)
 *	return: B_NAME_NOT_FOUND if no consumer could be found
 */

static status_t
find_normal_child(device_node_info *node, const char *dir,
	const char *filename_pattern, int num_parts,
	char *path, char *buffer, bool *found_normal_driver)
{
	status_t res;
	size_t *term_array;
	int i;

	// don't search for specific consumers if there is only a directory given 
	if (*filename_pattern) {
		res = compose_driver_names(node, dir, filename_pattern, num_parts, 
			path, buffer, &term_array);
		if (res != B_OK)
			return res;

		// try to find specific driver, starting with most specific, i.e.
		// the one with the longest name
		for (i = num_parts - 1; i >= 0; --i) {
			int j;

			TRACE(("%d: %lu\n", i, term_array[i]));
			path[term_array[i]] = 0;

			// first, check for user driver, then system driver
			for (j = 0; j < (disable_useraddons ? 1 : 2); ++j) {
				struct stat dummy;

				strcpy(buffer, pnp_registration_dirs[j]);
				strlcat(buffer, path, B_PATH_NAME_LENGTH);

				// do a stat to avoid error message if Specific Driver
				// isn't provided
				if (lstat(buffer, &dummy) == 0) {
					res = notify_probe_by_file(node, buffer);
					if (res == B_OK)
						// got him!
						break;
				} else {
					/*SHOW_ERROR( 4, "Specific driver %s doesn't exists",
						buffer );*/
				}
			}
		}

		free(term_array);

		if (i >= 0) {
			// found specific consumer
			*found_normal_driver = true;
			return B_OK;
		}
	}

	// no specific consumer - go through generic driver		
	strlcpy(path, dir, B_PATH_NAME_LENGTH);
	strlcat(path, GENERIC_SUBDIR, B_PATH_NAME_LENGTH);

	if (try_drivers(node, path, false, buffer) != B_OK) {
		*found_normal_driver = false;
		return B_NAME_NOT_FOUND;
	}
	
	*found_normal_driver = true;
	return B_OK;
}


/**	pre-process dynamic child name pattern.
 *	split into directory and pattern and count split positions;
 *	further, remove quotes from directory
 *	pattern		- pattern of consumer name
 *	buffer		- buffer to store results in 
 *				  (returned strings all point to <buffer>!)
 *	filename_pattern - pattern of file name
 *	*num_parts	- number of split positions
 */

static status_t
preprocess_child_names(const char *pattern, char *buffer,
	char **filename_pattern, int *const num_parts)
{
	char *str, *dest;
	bool parts_began;

	// make a copy of pattern as we will strip escapes from directory part
	strlcpy(buffer, pattern, B_PATH_NAME_LENGTH);

	// find directory part and count splitpoints
	parts_began = false;
	*num_parts = 1;

	for (str = buffer; *str; ++str) {
		switch (*str) {
			case '\\':
				// honour escaped characters, taking care of trailing escape
				if (str[1])
					++str;
				break;

			case '|':
				++*num_parts;
				parts_began = true;
				break;

			case '%':
				++str;

				// find end of attribute name, taking care of escape sequences
				for (; *str != 0; ++str) {
					if (*str == '\\') {
						if (str[1] != 0)
							++str;
					} else if (*str == '%')
						break;
				}

				if (*str == 0) {
					dprintf("missing matching '%%' in consumer pattern %s\n", pattern);
					return B_BAD_VALUE;
				}
				break;
		}
	}

	*filename_pattern = buffer;

	// remove escape sequences from directory
	for (str = buffer, dest = buffer; *str; ++str) {
		if (str[0] == '\\' && str[1] != 0)
			++str;

		*dest++ = *str;
	}

	*dest = '\0';

	TRACE(("filename_pattern = %s, num_parts = %d\n",
		*filename_pattern, *num_parts));

	return B_OK;
}


/**	find consumers for one given pattern
 *	has_normal_drivers - in: true - don't search for specific driver
 *		out: true - specific driver was found
 */

static status_t
register_dynamic_child_device(device_node_info *node, const char *bus,
	const char *pattern, bool *has_normal_driver)
{
	status_t status;
	char *buffers;
	char *filename_pattern;
	int num_parts;

	TRACE(("register_dynamic_child_device(bus = %s, pattern = %s, has_normal_driver = %d)\n",
		bus, pattern, *has_normal_driver));

	if (pattern == NULL)
		return B_OK;

	// we need three buffers - allocate them at once for simplicity
	buffers = (char *)malloc(3 * (B_PATH_NAME_LENGTH + 1));
	if (buffers == NULL)
		return B_NO_MEMORY;

	status = preprocess_child_names(pattern, buffers + 2 * (B_PATH_NAME_LENGTH + 1), 
			&filename_pattern, &num_parts);
	if (status < B_OK)
		goto err;

	if (!*has_normal_driver) {
		// find specific/generic consumer		
		status = find_normal_child(node, bus, filename_pattern, num_parts, 
			buffers, buffers + B_PATH_NAME_LENGTH + 1, has_normal_driver);
		if (status != B_OK && status != B_NAME_NOT_FOUND)
			// only abort if there was a "real" problem; 
			// if there is no specific/generic consumer, we don't bother
			// (having no driver is not funny but happens)
			goto err;
	}

	// tell universal drivers
	strlcpy(buffers, bus, B_PATH_NAME_LENGTH);
	strlcat(buffers, UNIVERSAL_SUBDIR, B_PATH_NAME_LENGTH);

	status = try_drivers(node, buffers, true, buffers + B_PATH_NAME_LENGTH + 1);
	if (status != B_OK && status != B_NAME_NOT_FOUND)
		// again, only abort on real problems
		goto err;

	free(buffers);

	TRACE(("done\n"));
	return B_OK;

err:
	free(buffers);
	return status;
}
#endif


static path_entry *
find_node_ref_in_list(struct list *list, dev_t device, ino_t node)
{
	path_entry *entry = NULL;
	
	while ((entry = (path_entry *)list_get_next_item(list, entry)) != NULL) {
		if (entry->device == device
			&& entry->node == node)
			return entry;
	}

	return NULL;
}


/**	Iterates over the given list and tries to load all drivers and modules
 *	in that list.
 *	The list is emptied and freed during the traversal.
 *
 *	ToDo: Old style drivers will be initialized as well, new style driver
 *	handling is not yet done.
 */

static status_t
try_drivers(struct list &list, bool tryBusDrivers)
{
	path_entry *entry;
	while ((entry = (path_entry *)list_remove_head_item(&list)) != NULL) {
		if (!tryBusDrivers && entry->busses) {
			free(entry->path);
			free(entry);
			continue;
		}

		image_id image = load_kernel_add_on(entry->path);
		if (image >= 0) {
			// check if it's a driver module
			module_info **modules;
			if (load_module(entry->path, &modules) == B_OK) {
				// we have a module
				dprintf("loaded module %s\n", entry->path);
			} else {
				// it can still be a standard old-style driver
				if (devfs_add_driver(entry->path) == B_OK) {
					// we have a driver
					dprintf("loaded driver %s\n", entry->path);
				}
			}

			unload_kernel_add_on(image);
		}

		free(entry->path);
		free(entry);
	}

	return B_OK;
}


static module_entry *
remove_best_entry(struct list *list)
{
	module_entry *bestEntry = NULL;	
	module_entry *entry = NULL;

	while ((entry = (module_entry *)list_get_next_item(list, entry)) != NULL) {
		if (entry->no_connection || entry->driver == NULL
			|| entry->driver->register_device == NULL
			|| entry->support <= 0.0)
			continue;

		if (bestEntry == NULL || bestEntry->support < entry->support)
			bestEntry = entry;
	}

	if (bestEntry) {
		list_remove_item(list, bestEntry);
		return bestEntry;
	}

	return NULL;
}


static void
register_supporting_child_devices(device_node_info *node, struct list *list)
{
	// See if this node has a connection to offer
	char *hasConnection = NULL;
		// this is a bit ugly as we don't access the string
		// but just test it against NULL
	if (pnp_get_attr_string(node, PNP_DRIVER_CONNECTION, &hasConnection, false) == B_OK)
		free(hasConnection);
	dprintf("has connection? %s\n", hasConnection ? "yes" : "no");

	// first ask each module for level of support

	module_entry *bestEntry = NULL;	
	module_entry *entry = NULL;
	while ((entry = (module_entry *)list_get_next_item(list, entry)) != NULL) {
		if (entry->driver == NULL) {
			if (get_module(entry->name, (module_info **)&entry->driver) != B_OK)
				continue;
		}

		entry->support = entry->driver->supports_device(node, &entry->no_connection);
		dprintf("module: %s, support: %d\n", entry->name, (int)(entry->support * 1000));

		if (!hasConnection)
			entry->no_connection = true;
	}

	// register best module with a connection

	while ((entry = remove_best_entry(list)) != NULL) {
		status_t status = entry->driver->register_device(node);
		dprintf("tried best module: %s: %s\n", entry->name, strerror(status));
		free_module_entry(entry);

		if (status == B_OK)
			break;
	}

	// register all modules that don't need a connection
	// and empty the list

	while ((entry = (module_entry *)list_remove_head_item(list)) != NULL) {
		if (entry->no_connection && entry->driver != NULL 
			&& entry->driver->register_device != NULL 
			&& entry->support > 0.0) {
			dprintf("register rest: %s\n", entry->name);
			entry->driver->register_device(node);
		}

		dprintf("free %s\n", entry->name);
		free_module_entry(entry);
	}
}


static bool
is_driver_module(const char *name, int32 length)
{
	if (length == -1)
		length = strlen(name);

	return !strcmp(name + length - 9, "device_v1");	
}


static status_t
get_loaded_modules(struct list *modules, const char *bus, const char *device)
{
	TRACE(("get_loaded_modules(bus = %s, device = %s)\n", bus, device));

	bool listWasEmpty = list_is_empty(modules);

	char name[B_FILE_NAME_LENGTH];
	size_t size = sizeof(name);
	uint32 cookie = 0;
	while (get_next_loaded_module_name(&cookie, name, &size) == B_OK) {
		int32 length = size;
		size = sizeof(name);

		if (!is_driver_module(name, length))
			continue;

		TRACE(("  \"%s\" is driver module!\n", name));

		driver_module_info *info;
		if (get_module(name, (module_info **)&info) == B_OK) {
			if (info->get_supported_paths == NULL)
				continue;

			const char **busses, **devices;
			info->get_supported_paths(&busses, &devices);

			bool support = bus == NULL;

			// search for the bus

			if (bus != NULL && busses != NULL) {
				for (int32 i = 0; busses[i]; i++) {
					if (!strcmp(bus, busses[i])) {
						dprintf("\tfound %s!!\n", busses[i]);
						support = true;
						break;
					}
				}
			}

			if (support) {
				// search for the device

				support = device == NULL;

				if (device != NULL && devices != NULL) {
					for (int32 i = 0; devices[i]; i++) {
						if (!strncmp(device, devices[i], strlen(device))) {
							dprintf("\tfound %s!!\n", devices[i]);
							support = true;
							break;
						}
					}
				}
			}

			if (support && !listWasEmpty) {
				// make sure we don't add the same module twice
				if (find_module_entry(modules, name) != NULL)
					support = false;
			}

			if (support) {
				module_entry *entry = new_module_entry(name, info);
				if (entry == NULL) {
					put_module(name);
					return B_NO_MEMORY;
				}

				dprintf("\tadd module %s to list\n", name);
				list_add_item(modules, entry);
				continue;
			}

			put_module(name);
		}
	}

	return B_OK;
}


static status_t
get_nodes_for_device_type(device_node_info *node, struct list *list, const char *type)
{
	bool matches = false;

	// see if there is a B_DRIVER_DEVICE_TYPE that matches the query

	char *deviceType = NULL;
	if (pnp_get_attr_string(node, B_DRIVER_DEVICE_TYPE, &deviceType, false) == B_OK) {
		if (!strncmp(deviceType, type, strlen(type)))
			matches = true;

		free(deviceType);
	}

	if (!matches) {
		// we also accept any dump busses
		uint8 onDemand = false;
		pnp_get_attr_uint8(node, B_DRIVER_FIND_DEVICES_ON_DEMAND, &onDemand, false);
		
		if (onDemand)
			matches = true;
	}

	if (matches) {
#ifdef TRACE_PROBE
		dprintf("** NODE MATCHES TYPE %s\n", type);
		dm_dump_node(node, 0);
#endif
		return add_device_node(list, node);
	}

	// search child nodes, then
	device_node_info *child = NULL;
	while (dm_get_next_child_node(node, &child, NULL) == B_OK) {
		status_t status = get_nodes_for_device_type(child, list, type);
		if (status != B_OK) {
			dm_put_node(child);
			return status;
		}
	}

	return B_OK;
}


//	#pragma mark -
//	device manager private API


/** Register the device of a child for the \a node that might accept it.
 *	childName - module name of child/consumer
 */

status_t
dm_register_child_device(device_node_info *node, const char *childName)
{
	driver_module_info *child;
	status_t status;

	TRACE(("dm_register_child_device(node = %p, child = %s)\n", node, childName));

	status = get_module(childName, (module_info **)&child);
	if (status < B_OK) {
		dprintf("Cannot load driver module %s (%s)\n", childName, strerror(status));
		return status;
	}

	if (child->register_device != NULL) {
		status = child->register_device(node);
		if (status != B_OK) {
			TRACE(("dm_register_child_device(): Driver %s returned: %s\n", 
				childName, strerror(status)));
		}
	} else {
		dprintf("Driver %s has no register_device() hook\n", childName);
		status = B_ERROR;
	}

	put_module(childName);
	return status;
}


/** find and notify dynamic consumers that device was added
 *	errors returned by consumers aren't reported, only problems
 *	like malformed consumer patterns
 */

status_t
dm_register_dynamic_child_devices(device_node_info *node)
{
	char *buffer;
	char *bus;
	status_t status = B_OK;
	int32 i, found = 0;

	TRACE(("dm_register_dynamic_child_devices(node = %p)\n", node));

	if (pnp_get_attr_string(node, B_DRIVER_BUS, &bus, false) != B_OK)
		return B_OK;

	TRACE(("  Search bus: \"%s\"\n", bus));

	if (gBootDevice < 0) {
		// there is no boot device yet, we have to scan the already
		// loaded and built-in modules
		struct list modules;
		list_init(&modules);

		status = get_loaded_modules(&modules, bus, NULL);
		if (status != B_OK)
			return status;

		register_supporting_child_devices(node, &modules);
		return B_OK;
	} else {
		// ToDo: this is completely outdated and must be redone!
		status = B_ERROR;
#if 0
		buffer = (char *)malloc(B_PATH_NAME_LENGTH + 1);
		if (buffer == NULL) {
			status = B_NO_MEMORY;
			goto err;
		}

		// first, append nothing, then "/0", "/1" etc.
		for (i = -1; ; ++i) {
			bool noSpecificDriver = false;
			char *consumer;

			strcpy(buffer, B_DRIVER_MAPPING);
			if (i >= 0)
				sprintf(buffer + strlen( buffer ), "/%ld", i);
	
			// if no more dynamic consumers, cancel loop silently
			if (pnp_get_attr_string(node, buffer, &consumer, false) != B_OK) {
				// starting with .../0 is OK, so ignore error if i = -1
				if (i == -1)
					continue;
				else
					break;
			}
	
			TRACE(("  Consumer pattern %ld: %s\n", i, consumer));
			status = register_dynamic_child_device(node, bus, consumer, &noSpecificDriver);
	
			free(consumer);
			found++;
	
			if (status != B_OK) {
				// this is only reached if a serious error occured,
				// see register_dynamic_child_device()
				break;
			}
		}
#endif
	}
#if 0
	if (found == 0) {
		// no requirement for a special mapping, so we're just scanning the bus directory
		bool noSpecificDriver = false;
		status = register_dynamic_child_device(node, bus, NULL, &noSpecificDriver);
	}

	// supposed to go through
	free(buffer);
#endif
err:
	free(bus);
	return status;
}


/**	Register all fixed child devices of of the given \a node; in contrast
 *	to dynamic child devices, errors reported by fixed child devices are
 *	not ignored but returned, and regarded critical.
 */

status_t
dm_register_fixed_child_devices(device_node_info *node)
{
	TRACE(("dm_register_fixed_child_devices(node = %p)\n", node));

	char *buffer = (char *)malloc(B_PATH_NAME_LENGTH + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	// first, append nothing, then "/0", "/1" etc.
	for (int32 i = -1; ; ++i) {
		char *childName;

		strcpy(buffer, B_DRIVER_FIXED_CHILD);
		if (i >= 0)
			sprintf(buffer + strlen(buffer), "/%ld", i);

		// if no more fixed consumers, cancel loop silently
		if (pnp_get_attr_string(node, buffer, &childName, false) != B_OK)
			break;

		TRACE(("Consumer %ld: %s\n", i, childName));

		if (dm_register_child_device(node, childName) != B_OK) {
			dprintf("Cannot register fixed child device %s\n", childName);

			// report error if fixed consumers couldn't be loaded
			// as they are obviously crucial (else they wouldn't be fixed)

			// ToDo: what about the devices we already registered up to this point?
			free(childName);
			free(buffer);
			return B_NAME_NOT_FOUND;
		}

		free(childName);
	}

	free(buffer);
	return B_OK;
}


static status_t
probe_for_driver_modules(const char *type)
{
	TRACE(("probe_for_driver_modules(type = %s)\n", type));

	// search a node with an open connection of the specified type
	// or notify bus managers to get one

	status_t status = B_OK;
	struct list drivers;
	list_init(&drivers);

	if (gBootDevice < 0) {
		struct list nodes;
		list_init(&nodes);

		get_nodes_for_device_type(gRootNode, &nodes, type);

		device_node_info *node;
		uint32 cookie = 0;
		while (get_next_device_node(&nodes, &cookie, &node) == B_OK) {
			char *bus;
			if (pnp_get_attr_string(node, B_DRIVER_BUS, &bus, false) == B_OK) {
				struct list drivers;
				list_init(&drivers);
				get_loaded_modules(&drivers, bus, type);

				register_supporting_child_devices(node, &drivers);

				free(bus);
			}
		}

		remove_device_nodes(&nodes);
		return B_OK;
	}

	// build list of potential drivers for that type

	DirectoryIterator iterator(kModulePaths, type, false);
	struct stat stat;
	KPath path;

	while (iterator.GetNext(path, stat) == B_OK) {
		if (S_ISDIR(stat.st_mode)) {
			// We need to make sure that drivers in ie. "audio/raw/" can
			// be found as well - therefore, we must make sure that "audio"
			// exists on /dev.
			size_t length = strlen("drivers/dev");
			if (strncmp(type, "drivers/dev", length))
				continue;

			path.SetTo(type);
			path.Append(iterator.CurrentName());
			devfs_publish_directory(path.Path() + length + 1);
			continue;
		}

		path_entry *entry = new_path_entry(path.Path(), stat.st_dev, stat.st_ino);
		if (entry == NULL)
			return B_NO_MEMORY;

		TRACE(("found potential driver: %s\n", path.Path()));
		list_add_item(&drivers, entry);
	}

	if (list_is_empty(&drivers))
		return B_OK;

	// Iterate through bus managers to shrink the list (let them publish
	// their own devices)

	iterator.SetTo(kModulePaths, "drivers/bus", false);

	while (iterator.GetNext(path, stat) == B_OK) {
		DirectoryIterator busIterator(path.Path(), NULL, true);

		struct list driversForBus;
		list_init(&driversForBus);

		dprintf("bus: %s\n", path.Leaf());

		while (busIterator.GetNext(path, stat) == B_OK) {
			path_entry *entry = find_node_ref_in_list(&drivers, stat.st_dev, stat.st_ino);
			if (entry == NULL)
				continue;

			entry->busses++;
				// mark this entry as being used by a bus

			// ToDo: we don't need to remember drivers when there is no bus for them
			// (ie. no need to store ISA drivers when there is no ISA bus in the system)

			entry = copy_path_entry(entry);
			if (entry == NULL)
				continue;

			list_add_item(&driversForBus, entry);
			dprintf("found driver for bus \"%s\": \"%s\"\n", path.Path(), entry->path);
		}

		// ToDo: do something with the bus drivers... :)
		// ToDo: ask bus manager for driver (via mapping)
		// ToDo: find all nodes where this driver could be attached to
		try_drivers(driversForBus, true);
	}

	// ToDo: do something with the remaining drivers... :)
	try_drivers(drivers, false);
	return B_OK;
}


status_t
probe_for_device_type(const char *type)
{
	TRACE(("probe_for_device_type(type = %s)\n", type));

	char deviceType[64];
	snprintf(deviceType, sizeof(deviceType), "drivers/dev%s%s", type[0] ? "/" : "", type);

	return probe_for_driver_modules(deviceType);
}
