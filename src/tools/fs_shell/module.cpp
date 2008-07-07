/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/** Manages kernel add-ons and their exported modules. */

#include "module.h"

#include <stdlib.h>

#include "fssh_errors.h"
#include "fssh_kernel_export.h"
#include "fssh_lock.h"
#include "fssh_module.h"
#include "fssh_string.h"
#include "hash.h"


//#define TRACE_MODULE
#ifdef TRACE_MODULE
#	define TRACE(x) fssh_dprintf x
#else
#	define TRACE(x) ;
#endif
#define FATAL(x) fssh_dprintf x


namespace FSShell {


#define MODULE_HASH_SIZE 16

enum module_state {
	MODULE_QUERIED = 0,
	MODULE_LOADED,
	MODULE_INIT,
	MODULE_READY,
	MODULE_UNINIT,
	MODULE_ERROR
};


/* Each known module will have this structure which is put in the
 * gModulesHash, and looked up by name.
 */

struct module {
	struct module		*next;
	char				*name;
	char				*file;
	int32_t				ref_count;
	fssh_module_info	*info;		/* will only be valid if ref_count > 0 */
	int32_t				offset;		/* this is the offset in the headers */
	module_state		state;		/* state of module */
	uint32_t			flags;
};

#define FSSH_B_BUILT_IN_MODULE	2


/* locking scheme: there is a global lock only; having several locks
 * makes trouble if dependent modules get loaded concurrently ->
 * they have to wait for each other, i.e. we need one lock per module;
 * also we must detect circular references during init and not dead-lock
 */
static fssh_recursive_lock sModulesLock;		

/* we store the loaded modules by directory path, and all known modules by module name
 * in a hash table for quick access
 */
static hash_table *sModulesHash;


/** calculates hash for a module using its name */

static uint32_t
module_hash(void *_module, const void *_key, uint32_t range)
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

	return fssh_strcmp(module->name, name);
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


/** Extract the information from the module_info structure pointed at
 *	by "info" and create the entries required for access to it's details.
 */

static fssh_status_t
create_module(fssh_module_info *info, const char *file, int offset, module **_module)
{
	module *module;

	TRACE(("create_module(info = %p, file = \"%s\", offset = %d, _module = %p)\n",
		info, file, offset, _module));

	if (!info->name)
		return FSSH_B_BAD_VALUE;

	module = (struct module *)hash_lookup(sModulesHash, info->name);
	if (module) {
		FATAL(("Duplicate module name (%s) detected... ignoring new one\n", info->name));
		return FSSH_B_FILE_EXISTS;
	}

	if ((module = (struct module *)malloc(sizeof(struct module))) == NULL)
		return FSSH_B_NO_MEMORY;

	TRACE(("create_module: name = \"%s\", file = \"%s\"\n", info->name, file));

	module->name = fssh_strdup(info->name);
	if (module->name == NULL) {
		free(module);
		return FSSH_B_NO_MEMORY;
	}

	module->file = fssh_strdup(file);
	if (module->file == NULL) {
		free(module->name);
		free(module);
		return FSSH_B_NO_MEMORY;
	}

	module->state = MODULE_QUERIED;
	module->info = info;
	module->offset = offset;
		// record where the module_info can be found in the module_info array
	module->ref_count = 0;
	module->flags = info->flags;

	fssh_recursive_lock_lock(&sModulesLock);
	hash_insert(sModulesHash, module);
	fssh_recursive_lock_unlock(&sModulesLock);

	if (_module)
		*_module = module;

	return FSSH_B_OK;
}


/** Initializes a loaded module depending on its state */

static inline fssh_status_t
init_module(module *module)
{
	switch (module->state) {
		case MODULE_QUERIED:
		case MODULE_LOADED:
		{
			fssh_status_t status;
			module->state = MODULE_INIT;

			// init module

			TRACE(("initializing module %s (at %p)... \n", module->name, module->info->std_ops));
			status = module->info->std_ops(FSSH_B_MODULE_INIT);
			TRACE(("...done (%s)\n", strerror(status)));

			if (status >= FSSH_B_OK)
				module->state = MODULE_READY;
			else {
				module->state = MODULE_LOADED;
			}

			return status;
		}

		case MODULE_READY:
			return FSSH_B_OK;

		case MODULE_INIT:
			FATAL(("circular reference to %s\n", module->name));
			return FSSH_B_ERROR;

		case MODULE_UNINIT:
			FATAL(("tried to load module %s which is currently unloading\n", module->name));
			return FSSH_B_ERROR;

		case MODULE_ERROR:
			FATAL(("cannot load module %s because its earlier unloading failed\n", module->name));
			return FSSH_B_ERROR;

		default:
			return FSSH_B_ERROR;
	}
	// never trespasses here
}


/** Uninitializes a module depeding on its state */

static inline int
uninit_module(module *module)
{
	TRACE(("uninit_module(%s)\n", module->name));

	switch (module->state) {
		case MODULE_QUERIED:
		case MODULE_LOADED:
			return FSSH_B_NO_ERROR;

		case MODULE_INIT:
			fssh_panic("Trying to unload module %s which is initializing\n", module->name);
			return FSSH_B_ERROR;

		case MODULE_UNINIT:
			fssh_panic("Trying to unload module %s which is un-initializing\n", module->name);
			return FSSH_B_ERROR;

		case MODULE_READY:
		{
			fssh_status_t status;

			module->state = MODULE_UNINIT;

			TRACE(("uninitializing module %s...\n", module->name));
			status = module->info->std_ops(FSSH_B_MODULE_UNINIT);
			TRACE(("...done (%s)\n", strerror(status)));

			if (status == FSSH_B_NO_ERROR) {
				module->state = MODULE_LOADED;
				return 0;
			}

			FATAL(("Error unloading module %s (%s)\n", module->name, fssh_strerror(status)));

			module->state = MODULE_ERROR;
			module->flags |= FSSH_B_KEEP_LOADED;

			return status;
		}
		default:	
			return FSSH_B_ERROR;		
	}
	// never trespasses here
}


void
register_builtin_module(struct fssh_module_info *info)
{
	info->flags |= FSSH_B_BUILT_IN_MODULE;
		// this is an internal flag, it doesn't have to be set by modules itself

	if (create_module(info, "", -1, NULL) != FSSH_B_OK)
		fssh_dprintf("creation of built-in module \"%s\" failed!\n", info->name);
}


static int
dump_modules(int argc, char **argv)
{
	hash_iterator iterator;
	struct module *module;

	hash_rewind(sModulesHash, &iterator);
	fssh_dprintf("-- known modules:\n");

	while ((module = (struct module *)hash_next(sModulesHash, &iterator)) != NULL) {
		fssh_dprintf("%p: \"%s\", \"%s\" (%d), refcount = %d, state = %d\n",
			module, module->name, module->file, (int)module->offset, (int)module->ref_count,
			module->state);
	}

	return 0;
}


//	#pragma mark -
//	Exported Kernel API (private part)


/** Setup the module structures and data for use - must be called
 *	before any other module call.
 */

fssh_status_t
module_init(kernel_args *args)
{
	fssh_recursive_lock_init(&sModulesLock, "modules rlock");

	sModulesHash = hash_init(MODULE_HASH_SIZE, 0, module_compare, module_hash);
	if (sModulesHash == NULL)
		return FSSH_B_NO_MEMORY;

	fssh_add_debugger_command("modules", &dump_modules, "list all known & loaded modules");

	return FSSH_B_OK;
}


}	// namespace FSShell


//	#pragma mark -
//	Exported Kernel API (public part)


using namespace FSShell;


fssh_status_t
fssh_get_module(const char *path, fssh_module_info **_info)
{
	module *module;
	fssh_status_t status;

	TRACE(("get_module(%s)\n", path));

	if (path == NULL)
		return FSSH_B_BAD_VALUE;

	fssh_recursive_lock_lock(&sModulesLock);

	module = (struct module *)hash_lookup(sModulesHash, path);
	if (module == NULL)
		goto err;

	// The state will be adjusted by the call to init_module
	// if we have just loaded the file
	if (module->ref_count == 0)
		status = init_module(module);
	else
		status = FSSH_B_OK;

	if (status == FSSH_B_OK) {
		inc_module_ref_count(module);
		*_info = module->info;
	}

	fssh_recursive_lock_unlock(&sModulesLock);
	return status;

err:
	fssh_recursive_lock_unlock(&sModulesLock);
	return FSSH_B_ENTRY_NOT_FOUND;
}


fssh_status_t
fssh_put_module(const char *path)
{
	module *module;

	TRACE(("put_module(path = %s)\n", path));

	fssh_recursive_lock_lock(&sModulesLock);

	module = (struct module *)hash_lookup(sModulesHash, path);
	if (module == NULL) {
		FATAL(("module: We don't seem to have a reference to module %s\n", path));
		fssh_recursive_lock_unlock(&sModulesLock);
		return FSSH_B_BAD_VALUE;
	}
	
	if ((module->flags & FSSH_B_KEEP_LOADED) == 0) {
		dec_module_ref_count(module);

		if (module->ref_count == 0)
			uninit_module(module);
	}

	fssh_recursive_lock_unlock(&sModulesLock);
	return FSSH_B_OK;
}
