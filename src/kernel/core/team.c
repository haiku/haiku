/* Team functions */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>
#include <kernel.h>
#include <thread.h>
#include <thread_types.h>
#include <int.h>
#include <khash.h>
#include <malloc.h>
#include <user_runtime.h>
#include <Errors.h>
#include <kerrors.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <atomic.h>
#include <syscalls.h>


struct team_key {
	team_id id;
};

struct team_arg {
	char *path;
	char **args;
	char **envp;
	unsigned int argc;
	unsigned int envc;
};

// team list
static void *team_hash = NULL;
static team_id next_team_id = 1;
static struct team *kernel_team = NULL;

spinlock team_spinlock = 0;

static struct team *create_team_struct(const char *name, bool kernel);
static void delete_team_struct(struct team *p);
static int team_struct_compare(void *_p, const void *_key);
static unsigned int team_struct_hash(void *_p, const void *_key, unsigned int range);
static void kfree_strings_array(char **strings, int strc);
static int user_copy_strings_array(char **strings, int strc, char ***kstrings);
static void _dump_team_info(struct team *p);
static int dump_team_info(int argc, char **argv);


static void
_dump_team_info(struct team *p)
{
	dprintf("TEAM: %p\n", p);
	dprintf("id:          0x%lx\n", p->id);
	dprintf("name:        '%s'\n", p->name);
	dprintf("next:        %p\n", p->next);
	dprintf("num_threads: %d\n", p->num_threads);
	dprintf("state:       %d\n", p->state);
	dprintf("pending_signals: 0x%x\n", p->pending_signals);
	dprintf("ioctx:       %p\n", p->ioctx);
	dprintf("path:        '%s'\n", p->path);
	dprintf("aspace_id:   0x%lx\n", p->_aspace_id);
	dprintf("aspace:      %p\n", p->aspace);
	dprintf("kaspace:     %p\n", p->kaspace);
	dprintf("main_thread: %p\n", p->main_thread);
	dprintf("thread_list: %p\n", p->thread_list);
}


static int
dump_team_info(int argc, char **argv)
{
	struct team *p;
	int id = -1;
	unsigned long num;
	struct hash_iterator i;

	if (argc < 2) {
		dprintf("team: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a hex number, treat it as such
	if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		num = atoul(argv[1]);
		if (num > vm_get_kernel_aspace()->virtual_map.base) {
			// XXX semi-hack
			_dump_team_info((struct team*)num);
			return 0;
		} else {
			id = num;
		}
	}

	// walk through the thread list, trying to match name or id
	hash_open(team_hash, &i);
	while ((p = hash_next(team_hash, &i)) != NULL) {
		if ((p->name && strcmp(argv[1], p->name) == 0) || p->id == id) {
			_dump_team_info(p);
			break;
		}
	}
	hash_close(team_hash, &i, false);
	return 0;
}


int
team_init(kernel_args *ka)
{
	// create the team hash table
	team_hash = hash_init(15, (addr)&kernel_team->next - (addr)kernel_team,
		&team_struct_compare, &team_struct_hash);

	// create the kernel team
	kernel_team = create_team_struct("kernel_team", true);
	if (kernel_team == NULL)
		panic("could not create kernel team!\n");
	kernel_team->state = TEAM_STATE_NORMAL;

	kernel_team->ioctx = vfs_new_io_context(NULL);
	if (kernel_team->ioctx == NULL)
		panic("could not create ioctx for kernel team!\n");

	//XXX should initialize kernel_team->path here. Set it to "/"?

	// stick it in the team hash
	hash_insert(team_hash, kernel_team);

	add_debugger_command("team", &dump_team_info, "list info about a particular team");
	return 0;
}


/**	Frees an array of strings in kernel space
 *	Parameters
 *		strings		strings array
 *		strc		number of strings in array
 */

static void
kfree_strings_array(char **strings, int strc)
{
	int cnt = strc;

	if (strings != NULL) {
		for (cnt = 0; cnt < strc; cnt++){
			free(strings[cnt]);
		}
	    free(strings);
	}
}


/**	Copy an array of strings from user space to kernel space
 *	Parameters
 *		strings		userspace strings array
 *		strc		number of strings in array
 *		kstrings	pointer to the kernel copy
 *	Returns < 0 on error and **kstrings = NULL
 */

static int
user_copy_strings_array(char **strings, int strc, char ***kstrings)
{
	char **lstrings;
	int err;
	int cnt;
	char *source;
	char buf[SYS_THREAD_STRING_LENGTH_MAX];

	*kstrings = NULL;

	if ((addr)strings >= KERNEL_BASE && (addr)strings <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	lstrings = (char **)malloc((strc + 1) * sizeof(char *));
	if (lstrings == NULL){
		return ENOMEM;
	}

	// scan all strings and copy to kernel space

	for (cnt = 0; cnt < strc; cnt++) {
		err = user_memcpy(&source, &(strings[cnt]), sizeof(char *));
		if(err < 0)
			goto error;

		if ((addr)source >= KERNEL_BASE && (addr)source <= KERNEL_TOP){
			err = ERR_VM_BAD_USER_MEMORY;
			goto error;
		}

		err = user_strncpy(buf, source, SYS_THREAD_STRING_LENGTH_MAX - 1);
		if (err < 0)
			goto error;
		buf[SYS_THREAD_STRING_LENGTH_MAX - 1] = 0;

		lstrings[cnt] = strdup(buf);
		if (lstrings[cnt] == NULL){
			err = ENOMEM;
			goto error;
		}
	}

	lstrings[strc] = NULL;

	*kstrings = lstrings;
	return B_NO_ERROR;

error:
	kfree_strings_array(lstrings, cnt);
	dprintf("user_copy_strings_array failed %d \n", err);
	return err;
}


int
user_team_wait_on_team(team_id id, int *uretcode)
{
	int retcode;
	int rc, rc2;

	if((addr)uretcode >= KERNEL_BASE && (addr)uretcode <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = team_wait_on_team(id, &retcode);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uretcode, &retcode, sizeof(int));
	if(rc2 < 0)
		return rc2;

	return rc;
}


int
team_wait_on_team(team_id id, int *retcode)
{
	struct team *p;
	thread_id tid;
	int state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	p = team_get_team_struct_locked(id);
	if(p && p->main_thread) {
		tid = p->main_thread->id;
	} else {
		tid = ERR_INVALID_HANDLE;
	}
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if(tid < 0)
		return tid;

	return thread_wait_on_thread(tid, retcode);
}


struct team *
team_get_team_struct(team_id id)
{
	struct team *p;
	int state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	p = team_get_team_struct_locked(id);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return p;
}


struct team *
team_get_team_struct_locked(team_id id)
{
	struct team_key key;

	key.id = id;

	return hash_lookup(team_hash, &key);
}


static int
team_struct_compare(void *_p, const void *_key)
{
	struct team *p = _p;
	const struct team_key *key = _key;

	if(p->id == key->id) return 0;
	else return 1;
}


static unsigned int
team_struct_hash(void *_p, const void *_key, unsigned int range)
{
	struct team *p = _p;
	const struct team_key *key = _key;

	if (p != NULL)
		return (p->id % range);

	return (key->id % range);
}


void
team_remove_team_from_hash(struct team *team)
{
	hash_remove(team_hash, team);
}


struct team *
team_get_kernel_team(void)
{
	return kernel_team;
}


team_id
team_get_kernel_team_id(void)
{
	if (!kernel_team)
		return 0;

	return kernel_team->id;
}


team_id
team_get_current_team_id(void)
{
	return thread_get_current_thread()->team->id;
}


static struct team *
create_team_struct(const char *name, bool kernel)
{
	struct team *p;

	p = (struct team *)malloc(sizeof(struct team));
	if (p == NULL)
		goto error;

	p->id = atomic_add(&next_team_id, 1);
	strncpy(&p->name[0], name, SYS_MAX_OS_NAME_LEN-1);
	p->name[SYS_MAX_OS_NAME_LEN-1] = 0;
	p->num_threads = 0;
	p->ioctx = NULL;
	p->path[0] = 0;
	p->_aspace_id = -1;
	p->aspace = NULL;
	p->kaspace = vm_get_kernel_aspace();
	vm_put_aspace(p->kaspace);
	p->thread_list = NULL;
	p->main_thread = NULL;
	p->state = TEAM_STATE_BIRTH;
	p->pending_signals = 0;
	p->death_sem = -1;
	p->user_env_base = 0;

	if (arch_team_init_team_struct(p, kernel) < 0)
		goto error1;

	return p;

error1:
	free(p);
error:
	return NULL;
}


static void
delete_team_struct(struct team *p)
{
	free(p);
}


static int
get_arguments_data_size(char **args,int argc)
{
	int cnt;
	int tot_size = 0;

	for(cnt = 0; cnt < argc; cnt++)
		tot_size += strlen(args[cnt]) + 1;
	tot_size += (argc + 1) * sizeof(char *);

	return tot_size + sizeof(struct uspace_prog_args_t);
}


static int
team_create_team2(void *args)
{
	int err;
	struct thread *t;
	struct team *p;
	struct team_arg *pargs = args;
	char *path;
	addr entry;
	char ustack_name[128];
	int tot_top_size;
	char **uargs;
	char **uenv;
	char *udest;
	struct uspace_prog_args_t *uspa;
	unsigned int arg_cnt;
	unsigned int env_cnt;

	t = thread_get_current_thread();
	p = t->team;

	dprintf("team_create_team2: entry thread %ld\n", t->id);

	// create an initial primary stack region

	tot_top_size = STACK_SIZE + ENV_SIZE + PAGE_ALIGN(get_arguments_data_size(pargs->args, pargs->argc));
	t->user_stack_base = ((USER_STACK_REGION - tot_top_size) + USER_STACK_REGION_SIZE);
	sprintf(ustack_name, "%s_primary_stack", p->name);
	t->user_stack_region_id = vm_create_anonymous_region(p->_aspace_id, ustack_name, (void **)&t->user_stack_base,
		REGION_ADDR_EXACT_ADDRESS, tot_top_size, REGION_WIRING_LAZY, LOCK_RW);
	if (t->user_stack_region_id < 0) {
		panic("team_create_team2: could not create default user stack region\n");
		return t->user_stack_region_id;
	}

	uspa  = (struct uspace_prog_args_t *)(t->user_stack_base + STACK_SIZE + ENV_SIZE);
	uargs = (char **)(uspa + 1);
	udest = (char  *)(uargs + pargs->argc + 1);
//	dprintf("addr: stack base=0x%x uargs = 0x%x  udest=0x%x tot_top_size=%d \n\n",t->user_stack_base,uargs,udest,tot_top_size);

	for(arg_cnt = 0; arg_cnt < pargs->argc; arg_cnt++) {
		uargs[arg_cnt] = udest;
		user_strcpy(udest, pargs->args[arg_cnt]);
		udest += (strlen(pargs->args[arg_cnt]) + 1);
	}
	uargs[arg_cnt] = NULL;

	p->user_env_base = t->user_stack_base + STACK_SIZE;
	uenv  = (char **)p->user_env_base;
	udest = (char *)p->user_env_base + ENV_SIZE - 1;
//	dprintf("team_create_team2: envc: %d, envp: 0x%p\n", pargs->envc, (void *)pargs->envp);
	for (env_cnt=0; env_cnt<pargs->envc; env_cnt++) {
		udest -= (strlen(pargs->envp[env_cnt]) + 1);
		uenv[env_cnt] = udest;
		user_strcpy(udest, pargs->envp[env_cnt]);
	}
	uenv[env_cnt] = NULL;

	user_memcpy(uspa->prog_name, p->name, sizeof(uspa->prog_name));
	user_memcpy(uspa->prog_path, pargs->path, sizeof(uspa->prog_path));
	uspa->argc = arg_cnt;
	uspa->argv = uargs;
	uspa->envc = env_cnt;
	uspa->envp = uenv;

	if (pargs->args != NULL)
		kfree_strings_array(pargs->args, pargs->argc);
	if (pargs->envp != NULL)
		kfree_strings_array(pargs->envp, pargs->envc);

	path = pargs->path;
	dprintf("team_create_team2: loading elf binary '%s'\n", path);

	err = elf_load_uspace("/boot/libexec/rld.so", p, 0, &entry);
	if(err < 0){
		// XXX clean up team
		return err;
	}

	// free the args
	free(pargs->path);
	free(pargs);

	dprintf("team_create_team2: loaded elf. entry = 0x%lx\n", entry);

	p->state = TEAM_STATE_NORMAL;

	// jump to the entry point in user space
	arch_thread_enter_uspace(entry, uspa, t->user_stack_base + STACK_SIZE);

	// never gets here
	return 0;
}


team_id
team_create_team(const char *path, const char *name, char **args, int argc, char **envp, int envc, int priority)
{
	struct team *p;
	thread_id tid;
	team_id pid;
	int err;
	unsigned int state;
//	int sem_retcode;
	struct team_arg *pargs;

	dprintf("team_create_team: entry '%s', name '%s' args = %p argc = %d\n", path, name, args, argc);

	p = create_team_struct(name, false);
	if (p == NULL)
		return ENOMEM;

	pid = p->id;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	hash_insert(team_hash, p);
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// copy the args over
	pargs = (struct team_arg *)malloc(sizeof(struct team_arg));
	if (pargs == NULL){
		err = ENOMEM;
		goto err1;
	}
	pargs->path = strdup(path);
	if (pargs->path == NULL){
		err = ENOMEM;
		goto err2;
	}
	pargs->argc = argc;
	pargs->args = args;
	pargs->envp = envp;
	pargs->envc = envc;

	// create a new ioctx for this team
	p->ioctx = vfs_new_io_context(thread_get_current_thread()->team->ioctx);
	if (!p->ioctx) {
		err = ENOMEM;
		goto err3;
	}

	//XXX should set p->path to path(?) here.

	// create an address space for this team
	p->_aspace_id = vm_create_aspace(p->name, USER_BASE, USER_SIZE, false);
	if (p->_aspace_id < 0) {
		err = p->_aspace_id;
		goto err4;
	}
	p->aspace = vm_get_aspace_by_id(p->_aspace_id);

	// create a kernel thread, but under the context of the new team
	tid = thread_create_kernel_thread_etc(name, team_create_team2, pargs, p);
	if (tid < 0) {
		err = tid;
		goto err5;
	}

	thread_resume_thread(tid);

	return pid;

err5:
	vm_put_aspace(p->aspace);
	vm_delete_aspace(p->_aspace_id);
err4:
	vfs_free_io_context(p->ioctx);
err3:
	free(pargs->path);
err2:
	free(pargs);
err1:
	// remove the team structure from the team hash table and delete the team structure
	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	hash_remove(team_hash, p);
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	delete_team_struct(p);
//err:
	return err;
}


team_id
user_team_create_team(const char *upath, const char *uname, char **args, int argc, char **envp, int envc, int priority)
{
	char path[SYS_MAX_PATH_LEN];
	char name[SYS_MAX_OS_NAME_LEN];
	char **kargs;
	char **kenv;
	int rc;

	dprintf("user_team_create_team : argc=%d \n",argc);

	if ((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if ((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_copy_strings_array(args, argc, &kargs);
	if (rc < 0)
		goto error;
	
	if (envp == NULL) {
		envp = (char **)thread_get_current_thread()->team->user_env_base;
		for (envc = 0; envp && (envp[envc]); envc++);
	}
	rc = user_copy_strings_array(envp, envc, &kenv);
	if (rc < 0)
		goto error;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if (rc < 0)
		goto error;

	path[SYS_MAX_PATH_LEN-1] = 0;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if (rc < 0)
		goto error;

	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	return team_create_team(path, name, kargs, argc, kenv, envc, priority);
error:
	kfree_strings_array(kargs, argc);
	kfree_strings_array(kenv, envc);
	return rc;
}


int
team_kill_team(team_id id)
{
	int state;
	struct team *p;
//	struct thread *t;
	thread_id tid = -1;
	int retval = 0;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	p = team_get_team_struct_locked(id);
	if(p != NULL) {
		tid = p->main_thread->id;
	} else {
		retval = ERR_INVALID_HANDLE;
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	if(retval < 0)
		return retval;

	// just kill the main thread in the team. The cleanup code there will
	// take care of the team
	return thread_kill_thread(tid);
}


status_t
user_get_team_info(team_id id, team_info *info)
{
	team_info kinfo;
	status_t rc = B_OK;
	status_t rc2;
	
	if ((addr)info >= KERNEL_BASE && (addr)info <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
		
	rc = _get_team_info(id, &kinfo, sizeof(team_info));
	if (rc != B_OK)
		return rc;
	
	rc2 = user_memcpy(info, &kinfo, sizeof(team_info));
	if (rc2 < 0)
		return rc2;
	
	return rc;
}


status_t
_get_team_info(team_id id, team_info *info, size_t size)
{
	int state;
	status_t rc = B_OK;
	struct team *team;
	
	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	
	team = team_get_team_struct_locked(id);
	if (!team) {
		rc = B_BAD_TEAM_ID;
		goto err;
	}
	// XXX- Set more informations for team_info
	memset(info, 0, sizeof(team_info));
	info->team = team->id;
	info->thread_count = team->num_threads;
	// XXX- make this to return real argc/argv
	strncpy(info->args, team->path, 64);
	info->args[63] = '\0';
	info->argc = 1;
err:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	
	return rc;
}


status_t
user_get_next_team_info(int32 *cookie, team_info *info)
{
	int32 kcookie;
	team_info kinfo;
	status_t rc = B_OK;
	status_t rc2;
	
	if ((addr)cookie >= KERNEL_BASE && (addr)cookie <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if ((addr)info >= KERNEL_BASE && (addr)info <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	
	rc2 = user_memcpy(&kcookie, cookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;
	
	rc = _get_next_team_info(&kcookie, &kinfo, sizeof(team_info));
	if (rc != B_OK)
		return rc;
	
	rc2 = user_memcpy(cookie, &kcookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;
	
	rc2 = user_memcpy(info, &kinfo, sizeof(team_info));
	if (rc2 < 0)
		return rc2;
	
	return rc;
}


status_t
_get_next_team_info(int32 *cookie, team_info *info, size_t size)
{
	int state;
	int slot;
	status_t rc = B_BAD_TEAM_ID;
	struct team *team = NULL;
	
	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	
	if (*cookie == 0)
		slot = 0;
	else {
		slot = *cookie;
		if (slot >= next_team_id)
			goto err;
	}
	while ((slot < next_team_id) && !(team = team_get_team_struct_locked(slot)))
		slot++;
	if (team) {
		memset(info, 0, sizeof(team_info));
		// XXX- Set more informations for team_info
		info->team = team->id;
		info->thread_count = team->num_threads;
		// XXX- make this to return real argc/argv
		strncpy(info->args, team->path, 64);
		info->args[63] = '\0';
		info->argc = 1;
		slot++;
		*cookie = slot;
		rc = B_OK;
	}
err:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	
	return rc;
}


int
user_setenv(const char *uname, const char *uvalue, int overwrite)
{
	char name[SYS_THREAD_STRING_LENGTH_MAX];
	char value[SYS_THREAD_STRING_LENGTH_MAX];
	int rc;

	if ((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if ((addr)uvalue >= KERNEL_BASE && (addr)uvalue <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_THREAD_STRING_LENGTH_MAX-1);
	if (rc < 0)
		return rc;

	name[SYS_THREAD_STRING_LENGTH_MAX-1] = 0;

	rc = user_strncpy(value, uvalue, SYS_THREAD_STRING_LENGTH_MAX-1);
	if(rc < 0)
		return rc;

	value[SYS_THREAD_STRING_LENGTH_MAX-1] = 0;

	return sys_setenv(name, value, overwrite);
}


int
sys_setenv(const char *name, const char *value, int overwrite)
{
	char var[SYS_THREAD_STRING_LENGTH_MAX];
	int state;
	addr env_space;
	char **envp;
	int envc;
	bool var_exists = false;
	int var_pos = 0;
	int name_size;
	int rc = 0;
	int i;
	char *p;
	
	dprintf("sys_setenv: entry (name=%s, value=%s)\n", name, value);
	
	if (strlen(name) + strlen(value) + 1 >= SYS_THREAD_STRING_LENGTH_MAX)
		return -1;
	
	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	
	strcpy(var, name);
	strncat(var, "=", SYS_THREAD_STRING_LENGTH_MAX-1);
	name_size = strlen(var);
	strncat(var, value, SYS_THREAD_STRING_LENGTH_MAX-1);
	
	env_space = (addr)thread_get_current_thread()->team->user_env_base;
	envp = (char **)env_space;
	for (envc=0; envp[envc]; envc++) {
		if (!strncmp(envp[envc], var, name_size)) {
			var_exists = true;
			var_pos = envc;
		}
	}
	if (!var_exists)
		var_pos = envc;
	dprintf("sys_setenv: variable does%s exist\n", var_exists ? "" : " not");
	if ((!var_exists) || (var_exists && overwrite)) {
		
		// XXX- make a better allocator
		if (var_exists) {
			if (strlen(var) <= strlen(envp[var_pos])) {
				strcpy(envp[var_pos], var);
			}
			else {
				for (p=(char *)env_space + ENV_SIZE - 1, i=0; envp[i]; i++)
					if (envp[i] < p)
						p = envp[i];
				p -= (strlen(var) + 1);
				if (p < (char *)env_space + (envc * sizeof(char *))) {
					rc = -1;
				}
				else {
					envp[var_pos] = p;
					strcpy(envp[var_pos], var);
				}
			}
		}
		else {
			for (p=(char *)env_space + ENV_SIZE - 1, i=0; envp[i]; i++)
				if (envp[i] < p)
					p = envp[i];
			p -= (strlen(var) + 1);
			if (p < (char *)env_space + ((envc + 1) * sizeof(char *))) {
				rc = -1;
			}
			else {
				envp[envc] = p;
				strcpy(envp[envc], var);
				envp[envc + 1] = NULL;
			}
		}
	}
	dprintf("sys_setenv: variable set.\n");

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	
	return rc;
}


int
user_getenv(const char *uname, char **uvalue)
{
	char name[SYS_THREAD_STRING_LENGTH_MAX];
	char *value;
	int rc;

	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if((addr)uvalue >= KERNEL_BASE && (addr)uvalue <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_THREAD_STRING_LENGTH_MAX-1);
	if (rc < 0)
		return rc;
	
	name[SYS_THREAD_STRING_LENGTH_MAX-1] = 0;
	
	rc = sys_getenv(name, &value);
	if (rc < 0)
		return rc;
	
	rc = user_memcpy(uvalue, &value, sizeof(char *));
	if (rc < 0)
		return rc;
	
	return 0;
}


int
sys_getenv(const char *name, char **value)
{
	char **envp;
	char *p;
	int state;
	int i;
	int len = strlen(name);
	int rc = -1;
	
	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	envp = (char **)thread_get_current_thread()->team->user_env_base;
	for (i=0; envp[i]; i++) {
		if (!strncmp(envp[i], name, len)) {
			p = envp[i] + len;
			if (*p == '=') {
				*value = (p + 1);
				rc = 0;
				break;
			}
		}
	}
	
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	
	return rc;
}


