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

#include <KernelExport.h>
#include <image.h>
#include <elf.h>
#include <kmodule.h>
#include <fs/KPath.h>
#include <util/Stack.h>
#include <util/kernel_cpp.h>
#include <devfs.h>

#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


#define TRACE_PROBE
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

		void Unset();
		void AddPath(const char *path, const char *subPath = NULL);

	private:
		Stack<KPath *>	fPaths;
		bool			fRecursive;
		DIR				*fDirectory;
		KPath			*fBasePath;
};


DirectoryIterator::DirectoryIterator(const char *path, const char *subPath, bool recursive)
	:
	fDirectory(NULL),
	fBasePath(NULL)
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

	path.SetTo(fBasePath->Path());
	path.Append(dirent->d_name);

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

	res = pnp_notify_probe_by_module(node, module_name);

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


/**	find normal consumer of node that are stored under <dir>; first, we
 *	look for a specific driver; if none could be found, find a generic
 *	one path, buffer - used as scratch buffer (all of size B_PATH_NAME_LENGTH + 1)
 *	return: B_NAME_NOT_FOUND if no consumer could be found
 */

static status_t
find_normal_consumer(device_node_info *node, const char *dir,
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


/**	pre-process dynamic consumer name pattern.
 *	split into directory and pattern and count split positions;
 *	further, remove quotes from directory
 *	pattern		- pattern of consumer name
 *	buffer		- buffer to store results in 
 *				  (returned strings all point to <buffer>!)
 *	filename_pattern - pattern of file name
 *	*num_parts	- number of split positions
 */

static status_t
preprocess_consumer_names(const char *pattern, char *buffer,
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
notify_dynamic_consumer(device_node_info *node, const char *bus,
	const char *pattern, bool *has_normal_driver)
{
	status_t status;
	char *buffers;
	char *filename_pattern;
	int num_parts;

	TRACE(("notify_dynamic_consumer(bus = %s, pattern = %s, has_normal_driver = %d)\n",
		bus, pattern, *has_normal_driver));

	if (pattern == NULL)
		return B_OK;

	// we need three buffers - allocate them at once for simplicity
	buffers = (char *)malloc(3 * (B_PATH_NAME_LENGTH + 1));
	if (buffers == NULL)
		return B_NO_MEMORY;

	status = preprocess_consumer_names(pattern, buffers + 2 * (B_PATH_NAME_LENGTH + 1), 
			&filename_pattern, &num_parts);
	if (status < B_OK)
		goto err;

	if (!*has_normal_driver) {
		// find specific/generic consumer		
		status = find_normal_consumer(node, bus, filename_pattern, num_parts, 
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


static image_id
load_driver(const char *path)
{
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	const char **devicePaths;
	int32 exported = 0;
	status_t status;

	// load the module
	image_id image = load_kernel_add_on(path);
	if (image < 0)
		return image;

	// for prettier debug output
	const char *name = strrchr(path, '/');
	if (name == NULL)
		name = path;
	else
		name++;

	// For a valid device driver the following exports are required
	
	device_hooks *(*find_device)(const char *);
	const char **(*publish_devices)(void);
	uint32 *api_version;
	if (get_image_symbol(image, "publish_devices", B_SYMBOL_TYPE_TEXT, (void **)&publish_devices) != B_OK
		|| get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA, (void **)&api_version) != B_OK
		|| get_image_symbol(image, "find_device", B_SYMBOL_TYPE_TEXT, (void **)&find_device) != B_OK) {
		dprintf("%s: mandatory driver symbol(s) missing!\n", name);
		status = B_BAD_VALUE;
		goto error1;
	}

	// test for init_hardware() and call it
	if (get_image_symbol(image, "init_hardware", B_SYMBOL_TYPE_TEXT,
			(void **)&init_hardware) == B_OK
		&& (status = init_hardware()) != B_OK) {
		dprintf("%s: init_hardware() failed: %s\n", name, strerror(status));
		status = ENXIO;
		goto error1;
	}

	/* OK, so we now have what appears to be a valid module that has
	 * completed init_hardware and thus thinks it should be used.
	 * ToDo:
	 *  - this is bogus!
	 *  - the driver init routines should be called by devfs and 
	 *       only when the driver is first needed. However, that level
	 *       level of support is not yet in devfs, so we have a hack
	 *       here that calls the init_driver function at this point.
	 *       As a result we will check to see if we actually manage to
	 *       publish the device, and if we do we will keep the module
	 *       loaded.
	 *  - remove this when devfs is fixed!
	 */
	if (get_image_symbol(image, "init_driver", B_SYMBOL_TYPE_TEXT,
			(void **)&init_driver) == B_OK
		&& (status = init_driver()) != B_OK) {
		dprintf("%s: init_driver() failed: %s\n", name, strerror(status));
		status = ENXIO;
		goto error2;
	}

	// we keep the driver loaded if it exports at least a single interface
	devicePaths = publish_devices();
	if (devicePaths == NULL) {
		dprintf("%s: publish_devices() returned NULL.\n", name);
		status = ENXIO;
		goto error3;
	}

	for (; devicePaths[0]; devicePaths++) {
		device_hooks *hooks = find_device(devicePaths[0]);

		if (hooks && devfs_publish_device(devicePaths[0], NULL, hooks) == 0)
			exported++;
	}

	// we're all done, driver will be kept loaded (for now, see above comment)
	if (exported > 0)
		return image;

	status = B_ERROR;	// whatever...


error3:
	{
		status_t (*uninit_driver)(void);
		if (get_image_symbol(image, "uninit_driver", B_SYMBOL_TYPE_TEXT,
				(void **)&uninit_driver) == B_OK)
			uninit_driver();
	}

error2:
{
	status_t (*uninit_hardware)(void);
	if (get_image_symbol(image, "uninit_hardware", B_SYMBOL_TYPE_TEXT,
			(void **)&uninit_hardware) == B_OK)
		uninit_hardware();
}
error1:
	/* If we've gotten here then the driver will be unloaded and an
	 * error code returned.
	 */
	unload_kernel_add_on(image);
	return status;
}


/** This is no longer part of the public kernel API, so we just export the symbol */

status_t load_driver_symbols(const char *driverName);
status_t
load_driver_symbols(const char *driverName)
{
	// This will be globally done for the whole kernel via the settings file.
	// We don't have to do anything here.

	return B_OK;
}


static status_t
try_drivers(struct list &list)
{
	path_entry *entry;
	while ((entry = (path_entry *)list_remove_head_item(&list)) != NULL) {
		image_id image = load_kernel_add_on(entry->path);
		if (image >= 0) {
			// check if it's a driver module
			module_info **modules;
			if (load_module(entry->path, &modules) == B_OK) {
				// we have a module
				dprintf("loaded module %s\n", entry->path);
			} else {
				// it can still be a standard old-style driver
				if (load_driver(entry->path) == B_OK) {
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


//	#pragma mark -
//	device manager private API


/** find and notify dynamic consumers that device was added
 *	errors returned by consumers aren't reported, only problems
 *	like malformed consumer patterns
 */

status_t
pnp_notify_dynamic_consumers(device_node_info *node)
{
#if 0
	char *buffer;
	char *bus;
	status_t status = B_OK;
	int32 i, found = 0;

	TRACE(("pnp_notify_dynamic_consumers(node = %p)\n", node));

	if (pnp_get_attr_string(node, PNP_DRIVER_CONSUMER_BUS, &bus, false) != B_OK)
		return B_OK;

	TRACE(("  Search bus: \"%s\"\n", bus));

	buffer = (char *)malloc(B_PATH_NAME_LENGTH + 1);
	if (buffer == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	// first, append nothing, then "/0", "/1" etc.
	for (i = -1; ; ++i) {
		bool noSpecificDriver = false;
		char *consumer;

		strcpy(buffer, PNP_DRIVER_CONSUMER_MAPPING);
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
		status = notify_dynamic_consumer(node, bus, consumer, &noSpecificDriver);

		free(consumer);
		found++;

		if (status != B_OK) {
			// this is only reached if a serious error occured,
			// see notify_dynamic_consumer()
			break;
		}
	}

	if (found == 0) {
		// no requirement for a special mapping, so we're just scanning the bus directory
		bool noSpecificDriver = false;
		status = notify_dynamic_consumer(node, bus, NULL, &noSpecificDriver);
	}

	// supposed to go through
	free(buffer);
err:
	free(bus);
	return status;
#else
	return B_OK;
#endif
}


/**	Notify fixed consumers that device was added; in contrast to dynamic
 *	consumers, errors reported by fixed consumers are not ignored but
 *	returned.
 */

status_t
pnp_notify_fixed_consumers(device_node_info *node)
{
	int i;
	char *buffer;

	TRACE(("pnp_notify_fixed_consumers(node = %p)\n", node));

	buffer = (char *)malloc(B_PATH_NAME_LENGTH + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	// first, append nothing, then "/0", "/1" etc.
	for (i = -1; ; ++i) {
		char *consumer;

		strcpy(buffer, PNP_DRIVER_FIXED_CONSUMER);
		if (i >= 0)
			sprintf(buffer + strlen(buffer), "/%d", i);

		// if no more fixed consumers, cancel loop silently
		if (pnp_get_attr_string(node, buffer, &consumer, false) != B_OK)
			break;

		TRACE(("Consumer %d: %s\n", i, consumer));

		if (pnp_notify_probe_by_module(node, consumer) != B_OK) {
			dprintf("Cannot notify fixed consumer %s\n", consumer);

			// report error if fixed consumers couldn't be loaded
			// as they are obviously crucial (else they wouldn't be fixed)
			free(consumer);
			free(buffer);
			return B_NAME_NOT_FOUND;
		}

		free(consumer);
	}

	free(buffer);
	return B_OK;
}


status_t
probe_for_device_type(const char *type)
{
	// search a node with an open connection of the specified type
	// or notify bus managers to get one

	status_t status = B_OK;

	// build list of potential drivers for that type

	struct list drivers;
	list_init(&drivers);

	char devType[64];
	snprintf(devType, sizeof(devType), "drivers/dev%s%s", type[0] ? "/" : "", type);

	DirectoryIterator iterator(kModulePaths, devType, false);
	struct stat stat;
	KPath path;

	while (iterator.GetNext(path, stat) == B_OK) {
		path_entry *entry = (path_entry *)malloc(sizeof(path_entry));
		if (entry == NULL)
			return B_NO_MEMORY;

		entry->path = strdup(path.Path());
		if (entry->path == NULL) {
			free(entry);
			return B_NO_MEMORY;
		}

		entry->device = stat.st_dev;
		entry->node = stat.st_ino;

		dprintf("found potential driver: %s\n", path.Path());
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

		while (busIterator.GetNext(path, stat) == B_OK) {
			path_entry *entry = find_node_ref_in_list(&drivers, stat.st_dev, stat.st_ino);
			if (entry == NULL)
				continue;

			// we found the driver here, so we should check it
			list_remove_link(&entry->link);
			list_add_item(&driversForBus, entry);
			dprintf("found driver for bus \"%s\": \"%s\"\n", path.Path(), entry->path);
		}

		// ToDo: do something with the bus drivers... :)
		// ToDo: ask bus manager for driver (via mapping)
		// ToDo: find all nodes where this driver could be attached to
		try_drivers(driversForBus);
	}

	// ToDo: do something with the remaining drivers... :)
	try_drivers(drivers);
	return B_OK;
}

