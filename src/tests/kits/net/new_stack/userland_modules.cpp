/* Userland modules emulation support
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include <drivers/KernelExport.h>
#include <drivers/module.h>

#include <app/Application.h>
#include <app/Roster.h>
#include <kernel/OS.h>
#include <kernel/image.h>
#include <storage/StorageDefs.h>
#include <storage/FindDirectory.h>
#include <storage/Path.h>
#include <storage/Directory.h>

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
	char * search_paths;
	char * search_path;
	char * next_path_token;
	BList * dir_stack;
	module_addon * ma;	// current module addon looked up
	module_info ** mi;	// current module addon module info
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

extern "C" {

_EXPORT status_t get_module(const char * name, module_info ** mi)
{
	status_t status;
	module * m;
	
	// printf("get_module(%s)\n", name);
	
	m = find_loaded_module_by_name(name);
	if (!m)
		m = search_module(name);
	
	if (!m)
		return B_NAME_NOT_FOUND;
	
	*mi = m->info;
	
	status = B_OK;

	if (m->addon) // built-in modules don't comes from addon...
		atomic_add(&m->addon->ref_count, 1);

	if (atomic_add(&m->ref_count, 1) == 0) {
		// first time we reference this module, so let's init it:
		status = init_module(m);
		if (status != B_OK) {
			printf("Failed to init module %s: %s.\n", m->name, strerror(status));
			unload_module_addon(m->addon);	// unload the module addon...
		};
	};
		
	return status;
}

_EXPORT status_t put_module(const char * name)
{
	module * m;

	// printf("put_module(%s)\n", name);
	
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


_EXPORT status_t get_next_loaded_module_name(uint32 *cookie, char *buf, size_t *bufsize)
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


_EXPORT void * open_module_list(const char *prefix)
{
	module_list_cookie * mlc;
	char * addon_path;
	
	if (prefix == NULL)
		return NULL;
		
	mlc = (module_list_cookie *) malloc(sizeof(*mlc));
	mlc->prefix = strdup(prefix);
	
	addon_path = getenv("ADDON_PATH");
	mlc->search_paths = (addon_path ? strdup(addon_path) : NULL);
	mlc->search_path = strtok_r(mlc->search_paths, ":", &mlc->next_path_token);
	mlc->dir_stack = new BList();
	
	mlc->ma = NULL;
	mlc->mi = NULL;

	return mlc;
}


_EXPORT status_t read_next_module_name(void *cookie, char *buf, size_t *bufsize)
{
	module_list_cookie * mlc = (module_list_cookie *) cookie;
	
	if (!bufsize)
		return B_BAD_VALUE;
	
	if (!mlc)
		return B_BAD_VALUE;
		
	/* Okay, take some time to understand how this function works!
	   Basicly, we iterate thru:
	   - each searchable add-ons path root
	   - each (sub-)directory under the current add-ons path root
	   - each module add-on file in the current (sub-)directory
	   - each module name published by current module add-on
	   
	   As the iteration involve sub-directory walks, we use recursive calls.
	   Sorry if this code sounds too complex...
	*/

	if (mlc->ma && mlc->mi) {
		// we have a module addon still loaded from a last call
		// so keep looking at his exported module names list
		while (*mlc->mi) {
			module_info * mi = *mlc->mi;
			mlc->mi++;
			if(strstr(mi->name, mlc->prefix)) {
				// We find a matching module name. At least. Yeah!!!
				if (buf) strncpy(buf, mi->name, *bufsize);
				*bufsize = strlen(mi->name);
				return B_OK;
			};
		};
		
		// We've iterate all module names of this module addon. Find another one...
		atomic_add(&mlc->ma->ref_count, -1);
		unload_module_addon(mlc->ma);
		mlc->ma = NULL;
		mlc->mi = NULL;
	};
		
	// Iterate all searchable add-ons paths
	while (mlc->search_path) {
		BDirectory * dir;
		BEntry entry;
		BPath path;
		status_t status;
			
		// Get current directory
		dir = (BDirectory *) mlc->dir_stack->LastItem();
		if (!dir) {
			// find add-ons root directory in this search path
			if (strncmp(mlc->search_path, "%A/", 3) == 0) {
				// resolve "%A/..." path
				app_info ai;
				
				be_app->GetAppInfo(&ai);
				entry.SetTo(&ai.ref);
				entry.GetPath(&path);
				path.GetParent(&path);
				path.Append(mlc->search_path + 3);
			} else {
				path.SetTo(mlc->search_path);
			};
			
			// We look *only* under prefix-matching sub-path 
			path.Append(mlc->prefix);

			// printf("Looking module(s) in %s/%s...\n", mlc->search_path, mlc->prefix);
			 
			dir = new BDirectory(path.Path());
			if (dir)
				mlc->dir_stack->AddItem(dir);
		};
		
		// Iterate current directory content
		if (dir) {  
			while (dir->GetNextEntry(&entry) == B_OK) {
				entry.GetPath(&path);
				// printf("  %s ?\n", path.Path());

				if (entry.IsDirectory()) {
					BDirectory * subdir;
					// push this directory on dir_stack
					subdir = new BDirectory(path.Path());
					if (!subdir)
						continue;
						
					mlc->dir_stack->AddItem(subdir);
					// recursivly search this sub-directory
					return read_next_module_name(cookie, buf, bufsize);
				};
				
				if (entry.IsFile() || entry.IsSymLink()) {
					mlc->ma = load_module_addon(path.Path());
		            if (!mlc->ma)
		            	// Oh-oh, not a loadable module addon!?
		            	// WTF it's doing there?!?
						continue;

					atomic_add(&mlc->ma->ref_count, 1);
					// call ourself to enter the module names list iteration at
					// function begining code...
					mlc->mi = mlc->ma->infos;
					return read_next_module_name(cookie, buf, bufsize);
				};
			};
			
			// We walk thru all this directory content, go back to parent
			status = mlc->dir_stack->RemoveItem(dir);
			delete dir;
		};
		
		if (!mlc->dir_stack->IsEmpty())
			continue;
			
		// We walk thru all this search path content, next now 
		mlc->search_path = strtok_r(NULL, ":", &mlc->next_path_token);
	};

	// Module(s) list search done, ending...
	return B_ERROR;		
}


_EXPORT status_t close_module_list(void *cookie)
{
	module_list_cookie * mlc = (module_list_cookie *) cookie;
	BDirectory * dir;

	ASSERT(mlc);
	ASSERT(mlc->prefix);

	if (mlc->ma) {
		atomic_add(&mlc->ma->ref_count, -1);
		unload_module_addon(mlc->ma);
	};

	while((dir = (BDirectory *) mlc->dir_stack->FirstItem())) {
		mlc->dir_stack->RemoveItem(dir);
		delete dir;
	};

	delete mlc->dir_stack;	
	
	free(mlc->search_paths);
	free(mlc->prefix);
	free(mlc);

	return B_ERROR;
}

// #pragma mark -
// Some KernelExport.h support from userland

_EXPORT void dprintf(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}


_EXPORT void kprintf(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}


_EXPORT status_t load_driver_symbols(const char *driver_name)
{
	// Userland debugger will extract symbols itself...
	return B_OK;
}


_EXPORT thread_id spawn_kernel_thread(thread_entry func, const char *name, long priority, void *arg)
{
	return spawn_thread(func, name, priority, arg);
}



_EXPORT int send_signal_etc(pid_t thid, uint sig, uint32 flags)
{
	return send_signal(thid, sig);
}	


}  // extern "C"


// #pragma mark -
// Private routines

static module_addon * load_module_addon(const char * path)
{
	module_addon * ma;
	image_id addon_id;
	module_info ** mi;
	status_t status;

	ASSERT(path);

	addon_id = load_add_on(path);
	if (addon_id < 0) {
		printf("Failed to load %s addon: %s.\n", path, strerror(addon_id));
		return NULL;
	};
	
	// printf("Addon %s loaded.\n", path);
		
	ma = NULL;
		
	status = get_image_symbol(addon_id, "modules", B_SYMBOL_TYPE_DATA, (void **) &mi);
	if (status != B_OK) {
		//  No "modules" symbol found in this addon
		printf("Symbol \"modules\" not found in %s addon: not a module addon!\n", path);
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

	unload_add_on(addon_id);
	// printf("Addon %s unloaded.\n", path);
	return NULL;
}

static status_t unload_module_addon(module_addon * ma)
{
	module * m;
	module * prev;
	status_t status;
	
	if (!ma)
		// built-in modules are addon-less, so nothing to do...
		return B_OK;

	if (ma->keep_loaded) {
		printf("B_KEEP_LOADED flag set for %s module addon. Will be *never* unloaded!\n",
			ma->path); 
		return B_OK;
	};

	if (ma->ref_count)
		// still someone needing this module addon, it seems?
		return B_OK;

	if (ma->addon_image < 0)
		// built-in addon, it seems...
		return B_OK;

	status = unload_add_on(ma->addon_image);
	if (status != B_OK) {
		printf("Failed to unload %s addon: %s.\n", ma->path, strerror(status));
		return status;
	};
	// printf("Addon %s unloaded.\n", ma->path);
		
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
	module * found_module;
	char * search_paths;
	char * search_path;
	char * next_path_token; 

	// printf("search_module(%s):\n", name);

	search_paths = getenv("ADDON_PATH");
	if (!search_paths)
		// Nowhere to search addons!!!
		return NULL;
		
	search_paths = strdup(search_paths);
	search_path = strtok_r(search_paths, ":", &next_path_token);
	
	found_module = NULL;
	while (search_path && found_module == NULL) {
		if (strncmp(search_path, "%A/", 3) == 0) {
			// compute "%A/..." path
			app_info ai;
			
			be_app->GetAppInfo(&ai);
			entry.SetTo(&ai.ref);
			entry.GetPath(&addons_path);
			addons_path.GetParent(&addons_path);
			addons_path.Append(search_path + 3);
		} else {
			addons_path.SetTo(search_path);
		};
		
		// printf("Looking into %s\n", search_path);

		path.SetTo(addons_path.Path());
		path.Append(name);
		
		while(path != addons_path) {
			// printf("  %s ?\n", path.Path());
			entry.SetTo(path.Path());
			if (entry.IsFile() || entry.IsSymLink()) {
				module_addon * ma;
			
				// try to load the module addon	
				ma = load_module_addon(path.Path());
	            if (ma) {
					found_module = find_loaded_module_by_name(name);
					if (found_module)
						break;

					unload_module_addon(ma);
				};	// if (ma)
			};	// if (entry.IsFile() || entry.IsSymLink())

			// okay, remove the current path leaf and try again...
			path.GetParent(&path);
		};
		
		search_path = strtok_r(NULL, ":", &next_path_token);
	};

	free(search_paths);

/*
	if (found_module)
		printf("  Found it in %s addon module!\n",
				found_module->addon ? found_module->addon->path : "BUILTIN");
*/

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
		// printf("Initing module %s... ", m->name);
		status = m->info->std_ops(B_MODULE_INIT);
		// printf("done (%s).\n", strerror(status));
		m->state = (status == B_OK) ? MODULE_READY : MODULE_LOADED;
		
		if (m->state == MODULE_READY && m->keep_loaded && m->addon) {
  			// one module (at least) was inited and request to never being
			// unload from memory, so keep the corresponding addon loaded
   			// printf("module %s set B_KEEP_LOADED flag:\nmodule addon %s will never be unloaded!\n",
			//	m->name, m->addon->path);
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
		// printf("Uniniting module %s... ", m->name);
		status = m->info->std_ops(B_MODULE_UNINIT);
		// printf("done (%s).\n", strerror(status));
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

#if 0
// #pragma mark -

#define NET_CORE_MODULE_NAME 		"network/core/v1"
#define NET_ETHERNET_MODULE_NAME 	"network/interfaces/ethernet"
#define NET_IPV4_MODULE_NAME 		"network/protocols/ipv4/v1"

#define MODULE_LIST_PREFIX	"network"

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
		if (strlen(module_name))
			printf("  %s\n", module_name);
		sz = sizeof(module_name);
	};
	close_module_list(ml_cookie);
	printf("close_module_list()\n");
	// return 0;
	
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
#endif


