/* Team functions */

/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>

#include <team.h>
#include <int.h>
#include <khash.h>
#include <port.h>
#include <sem.h>
#include <user_runtime.h>
#include <kimage.h>
#include <elf.h>
#include <syscalls.h>
#include <tls.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define TRACE_TEAM
#ifdef TRACE_TEAM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


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
static uint32 team_struct_hash(void *_p, const void *_key, uint32 range);
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
	dprintf("parent:      %p\n", p->parent);
	dprintf("children:    %p\n", p->children);
	dprintf("num_threads: %d\n", p->num_threads);
	dprintf("state:       %d\n", p->state);
	dprintf("pending_signals: 0x%x\n", p->pending_signals);
	dprintf("io_context:  %p\n", p->io_context);
//	dprintf("path:        '%s'\n", p->path);
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
		num = strtoul(argv[1], NULL, 16);
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

	kernel_team->io_context = vfs_new_io_context(NULL);
	if (kernel_team->io_context == NULL)
		panic("could not create io_context for kernel team!\n");

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
user_copy_strings_array(char **userStrings, int strc, char ***kstrings)
{
	char **lstrings;
	int err;
	int cnt;
	char *source;
	char buffer[SYS_THREAD_STRING_LENGTH_MAX];

	*kstrings = NULL;

	if (!IS_USER_ADDRESS(userStrings))
		return B_BAD_ADDRESS;

	lstrings = (char **)malloc((strc + 1) * sizeof(char *));
	if (lstrings == NULL)
		return B_NO_MEMORY;

	// scan all strings and copy to kernel space

	for (cnt = 0; cnt < strc; cnt++) {
		err = user_memcpy(&source, &(userStrings[cnt]), sizeof(char *));
		if (err < 0)
			goto error;

		if (!IS_USER_ADDRESS(source)) {
			err = B_BAD_ADDRESS;
			goto error;
		}

		err = user_strlcpy(buffer, source, SYS_THREAD_STRING_LENGTH_MAX);
		if (err < 0)
			goto error;

		lstrings[cnt] = strdup(buffer);
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


/** Quick check to see if we have a valid team ID.
 */

bool
team_is_valid(team_id id)
{
	struct team *team;
	int state;

	if (id <= 0)
		return false;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return team != NULL;
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

	if (p->id == key->id)
		return 0;

	return 1;
}


static uint32
team_struct_hash(void *_p, const void *_key, uint32 range)
{
	struct team *p = _p;
	const struct team_key *key = _key;

	if (p != NULL)
		return p->id % range;

	return key->id % range;
}


static void
insert_team_into_parent(struct team *parent, struct team *team)
{
	ASSERT(parent != NULL);

	team->siblings_next = parent->children;
	parent->children = team;
	team->parent = parent;
}


/**	Note: must have TEAM lock held
 */

static void
remove_team_from_parent(struct team *parent, struct team *team)
{
	struct team *child, *last = NULL;

	for (child = parent->children; child != NULL; child = child->siblings_next) {
		if (child == team) {
			if (last == NULL)
				parent->children = child->siblings_next;
			else
				last->siblings_next = child->siblings_next;

			team->parent = NULL;
			break;
		}
		last = child;
	}
}


/**	Reparent each of our children
 *	Note: must have TEAM lock held
 */

static void
reparent_children(struct team *team)
{
	struct team *child = team->children;

	while (child != NULL) {
		// remove the child from the current proc and add to the parent
		remove_team_from_parent(team, child);
		insert_team_into_parent(team->parent, child);

		child = team->children;
	}
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
	struct team *team = (struct team *)malloc(sizeof(struct team));
	if (team == NULL)
		return NULL;

	team->next = team->siblings_next = team->children = team->parent = NULL;
	team->id = atomic_add(&next_team_id, 1);
	strlcpy(team->name, name, B_OS_NAME_LENGTH);
	team->num_threads = 0;
	team->io_context = NULL;
	team->_aspace_id = -1;
	team->aspace = NULL;
	team->kaspace = vm_get_kernel_aspace();
	vm_put_aspace(team->kaspace);
	team->thread_list = NULL;
	team->main_thread = NULL;
	team->state = TEAM_STATE_BIRTH;
	team->pending_signals = 0;
	team->death_sem = -1;
	team->user_env_base = 0;
	list_init(&team->image_list);

	if (arch_team_init_team_struct(team, kernel) < 0)
		goto error;

	return team;

error:
	free(team);
	return NULL;
}


static void
delete_team_struct(struct team *team)
{
	free(team);
}


void
team_remove_team(struct team *team)
{
	hash_remove(team_hash, team);
	team->state = TEAM_STATE_DEATH;

	// reparent each of the team's children
	reparent_children(team);

	// remove us from our parent 
	remove_team_from_parent(team->parent, team);
}


void
team_delete_team(struct team *team)
{
	if (team->num_threads > 0) {
		// there are other threads still in this team,
		// cycle through and signal kill on each of the threads
		// XXX this can be optimized. There's got to be a better solution.
		cpu_status state;
		struct thread *temp_thread;
		char death_sem_name[B_OS_NAME_LENGTH];

		sprintf(death_sem_name, "team %ld death sem", team->id);
		team->death_sem = create_sem(0, death_sem_name);
		if (team->death_sem < 0)
			panic("thread_exit: cannot init death sem for team %ld\n", team->id);

		state = disable_interrupts();
		GRAB_TEAM_LOCK();
		// we can safely walk the list because of the lock. no new threads can be created
		// because of the TEAM_STATE_DEATH flag on the team
		temp_thread = team->thread_list;
		while (temp_thread) {
			struct thread *next = temp_thread->team_next;

			send_signal_etc(temp_thread->id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
			temp_thread = next;
		}
		RELEASE_TEAM_LOCK();
		restore_interrupts(state);

		// wait until all threads in team are dead.
		acquire_sem_etc(team->death_sem, team->num_threads, 0, 0);
		delete_sem(team->death_sem);
	}

	// free team resources
	vm_put_aspace(team->aspace);
	vm_delete_aspace(team->_aspace_id);
	delete_owned_ports(team->id);
	sem_delete_owned_sems(team->id);
	remove_images(team);
	vfs_free_io_context(team->io_context);

	free(team);
}


static int
get_arguments_data_size(char **args, int argc)
{
	uint32 size = 0;
	int count;

	for (count = 0; count < argc; count++)
		size += strlen(args[count]) + 1;

	return size + (argc + 1) * sizeof(char *) + sizeof(struct uspace_program_args);
}


static int32
team_create_team2(void *args)
{
	int err;
	struct thread *t;
	struct team *team;
	struct team_arg *teamArgs = args;
	char *path;
	addr entry;
	char ustack_name[128];
	uint32 totalSize;
	char **uargs;
	char **uenv;
	char *udest;
	struct uspace_program_args *uspa;
	unsigned int arg_cnt;
	unsigned int env_cnt;

	t = thread_get_current_thread();
	team = t->team;

	TRACE(("team_create_team2: entry thread %ld\n", t->id));

	// create an initial primary stack region

	// ToDo: make ENV_SIZE variable and put it on the heap?
	// ToDo: we could reserve the whole USER_STACK_REGION upfront...

	totalSize = PAGE_ALIGN(MAIN_THREAD_STACK_SIZE + TLS_SIZE + ENV_SIZE +
		get_arguments_data_size(teamArgs->args, teamArgs->argc));
	t->user_stack_base = USER_STACK_REGION + USER_STACK_REGION_SIZE - totalSize;
	t->user_stack_size = MAIN_THREAD_STACK_SIZE;
		// the exact location at the end of the user stack region

	sprintf(ustack_name, "%s_main_stack", team->name);
	t->user_stack_region_id = create_area_etc(team, ustack_name, (void **)&t->user_stack_base,
		B_EXACT_ADDRESS, totalSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (t->user_stack_region_id < 0) {
		dprintf("team_create_team2: could not create default user stack region\n");
		return t->user_stack_region_id;
	}

	// now that the TLS area is allocated, initialize TLS
	arch_thread_init_tls(t);

	uspa = (struct uspace_program_args *)(t->user_stack_base + STACK_SIZE + TLS_SIZE + ENV_SIZE);
	uargs = (char **)(uspa + 1);
	udest = (char  *)(uargs + teamArgs->argc + 1);

	TRACE(("addr: stack base = 0x%lx, uargs = %p, udest = %p, totalSize = %lu\n",
		t->user_stack_base, uargs, udest, totalSize));

	for (arg_cnt = 0; arg_cnt < teamArgs->argc; arg_cnt++) {
		uargs[arg_cnt] = udest;
		udest += user_strlcpy(udest, teamArgs->args[arg_cnt], totalSize) + 1;
	}
	uargs[arg_cnt] = NULL;

	team->user_env_base = t->user_stack_base + t->user_stack_size + TLS_SIZE;
	uenv = (char **)team->user_env_base;
	udest = (char *)team->user_env_base + ENV_SIZE - 1;

	TRACE(("team_create_team2: envc: %d, envp: 0x%p\n", teamArgs->envc, (void *)teamArgs->envp));

	for (env_cnt = 0; env_cnt < teamArgs->envc; env_cnt++) {
		size_t length = strlen(teamArgs->envp[env_cnt]) + 1;
		udest -= length;
		uenv[env_cnt] = udest;
		user_memcpy(udest, teamArgs->envp[env_cnt], length);
	}
	uenv[env_cnt] = NULL;

	user_memcpy(uspa->program_name, team->name, sizeof(uspa->program_name));
	user_memcpy(uspa->program_path, teamArgs->path, sizeof(uspa->program_path));
	uspa->argc = arg_cnt;
	uspa->argv = uargs;
	uspa->envc = env_cnt;
	uspa->envp = uenv;

	kfree_strings_array(teamArgs->args, teamArgs->argc);
	kfree_strings_array(teamArgs->envp, teamArgs->envc);

	path = teamArgs->path;
	TRACE(("team_create_team2: loading elf binary '%s'\n", path));

	// ToDo: don't use fixed paths!
	err = elf_load_user_image("/boot/beos/system/lib/rld.so", team, 0, &entry);

	// free the args
	free(teamArgs->path);
	free(teamArgs);

	if (err < 0) {
		// Luckily, we don't have to clean up the mess we created - that's
		// done for us by the normal team deletion process
		return err;
	}

	TRACE(("team_create_team2: loaded elf. entry = 0x%lx\n", entry));

	team->state = TEAM_STATE_NORMAL;

	// jump to the entry point in user space
	arch_thread_enter_uspace(t, entry, uspa, NULL);

	// never gets here
	return 0;
}


team_id
team_create_team(const char *path, const char *name, char **args, int argc, char **envp, int envc, int priority)
{
	struct team *team, *parent;
	const char *threadName;
	thread_id tid;
	team_id pid;
	int err;
	cpu_status state;
	struct team_arg *teamArgs;

	TRACE(("team_create_team: entry '%s', name '%s' args = %p argc = %d\n",
		path, name, args, argc));

	team = create_team_struct(name, false);
	if (team == NULL)
		return B_NO_MEMORY;

	pid = team->id;
	parent = thread_get_current_thread()->team;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	hash_insert(team_hash, team);
	insert_team_into_parent(parent, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// copy the args over
	teamArgs = (struct team_arg *)malloc(sizeof(struct team_arg));
	if (teamArgs == NULL){
		err = B_NO_MEMORY;
		goto err1;
	}
	teamArgs->path = strdup(path);
	if (teamArgs->path == NULL){
		err = B_NO_MEMORY;
		goto err2;
	}
	teamArgs->argc = argc;
	teamArgs->args = args;
	teamArgs->envp = envp;
	teamArgs->envc = envc;

	// create a new io_context for this team
	team->io_context = vfs_new_io_context(thread_get_current_thread()->team->io_context);
	if (!team->io_context) {
		err = B_NO_MEMORY;
		goto err3;
	}

	// create an address space for this team
	team->_aspace_id = vm_create_aspace(team->name, USER_BASE, USER_SIZE, false);
	if (team->_aspace_id < 0) {
		err = team->_aspace_id;
		goto err4;
	}
	team->aspace = vm_get_aspace_by_id(team->_aspace_id);

	// cut the path from the main thread name
	threadName = strrchr(name, '/');
	if (threadName != NULL)
		threadName++;
	else
		threadName = name;

	// create a kernel thread, but under the context of the new team
	tid = spawn_kernel_thread_etc(team_create_team2, threadName, B_NORMAL_PRIORITY, teamArgs, team->id);
	if (tid < 0) {
		err = tid;
		goto err5;
	}

	resume_thread(tid);

	return pid;

err5:
	vm_put_aspace(team->aspace);
	vm_delete_aspace(team->_aspace_id);
err4:
	vfs_free_io_context(team->io_context);
err3:
	free(teamArgs->path);
err2:
	free(teamArgs);
err1:
	// remove the team structure from the team hash table and delete the team structure
	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	remove_team_from_parent(parent, team);
	hash_remove(team_hash, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	delete_team_struct(team);
//err:
	return err;
}


/** This is the kernel backend for waitpid(). It is a bit more powerful when it comes
 *	to the reason why a thread has died than waitpid() can be.
 */


static thread_id
wait_for_child(thread_id child, uint32 flags, int32 *_reason, status_t *_returnCode)
{
	// ToDo: implement me! We need to store the death of children in the team structure!

	dprintf("wait_for_child(child = %ld, flags = %lu) is not yet implemented\n", child, flags);

	if (child > 0) {
		// wait for the specified child
	} else if (child == -1) {
		// wait for any children of this team to die
	} else if (child == 0) {
		// wait for any children of this process group to die
	} else {
		// wait for any children with progress group of the absolute value of "child"
	}

	return B_ERROR;
}


//	#pragma mark -
// public team API


status_t
wait_for_team(team_id id, status_t *_returnCode)
{
	struct team *team;
	thread_id thread;
	cpu_status state;

	// find main thread and wait for that

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team && team->main_thread)
		thread = team->main_thread->id;
	else
		thread = B_BAD_THREAD_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (thread < 0)
		return thread;

	return wait_for_thread(thread, _returnCode);
}


status_t
kill_team(team_id id)
{
	int state;
	struct team *team;
//	struct thread *t;
	thread_id tid = -1;
	int retval = 0;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team != NULL)
		tid = team->main_thread->id;
	else
		retval = B_BAD_THREAD_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (retval < 0)
		return retval;

	// just kill the main thread in the team. The cleanup code there will
	// take care of the team
	return kill_thread(tid);
}


/** Fills the team_info structure with information from the specified
 *	team.
 *	The team lock must be held when called.
 */

static status_t
fill_team_info(struct team *team, team_info *info, size_t size)
{
	if (size != sizeof(team_info))
		return B_BAD_VALUE;

	// ToDo: Set more informations for team_info
	memset(info, 0, size);

	info->team = team->id;
	info->thread_count = team->num_threads;
	info->image_count = count_images(team);
	//info->area_count = 
	//info->debugger_nub_thread = 
	//info->debugger_nub_port = 
	//info->argc = 
	//info->args[64] = 
	//info->uid = 
	//info->gid = 

	// ToDo: make this to return real argc/argv
	strlcpy(info->args, team->name, sizeof(info->args));
	info->argc = 1;

	return B_OK;
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

	rc = fill_team_info(team, info, size);

err:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);
	
	return rc;
}


status_t
_get_next_team_info(int32 *cookie, team_info *info, size_t size)
{
	status_t status = B_BAD_TEAM_ID;
	struct team *team = NULL;
	int32 slot = *cookie;

	int state = disable_interrupts();
	GRAB_TEAM_LOCK();

	if (slot >= next_team_id)
		goto err;

	// get next valid team
	while ((slot < next_team_id) && !(team = team_get_team_struct_locked(slot)))
		slot++;

	if (team) {
		status = fill_team_info(team, info, size);
		*cookie = ++slot;
	}

err:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
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

	// ToDo: please put me out of the kernel into libroot.so!

	TRACE(("sys_setenv: entry (name=%s, value=%s)\n", name, value));

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
	for (envc = 0; envp[envc]; envc++) {
		if (!strncmp(envp[envc], var, name_size)) {
			var_exists = true;
			var_pos = envc;
		}
	}
	if (!var_exists)
		var_pos = envc;

	TRACE(("sys_setenv: variable does%s exist\n", var_exists ? "" : " not"));

	if ((!var_exists) || (var_exists && overwrite)) {
		// XXX- make a better allocator
		if (var_exists) {
			if (strlen(var) <= strlen(envp[var_pos])) {
				strcpy(envp[var_pos], var);
			} else {
				for (p = (char *)env_space + ENV_SIZE - 1, i = 0; envp[i]; i++)
					if (envp[i] < p)
						p = envp[i];
				p -= (strlen(var) + 1);
				if (p < (char *)env_space + (envc * sizeof(char *))) {
					rc = -1;
				} else {
					envp[var_pos] = p;
					strcpy(envp[var_pos], var);
				}
			}
		}
		else {
			for (p = (char *)env_space + ENV_SIZE - 1, i=0; envp[i]; i++)
				if (envp[i] < p)
					p = envp[i];
			p -= (strlen(var) + 1);
			if (p < (char *)env_space + ((envc + 1) * sizeof(char *))) {
				rc = -1;
			} else {
				envp[envc] = p;
				strcpy(envp[envc], var);
				envp[envc + 1] = NULL;
			}
		}
	}
	TRACE(("sys_setenv: variable set.\n"));

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return rc;
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

	// ToDo: please put me out of the kernel into libroot.so!

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	envp = (char **)thread_get_current_thread()->team->user_env_base;
	for (i = 0; envp[i]; i++) {
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


//	#pragma mark -
//	User syscalls


status_t
_user_exec(const char *path, int32 argc, char * const *userArgv, int32 envCount, char * const *userEnvironment)
{
	int32 i;
	dprintf("exec(path = \"%s\", argc = %ld, envc = %ld) is not yet implemented\n", path, argc, envCount);
	for (i = 0; i < argc; i++) {
		char argv[B_FILE_NAME_LENGTH];
		user_strlcpy(argv, userArgv[i], sizeof(argv));

		dprintf("  [%ld] %s\n", i, argv);
	}
	for (i = 0; i < envCount; i++) {
		char env[B_FILE_NAME_LENGTH];
		user_strlcpy(env, userEnvironment[i], sizeof(env));

		dprintf("  (%ld) %s\n", i, env);
	}
	return B_ERROR;
}


thread_id
_user_fork(void)
{
	dprintf("fork() is not yet implemented!\n");
	return B_ERROR;
}


thread_id
_user_wait_for_child(thread_id child, uint32 flags, int32 *_userReason, status_t *_userReturnCode)
{
	status_t returnCode;
	int32 reason;
	thread_id deadChild;

	if ((_userReason != NULL && !IS_USER_ADDRESS(_userReason))
		|| (_userReturnCode != NULL && !IS_USER_ADDRESS(_userReturnCode)))
		return B_BAD_ADDRESS;

	deadChild = wait_for_child(child, flags, &reason, &returnCode);

	if (deadChild >= B_OK) {
		// copy result data on successful completion
		if ((_userReason != NULL && user_memcpy(_userReason, &reason, sizeof(int32)) < B_OK)
			|| (_userReturnCode != NULL && user_memcpy(_userReturnCode, &returnCode, sizeof(status_t)) < B_OK))
			return B_BAD_ADDRESS;
	}

	return deadChild;
}


status_t
_user_wait_for_team(team_id id, status_t *_userReturnCode)
{
	status_t returnCode;
	status_t status;

	if (_userReturnCode != NULL && !IS_USER_ADDRESS(_userReturnCode))
		return B_BAD_ADDRESS;

	status = wait_for_team(id, &returnCode);
	if (status >= B_OK && _userReturnCode != NULL) {
		if (user_memcpy(_userReturnCode, &returnCode, sizeof(returnCode)) < B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


team_id
_user_create_team(const char *userPath, const char *userName, char **userArgs,
	int argCount, char **userEnv, int envCount, int priority)
{
	char path[SYS_MAX_PATH_LEN];
	char name[B_OS_NAME_LENGTH];
	char **args = NULL;
	char **env = NULL;
	int rc;

	TRACE(("user_team_create_team: argc = %d\n", argCount));

	if (!IS_USER_ADDRESS(userPath)
		|| !IS_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	rc = user_copy_strings_array(userArgs, argCount, &args);
	if (rc < 0)
		goto error;

	if (userEnv == NULL) {
		// ToDo: this doesn't look particularly safe to me - where
		//	is user_env_base?
		userEnv = (char **)thread_get_current_thread()->team->user_env_base;
		for (envCount = 0; userEnv && (userEnv[envCount]); envCount++);
	}
	if (user_copy_strings_array(userEnv, envCount, &env) < B_OK
		|| user_strlcpy(path, userPath, SYS_MAX_PATH_LEN) < B_OK
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK) {
		rc = B_BAD_ADDRESS;
		goto error;
	}

	return team_create_team(path, name, args, argCount, env, envCount, priority);

error:
	kfree_strings_array(args, argCount);
	kfree_strings_array(env, envCount);
	return rc;
}


status_t
_user_kill_team(team_id team)
{
	return kill_team(team);
}


status_t
_user_get_team_info(team_id id, team_info *userInfo)
{
	status_t status;
	team_info info;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;	

	status = _get_team_info(id, &info, sizeof(team_info));
	if (status == B_OK) {
		if (user_memcpy(userInfo, &info, sizeof(team_info)) < B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


status_t
_user_get_next_team_info(int32 *userCookie, team_info *userInfo)
{
	status_t status;
	team_info info;
	int32 cookie;

	if (!IS_USER_ADDRESS(userCookie)
		|| !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = _get_next_team_info(&cookie, &info, sizeof(team_info));
	if (status != B_OK)
		return status;

	if (user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK
		|| user_memcpy(userInfo, &info, sizeof(team_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


team_id
_user_get_current_team(void)
{
	return team_get_current_team_id();
}


int
_user_getenv(const char *userName, char **_userValue)
{
	char name[SYS_THREAD_STRING_LENGTH_MAX];
	char *value;
	int rc;

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(_userValue)
		|| user_strlcpy(name, userName, SYS_THREAD_STRING_LENGTH_MAX) < B_OK)
		return B_BAD_ADDRESS;

	rc = sys_getenv(name, &value);
	if (rc < 0)
		return rc;

	if (user_memcpy(_userValue, &value, sizeof(char *)) < B_OK)
		return B_BAD_ADDRESS;

	return rc;
}


int
_user_setenv(const char *userName, const char *userValue, int overwrite)
{
	char name[SYS_THREAD_STRING_LENGTH_MAX];
	char value[SYS_THREAD_STRING_LENGTH_MAX];

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(userValue)
		|| user_strlcpy(name, userName, SYS_THREAD_STRING_LENGTH_MAX) < B_OK
		|| user_strlcpy(value, userValue, SYS_THREAD_STRING_LENGTH_MAX) < B_OK)
		return B_BAD_ADDRESS;

	return sys_setenv(name, value, overwrite);
}

