/* Userland modules emulation support
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app/Application.h>
#include <app/Roster.h>
#include <kernel/OS.h>
#include <kernel/image.h>
#include <drivers/module.h>
#include <storage/StorageDefs.h>
#include <storage/FindDirectory.h>
#include <storage/Path.h>

#define ASSERT(condition)	if (!(condition)) { debugger("Assertion failed!"); }

typedef enum {
  MODULE_LOADED = 0,
  MODULE_INITING,
  MODULE_READY,
  MODULE_UNINITING,
  MODULE_ERROR
} module_state;

typedef struct module {
	struct module * next;
	uint32 id;
	char * name;
	module_info * info;
	struct module_addon * addon;	// the module addon this module live in
									// if NULL, builtin module addon
	int32 ref_count;	// reference count of get_module() made on this module
	bool keep_loaded;
	module_state state;
} module;

typedef struct module_addon {
	struct module_addon * next;
	int32 ref_count;	// reference count of get_module() made using this addon
	bool keep_loaded;
	char * path;
	image_id addon_image;	// if -1, not loaded in memory currently
	module_info ** infos;	// valid only when addon_image != -1
} module_addon;

typedef struct module_list_cookie {
	char * prefix;
	// TODO
} module_list_cookie;

#define LOCK_MODULES		acquire_sem(g_modules_lock)
#define UNLOCK_MODULES		release_sem(g_modules_lock)

// local prototypes
// ------------------

static module * search_module(const char * name);
static status_t init_module(module * m);
static status_t uninit_module(module * m);
static module * find_loaded_module_by_name(const char * name);
static module * find_loaded_module_by_id(uint32 id);

static module_addon * load_module_addon(const char * path);
static status_t unload_module_addon(module_addon * ma);

// globals
// ------------------

static sem_id g_modules_lock = -1;	// One lock for rule them all, etc...
static module * g_modules = NULL;
static module_addon * g_module_addons = NULL;
static int32 g_next_module_id = 1;


// Public routines
// ---------------

status_t get_module(const char * name, module_info ** mi)
{
	module * m;
	
	printf("get_module(%s)\n", name);
	
	m = find_loaded_module_by_name(name);
	if (!m)
		m = search_module(name);
	
	if (!m)
		return B_NAME_NOT_FOUND;
	
	*mi = m->info;
	
	if (m->addon) // built-in modules don't comes from addon...
		atomic_add(&m->addon->ref_count, 1);

	if (atomic_add(&m->ref_count, 1) == 0)
		// first time we reference this module, so let's init it:
		return init_module(m);
		
	return B_OK;
}

status_t put_module(const char * name)
{
	module * m;

	printf("put_module(%s)\n", name);
	
	m = find_loaded_module_by_name(name);
	if (!m)
		// Hum??? Sorry, this module name was never get_module()'d
		return B_NAME_NOT_FOUND;

	if (atomic_add(&m->ref_count, -1) <= 1)
		// this module is no more used...
		uninit_module(m);
	
	if (!m->addon)
		// built-in modules are module addon less...
		return B_OK;
	
	if (atomic_add(&m->addon->ref_count, -1) > 1)
		// Still other module(s) using this module addon
		return B_OK;
		
	// okay, this module addon is no more used
	// let's free up some memory
	return unload_module_addon(m->addon);
}


status_t get_next_loaded_module_name(uint32 *cookie, char *buf, size_t *bufsize)
{
	module * m;
	status_t status;
	
	if (buf == NULL && bufsize == NULL)
		return B_BAD_VALUE;

	LOCK_MODULES;

	if (*cookie == 0)
		// first call expected value
		m = g_modules;
	else {
		// find last loaded module returned, and seek to next one
		m = (module *) find_loaded_module_by_id((int) *cookie);
		if (m)
			m = m->next;
	};

	// find next loaded module
	while (m) {
		if (m->ref_count)
			break;
		m = m->next;
	};

	status = B_OK;
	if (m) {
		ASSERT(m->info);
		if (buf != NULL)
			strncpy(buf, m->info->name, *bufsize);
		else
			*bufsize = strlen(m->info->name + 1);
		*cookie = m->id;
	} else
		status = B_BAD_INDEX;

	UNLOCK_MODULES;

	return status;
}


void * open_module_list(const char *prefix)
{
	module_list_cookie * mlc;
	
	if (prefix == NULL)
		return NULL;
		
	mlc = (module_list_cookie *) malloc(sizeof(*mlc));
	mlc->prefix = strdup(prefix);
	
	return mlc;
}


status_t read_next_module_name(void *cookie, char *buf, size_t *bufsize)
{
	module_list_cookie * mlc = (module_list_cookie *) cookie;
	
	if (!mlc)
		return B_BAD_VALUE;
	
	// TODO
	return B_ERROR;
}


status_t close_module_list(void *cookie)
{
	module_list_cookie * mlc = (module_list_cookie *) cookie;

	ASSERT(mlc);
	ASSERT(mlc->prefix);
	
	free(mlc->prefix);
	free(mlc);

	return B_ERROR;
}


// #pragma mark -
// Private routines

static module_addon * load_module_addon(const char * path)
{
	module_addon * ma;
	image_id addon_id;
	module_info ** mi;
	status_t status;

	ASSERT(path);

	printf("Loading %s addon\n", path);
	addon_id = load_add_on(path);
	if (addon_id < 0)
		return NULL;
		
	ma = NULL;
		
	status = get_image_symbol(addon_id, "modules", B_SYMBOL_TYPE_DATA, (void **) &mi);
	if (status != B_OK) {
		//  No "modules" symbol found in this addon
		printf("No \"modules\" symbol found in %s addon: not a module addon!\n", path);
		goto error;
	};

	ma = (module_addon *) malloc(sizeof(*ma));
	if (!ma)
		// Gasp: not enough memory!
		goto error;
	
	LOCK_MODULES;

	ma->ref_count = 0;
	ma->keep_loaded = false;
	ma->path = strdup(path);
	ma->addon_image = addon_id;
	ma->infos = mi;

   	while(*mi)  {
		module * m;

		m = (module *) malloc(sizeof(*m));
		if (!m)
			// Gasp, again: not enough memory!
			goto error;
		
		m->ref_count = 0;
		m->id = atomic_add(&g_next_module_id, 1);
		m->info = (*mi);
		m->name = strdup(m->info->name);
		m->addon = ma;
		m->keep_loaded = (m->info->flags & B_KEEP_LOADED) ? true : false;
		            			
   		m->state = MODULE_LOADED;
		            			
		m->next = g_modules;
		g_modules = m;

   		mi++;
	};

	// add this module addon to the list
	ma->next = g_module_addons;
	g_module_addons = ma;

	UNLOCK_MODULES;
   			
	return ma;

error:
	printf("Error while load_module_addon(%s)\n", path);

	if (ma) {
		// remove any appended modules by this module addon until we got error... 
		module * prev;
		module * m;
		
		prev = NULL;
		m = g_modules;
		while (m) {
			if (m->addon == ma) {
				module * tmp = m;
				
				m = tmp->next;
				
				if (prev)
					prev->next = tmp->next;
				else
					g_modules = tmp->next;
					
				if (tmp->name)
					free(tmp->name);
				free(tmp);
				continue;
			};
			
			prev = m;
			m = m->next;
		};


		UNLOCK_MODULES;
	
		if (ma->path)
			free(ma->path);	
		free(ma);
	};

	printf("Unloading %s addon\n", path);
	unload_add_on(addon_id);
	return NULL;
}

static status_t unload_module_addon(module_addon * ma)
{
	module * m;
	module * prev;
	status_t status;
	
	ASSERT(ma);

	if (ma->keep_loaded) {
		printf("B_KEEP_LOADED flag set for %s module addon. Will be *never* unloaded!\n",
			ma->path); 
		return B_OK;
	};

	if (ma->ref_count)
		// still someone needing this module addon, it seems?
		return B_OK;

	if (ma->addon_image < 0)
		return B_OK;

	status = B_ERROR;
	
	printf("Unloading %s addon\n", ma->path);
	status = unload_add_on(ma->addon_image);
	if (status != B_OK) {
		printf("unload_add_on(%ld -> %s) failed: %ld!\n", ma->addon_image, ma->path, status);
		return status;
	};
		
	LOCK_MODULES;

	// remove the modules coming from this module addon from g_modules list
	prev = NULL;
	m = g_modules;
	while (m) {
		if (m->addon == ma) {
			module * tmp = m;
				
			m = tmp->next;
				
			if (prev)
				prev->next = tmp->next;
			else
				g_modules = tmp->next;
					
			if (tmp->name)
				free(tmp->name);
			free(tmp);
			continue;
		};
			
		prev = m;
		m = m->next;
	};

	// remove the module addon from g_module_addons list:
	if (g_module_addons == ma)
		g_module_addons = ma->next;
	else {
		module_addon * tmp;
		tmp = g_module_addons;
		while (tmp && tmp->next != ma)
			tmp = tmp->next;
			
		ASSERT(tmp);
		tmp->next = ma->next;
	};

	if (ma->path)
		free(ma->path);
	free(ma);
	
	UNLOCK_MODULES;
	
	return B_OK;
}


static module * search_module(const char * name)
{
	BPath path;
	BPath addons_path;
	BEntry entry;
	int i;
	module * found_module;
	directory_which addons_paths[] = {
		(directory_which) -2,	// APP_ADDONS_DIRECTORY
		B_USER_ADDONS_DIRECTORY,
		// B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY,
		(directory_which) -1
	};
	
	printf("search_module(%s):\n", name);

	found_module = NULL;
	for (i = 0; addons_paths[i] != -1 && found_module == NULL; i++) {
		if (addons_paths[i] == -2) {
			// compute "%A/add-ons" path
			app_info ai;
			be_app->GetAppInfo(&ai);
			entry.SetTo(&ai.ref);
			entry.GetPath(&addons_path);
			addons_path.GetParent(&addons_path);
			addons_path.Append("add-ons");
		} else {
			// try in "net_server" subfolder of officials add-ons repositories
			if (find_directory(addons_paths[i], &addons_path, false, NULL) != B_OK)
				continue;
			addons_path.Append("net_server");
		};
		
		printf("Looking into %s\n", addons_path.Path());

		path.SetTo(addons_path.Path());
		path.Append(name);
		
		while(path != addons_path) {
			printf("  %s ?\n", path.Path());
			entry.SetTo(path.Path());
			if (entry.IsFile()) {
				module_addon * ma;
			
				// try to load the module addon	
				ma = load_module_addon(path.Path());
	            if (ma) {
					found_module = find_loaded_module_by_name(name);
					if (found_module)
						break;

					unload_module_addon(ma);
				};	// if (ma)
			};	// if (entry.IsFile())

			// okay, remove the current path leaf and try again...
			path.GetParent(&path);
		};
	};

	if (found_module)
		printf("  Found it in %s addon module!\n",
				found_module->addon ? found_module->addon->path : "BUILTIN");
	
	return found_module;
}


static status_t init_module(module * m)
{
	status_t status;
	
	ASSERT(m);
	
	switch (m->state) {
	case MODULE_LOADED:
		m->state = MODULE_INITING;
		ASSERT(m->info);
		printf("Initing module %s... ", m->name);
		status = m->info->std_ops(B_MODULE_INIT);
		printf("done (%s).\n", strerror(status));
		m->state = (status == B_OK) ? MODULE_READY : MODULE_LOADED;
		
		if (m->state == MODULE_READY && m->keep_loaded && m->addon) {
  			// one module (at least) was inited and request to never being
			// unload from memory, so keep the corresponding addon loaded
   			printf("module %s set B_KEEP_LOADED flag:\nmodule addon %s will never be unloaded!\n",
				m->name, m->addon->path);
   			m->addon->keep_loaded = true;
   		};
		break;
		
	case MODULE_READY:
		status = B_OK;
		break;
		
	case MODULE_INITING: 	// circular reference!!!
	case MODULE_UNINITING:	// initing a module currently unloading...
	case MODULE_ERROR: 		// module failed to unload previously...
	default:				// Unknown module state!!!
		status = B_ERROR;
		break;
	};
	
	return status;
}


static status_t uninit_module(module * m)
{
	status_t status;
	
	ASSERT(m);
	
	switch (m->state) {
	case MODULE_READY:		
		m->state = MODULE_UNINITING;
		ASSERT(m->info);
		printf("Uniniting module %s... ", m->name);
		status = m->info->std_ops(B_MODULE_UNINIT);
		printf("done (%s).\n", strerror(status));
		m->state = (status == B_OK) ? MODULE_LOADED : MODULE_ERROR;
		break;
		
	case MODULE_LOADED:
		// No need to uninit it, all is fine so.
		status = B_OK;
		break;
		
	case MODULE_INITING:	// uniniting while initializing
	case MODULE_UNINITING:	// uniniting already pending
	case MODULE_ERROR: 		// module failed previously...
	default:				// Unknown module state!!!
		status = B_ERROR;
		break;
	};
	
	return status;
}


static module * find_loaded_module_by_name(const char * name)
{
	module * m;
	
	LOCK_MODULES;
	
	m = g_modules;
	while (m) {
		if (strcmp(name, m->name) == 0)
			break;
		m = m->next;
	};
	
	UNLOCK_MODULES;
	return m;
}


static module * find_loaded_module_by_id(uint32 id)
{
	module * m;
	
	LOCK_MODULES;
	
	m = g_modules;
	while (m) {
		if (m->id == id)
			break;
		m = m->next;
	};
	
	UNLOCK_MODULES;
	return m;
}


// #pragma mark -

#define NET_CORE_MODULE_NAME 		"network/core/v1"
#define NET_ETHERNET_MODULE_NAME 	"network/interfaces/ethernet"
#define NET_IPV4_MODULE_NAME 		"network/protocols/ipv4/v1"

#define MODULE_LIST_PREFIX	"network/protocols"

int main(int argc, char **argv)
{
	module_info * core;
	module_info * ethernet;
	module_info * ipv4;
	char module_name[256];
	uint32 cookie;
	size_t sz;
	void * ml_cookie;
	
	new BApplication("application/x-vnd-OBOS-net_server");
	
	printf("open_module_list(%s):\n", MODULE_LIST_PREFIX);
	ml_cookie = open_module_list(MODULE_LIST_PREFIX);
	sz = sizeof(module_name);
	while(read_next_module_name(ml_cookie, module_name, &sz) == B_OK) {
		printf("  %s\n", module_name);
		sz = sizeof(module_name);
	};
	close_module_list(ml_cookie);
	printf("close_module_list()\n");
	
	
	core = NULL;
	get_module(NET_CORE_MODULE_NAME, (module_info **) &core);
	
	ethernet = NULL;
	get_module(NET_ETHERNET_MODULE_NAME, (module_info **) &ethernet);
	
	ipv4 = NULL;
	get_module(NET_IPV4_MODULE_NAME, (module_info **) &ipv4);
	
	printf("get_next_loaded_module_name() test:\n");
	cookie = 0;
	sz = sizeof(module_name);
	while (get_next_loaded_module_name(&cookie, module_name, &sz) == B_OK)
		printf("%ld: %s\n", cookie, module_name);

	if (ipv4)
		put_module(NET_IPV4_MODULE_NAME);

	if (ethernet)
		put_module(NET_ETHERNET_MODULE_NAME);

	if (core)
		put_module(NET_CORE_MODULE_NAME);

	printf("get_next_loaded_module_name() test:\n");
	cookie = 0;
	sz = sizeof(module_name);
	while (get_next_loaded_module_name(&cookie, module_name, &sz) == B_OK)
		printf("%ld: %s\n", cookie, module_name);

	delete be_app;
	return 0;
}

