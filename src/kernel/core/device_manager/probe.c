/* 
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP Manager
	Probing for consumers.

	Here is all the core logic how consumers are found for one node.
*/


#include "device_manager_private.h"

#include <KernelExport.h>

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

// ToDo: for now - it won't be necessary to do this later anymore
#define pnp_boot_safe_realpath realpath
#define pnp_boot_safe_lstat lstat
#define pnp_boot_safe_opendir opendir
#define pnp_boot_safe_readdir readdir
#define pnp_boot_safe_closedir closedir

// list of driver registration directories
const char *pnp_registration_dirs[2] = {
	COMMON_DRIVER_REGISTRATION,
	SYSTEM_DRIVER_REGISTRATION
};


// list of module directories
static const char *modules_dirs[2] = {
	COMMON_MODULES_DIR,
	SYSTEM_MODULES_DIR
};


/** notify a consumer that a device he might handle is added
 *	consumer_name - file name (!) of consumer
 *	(moved to end of file to avoid inlining)
 */

static status_t
notify_probe_by_file(pnp_node_info *node, const char *consumer_name)
{
	char *type;
	char *resolved_path;
	char *module_name;
	int i;
	bool valid_module_name;
	status_t res;

	TRACE(("notify_probe_by_file(%s)\n", consumer_name));

	res = pnp_get_attr_string(node, PNP_DRIVER_TYPE, &type, false);
	if (res != B_OK)
		return res;

	// resolve link to actual driver file
	resolved_path = malloc(PATH_MAX + 1);
	if (resolved_path == NULL) {
		res = B_NO_MEMORY;
		goto err;
	}

	// ToDo: do something about this; realpath() doesn't exist in the kernel!
	//module_name = pnp_boot_safe_realpath(consumer_name, resolved_path);
	module_name = NULL;
	if (module_name == NULL) {
		// broken link or something
		dprintf("Cannot resolve driver file name: %s\n", consumer_name);
		res = errno;
		goto err2;
	}

	// make sure both consumer and module file are either in 
	// system or user modules directory
	valid_module_name = false;

	for (i = 0; i < (disable_useraddons ? 1 : 2); ++i) {
		int len = strlen(modules_dirs[i]);

		if (!strncmp(consumer_name, modules_dirs[i], len)
			&& !strncmp(module_name, modules_dirs[i], len)) {
			valid_module_name = true;
			module_name = module_name + len;
			break;
		}
	}

	if (!valid_module_name) {
		TRACE(("Module file %s of consumer %s is in wrong path\n",
			consumer_name, module_name));
		res = B_NAME_NOT_FOUND;
		goto err2;
	}

	// append driver type to get specific module name
	strlcat(module_name, "/", PATH_MAX);
	strlcat(module_name, type, PATH_MAX);	

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
compose_driver_names(pnp_node_info *node, const char *dir,
	const char *filename_pattern, int num_parts, 
	char *path, char *buffer, size_t **res_term_array)
{
	size_t *term_array;
	int id;
	status_t res;

	term_array = malloc(num_parts * sizeof(size_t));
	if (term_array == NULL)
		return B_NO_MEMORY;

	strlcpy(path, dir, PATH_MAX);
	strlcat(path, "/", PATH_MAX);

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
 *	buffer - scratch buffer of size PATH_MAX + 1 ; destroyed on exit
 *	return: B_NAME_NOT_FOUND, if tell_all is false and no driver returned B_OK
 */

static status_t
try_drivers(pnp_node_info *node, char *directory, 
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
		strlcat(buffer, directory, PATH_MAX);

		TRACE(("cur_dir: %s\n", buffer));

		dir_len = strlen(buffer);

		dir = pnp_boot_safe_opendir(buffer);
		if (dir == NULL) {
			//SHOW_ERROR(3, "Directory %s doesn't exists", buffer);
			// directory doesn't exist
			return tell_all ? B_OK : B_NAME_NOT_FOUND;
		}

		while ((entry = pnp_boot_safe_readdir(dir)) != NULL) {
			buffer[dir_len] = 0;
			strlcat(buffer, "/", PATH_MAX);
			strlcat(buffer, entry->d_name, PATH_MAX);

			// skip default directory entries
			if (!strcmp( entry->d_name, ".")
				|| !strcmp( entry->d_name, ".."))
				continue;

			if (notify_probe_by_file( node, buffer) == B_OK && !tell_all) {
				// tell_all is false, return on first hit
				pnp_boot_safe_closedir(dir);
				return B_OK;
			}
		}

		pnp_boot_safe_closedir(dir);
	}

	return tell_all ? B_OK : B_NAME_NOT_FOUND;
}


/**	find normal consumer of node that are stored under <dir>; first, we
 *	look for a specific driver; if none could be found, find a generic
 *	one path, buffer - used as scratch buffer (all of size PATH_MAX + 1)
 *	return: B_NAME_NOT_FOUND if no consumer could be found
 */

static status_t
find_normal_consumer(pnp_node_info *node, const char *dir,
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
				strlcat(buffer, path, PATH_MAX);

				// do a stat to avoid error message if Specific Driver
				// isn't provided
				if (pnp_boot_safe_lstat(buffer, &dummy) == 0) {
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
	strlcpy(path, dir, PATH_MAX);
	strlcat(path, GENERIC_SUBDIR, PATH_MAX);

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
 *	dir			- directory
 *	filename_pattern - pattern of file name
 *	*num_parts	- number of split positions
 */

static status_t
preprocess_consumer_names(const char *pattern, char *buffer,
	char **dir, char **filename_pattern, int *const num_parts)
{
	char *last_slash, *str, *dest;
	bool parts_began;

	// make a copy of pattern as we will strip escapes from directory part
	strlcpy(buffer, pattern, PATH_MAX);

	// find directory part and count splitpoints
	parts_began = false;
	last_slash = NULL;
	*num_parts = 1;

	for (str = buffer; *str; ++str) {
		switch (*str) {
			case '^':
				// honour escaped characters, taking care of trailing escape
				if (*(str + 1))
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
					if (*str == '^') {
						if (*(str + 1) != 0)
							++str;
					} else if (*str == '%')
						break;
				}

				if (*str == 0) {
					dprintf("missing matching '%%' in consumer pattern %s\n", pattern);
					return B_BAD_VALUE;
				}
				break;

			case '/':
				// directory finishes at last "/" before first "|"
				if (!parts_began)
					last_slash = str;
				break;
		}
	}

	if (last_slash == 0) {
		dprintf("missing directory in consumer pattern %s\n", pattern);
		return B_BAD_VALUE;
	}

	// split into directory and filename pattern in place 
	*dir = buffer;
	*last_slash = 0;
	*filename_pattern = last_slash + 1;

	// remove escape sequences from directory
	for (str = buffer, dest = buffer; *str; ++str) {
		if (*str == '^' && (str + 1) != 0)
			++str;

		*dest++ = *str;
	}

	*dest = 0;

	TRACE(("dir=%s, filename_pattern=%s, num_parts=%d\n",
		*dir, *filename_pattern, *num_parts));

	return B_OK;
}


/**	find consumers for one given pattern
 *	has_normal_drivers - in: true - don't search for specific driver
 *		out: true - specific driver was found
 */

static status_t
notify_dynamic_consumer(pnp_node_info *node, const char *pattern,
	bool *has_normal_driver)
{	
	status_t res;
	char *buffers;
	char *dir, *filename_pattern;
	int num_parts;

	TRACE(("notify_dynamic_consumer(pattern=%s, has_normal_driver=%d)\n",
		pattern, *has_normal_driver));

	// we need three buffers - allocate them at once for simpliness	
	buffers = malloc(3 * (PATH_MAX + 1));
	if (buffers == NULL)
		return B_NO_MEMORY;

	res = preprocess_consumer_names(pattern, buffers + 2 * (PATH_MAX + 1), 
		&dir, &filename_pattern, &num_parts);
	if (res < B_OK)
		goto err;

	if (!*has_normal_driver) {
		// find specific/generic consumer		
		res = find_normal_consumer(node, dir, filename_pattern, num_parts, 
			buffers, buffers + PATH_MAX + 1, has_normal_driver);
		if (res != B_OK && res != B_NAME_NOT_FOUND)
			// only abort if there was a "real" problem; 
			// if there is no specific/generic consumer, we don't bother
			// (having no driver is not funny but happens)
			goto err;
	}

	// tell universal drivers
	strlcpy(buffers, dir, PATH_MAX);
	strlcat(buffers, UNIVERSAL_SUBDIR, PATH_MAX);

	res = try_drivers(node, buffers, true, buffers + PATH_MAX + 1);
	if (res != B_OK && res != B_NAME_NOT_FOUND)
		// again, only abort on real problems
		goto err;

	free(buffers);

	TRACE(("done\n"));
	return B_OK;

err:
	free(buffers);
	return res;
}


//	#pragma mark -
//	device manager private API


/** find and notify dynamic consumers that device was added
 *	errors returned by consumers aren't reported, only problems
 *	like malformed consumer patterns
 */

status_t
pnp_notify_dynamic_consumers(pnp_node_info *node)
{
	int i;
	char *buffer;
	status_t res;
	bool has_normal_driver = false;

	TRACE(("pnp_notify_dynamic_consumers()\n"));

	buffer = malloc(PATH_MAX + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	//pnp_load_boot_links();

	// first, append nothing, then "/0", "/1" etc.
	for (i = -1; ; ++i) {
		char *consumer;

		strcpy(buffer, PNP_DRIVER_DYNAMIC_CONSUMER);
		if (i >= 0)
			sprintf(buffer + strlen( buffer ), "/%d", i);

		// if no more dynamic consumers, cancel loop silently
		if (pnp_get_attr_string(node, buffer, &consumer, false) != B_OK) {
			// starting with .../0 is OK, so ignore error if i=-1
			if (i == -1)
				continue;
			else
				break;
		}

		TRACE(("Consumer pattern %d: %s\n", i, consumer));

		res = notify_dynamic_consumer(node, consumer, &has_normal_driver);

		free(consumer);

		if (res != B_OK) {
			// this is only reached if a serious error occured,
			// see notify_dynamic_consumer()
			goto err;
		}
	}

	free(buffer);
	//pnp_unload_boot_links();

	return B_OK;

err:
	free(buffer);
	//pnp_unload_boot_links();

	return res;
}


/**	Notify fixed consumers that device was added; in contrast to dynamic
 *	consumers, errors reported by fixed consumers are not ignored but
 *	returned.
 */

status_t
pnp_notify_fixed_consumers(pnp_node_info *node)
{
	int i;
	char *buffer;

	TRACE(("pnp_notify_fixed_consumers()\n"));

	buffer = malloc(PATH_MAX + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	// first, append nothing, then "/0", "/1" etc.
	for (i = -1; ; ++i) {
		char *consumer;

		strcpy(buffer, PNP_DRIVER_FIXED_CONSUMER);
		if (i >= 0)
			sprintf(buffer + strlen(buffer), "/%d", i);

		// if no more fixed consumers, cancel loop silently
		if (pnp_get_attr_string( node, buffer, &consumer, false) != B_OK)
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

