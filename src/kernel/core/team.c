/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/* Team functions */

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
#include <syscall_process_info.h>
#include <tls.h>

#include <sys/wait.h>
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
	uint32	arg_count;
	char	**args;
	uint32	env_count;
	char	**env;
};

struct fork_arg {
	area_id	user_stack_area;
	addr_t	user_stack_base;
	size_t	user_stack_size;
	addr_t	user_local_storage;

	struct arch_fork_arg arch_info;
};


// team list
static void *team_hash = NULL;
static team_id next_team_id = 1;
static struct team *kernel_team = NULL;

spinlock team_spinlock = 0;

static void insert_group_into_session(struct process_session *session, struct process_group *group);
static void insert_team_into_group(struct process_group *group, struct team *team);
static struct process_session *create_process_session(pid_t id);
static struct process_group *create_process_group(pid_t id);
static struct team *create_team_struct(const char *name, bool kernel);
static void delete_team_struct(struct team *p);
static int team_struct_compare(void *_p, const void *_key);
static uint32 team_struct_hash(void *_p, const void *_key, uint32 range);
static void free_strings_array(char **strings, int32 count);
static status_t user_copy_strings_array(char * const *strings, int32 count, char ***_strings);
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
	dprintf("aspace:      %p (id = %ld)\n", p->aspace, p->aspace->id);
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
	struct process_session *session;
	struct process_group *group;

	// create the team hash table
	team_hash = hash_init(15, (addr)&kernel_team->next - (addr)kernel_team,
		&team_struct_compare, &team_struct_hash);

	// create initial session and process groups

	session = create_process_session(1);
	if (session == NULL)
		panic("Could not create initial session.\n");

	group = create_process_group(1);
	if (group == NULL)
		panic("Could not create initial process group.\n");

	insert_group_into_session(session, group);

	// create the kernel team
	kernel_team = create_team_struct("kernel_team", true);
	if (kernel_team == NULL)
		panic("could not create kernel team!\n");
	kernel_team->state = TEAM_STATE_NORMAL;

	insert_team_into_group(group, kernel_team);

	kernel_team->io_context = vfs_new_io_context(NULL);
	if (kernel_team->io_context == NULL)
		panic("could not create io_context for kernel team!\n");

	// stick it in the team hash
	hash_insert(team_hash, kernel_team);

	add_debugger_command("team", &dump_team_info, "list info about a particular team");
	return 0;
}


/**	Frees an array of strings in kernel space.
 *
 *	\param strings strings array
 *	\param count number of strings in array
 */

static void
free_strings_array(char **strings, int32 count)
{
	int32 i;

	if (strings == NULL)
		return;

	for (i = 0; i < count; i++)
		free(strings[i]);

    free(strings);
}


/**	Copy an array of strings in kernel space
 *
 *	\param strings strings array to be copied
 *	\param count number of strings in array
 *	\param kstrings	pointer to the kernel copy
 *	\return \c B_OK on success, or an appropriate error code on
 *		failure.
 */

static status_t
copy_strings_array(const char **in, int32 count, char ***_strings)
{
	status_t status;
	char **strings;
	int32 i = 0;

	strings = (char **)malloc((count + 1) * sizeof(char *));
	if (strings == NULL)
		return B_NO_MEMORY;

	for (; i < count; i++) {
		strings[i] = strdup(in[i]);
		if (strings[i] == NULL) {
			status = B_NO_MEMORY;
			goto error;
		}
	}

	strings[count] = NULL;
	*_strings = strings;

	return B_OK;

error:
	free_strings_array(strings, i);
	return status;
}


/**	Copy an array of strings from user space to kernel space
 *
 *	\param strings userspace strings array
 *	\param count number of strings in array
 *	\param kstrings	pointer to the kernel copy
 *	\return \c B_OK on success, or an appropriate error code on
 *		failure.
 */

static status_t
user_copy_strings_array(char * const *userStrings, int32 count, char ***_strings)
{
	char buffer[SYS_THREAD_STRING_LENGTH_MAX];
	char **strings;
	status_t err;
	int32 i = 0;

	if (!IS_USER_ADDRESS(userStrings))
		return B_BAD_ADDRESS;

	strings = (char **)malloc((count + 1) * sizeof(char *));
	if (strings == NULL)
		return B_NO_MEMORY;

	if ((err = user_memcpy(strings, userStrings, count * sizeof(char *))) < B_OK)
		goto error;

	// scan all strings and copy to kernel space

	for (; i < count; i++) {
		err = user_strlcpy(buffer, strings[i], SYS_THREAD_STRING_LENGTH_MAX);
		if (err < B_OK)
			goto error;

		strings[i] = strdup(buffer);
		if (strings[i] == NULL) {
			err = B_NO_MEMORY;
			goto error;
		}
	}

	strings[count] = NULL;
	*_strings = strings;

	return B_OK;

error:
	free_strings_array(strings, i);

	TRACE(("user_copy_strings_array failed %d \n", err));
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


static bool
is_process_group_leader(struct team *team)
{
	return team->group_id == team->main_thread->id;
}


static void
insert_group_into_session(struct process_session *session, struct process_group *group)
{
	if (group == NULL)
		return;

	group->session = session;
	list_add_link_to_tail(&session->groups, group);
}


static void
insert_team_into_group(struct process_group *group, struct team *team)
{
	team->group = group;
	team->group_id = group->id;
	team->session_id = group->session->id;
	atomic_add(&team->dead_children.wait_for_any, group->wait_for_any);

	team->group_next = group->teams;
	group->teams = team;
}


/** Removes a group from a session, and puts the session object
 *	back into the session cache, if it's not used anymore.
 *	You must hold the team lock when calling this function.
 */

static void
remove_group_from_session(struct process_group *group)
{
	struct process_session *session = group->session;

	// the group must be in any session to let this function have any effect
	if (session == NULL)
		return;

	list_remove_link(group);

	// we cannot free the resource here, so we're keeping the group link
	// around - this way it'll be freed by free_process_group()
	if (!list_is_empty(&session->groups))
		group->session = NULL;
}


void
team_delete_process_group(struct process_group *group)
{
	if (group == NULL)
		return;

	TRACE(("team_delete_process_group(id = %ld)\n", group->id));

	delete_sem(group->dead_child_sem);

	// remove_group_from_session() keeps this pointer around
	// only if the session can be freed as well
	if (group->session) {
		TRACE(("team_delete_process_group(): frees session %ld\n", group->session->id));
		free(group->session);
	}

	free(group);
}


/**	Removes the team from the group. If that group becomes therefore
 *	unused, it will set \a _freeGroup to point to the group - otherwise
 *	it will be \c NULL.
 *	It cannot be freed here because this function has to be called
 *	with having the team lock held.
 *
 *	\param team the team that'll be removed from it's group
 *	\param _freeGroup points to the group to be freed or NULL
 */

static void
remove_team_from_group(struct team *team, struct process_group **_freeGroup)
{
	struct process_group *group = team->group;
	struct team *current, *last = NULL;

	*_freeGroup = NULL;

	// the team must be in any team to let this function have any effect
	if  (group == NULL)
		return;

	for (current = group->teams; current != NULL; current = current->group_next) {
		if (current == team) {
			if (last == NULL)
				group->teams = current->group_next;
			else
				last->group_next = current->group_next;

			team->group = NULL;
			break;
		}
		last = current;
	}

	// all wait_for_child() that wait on the group don't wait for us anymore
	atomic_add(&team->dead_children.wait_for_any, -group->wait_for_any);

	team->group = NULL;
	team->group_next = NULL;

	if (group->teams != NULL)
		return;

	// we can remove this group as it is no longer used

	remove_group_from_session(group);
	*_freeGroup = group;
}


static struct process_group *
create_process_group(pid_t id)
{
	struct process_group *group = (struct process_group *)malloc(sizeof(struct process_group));
	if (group == NULL)
		return NULL;

	group->dead_child_sem = create_sem(0, "dead group children");
	if (group->dead_child_sem < B_OK) {
		free(group);
		return NULL;
	}

	group->id = id;
	group->session = NULL;
	group->teams = NULL;
	group->wait_for_any = 0;

	return group;
}


static struct process_session *
create_process_session(pid_t id)
{
	struct process_session *session = (struct process_session *)malloc(sizeof(struct process_session));
	if (session == NULL)
		return NULL;

	session->id = id;
	list_init(&session->groups);

	return session;
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


status_t
team_get_address_space(team_id id, vm_address_space **_addressSpace)
{
	cpu_status state;
	struct team *team;
	status_t status;

	// ToDo: we need to do something about B_SYSTEM_TEAM vs. its real ID (1)
	if (id == 1) {
		// we're the kernel team, so we don't have to go through all
		// the hassle (locking and hash lookup)
		atomic_add(&kernel_team->kaspace->ref_count, 1);
		*_addressSpace = kernel_team->kaspace;
		return B_OK;
	}

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team != NULL) {
		atomic_add(&team->aspace->ref_count, 1);
		*_addressSpace = team->aspace;
		status = B_OK;
	} else
		status = B_BAD_VALUE;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
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
	team->aspace = NULL;
	team->kaspace = vm_get_kernel_aspace();
	vm_put_aspace(team->kaspace);
	team->thread_list = NULL;
	team->main_thread = NULL;
	team->state = TEAM_STATE_BIRTH;
	team->pending_signals = 0;
	team->death_sem = -1;
	team->user_env_base = 0;

	list_init(&team->dead_children.list);
	team->dead_children.count = 0;
	team->dead_children.wait_for_any = 0;
	team->dead_children.sem = create_sem(0, "dead children");
	if (team->dead_children.sem < B_OK)
		goto error1;

	list_init(&team->image_list);

	if (arch_team_init_team_struct(team, kernel) < 0)
		goto error2;

	return team;

error2:
	delete_sem(team->dead_children.sem);
error1:
	free(team);
	return NULL;
}


static void
delete_team_struct(struct team *team)
{
	struct death_entry *death = NULL;

	delete_sem(team->dead_children.sem);

	while ((death = list_get_next_item(&team->dead_children.list, death)) != NULL)
		free(death);

	free(team);
}


/**	Removes the specified team from the global team hash, and from its parent.
 *	It also moves all of its children up to the parent.
 *	You must hold the team lock when you call this function.
 *	If \a _freeGroup is set to a value other than \c NULL, it must be freed
 *	from the calling function.
 */

void
team_remove_team(struct team *team, struct process_group **_freeGroup)
{
	hash_remove(team_hash, team);
	team->state = TEAM_STATE_DEATH;

	// reparent each of the team's children
	reparent_children(team);

	// remove us from our process group
	remove_team_from_group(team, _freeGroup);

	// remove us from our parent 
	remove_team_from_parent(team->parent, team);
}


void
team_delete_team(struct team *team)
{
	if (team->num_threads > 0) {
		// there are other threads still in this team,
		// cycle through and signal kill on each of the threads
		// ToDo: this can be optimized. There's got to be a better solution.
		struct thread *temp_thread;
		char death_sem_name[B_OS_NAME_LENGTH];
		cpu_status state;

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

	vm_delete_aspace(team->aspace);
	delete_owned_ports(team->id);
	sem_delete_owned_sems(team->id);
	remove_images(team);
	vfs_free_io_context(team->io_context);

	free(team);
}


static uint32
get_arguments_data_size(char **args, int32 argc)
{
	uint32 size = 0;
	int32 count;

	for (count = 0; count < argc; count++)
		size += strlen(args[count]) + 1;

	return size + (argc + 1) * sizeof(char *) + sizeof(struct uspace_program_args);
}


static void
free_team_arg(struct team_arg *teamArg)
{
	free_strings_array(teamArg->args, teamArg->arg_count);
	free_strings_array(teamArg->env, teamArg->env_count);

	free(teamArg);
}


static struct team_arg *
create_team_arg(int32 argc, char **args, int32 envCount, char **env)
{
	struct team_arg *teamArg = (struct team_arg *)malloc(sizeof(struct team_arg));
	if (teamArg == NULL)
		return NULL;

	teamArg->arg_count = argc;
	teamArg->args = args;
	teamArg->env_count = envCount;
	teamArg->env = env;

	return teamArg;
}


static int32
team_create_thread_start(void *args)
{
	int err;
	struct thread *t;
	struct team *team;
	struct team_arg *teamArgs = args;
	const char *path;
	addr entry;
	char ustack_name[128];
	uint32 sizeLeft;
	char **uargs;
	char **uenv;
	char *udest;
	struct uspace_program_args *uspa;
	uint32 argCount, envCount, i;

	t = thread_get_current_thread();
	team = t->team;

	TRACE(("team_create_thread_start: entry thread %ld\n", t->id));

	// create an initial primary stack region

	// ToDo: make ENV_SIZE variable and put it on the heap?
	// ToDo: we could reserve the whole USER_STACK_REGION upfront...

	sizeLeft = PAGE_ALIGN(MAIN_THREAD_STACK_SIZE + TLS_SIZE + ENV_SIZE +
		get_arguments_data_size(teamArgs->args, teamArgs->arg_count));
	t->user_stack_base = USER_STACK_REGION + USER_STACK_REGION_SIZE - sizeLeft;
	t->user_stack_size = MAIN_THREAD_STACK_SIZE;
		// the exact location at the end of the user stack region

	sprintf(ustack_name, "%s_main_stack", team->name);
	t->user_stack_region_id = create_area_etc(team, ustack_name, (void **)&t->user_stack_base,
		B_EXACT_ADDRESS, sizeLeft, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (t->user_stack_region_id < 0) {
		dprintf("team_create_thread_start: could not create default user stack region\n");
		return t->user_stack_region_id;
	}

	// now that the TLS area is allocated, initialize TLS
	arch_thread_init_tls(t);

	argCount = teamArgs->arg_count;
	envCount = teamArgs->env_count;

	uspa = (struct uspace_program_args *)(t->user_stack_base + STACK_SIZE + TLS_SIZE + ENV_SIZE);
	uargs = (char **)(uspa + 1);
	udest = (char  *)(uargs + argCount + 1);

	TRACE(("addr: stack base = 0x%lx, uargs = %p, udest = %p, sizeLeft = %lu\n",
		t->user_stack_base, uargs, udest, sizeLeft));

	for (i = 0; i < argCount; i++) {
		ssize_t length = user_strlcpy(udest, teamArgs->args[i], sizeLeft) + 1;

		uargs[i] = udest;
		udest += length;
		sizeLeft -= length;
	}
	uargs[argCount] = NULL;

	team->user_env_base = t->user_stack_base + t->user_stack_size + TLS_SIZE;
	uenv = (char **)team->user_env_base;
	udest = (char *)team->user_env_base + ENV_SIZE - 1;
	path = teamArgs->args[0];

	TRACE(("team_create_thread_start: envc: %d, envp: 0x%p\n", teamArgs->env_count, (void *)teamArgs->env));

	for (i = 0; i < envCount; i++) {
		ssize_t length = user_strlcpy(udest, teamArgs->env[i], sizeLeft) + 1;

		uenv[i] = udest;
		udest += length;
		sizeLeft -= length;
	}
	uenv[envCount] = NULL;

	user_memcpy(uspa->program_path, path, sizeof(uspa->program_path));
	uspa->argc = argCount;
	uspa->argv = uargs;
	uspa->envc = envCount;
	uspa->envp = uenv;

	TRACE(("team_create_thread_start: loading elf binary '%s'\n", path));

	free_team_arg(teamArgs);

	// ToDo: don't use fixed paths!
	err = elf_load_user_image("/boot/beos/system/lib/rld.so", team, 0, &entry);
	if (err < 0) {
		// Luckily, we don't have to clean up the mess we created - that's
		// done for us by the normal team deletion process
		return err;
	}

	TRACE(("team_create_thread_start: loaded elf. entry = 0x%lx\n", entry));

	team->state = TEAM_STATE_NORMAL;

	// jump to the entry point in user space
	arch_thread_enter_uspace(t, entry, uspa, NULL);

	// never gets here
	return 0;
}


/** The BeOS kernel exports a function with this name, but most probably with
 *	different parameters; we should not make it public.
 */

static thread_id
load_image_etc(int32 argCount, char **args, int32 envCount, char **env, int32 priority)
{
	struct process_group *group;
	struct team *team, *parent;
	const char *threadName;
	thread_id thread;
	int err;
	cpu_status state;
	struct team_arg *teamArgs;

	if (args == NULL || argCount == 0)
		return B_BAD_VALUE;

	TRACE(("load_image_etc: name '%s', args = %p, argCount = %ld\n",
		args[0], args, argCount));

	team = create_team_struct(args[0], false);
	if (team == NULL)
		return B_NO_MEMORY;

	parent = thread_get_current_thread()->team;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	hash_insert(team_hash, team);
	insert_team_into_parent(parent, team);
	insert_team_into_group(parent->group, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// copy the args over
	teamArgs = create_team_arg(argCount, args, envCount, env);
	if (teamArgs == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}

	// create a new io_context for this team
	team->io_context = vfs_new_io_context(parent->io_context);
	if (!team->io_context) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create an address space for this team
	err = vm_create_aspace(team->name, USER_BASE, USER_SIZE, false, &team->aspace);
	if (err < B_OK)
		goto err3;

	// cut the path from the main thread name
	threadName = strrchr(args[0], '/');
	if (threadName != NULL)
		threadName++;
	else
		threadName = args[0];

	// create a kernel thread, but under the context of the new team
	thread = spawn_kernel_thread_etc(team_create_thread_start, threadName, B_NORMAL_PRIORITY, teamArgs, team->id);
	if (thread < 0) {
		err = thread;
		goto err4;
	}

	return thread;

err4:
	vm_put_aspace(team->aspace);
err3:
	vfs_free_io_context(team->io_context);
err2:
	free_team_arg(teamArgs);
err1:
	// remove the team structure from the team hash table and delete the team structure
	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	remove_team_from_group(team, &group);
	remove_team_from_parent(parent, team);
	hash_remove(team_hash, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	team_delete_process_group(group);
	delete_team_struct(team);

	return err;
}


/**	Almost shuts down the current team and loads a new image into it.
 *	If successful, this function does not return and will takeover ownership of
 *	the arguments provided.
 */

static status_t
exec_team(int32 argCount, char **args, int32 envCount, char **env)
{
	struct team *team = thread_get_current_thread()->team;
	struct team_arg *teamArgs;
	status_t status;

	TRACE(("exec_team(path = \"%s\", argc = %ld, envCount = %ld)\n", args[0], argCount, envCount));

	// switching the kernel at run time is probably not a good idea :)
	if (team == team_get_kernel_team())
		return B_NOT_ALLOWED;

	// we currently need to be single threaded here
	// ToDo: maybe we should just kill all other threads and
	//	make the current thread the team's main thread?
	if (team->main_thread != thread_get_current_thread()
		|| team->main_thread != team->thread_list
		|| team->main_thread->team_next != NULL)
		return B_NOT_ALLOWED;

	// ToDo: maybe we should make sure upfront that the target path is an app?

	teamArgs = create_team_arg(argCount, args, envCount, env);
	if (teamArgs == NULL)
		return B_NO_MEMORY;

	// ToDo: remove team resources if there are any left
	// alarm, signals
	// thread_atkernel_exit() might not be called at all

	vm_delete_areas(team->aspace);
	delete_owned_ports(team->id);
	sem_delete_owned_sems(team->id);
	remove_images(team);
	vfs_exec_io_context(team->io_context);

	status = team_create_thread_start(teamArgs);
		// this one usually doesn't return...

	// sorry, we have to kill us, there is no way out anymore (without any areas left and all that)
	exit_thread(status);
	
	// we'll never make it here
	return B_ERROR;
}


/** This is the first function to be called from the newly created
 *	main child thread.
 *	It will fill in everything what's left to do from fork_arg, and
 *	return from the parent's fork() syscall to the child.
 */

static int32
fork_team_thread_start(void *_args)
{
	struct thread *thread = thread_get_current_thread();
	struct fork_arg *forkArgs = (struct fork_arg *)_args;

	struct arch_fork_arg archArgs = forkArgs->arch_info;
		// we need a local copy of the arch dependent part

	thread->user_stack_region_id = forkArgs->user_stack_area;
	thread->user_stack_base = forkArgs->user_stack_base;
	thread->user_stack_size = forkArgs->user_stack_size;
	thread->user_local_storage = forkArgs->user_local_storage;

	arch_thread_init_tls(thread);

	free(forkArgs);

	// set frame of the parent thread to this one, too

	arch_restore_fork_frame(&archArgs);
		// This one won't return here

	return 0;
}


static thread_id
fork_team(void)
{
	struct team *parentTeam = thread_get_current_thread()->team, *team;
	struct thread *parentThread = thread_get_current_thread();
	struct process_group *group = NULL;
	struct fork_arg *forkArgs;
	struct area_info info;
	thread_id threadID;
	cpu_status state;
	status_t status;
	int32 cookie;

	TRACE(("fork_team()\n"));

	if (parentTeam == team_get_kernel_team())
		return B_NOT_ALLOWED;

	// create a new team
	// ToDo: this is very similar to team_create_team() - maybe we can do something about it :)

	team = create_team_struct(parentTeam->name, false);
	if (team == NULL)
		return B_NO_MEMORY;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	hash_insert(team_hash, team);
	insert_team_into_parent(parentTeam, team);
	insert_team_into_group(parentTeam->group, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	forkArgs = (struct fork_arg *)malloc(sizeof(struct fork_arg));
	if (forkArgs == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	// create a new io_context for this team
	team->io_context = vfs_new_io_context(parentTeam->io_context);
	if (!team->io_context) {
		status = B_NO_MEMORY;
		goto err2;
	}

	// create an address space for this team
	status = vm_create_aspace(team->name, USER_BASE, USER_SIZE, false, &team->aspace);
	if (status < B_OK)
		goto err3;

	// copy all areas of the team
	// ToDo: should be able to handle stack areas differently (ie. don't have them copy-on-write)
	// ToDo: all stacks of other threads than the current one could be left out

	cookie = 0;
	while (get_next_area_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		void *address;
		area_id area = vm_copy_area(team->aspace->id, info.name, &address, B_CLONE_ADDRESS,
							info.protection, info.area);
		if (area < B_OK) {
			status = area;
			break;
		}

		if (info.area == parentThread->user_stack_region_id)
			forkArgs->user_stack_area = area;
	}

	if (status < B_OK)
		goto err4;

	forkArgs->user_stack_base = parentThread->user_stack_base;
	forkArgs->user_stack_size = parentThread->user_stack_size;
	forkArgs->user_local_storage = parentThread->user_local_storage;
	arch_store_fork_frame(&forkArgs->arch_info);

	team->user_env_base = parentTeam->user_env_base;
	// ToDo: copy image list

	// create a kernel thread under the context of the new team
	threadID = spawn_kernel_thread_etc(fork_team_thread_start, parentThread->name,
					parentThread->priority, forkArgs, team->id);
	if (threadID < 0) {
		status = threadID;
		goto err4;
	}

	resume_thread(threadID);
	return threadID;

err4:
	vm_delete_aspace(team->aspace);
err3:
	vfs_free_io_context(team->io_context);
err2:
	free(forkArgs);
err1:
	// remove the team structure from the team hash table and delete the team structure
	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	remove_team_from_group(team, &group);
	remove_team_from_parent(parentTeam, team);
	hash_remove(team_hash, team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	team_delete_process_group(group);
	delete_team_struct(team);

	return status;
}


/** This searches the session of the team for the specified group ID.
 *	You must hold the team lock when you call this function.
 */

static struct process_group *
get_process_group_locked(struct team *team, pid_t id)
{
	struct list *groups = &team->group->session->groups;
	struct process_group *group = NULL;

	// a short cut when the current team's group is asked for
	if (team->group->id == id)
		return team->group;

	while ((group = list_get_next_item(groups, group)) != NULL) {
		if (group->id == id)
			return group;
	}

	return NULL;
}


static status_t
register_wait_for_any(struct team *team, thread_id child)
{
	struct process_group *group;
	cpu_status state;

	if (child > 0)
		return B_OK;

	if (child == -1) {
		// we only wait for children of the current team
		atomic_add(&team->dead_children.wait_for_any, 1);
		return B_OK;
	}

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	if (child < 0) {
		// we wait for all children of the specified process group
		group = get_process_group_locked(team, -child);
	} else {
		// we wait for any children of the current team's group
		group = team->group;
	}

	if (group != NULL) {
		for (team = group->teams; team; team = team->group_next) {
			atomic_add(&team->dead_children.wait_for_any, 1);
		}

		atomic_add(&group->wait_for_any, 1);
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return group != NULL ? B_OK : B_BAD_THREAD_ID;
}


static status_t
get_team_death_entry(struct team *team, thread_id child, struct death_entry *death,
	struct death_entry **_freeDeath)
{
	struct death_entry *entry = NULL;

	// find matching death entry structure

	while ((entry = list_get_next_item(&team->dead_children.list, entry)) != NULL) {
		if (child != -1 && entry->thread != child)
			continue;

		// we found one

		*death = *entry;

		// only remove the death entry if there aren't any other interested parties
		if ((child == -1 && atomic_add(&team->dead_children.wait_for_any, -1) == 1)
			|| (child != -1 && team->dead_children.wait_for_any == 0)) {
			list_remove_link(entry);
			team->dead_children.count--;
			*_freeDeath = entry;
		}

		return B_OK;
	}

	return child > 0 ? B_BAD_THREAD_ID : B_WOULD_BLOCK;
}


static status_t
get_death_entry(struct team *team, pid_t child, struct death_entry *death,
	struct death_entry **_freeDeath)
{
	struct process_group *group;
	status_t status;

	if (child == -1 || child > 0) {
		// wait for any children or a specific child of this team to die
		return get_team_death_entry(team, child, death, _freeDeath);
	} else if (child < 0) {
		// we wait for all children of the specified process group
		group = get_process_group_locked(team, -child);
		if (group == NULL)
			return B_BAD_THREAD_ID;
	} else {
		// we wait for any children of the current team's group
		group = team->group;
	}

	for (team = group->teams; team; team = team->group_next) {
		status = get_team_death_entry(team, -1, death, _freeDeath);
		if (status == B_OK) {
			atomic_add(&group->wait_for_any, -1);
			return B_OK;
		}
	}

	return B_WOULD_BLOCK;
}


/** This is the kernel backend for waitpid(). It is a bit more powerful when it comes
 *	to the reason why a thread has died than waitpid() can be.
 */


static thread_id
wait_for_child(thread_id child, uint32 flags, int32 *_reason, status_t *_returnCode)
{
	struct team *team = thread_get_current_thread()->team;
	struct death_entry death, *freeDeath = NULL;
	status_t status = B_OK;
	cpu_status state;

	TRACE(("wait_for_child(child = %ld, flags = %ld)\n", child, flags));

	if (child == 0 || child < -1) {
		dprintf("wait_for_child() process group ID waiting not yet implemented!\n");
		return EOPNOTSUPP;
	}

	if (child <= 0) {
		// we need to make sure the death entries won't get deleted too soon
		status = register_wait_for_any(team, child);
		if (status != B_OK)
			return status;
	}

	while (true) {
		if (child > 0) {
			// wait for the specified child

			if (thread_get_thread_struct(child) != NULL) {
				// team is still running, so we would need to block
				return B_WOULD_BLOCK;
			}
		}

		// see if there is any death entry for us already

		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		status = get_death_entry(team, child, &death, &freeDeath);

		RELEASE_TEAM_LOCK();
		restore_interrupts(state);

		// we got our death entry and can return to our caller
		if (status == B_OK)
			break;

		// there was no matching group/child we could wait for
		if (status == B_BAD_THREAD_ID)
			return B_BAD_THREAD_ID;

		if ((flags & WNOHANG) != 0)
			return B_WOULD_BLOCK;

		status = acquire_sem(team->dead_children.sem);
		if (status == B_INTERRUPTED)
			return B_INTERRUPTED;
	}

	free(freeDeath);

	// when we got here, we have a valid death entry
	*_returnCode = death.status;
	*_reason = death.reason;

	return death.thread;
}


//	#pragma mark -
// public team API


thread_id
load_image(int32 argCount, const char **args, const char **env)
{
	char **argsCopy, **envCopy;
	int32 envCount = 0;

	if (copy_strings_array(args, argCount, &argsCopy) != B_OK)
		return B_NO_MEMORY;

	// count env variables
	while (env && env[envCount] != NULL)
		envCount++;

	if (copy_strings_array(env, envCount, &envCopy) != B_OK) {
		free_strings_array(argsCopy, argCount);
		return B_NO_MEMORY;
	}

	return load_image_etc(argCount, argsCopy, envCount, envCopy, B_NORMAL_PRIORITY);
}


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


pid_t
getpid(void)
{
	return thread_get_current_thread()->team->main_thread->id;
}


pid_t
getppid(void)
{
	struct team *team = thread_get_current_thread()->team;
	cpu_status state;
	pid_t parent;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	parent = team->parent->main_thread->id;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return parent;
}


pid_t
getpgid(pid_t process)
{
	struct thread *thread;
	pid_t result = -1;
	cpu_status state;

	if (process == 0)
		process = thread_get_current_thread()->team->main_thread->id;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(process);
	if (thread != NULL)
		result = thread->team->group_id;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return thread != NULL ? result : B_BAD_VALUE;
}


pid_t
getsid(pid_t process)
{
	struct thread *thread;
	pid_t result = -1;
	cpu_status state;

	if (process == 0)
		process = thread_get_current_thread()->team->main_thread->id;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(process);
	if (thread != NULL)
		result = thread->team->session_id;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return thread != NULL ? result : B_BAD_VALUE;
}


//	#pragma mark -
//	syscalls


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
_user_exec(const char *userPath, int32 argCount, char * const *userArgs,
	int32 envCount, char * const *userEnvironment)
{
	char path[B_PATH_NAME_LENGTH];
	status_t status;
	char **args;
	char **env;

	if (argCount < 1)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userPath) || !IS_USER_ADDRESS(userArgs) || !IS_USER_ADDRESS(userEnvironment)
		|| user_strlcpy(path, userPath, sizeof(path)) < B_OK)
		return B_BAD_ADDRESS;

	status = user_copy_strings_array(userArgs, argCount, &args);
	if (status < B_OK)
		return status;

	status = user_copy_strings_array(userEnvironment, envCount, &env);
	if (status < B_OK) {
		free_strings_array(args, argCount);
		return status;
	}

	// replace args[0] with the path argument, just to be on the safe side
	free(args[0]);
	args[0] = strdup(path);

	status = exec_team(argCount, args, envCount, env);
		// this one only returns in case of error

	free_strings_array(args, argCount);
	free_strings_array(env, envCount);
	return status;
}


thread_id
_user_fork(void)
{
	return fork_team();
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


pid_t
_user_process_info(pid_t process, int32 which)
{
	// we only allow to return the parent of the current process
	if (which == PARENT_ID
		&& process != 0 && process != thread_get_current_thread()->team->main_thread->id)
		return B_BAD_VALUE;

	switch (which) {
		case SESSION_ID:
			return getsid(process);
		case GROUP_ID:
			return getpgid(process);
		case PARENT_ID:
			return getppid();
	}

	return B_BAD_VALUE;
}


pid_t
_user_setpgid(pid_t processID, pid_t groupID)
{
	struct team *currentTeam = thread_get_current_thread()->team;
	struct process_group *group = NULL, *freeGroup = NULL;
	struct thread *thread;
	struct team *team;
	cpu_status state;
	team_id teamID = -1;
	status_t status = B_OK;

	if (groupID < 0)
		return B_BAD_VALUE;

	if (processID == 0) {
		// get our own process ID
		processID = currentTeam->main_thread->id;
		teamID = currentTeam->id;

		// we must not change our process group ID if we're a group leader
		if (is_process_group_leader(currentTeam))
			return B_NOT_ALLOWED;

		status = B_OK;
	} else {
		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(processID);

		// the thread must be the team's main thread, as that
		// determines its process ID
		if (thread != NULL && thread == thread->team->main_thread) {
			// check if the thread is in a child team of the calling team and
			// if it's already a process group leader and in the same session
			if (thread->team->parent != currentTeam
				|| is_process_group_leader(thread->team)
				|| thread->team->session_id != currentTeam->session_id)
				status = B_NOT_ALLOWED;
			else
				teamID = thread->team->id;
		} else
			status = B_BAD_THREAD_ID;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	if (status != B_OK)
		return status;

	// if the group ID is not specified, a new group should be created
	if (groupID == 0)
		groupID = processID;

	if (groupID == processID) {
		// We need to create a new process group for this team
		group = create_process_group(groupID);
		if (group == NULL)
			return B_NO_MEMORY;
	}

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team != NULL) {
		if (processID == groupID) {
			// we created a new process group, let us insert it into the team's session
			insert_group_into_session(team->group->session, group);
			remove_team_from_group(team, &freeGroup);
			insert_team_into_group(group, team);
		} else {
			struct process_session *session = team->group->session;
			struct process_group *group = NULL;

			// check if this team can have the group ID; there must be one matching
			// process ID in the team's session

			while ((group = list_get_next_item(&session->groups, group)) != NULL) {
				if (group->id == groupID)
					break;
			}

			if (group) {
				// we got a group, let's move the team there
				remove_team_from_group(team, &freeGroup);
				insert_team_into_group(group, team);
			} else
				status = B_NOT_ALLOWED;
		}
	} else
		status = B_NOT_ALLOWED;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (status != B_OK && group != NULL)
		team_delete_process_group(group);

	team_delete_process_group(freeGroup);

	return status == B_OK ? groupID : status;
}


pid_t
_user_setsid(void)
{
	struct team *team = thread_get_current_thread()->team;
	struct process_session *session;
	struct process_group *group, *freeGroup = NULL;
	cpu_status state;
	bool failed = false;

	// the team must not already be a process group leader
	if (is_process_group_leader(team))
		return B_NOT_ALLOWED;

	group = create_process_group(team->main_thread->id);
	if (group == NULL)
		return B_NO_MEMORY;

	session = create_process_session(group->id);
	if (session == NULL) {
		team_delete_process_group(group);
		return B_NO_MEMORY;
	}

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	// this may have changed since the check above
	if (!is_process_group_leader(team)) {
		remove_team_from_group(team, &freeGroup);

		insert_group_into_session(session, group);
		insert_team_into_group(group, team);
	} else
		failed = true;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (failed) {
		team_delete_process_group(group);
		free(session);
		return B_NOT_ALLOWED;
	} else
		team_delete_process_group(freeGroup);

	return team->group_id;
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
_user_load_image(int32 argCount, const char **userArgs, int32 envCount,
	const char **userEnv, int32 priority)
{
	char **args = NULL;
	char **env = NULL;

	TRACE(("_user_load_image_etc: argc = %d\n", argCount));

	if (argCount < 1 || userArgs == NULL || userEnv == NULL)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userArgs) || !IS_USER_ADDRESS(userEnv)
		|| user_copy_strings_array((char * const *)userArgs, argCount, &args) < B_OK)
		return B_BAD_ADDRESS;

	if (user_copy_strings_array((char * const *)userEnv, envCount, &env) < B_OK) {
		free_strings_array(args, argCount);
		return B_BAD_ADDRESS;
	}

	return load_image_etc(argCount, args, envCount, env, priority);
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

