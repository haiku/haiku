/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*!	Team functions */


#include <team.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include <OS.h>

#include <AutoDeleter.h>
#include <FindDirectory.h>

#include <extended_system_info_defs.h>

#include <boot_device.h>
#include <elf.h>
#include <file_cache.h>
#include <fs/KPath.h>
#include <heap.h>
#include <int.h>
#include <kernel.h>
#include <kimage.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <Notifications.h>
#include <port.h>
#include <posix/realtime_sem.h>
#include <posix/xsi_semaphore.h>
#include <sem.h>
#include <syscall_process_info.h>
#include <syscall_restart.h>
#include <syscalls.h>
#include <tls.h>
#include <tracing.h>
#include <user_runtime.h>
#include <user_thread.h>
#include <usergroup.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <util/AutoLock.h>
#include <util/khash.h>

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
	char	*path;
	char	**flat_args;
	size_t	flat_args_size;
	uint32	arg_count;
	uint32	env_count;
	mode_t	umask;
	port_id	error_port;
	uint32	error_token;
};

struct fork_arg {
	area_id				user_stack_area;
	addr_t				user_stack_base;
	size_t				user_stack_size;
	addr_t				user_local_storage;
	sigset_t			sig_block_mask;
	struct sigaction	sig_action[32];
	addr_t				signal_stack_base;
	size_t				signal_stack_size;
	bool				signal_stack_enabled;

	struct user_thread* user_thread;

	struct arch_fork_arg arch_info;
};

class TeamNotificationService : public DefaultNotificationService {
public:
							TeamNotificationService();

			void			Notify(uint32 eventCode, Team* team);
};


struct TeamHashDefinition {
	typedef team_id		KeyType;
	typedef	Team		ValueType;

	size_t HashKey(team_id key) const
	{
		return key;
	}

	size_t Hash(Team* value) const
	{
		return HashKey(value->id);
	}

	bool Compare(team_id key, Team* value) const
	{
		return value->id == key;
	}

	Team*& GetLink(Team* value) const
	{
		return value->next;
	}
};

typedef BOpenHashTable<TeamHashDefinition> TeamHashTable;


static TeamHashTable sTeamHash;
static hash_table* sGroupHash = NULL;
static Team* sKernelTeam = NULL;

// some arbitrary chosen limits - should probably depend on the available
// memory (the limit is not yet enforced)
static int32 sMaxTeams = 2048;
static int32 sUsedTeams = 1;

static TeamNotificationService sNotificationService;

spinlock gTeamSpinlock = B_SPINLOCK_INITIALIZER;


// #pragma mark - Tracing


#if TEAM_TRACING
namespace TeamTracing {

class TeamForked : public AbstractTraceEntry {
public:
	TeamForked(thread_id forkedThread)
		:
		fForkedThread(forkedThread)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("team forked, new thread %ld", fForkedThread);
	}

private:
	thread_id			fForkedThread;
};


class ExecTeam : public AbstractTraceEntry {
public:
	ExecTeam(const char* path, int32 argCount, const char* const* args,
			int32 envCount, const char* const* env)
		:
		fArgCount(argCount),
		fArgs(NULL)
	{
		fPath = alloc_tracing_buffer_strcpy(path, B_PATH_NAME_LENGTH,
			false);

		// determine the buffer size we need for the args
		size_t argBufferSize = 0;
		for (int32 i = 0; i < argCount; i++)
			argBufferSize += strlen(args[i]) + 1;

		// allocate a buffer
		fArgs = (char*)alloc_tracing_buffer(argBufferSize);
		if (fArgs) {
			char* buffer = fArgs;
			for (int32 i = 0; i < argCount; i++) {
				size_t argSize = strlen(args[i]) + 1;
				memcpy(buffer, args[i], argSize);
				buffer += argSize;
			}
		}

		// ignore env for the time being
		(void)envCount;
		(void)env;

		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("team exec, \"%p\", args:", fPath);

		if (fArgs != NULL) {
			char* args = fArgs;
			for (int32 i = 0; !out.IsFull() && i < fArgCount; i++) {
				out.Print(" \"%s\"", args);
				args += strlen(args) + 1;
			}
		} else
			out.Print(" <too long>");
	}

private:
	char*	fPath;
	int32	fArgCount;
	char*	fArgs;
};


static const char*
job_control_state_name(job_control_state state)
{
	switch (state) {
		case JOB_CONTROL_STATE_NONE:
			return "none";
		case JOB_CONTROL_STATE_STOPPED:
			return "stopped";
		case JOB_CONTROL_STATE_CONTINUED:
			return "continued";
		case JOB_CONTROL_STATE_DEAD:
			return "dead";
		default:
			return "invalid";
	}
}


class SetJobControlState : public AbstractTraceEntry {
public:
	SetJobControlState(team_id team, job_control_state newState, int signal)
		:
		fTeam(team),
		fNewState(newState),
		fSignal(signal)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("team set job control state, team %ld, "
			"new state: %s, signal: %d",
			fTeam, job_control_state_name(fNewState), fSignal);
	}

private:
	team_id				fTeam;
	job_control_state	fNewState;
	int					fSignal;
};


class WaitForChild : public AbstractTraceEntry {
public:
	WaitForChild(pid_t child, uint32 flags)
		:
		fChild(child),
		fFlags(flags)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("team wait for child, child: %ld, "
			"flags: 0x%lx", fChild, fFlags);
	}

private:
	pid_t	fChild;
	uint32	fFlags;
};


class WaitForChildDone : public AbstractTraceEntry {
public:
	WaitForChildDone(const job_control_entry& entry)
		:
		fState(entry.state),
		fTeam(entry.thread),
		fStatus(entry.status),
		fReason(entry.reason),
		fSignal(entry.signal)
	{
		Initialized();
	}

	WaitForChildDone(status_t error)
		:
		fTeam(error)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		if (fTeam >= 0) {
			out.Print("team wait for child done, team: %ld, "
				"state: %s, status: 0x%lx, reason: 0x%x, signal: %d\n",
				fTeam, job_control_state_name(fState), fStatus, fReason,
				fSignal);
		} else {
			out.Print("team wait for child failed, error: "
				"0x%lx, ", fTeam);
		}
	}

private:
	job_control_state	fState;
	team_id				fTeam;
	status_t			fStatus;
	uint16				fReason;
	uint16				fSignal;
};

}	// namespace TeamTracing

#	define T(x) new(std::nothrow) TeamTracing::x;
#else
#	define T(x) ;
#endif


//	#pragma mark - TeamNotificationService


TeamNotificationService::TeamNotificationService()
	: DefaultNotificationService("teams")
{
}


void
TeamNotificationService::Notify(uint32 eventCode, Team* team)
{
	char eventBuffer[128];
	KMessage event;
	event.SetTo(eventBuffer, sizeof(eventBuffer), TEAM_MONITOR);
	event.AddInt32("event", eventCode);
	event.AddInt32("team", team->id);
	event.AddPointer("teamStruct", team);

	DefaultNotificationService::Notify(event, eventCode);
}


//	#pragma mark - Private functions


static void
_dump_team_info(Team* team)
{
	kprintf("TEAM: %p\n", team);
	kprintf("id:               %ld (%#lx)\n", team->id, team->id);
	kprintf("name:             '%s'\n", team->name);
	kprintf("args:             '%s'\n", team->args);
	kprintf("next:             %p\n", team->next);
	kprintf("parent:           %p", team->parent);
	if (team->parent != NULL) {
		kprintf(" (id = %ld)\n", team->parent->id);
	} else
		kprintf("\n");

	kprintf("children:         %p\n", team->children);
	kprintf("num_threads:      %d\n", team->num_threads);
	kprintf("state:            %d\n", team->state);
	kprintf("flags:            0x%lx\n", team->flags);
	kprintf("io_context:       %p\n", team->io_context);
	if (team->address_space)
		kprintf("address_space:    %p\n", team->address_space);
	kprintf("user data:        %p (area %ld)\n", (void*)team->user_data,
		team->user_data_area);
	kprintf("free user thread: %p\n", team->free_user_threads);
	kprintf("main_thread:      %p\n", team->main_thread);
	kprintf("thread_list:      %p\n", team->thread_list);
	kprintf("group_id:         %ld\n", team->group_id);
	kprintf("session_id:       %ld\n", team->session_id);
}


static int
dump_team_info(int argc, char** argv)
{
	team_id id = -1;
	bool found = false;

	if (argc < 2) {
		Thread* thread = thread_get_current_thread();
		if (thread != NULL && thread->team != NULL)
			_dump_team_info(thread->team);
		else
			kprintf("No current team!\n");
		return 0;
	}

	id = strtoul(argv[1], NULL, 0);
	if (IS_KERNEL_ADDRESS(id)) {
		// semi-hack
		_dump_team_info((Team*)id);
		return 0;
	}

	// walk through the thread list, trying to match name or id
	for (TeamHashTable::Iterator it = sTeamHash.GetIterator();
		Team* team = it.Next();) {
		if ((team->name && strcmp(argv[1], team->name) == 0)
			|| team->id == id) {
			_dump_team_info(team);
			found = true;
			break;
		}
	}

	if (!found)
		kprintf("team \"%s\" (%ld) doesn't exist!\n", argv[1], id);
	return 0;
}


static int
dump_teams(int argc, char** argv)
{
	kprintf("team           id  parent      name\n");

	for (TeamHashTable::Iterator it = sTeamHash.GetIterator();
		Team* team = it.Next();) {
		kprintf("%p%7ld  %p  %s\n", team, team->id, team->parent, team->name);
	}

	return 0;
}


static int
process_group_compare(void* _group, const void* _key)
{
	struct process_group* group = (struct process_group*)_group;
	const struct team_key* key = (const struct team_key*)_key;

	if (group->id == key->id)
		return 0;

	return 1;
}


static uint32
process_group_hash(void* _group, const void* _key, uint32 range)
{
	struct process_group* group = (struct process_group*)_group;
	const struct team_key* key = (const struct team_key*)_key;

	if (group != NULL)
		return group->id % range;

	return (uint32)key->id % range;
}


static void
insert_team_into_parent(Team* parent, Team* team)
{
	ASSERT(parent != NULL);

	team->siblings_next = parent->children;
	parent->children = team;
	team->parent = parent;
}


/*!	Note: must have team lock held */
static void
remove_team_from_parent(Team* parent, Team* team)
{
	Team* child;
	Team* last = NULL;

	for (child = parent->children; child != NULL;
			child = child->siblings_next) {
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


/*!	Reparent each of our children
	Note: must have team lock held
*/
static void
reparent_children(Team* team)
{
	Team* child;

	while ((child = team->children) != NULL) {
		// remove the child from the current proc and add to the parent
		remove_team_from_parent(team, child);
		insert_team_into_parent(sKernelTeam, child);
	}

	// move job control entries too
	sKernelTeam->stopped_children.entries.MoveFrom(
		&team->stopped_children.entries);
	sKernelTeam->continued_children.entries.MoveFrom(
		&team->continued_children.entries);

	// Note, we don't move the dead children entries. Those will be deleted
	// when the team structure is deleted.
}


static bool
is_session_leader(Team* team)
{
	return team->session_id == team->id;
}


static bool
is_process_group_leader(Team* team)
{
	return team->group_id == team->id;
}


static void
deferred_delete_process_group(struct process_group* group)
{
	if (group == NULL)
		return;

	// remove_group_from_session() keeps this pointer around
	// only if the session can be freed as well
	if (group->session) {
		TRACE(("deferred_delete_process_group(): frees session %ld\n",
			group->session->id));
		deferred_free(group->session);
	}

	deferred_free(group);
}


/*!	Removes a group from a session, and puts the session object
	back into the session cache, if it's not used anymore.
	You must hold the team lock when calling this function.
*/
static void
remove_group_from_session(struct process_group* group)
{
	struct process_session* session = group->session;

	// the group must be in any session to let this function have any effect
	if (session == NULL)
		return;

	hash_remove(sGroupHash, group);

	// we cannot free the resource here, so we're keeping the group link
	// around - this way it'll be freed by free_process_group()
	if (--session->group_count > 0)
		group->session = NULL;
}


/*!	Team lock must be held.
*/
static void
acquire_process_group_ref(pid_t groupID)
{
	process_group* group = team_get_process_group_locked(NULL, groupID);
	if (group == NULL) {
		panic("acquire_process_group_ref(): unknown group ID: %ld", groupID);
		return;
	}

	group->refs++;
}


/*!	Team lock must be held.
*/
static void
release_process_group_ref(pid_t groupID)
{
	process_group* group = team_get_process_group_locked(NULL, groupID);
	if (group == NULL) {
		panic("release_process_group_ref(): unknown group ID: %ld", groupID);
		return;
	}

	if (group->refs <= 0) {
		panic("release_process_group_ref(%ld): ref count already 0", groupID);
		return;
	}

	if (--group->refs > 0)
		return;

	// group is no longer used

	remove_group_from_session(group);
	deferred_delete_process_group(group);
}


/*!	You must hold the team lock when calling this function. */
static void
insert_group_into_session(struct process_session* session,
	struct process_group* group)
{
	if (group == NULL)
		return;

	group->session = session;
	hash_insert(sGroupHash, group);
	session->group_count++;
}


/*!	You must hold the team lock when calling this function. */
static void
insert_team_into_group(struct process_group* group, Team* team)
{
	team->group = group;
	team->group_id = group->id;
	team->session_id = group->session->id;

	team->group_next = group->teams;
	group->teams = team;
	acquire_process_group_ref(group->id);
}


/*!	Removes the team from the group.

	\param team the team that'll be removed from it's group
*/
static void
remove_team_from_group(Team* team)
{
	struct process_group* group = team->group;
	Team* current;
	Team* last = NULL;

	// the team must be in any team to let this function have any effect
	if  (group == NULL)
		return;

	for (current = group->teams; current != NULL;
			current = current->group_next) {
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

	team->group = NULL;
	team->group_next = NULL;

	release_process_group_ref(group->id);
}


static struct process_group*
create_process_group(pid_t id)
{
	struct process_group* group
		= (struct process_group*)malloc(sizeof(struct process_group));
	if (group == NULL)
		return NULL;

	group->id = id;
	group->refs = 0;
	group->session = NULL;
	group->teams = NULL;
	group->orphaned = true;
	return group;
}


static struct process_session*
create_process_session(pid_t id)
{
	struct process_session* session
		= (struct process_session*)malloc(sizeof(struct process_session));
	if (session == NULL)
		return NULL;

	session->id = id;
	session->group_count = 0;
	session->controlling_tty = -1;
	session->foreground_group = -1;

	return session;
}


static void
set_team_name(Team* team, const char* name)
{
	if (const char* lastSlash = strrchr(name, '/'))
		name = lastSlash + 1;

	strlcpy(team->name, name, B_OS_NAME_LENGTH);
}


static Team*
create_team_struct(const char* name, bool kernel)
{
	Team* team = new(std::nothrow) Team;
	if (team == NULL)
		return NULL;
	ObjectDeleter<Team> teamDeleter(team);

	team->next = team->siblings_next = team->children = team->parent = NULL;
	team->id = allocate_thread_id();
	set_team_name(team, name);
	team->args[0] = '\0';
	team->num_threads = 0;
	team->io_context = NULL;
	team->address_space = NULL;
	team->realtime_sem_context = NULL;
	team->xsi_sem_context = NULL;
	team->thread_list = NULL;
	team->main_thread = NULL;
	team->loading_info = NULL;
	team->state = TEAM_STATE_BIRTH;
	team->flags = 0;
	team->death_entry = NULL;
	team->user_data_area = -1;
	team->user_data = 0;
	team->used_user_data = 0;
	team->user_data_size = 0;
	team->free_user_threads = NULL;

	team->supplementary_groups = NULL;
	team->supplementary_group_count = 0;

	team->dead_threads_kernel_time = 0;
	team->dead_threads_user_time = 0;

	// dead threads
	list_init(&team->dead_threads);
	team->dead_threads_count = 0;

	// dead children
	team->dead_children.count = 0;
	team->dead_children.kernel_time = 0;
	team->dead_children.user_time = 0;

	// job control entry
	team->job_control_entry = new(nothrow) job_control_entry;
	if (team->job_control_entry == NULL)
		return NULL;
	ObjectDeleter<job_control_entry> jobControlEntryDeleter(
		team->job_control_entry);
	team->job_control_entry->state = JOB_CONTROL_STATE_NONE;
	team->job_control_entry->thread = team->id;
	team->job_control_entry->team = team;

	list_init(&team->sem_list);
	list_init(&team->port_list);
	list_init(&team->image_list);
	list_init(&team->watcher_list);

	clear_team_debug_info(&team->debug_info, true);

	if (arch_team_init_team_struct(team, kernel) < 0)
		return NULL;

	// publish dead/stopped/continued children condition vars
	team->dead_children.condition_variable.Init(&team->dead_children,
		"team children");

	// keep all allocated structures
	jobControlEntryDeleter.Detach();
	teamDeleter.Detach();

	return team;
}


static void
delete_team_struct(Team* team)
{
	// get rid of all associated data
	team->PrepareForDeletion();

	while (death_entry* threadDeathEntry = (death_entry*)list_remove_head_item(
			&team->dead_threads)) {
		free(threadDeathEntry);
	}

	while (job_control_entry* entry = team->dead_children.entries.RemoveHead())
		delete entry;

	while (free_user_thread* entry = team->free_user_threads) {
		team->free_user_threads = entry->next;
		free(entry);
	}

	malloc_referenced_release(team->supplementary_groups);

	delete team->job_control_entry;
		// usually already NULL and transferred to the parent
	delete team;
}


static status_t
create_team_user_data(Team* team)
{
	void* address;
	size_t size = 4 * B_PAGE_SIZE;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = (void*)KERNEL_USER_DATA_BASE;
	virtualRestrictions.address_specification = B_BASE_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	team->user_data_area = create_area_etc(team->id, "user area", size,
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA, 0, &virtualRestrictions,
		&physicalRestrictions, &address);
	if (team->user_data_area < 0)
		return team->user_data_area;

	team->user_data = (addr_t)address;
	team->used_user_data = 0;
	team->user_data_size = size;
	team->free_user_threads = NULL;

	return B_OK;
}


static void
delete_team_user_data(Team* team)
{
	if (team->user_data_area >= 0) {
		vm_delete_area(team->id, team->user_data_area, true);
		team->user_data = 0;
		team->used_user_data = 0;
		team->user_data_size = 0;
		team->user_data_area = -1;
		while (free_user_thread* entry = team->free_user_threads) {
			team->free_user_threads = entry->next;
			free(entry);
		}
	}
}


static status_t
copy_user_process_args(const char* const* userFlatArgs, size_t flatArgsSize,
	int32 argCount, int32 envCount, char**& _flatArgs)
{
	if (argCount < 0 || envCount < 0)
		return B_BAD_VALUE;

	if (flatArgsSize > MAX_PROCESS_ARGS_SIZE)
		return B_TOO_MANY_ARGS;
	if ((argCount + envCount + 2) * sizeof(char*) > flatArgsSize)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userFlatArgs))
		return B_BAD_ADDRESS;

	// allocate kernel memory
	char** flatArgs = (char**)malloc(flatArgsSize);
	if (flatArgs == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(flatArgs, userFlatArgs, flatArgsSize) != B_OK) {
		free(flatArgs);
		return B_BAD_ADDRESS;
	}

	// check and relocate the array
	status_t error = B_OK;
	const char* stringBase = (char*)flatArgs + argCount + envCount + 2;
	const char* stringEnd = (char*)flatArgs + flatArgsSize;
	for (int32 i = 0; i < argCount + envCount + 2; i++) {
		if (i == argCount || i == argCount + envCount + 1) {
			// check array null termination
			if (flatArgs[i] != NULL) {
				error = B_BAD_VALUE;
				break;
			}
		} else {
			// check string
			char* arg = (char*)flatArgs + (flatArgs[i] - (char*)userFlatArgs);
			size_t maxLen = stringEnd - arg;
			if (arg < stringBase || arg >= stringEnd
					|| strnlen(arg, maxLen) == maxLen) {
				error = B_BAD_VALUE;
				break;
			}

			flatArgs[i] = arg;
		}
	}

	if (error == B_OK)
		_flatArgs = flatArgs;
	else
		free(flatArgs);

	return error;
}


static void
free_team_arg(struct team_arg* teamArg)
{
	if (teamArg != NULL) {
		free(teamArg->flat_args);
		free(teamArg->path);
		free(teamArg);
	}
}


static status_t
create_team_arg(struct team_arg** _teamArg, const char* path, char** flatArgs,
	size_t flatArgsSize, int32 argCount, int32 envCount, mode_t umask,
	port_id port, uint32 token)
{
	struct team_arg* teamArg = (struct team_arg*)malloc(sizeof(team_arg));
	if (teamArg == NULL)
		return B_NO_MEMORY;

	teamArg->path = strdup(path);
	if (teamArg->path == NULL) {
		free(teamArg);
		return B_NO_MEMORY;
	}

	// copy the args over

	teamArg->flat_args = flatArgs;
	teamArg->flat_args_size = flatArgsSize;
	teamArg->arg_count = argCount;
	teamArg->env_count = envCount;
	teamArg->umask = umask;
	teamArg->error_port = port;
	teamArg->error_token = token;

	*_teamArg = teamArg;
	return B_OK;
}


static int32
team_create_thread_start(void* args)
{
	status_t err;
	Thread* thread;
	Team* team;
	struct team_arg* teamArgs = (struct team_arg*)args;
	const char* path;
	addr_t entry;
	char userStackName[128];
	uint32 sizeLeft;
	char** userArgs;
	char** userEnv;
	struct user_space_program_args* programArgs;
	uint32 argCount, envCount, i;

	thread = thread_get_current_thread();
	team = thread->team;
	cache_node_launched(teamArgs->arg_count, teamArgs->flat_args);

	TRACE(("team_create_thread_start: entry thread %ld\n", thread->id));

	// get a user thread for the main thread
	thread->user_thread = team_allocate_user_thread(team);

	// create an initial primary stack area

	// Main stack area layout is currently as follows (starting from 0):
	//
	// size								| usage
	// ---------------------------------+--------------------------------
	// USER_MAIN_THREAD_STACK_SIZE		| actual stack
	// TLS_SIZE							| TLS data
	// sizeof(user_space_program_args)	| argument structure for the runtime
	//									| loader
	// flat arguments size				| flat process arguments and environment

	// TODO: ENV_SIZE is a) limited, and b) not used after libroot copied it to
	// the heap
	// TODO: we could reserve the whole USER_STACK_REGION upfront...

	sizeLeft = PAGE_ALIGN(USER_MAIN_THREAD_STACK_SIZE
		+ USER_STACK_GUARD_PAGES * B_PAGE_SIZE + TLS_SIZE
		+ sizeof(struct user_space_program_args) + teamArgs->flat_args_size);
	thread->user_stack_base
		= USER_STACK_REGION + USER_STACK_REGION_SIZE - sizeLeft;
	thread->user_stack_size = USER_MAIN_THREAD_STACK_SIZE
		+ USER_STACK_GUARD_PAGES * B_PAGE_SIZE;
		// the exact location at the end of the user stack area

	sprintf(userStackName, "%s_main_stack", team->name);
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address = (void*)thread->user_stack_base;
	virtualRestrictions.address_specification = B_EXACT_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	thread->user_stack_area = create_area_etc(team->id, userStackName, sizeLeft,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA | B_STACK_AREA, 0,
		&virtualRestrictions, &physicalRestrictions, NULL);
	if (thread->user_stack_area < 0) {
		dprintf("team_create_thread_start: could not create default user stack "
			"region: %s\n", strerror(thread->user_stack_area));

		free_team_arg(teamArgs);
		return thread->user_stack_area;
	}

	// now that the TLS area is allocated, initialize TLS
	arch_thread_init_tls(thread);

	argCount = teamArgs->arg_count;
	envCount = teamArgs->env_count;

	programArgs = (struct user_space_program_args*)(thread->user_stack_base
		+ thread->user_stack_size + TLS_SIZE);

	userArgs = (char**)(programArgs + 1);
	userEnv = userArgs + argCount + 1;
	path = teamArgs->path;

	if (user_strlcpy(programArgs->program_path, path,
				sizeof(programArgs->program_path)) < B_OK
		|| user_memcpy(&programArgs->arg_count, &argCount, sizeof(int32)) < B_OK
		|| user_memcpy(&programArgs->args, &userArgs, sizeof(char**)) < B_OK
		|| user_memcpy(&programArgs->env_count, &envCount, sizeof(int32)) < B_OK
		|| user_memcpy(&programArgs->env, &userEnv, sizeof(char**)) < B_OK
		|| user_memcpy(&programArgs->error_port, &teamArgs->error_port,
				sizeof(port_id)) < B_OK
		|| user_memcpy(&programArgs->error_token, &teamArgs->error_token,
				sizeof(uint32)) < B_OK
		|| user_memcpy(&programArgs->umask, &teamArgs->umask, sizeof(mode_t)) < B_OK
		|| user_memcpy(userArgs, teamArgs->flat_args,
				teamArgs->flat_args_size) < B_OK) {
		// the team deletion process will clean this mess
		free_team_arg(teamArgs);
		return B_BAD_ADDRESS;
	}

	TRACE(("team_create_thread_start: loading elf binary '%s'\n", path));

	// add args to info member
	team->args[0] = 0;
	strlcpy(team->args, path, sizeof(team->args));
	for (i = 1; i < argCount; i++) {
		strlcat(team->args, " ", sizeof(team->args));
		strlcat(team->args, teamArgs->flat_args[i], sizeof(team->args));
	}

	free_team_arg(teamArgs);
		// the arguments are already on the user stack, we no longer need
		// them in this form

	// NOTE: Normally arch_thread_enter_userspace() never returns, that is
	// automatic variables with function scope will never be destroyed.
	{
		// find runtime_loader path
		KPath runtimeLoaderPath;
		err = find_directory(B_BEOS_SYSTEM_DIRECTORY, gBootDevice, false,
			runtimeLoaderPath.LockBuffer(), runtimeLoaderPath.BufferSize());
		if (err < B_OK) {
			TRACE(("team_create_thread_start: find_directory() failed: %s\n",
				strerror(err)));
			return err;
		}
		runtimeLoaderPath.UnlockBuffer();
		err = runtimeLoaderPath.Append("runtime_loader");

		if (err == B_OK) {
			err = elf_load_user_image(runtimeLoaderPath.Path(), team, 0,
				&entry);
		}
	}

	if (err < B_OK) {
		// Luckily, we don't have to clean up the mess we created - that's
		// done for us by the normal team deletion process
		TRACE(("team_create_thread_start: elf_load_user_image() failed: "
			"%s\n", strerror(err)));
		return err;
	}

	TRACE(("team_create_thread_start: loaded elf. entry = %#lx\n", entry));

	team->state = TEAM_STATE_NORMAL;

	// jump to the entry point in user space
	return arch_thread_enter_userspace(thread, entry, programArgs, NULL);
		// only returns in case of error
}


static thread_id
load_image_internal(char**& _flatArgs, size_t flatArgsSize, int32 argCount,
	int32 envCount, int32 priority, team_id parentID, uint32 flags,
	port_id errorPort, uint32 errorToken)
{
	char** flatArgs = _flatArgs;
	Team* team;
	const char* threadName;
	thread_id thread;
	status_t status;
	cpu_status state;
	struct team_arg* teamArgs;
	struct team_loading_info loadingInfo;
	io_context* parentIOContext = NULL;

	if (flatArgs == NULL || argCount == 0)
		return B_BAD_VALUE;

	const char* path = flatArgs[0];

	TRACE(("load_image_internal: name '%s', args = %p, argCount = %ld\n",
		path, flatArgs, argCount));

	team = create_team_struct(path, false);
	if (team == NULL)
		return B_NO_MEMORY;

	if (flags & B_WAIT_TILL_LOADED) {
		loadingInfo.thread = thread_get_current_thread();
		loadingInfo.result = B_ERROR;
		loadingInfo.done = false;
		team->loading_info = &loadingInfo;
	}

 	InterruptsSpinLocker teamLocker(gTeamSpinlock);

	// get the parent team
	Team* parent;

	if (parentID == B_CURRENT_TEAM)
		parent = thread_get_current_thread()->team;
	else
		parent = team_get_team_struct_locked(parentID);

	if (parent == NULL) {
		teamLocker.Unlock();
		status = B_BAD_TEAM_ID;
		goto err0;
	}

	// inherit the parent's user/group
	inherit_parent_user_and_group_locked(team, parent);

	sTeamHash.InsertUnchecked(team);
	insert_team_into_parent(parent, team);
	insert_team_into_group(parent->group, team);
	sUsedTeams++;

	// get a reference to the parent's I/O context -- we need it to create ours
	parentIOContext = parent->io_context;
	vfs_get_io_context(parentIOContext);

	teamLocker.Unlock();

	// check the executable's set-user/group-id permission
	update_set_id_user_and_group(team, path);

	status = create_team_arg(&teamArgs, path, flatArgs, flatArgsSize, argCount,
		envCount, (mode_t)-1, errorPort, errorToken);
	if (status != B_OK)
		goto err1;

	_flatArgs = NULL;
		// args are owned by the team_arg structure now

	// create a new io_context for this team
	team->io_context = vfs_new_io_context(parentIOContext, true);
	if (!team->io_context) {
		status = B_NO_MEMORY;
		goto err2;
	}

	// We don't need the parent's I/O context any longer.
	vfs_put_io_context(parentIOContext);
	parentIOContext = NULL;

	// remove any fds that have the CLOEXEC flag set (emulating BeOS behaviour)
	vfs_exec_io_context(team->io_context);

	// create an address space for this team
	status = VMAddressSpace::Create(team->id, USER_BASE, USER_SIZE, false,
		&team->address_space);
	if (status != B_OK)
		goto err3;

	// cut the path from the main thread name
	threadName = strrchr(path, '/');
	if (threadName != NULL)
		threadName++;
	else
		threadName = path;

	// create the user data area
	status = create_team_user_data(team);
	if (status != B_OK)
		goto err4;

	// notify team listeners
	sNotificationService.Notify(TEAM_ADDED, team);

	// Create a kernel thread, but under the context of the new team
	// The new thread will take over ownership of teamArgs
	thread = spawn_kernel_thread_etc(team_create_thread_start, threadName,
		B_NORMAL_PRIORITY, teamArgs, team->id, team->id);
	if (thread < 0) {
		status = thread;
		goto err5;
	}

	// wait for the loader of the new team to finish its work
	if ((flags & B_WAIT_TILL_LOADED) != 0) {
		Thread* mainThread;

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		mainThread = thread_get_thread_struct_locked(thread);
		if (mainThread) {
			// resume the team's main thread
			if (mainThread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(mainThread);

			// Now suspend ourselves until loading is finished.
			// We will be woken either by the thread, when it finished or
			// aborted loading, or when the team is going to die (e.g. is
			// killed). In either case the one setting `loadingInfo.done' is
			// responsible for removing the info from the team structure.
			while (!loadingInfo.done) {
				thread_get_current_thread()->next_state = B_THREAD_SUSPENDED;
				scheduler_reschedule();
			}
		} else {
			// Impressive! Someone managed to kill the thread in this short
			// time.
		}

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		if (loadingInfo.result < B_OK)
			return loadingInfo.result;
	}

	// notify the debugger
	user_debug_team_created(team->id);

	return thread;

err5:
	sNotificationService.Notify(TEAM_REMOVED, team);
	delete_team_user_data(team);
err4:
	team->address_space->Put();
err3:
	vfs_put_io_context(team->io_context);
err2:
	free_team_arg(teamArgs);
err1:
	if (parentIOContext != NULL)
		vfs_put_io_context(parentIOContext);

	// Remove the team structure from the team hash table and delete the team
	// structure
	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	remove_team_from_group(team);
	remove_team_from_parent(team->parent, team);
	sTeamHash.RemoveUnchecked(team);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

err0:
	delete_team_struct(team);

	return status;
}


/*!	Almost shuts down the current team and loads a new image into it.
	If successful, this function does not return and will takeover ownership of
	the arguments provided.
	This function may only be called from user space.
*/
static status_t
exec_team(const char* path, char**& _flatArgs, size_t flatArgsSize,
	int32 argCount, int32 envCount, mode_t umask)
{
	// NOTE: Since this function normally doesn't return, don't use automatic
	// variables that need destruction in the function scope.
	char** flatArgs = _flatArgs;
	Team* team = thread_get_current_thread()->team;
	struct team_arg* teamArgs;
	const char* threadName;
	status_t status = B_OK;
	cpu_status state;
	Thread* thread;
	thread_id nubThreadID = -1;

	TRACE(("exec_team(path = \"%s\", argc = %ld, envCount = %ld): team %ld\n",
		path, argCount, envCount, team->id));

	T(ExecTeam(path, argCount, flatArgs, envCount, flatArgs + argCount + 1));

	// switching the kernel at run time is probably not a good idea :)
	if (team == team_get_kernel_team())
		return B_NOT_ALLOWED;

	// we currently need to be single threaded here
	// ToDo: maybe we should just kill all other threads and
	//	make the current thread the team's main thread?
	if (team->main_thread != thread_get_current_thread())
		return B_NOT_ALLOWED;

	// The debug nub thread, a pure kernel thread, is allowed to survive.
	// We iterate through the thread list to make sure that there's no other
	// thread.
	state = disable_interrupts();
	GRAB_TEAM_LOCK();
	GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED)
		nubThreadID = team->debug_info.nub_thread;

	RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);

	for (thread = team->thread_list; thread; thread = thread->team_next) {
		if (thread != team->main_thread && thread->id != nubThreadID) {
			status = B_NOT_ALLOWED;
			break;
		}
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (status != B_OK)
		return status;

	status = create_team_arg(&teamArgs, path, flatArgs, flatArgsSize, argCount,
		envCount, umask, -1, 0);
	if (status != B_OK)
		return status;

	_flatArgs = NULL;
		// args are owned by the team_arg structure now

	// TODO: remove team resources if there are any left
	// thread_atkernel_exit() might not be called at all

	thread_reset_for_exec();

	user_debug_prepare_for_exec();

	delete_team_user_data(team);
	vm_delete_areas(team->address_space, false);
	xsi_sem_undo(team);
	delete_owned_ports(team);
	sem_delete_owned_sems(team);
	remove_images(team);
	vfs_exec_io_context(team->io_context);
	delete_realtime_sem_context(team->realtime_sem_context);
	team->realtime_sem_context = NULL;

	status = create_team_user_data(team);
	if (status != B_OK) {
		// creating the user data failed -- we're toast
		// TODO: We should better keep the old user area in the first place.
		free_team_arg(teamArgs);
		exit_thread(status);
		return status;
	}

	user_debug_finish_after_exec();

	// rename the team

	set_team_name(team, path);

	// cut the path from the team name and rename the main thread, too
	threadName = strrchr(path, '/');
	if (threadName != NULL)
		threadName++;
	else
		threadName = path;
	rename_thread(thread_get_current_thread_id(), threadName);

	atomic_or(&team->flags, TEAM_FLAG_EXEC_DONE);

	// Update user/group according to the executable's set-user/group-id
	// permission.
	update_set_id_user_and_group(team, path);

	user_debug_team_exec();

	// notify team listeners
	sNotificationService.Notify(TEAM_EXEC, team);

	status = team_create_thread_start(teamArgs);
		// this one usually doesn't return...

	// sorry, we have to kill us, there is no way out anymore
	// (without any areas left and all that)
	exit_thread(status);

	// we return a status here since the signal that is sent by the
	// call above is not immediately handled
	return B_ERROR;
}


/*! This is the first function to be called from the newly created
	main child thread.
	It will fill in everything what's left to do from fork_arg, and
	return from the parent's fork() syscall to the child.
*/
static int32
fork_team_thread_start(void* _args)
{
	Thread* thread = thread_get_current_thread();
	struct fork_arg* forkArgs = (struct fork_arg*)_args;

	struct arch_fork_arg archArgs = forkArgs->arch_info;
		// we need a local copy of the arch dependent part

	thread->user_stack_area = forkArgs->user_stack_area;
	thread->user_stack_base = forkArgs->user_stack_base;
	thread->user_stack_size = forkArgs->user_stack_size;
	thread->user_local_storage = forkArgs->user_local_storage;
	thread->sig_block_mask = forkArgs->sig_block_mask;
	thread->user_thread = forkArgs->user_thread;
	memcpy(thread->sig_action, forkArgs->sig_action,
		sizeof(forkArgs->sig_action));
	thread->signal_stack_base = forkArgs->signal_stack_base;
	thread->signal_stack_size = forkArgs->signal_stack_size;
	thread->signal_stack_enabled = forkArgs->signal_stack_enabled;

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
	Thread* parentThread = thread_get_current_thread();
	Team* parentTeam = parentThread->team;
	Team* team;
	struct fork_arg* forkArgs;
	struct area_info info;
	thread_id threadID;
	status_t status;
	int32 cookie;

	TRACE(("fork_team(): team %ld\n", parentTeam->id));

	if (parentTeam == team_get_kernel_team())
		return B_NOT_ALLOWED;

	// create a new team
	// TODO: this is very similar to load_image_internal() - maybe we can do
	// something about it :)

	team = create_team_struct(parentTeam->name, false);
	if (team == NULL)
		return B_NO_MEMORY;

	strlcpy(team->args, parentTeam->args, sizeof(team->args));

	InterruptsSpinLocker teamLocker(gTeamSpinlock);

	// Inherit the parent's user/group.
	inherit_parent_user_and_group_locked(team, parentTeam);

	sTeamHash.InsertUnchecked(team);
	insert_team_into_parent(parentTeam, team);
	insert_team_into_group(parentTeam->group, team);
	sUsedTeams++;

	teamLocker.Unlock();

	// inherit some team debug flags
	team->debug_info.flags |= atomic_get(&parentTeam->debug_info.flags)
		& B_TEAM_DEBUG_INHERITED_FLAGS;

	forkArgs = (struct fork_arg*)malloc(sizeof(struct fork_arg));
	if (forkArgs == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	// create a new io_context for this team
	team->io_context = vfs_new_io_context(parentTeam->io_context, false);
	if (!team->io_context) {
		status = B_NO_MEMORY;
		goto err2;
	}

	// duplicate the realtime sem context
	if (parentTeam->realtime_sem_context) {
		team->realtime_sem_context = clone_realtime_sem_context(
			parentTeam->realtime_sem_context);
		if (team->realtime_sem_context == NULL) {
			status = B_NO_MEMORY;
			goto err25;
		}
	}

	// create an address space for this team
	status = VMAddressSpace::Create(team->id, USER_BASE, USER_SIZE, false,
		&team->address_space);
	if (status < B_OK)
		goto err3;

	// copy all areas of the team
	// TODO: should be able to handle stack areas differently (ie. don't have
	// them copy-on-write)
	// TODO: all stacks of other threads than the current one could be left out

	forkArgs->user_thread = NULL;

	cookie = 0;
	while (get_next_area_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.area == parentTeam->user_data_area) {
			// don't clone the user area; just create a new one
			status = create_team_user_data(team);
			if (status != B_OK)
				break;

			forkArgs->user_thread = team_allocate_user_thread(team);
		} else {
			void* address;
			area_id area = vm_copy_area(team->address_space->ID(), info.name,
				&address, B_CLONE_ADDRESS, info.protection, info.area);
			if (area < B_OK) {
				status = area;
				break;
			}

			if (info.area == parentThread->user_stack_area)
				forkArgs->user_stack_area = area;
		}
	}

	if (status < B_OK)
		goto err4;

	if (forkArgs->user_thread == NULL) {
#if KDEBUG
		panic("user data area not found, parent area is %ld",
			parentTeam->user_data_area);
#endif
		status = B_ERROR;
		goto err4;
	}

	forkArgs->user_stack_base = parentThread->user_stack_base;
	forkArgs->user_stack_size = parentThread->user_stack_size;
	forkArgs->user_local_storage = parentThread->user_local_storage;
	forkArgs->sig_block_mask = parentThread->sig_block_mask;
	memcpy(forkArgs->sig_action, parentThread->sig_action,
		sizeof(forkArgs->sig_action));
	forkArgs->signal_stack_base = parentThread->signal_stack_base;
	forkArgs->signal_stack_size = parentThread->signal_stack_size;
	forkArgs->signal_stack_enabled = parentThread->signal_stack_enabled;

	arch_store_fork_frame(&forkArgs->arch_info);

	// copy image list
	image_info imageInfo;
	cookie = 0;
	while (get_next_image_info(parentTeam->id, &cookie, &imageInfo) == B_OK) {
		image_id image = register_image(team, &imageInfo, sizeof(imageInfo));
		if (image < 0)
			goto err5;
	}

	// notify team listeners
	sNotificationService.Notify(TEAM_ADDED, team);

	// create a kernel thread under the context of the new team
	threadID = spawn_kernel_thread_etc(fork_team_thread_start,
		parentThread->name, parentThread->priority, forkArgs,
		team->id, team->id);
	if (threadID < 0) {
		status = threadID;
		goto err5;
	}

	// notify the debugger
	user_debug_team_created(team->id);

	T(TeamForked(threadID));

	resume_thread(threadID);
	return threadID;

err5:
	sNotificationService.Notify(TEAM_REMOVED, team);
	remove_images(team);
err4:
	team->address_space->RemoveAndPut();
err3:
	delete_realtime_sem_context(team->realtime_sem_context);
err25:
	vfs_put_io_context(team->io_context);
err2:
	free(forkArgs);
err1:
	// remove the team structure from the team hash table and delete the team
	// structure
	teamLocker.Lock();

	remove_team_from_group(team);
	remove_team_from_parent(parentTeam, team);
	sTeamHash.RemoveUnchecked(team);

	teamLocker.Unlock();

	delete_team_struct(team);

	return status;
}


/*!	Returns if the specified \a team has any children belonging to the
	specified \a group.
	Must be called with the team lock held.
*/
static bool
has_children_in_group(Team* parent, pid_t groupID)
{
	Team* team;

	struct process_group* group = team_get_process_group_locked(
		parent->group->session, groupID);
	if (group == NULL)
		return false;

	for (team = group->teams; team; team = team->group_next) {
		if (team->parent == parent)
			return true;
	}

	return false;
}


static job_control_entry*
get_job_control_entry(team_job_control_children& children, pid_t id)
{
	for (JobControlEntryList::Iterator it = children.entries.GetIterator();
		 job_control_entry* entry = it.Next();) {

		if (id > 0) {
			if (entry->thread == id)
				return entry;
		} else if (id == -1) {
			return entry;
		} else {
			pid_t processGroup
				= (entry->team ? entry->team->group_id : entry->group_id);
			if (processGroup == -id)
				return entry;
		}
	}

	return NULL;
}


static job_control_entry*
get_job_control_entry(Team* team, pid_t id, uint32 flags)
{
	job_control_entry* entry = get_job_control_entry(team->dead_children, id);

	if (entry == NULL && (flags & WCONTINUED) != 0)
		entry = get_job_control_entry(team->continued_children, id);

	if (entry == NULL && (flags & WUNTRACED) != 0)
		entry = get_job_control_entry(team->stopped_children, id);

	return entry;
}


job_control_entry::job_control_entry()
	:
	has_group_ref(false)
{
}


job_control_entry::~job_control_entry()
{
	if (has_group_ref) {
		InterruptsSpinLocker locker(gTeamSpinlock);
		release_process_group_ref(group_id);
	}
}


/*!	Team and thread lock must be held.
*/
void
job_control_entry::InitDeadState()
{
	if (team != NULL) {
		Thread* thread = team->main_thread;
		group_id = team->group_id;
		this->thread = thread->id;
		status = thread->exit.status;
		reason = thread->exit.reason;
		signal = thread->exit.signal;
		team = NULL;
		acquire_process_group_ref(group_id);
		has_group_ref = true;
	}
}


job_control_entry&
job_control_entry::operator=(const job_control_entry& other)
{
	state = other.state;
	thread = other.thread;
	has_group_ref = false;
	team = other.team;
	group_id = other.group_id;
	status = other.status;
	reason = other.reason;
	signal = other.signal;

	return *this;
}


/*! This is the kernel backend for waitpid(). It is a bit more powerful when it
	comes to the reason why a thread has died than waitpid() can be.
*/
static thread_id
wait_for_child(pid_t child, uint32 flags, int32* _reason,
	status_t* _returnCode)
{
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;
	struct job_control_entry foundEntry;
	struct job_control_entry* freeDeathEntry = NULL;
	status_t status = B_OK;

	TRACE(("wait_for_child(child = %ld, flags = %ld)\n", child, flags));

	T(WaitForChild(child, flags));

	if (child == 0) {
		// wait for all children in the process group of the calling team
		child = -team->group_id;
	}

	bool ignoreFoundEntries = false;
	bool ignoreFoundEntriesChecked = false;

	while (true) {
		InterruptsSpinLocker locker(gTeamSpinlock);

		// check whether any condition holds
		job_control_entry* entry = get_job_control_entry(team, child, flags);

		// If we don't have an entry yet, check whether there are any children
		// complying to the process group specification at all.
		if (entry == NULL) {
			// No success yet -- check whether there are any children we could
			// wait for.
			bool childrenExist = false;
			if (child == -1) {
				childrenExist = team->children != NULL;
			} else if (child < -1) {
				childrenExist = has_children_in_group(team, -child);
			} else {
				if (Team* childTeam = team_get_team_struct_locked(child))
					childrenExist = childTeam->parent == team;
			}

			if (!childrenExist) {
				// there is no child we could wait for
				status = ECHILD;
			} else {
				// the children we're waiting for are still running
				status = B_WOULD_BLOCK;
			}
		} else {
			// got something
			foundEntry = *entry;
			if (entry->state == JOB_CONTROL_STATE_DEAD) {
				// The child is dead. Reap its death entry.
				freeDeathEntry = entry;
				team->dead_children.entries.Remove(entry);
				team->dead_children.count--;
			} else {
				// The child is well. Reset its job control state.
				team_set_job_control_state(entry->team,
					JOB_CONTROL_STATE_NONE, 0, false);
			}
		}

		// If we haven't got anything yet, prepare for waiting for the
		// condition variable.
		ConditionVariableEntry deadWaitEntry;

		if (status == B_WOULD_BLOCK && (flags & WNOHANG) == 0)
			team->dead_children.condition_variable.Add(&deadWaitEntry);

		locker.Unlock();

		// we got our entry and can return to our caller
		if (status == B_OK) {
			if (ignoreFoundEntries) {
				// ... unless we shall ignore found entries
				delete freeDeathEntry;
				freeDeathEntry = NULL;
				continue;
			}

			break;
		}

		if (status != B_WOULD_BLOCK || (flags & WNOHANG) != 0) {
			T(WaitForChildDone(status));
			return status;
		}

		status = deadWaitEntry.Wait(B_CAN_INTERRUPT);
		if (status == B_INTERRUPTED) {
			T(WaitForChildDone(status));
			return status;
		}

		// If SA_NOCLDWAIT is set or SIGCHLD is ignored, we shall wait until
		// all our children are dead and fail with ECHILD. We check the
		// condition at this point.
		if (!ignoreFoundEntriesChecked) {
			struct sigaction& handler = thread->sig_action[SIGCHLD - 1];
			if ((handler.sa_flags & SA_NOCLDWAIT) != 0
				|| handler.sa_handler == SIG_IGN) {
				ignoreFoundEntries = true;
			}

			ignoreFoundEntriesChecked = true;
		}
	}

	delete freeDeathEntry;

	// when we got here, we have a valid death entry, and
	// already got unregistered from the team or group
	int reason = 0;
	switch (foundEntry.state) {
		case JOB_CONTROL_STATE_DEAD:
			reason = foundEntry.reason;
			break;
		case JOB_CONTROL_STATE_STOPPED:
			reason = THREAD_STOPPED;
			break;
		case JOB_CONTROL_STATE_CONTINUED:
			reason = THREAD_CONTINUED;
			break;
		case JOB_CONTROL_STATE_NONE:
			// can't happen
			break;
	}

	*_returnCode = foundEntry.status;
	*_reason = (foundEntry.signal << 16) | reason;

	// If SIGCHLD is blocked, we shall clear pending SIGCHLDs, if no other child
	// status is available.
	if (is_signal_blocked(SIGCHLD)) {
		InterruptsSpinLocker locker(gTeamSpinlock);

		if (get_job_control_entry(team, child, flags) == NULL)
			atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(SIGCHLD));
	}

	// When the team is dead, the main thread continues to live in the kernel
	// team for a very short time. To avoid surprises for the caller we rather
	// wait until the thread is really gone.
	if (foundEntry.state == JOB_CONTROL_STATE_DEAD)
		wait_for_thread(foundEntry.thread, NULL);

	T(WaitForChildDone(foundEntry));

	return foundEntry.thread;
}


/*! Fills the team_info structure with information from the specified
	team.
	The team lock must be held when called.
*/
static status_t
fill_team_info(Team* team, team_info* info, size_t size)
{
	if (size != sizeof(team_info))
		return B_BAD_VALUE;

	// ToDo: Set more informations for team_info
	memset(info, 0, size);

	info->team = team->id;
	info->thread_count = team->num_threads;
	info->image_count = count_images(team);
	//info->area_count =
	info->debugger_nub_thread = team->debug_info.nub_thread;
	info->debugger_nub_port = team->debug_info.nub_port;
	//info->uid =
	//info->gid =

	strlcpy(info->args, team->args, sizeof(info->args));
	info->argc = 1;

	return B_OK;
}


/*!	Updates the \c orphaned field of a process_group and returns its new value.
	Interrupts must be disabled and team lock be held.
*/
static bool
update_orphaned_process_group(process_group* group, pid_t dyingProcess)
{
	// Orphaned Process Group: "A process group in which the parent of every
	// member is either itself a member of the group or is not a member of the
	// group's session." (Open Group Base Specs Issue 6)

	// once orphaned, things won't change (exception: cf. setpgid())
	if (group->orphaned)
		return true;

	Team* team = group->teams;
	while (team != NULL) {
		Team* parent = team->parent;
		if (team->id != dyingProcess && parent != NULL
			&& parent->id != dyingProcess
			&& parent->group_id != group->id
			&& parent->session_id == group->session->id) {
			return false;
		}

		team = team->group_next;
	}

	group->orphaned = true;
	return true;
}


/*!	Returns whether the process group contains stopped processes.
	Interrupts must be disabled and team lock be held.
*/
static bool
process_group_has_stopped_processes(process_group* group)
{
	SpinLocker _(gThreadSpinlock);

	Team* team = group->teams;
	while (team != NULL) {
		if (team->main_thread->state == B_THREAD_SUSPENDED)
			return true;

		team = team->group_next;
	}

	return false;
}


//	#pragma mark - Private kernel API


status_t
team_init(kernel_args* args)
{
	struct process_session* session;
	struct process_group* group;

	// create the team hash table
	new(&sTeamHash) TeamHashTable;
	if (sTeamHash.Init(32) != B_OK)
		panic("Failed to init team hash table!");

	sGroupHash = hash_init(16, offsetof(struct process_group, next),
		&process_group_compare, &process_group_hash);

	// create initial session and process groups

	session = create_process_session(1);
	if (session == NULL)
		panic("Could not create initial session.\n");

	group = create_process_group(1);
	if (group == NULL)
		panic("Could not create initial process group.\n");

	insert_group_into_session(session, group);

	// create the kernel team
	sKernelTeam = create_team_struct("kernel_team", true);
	if (sKernelTeam == NULL)
		panic("could not create kernel team!\n");
	strcpy(sKernelTeam->args, sKernelTeam->name);
	sKernelTeam->state = TEAM_STATE_NORMAL;

	sKernelTeam->saved_set_uid = 0;
	sKernelTeam->real_uid = 0;
	sKernelTeam->effective_uid = 0;
	sKernelTeam->saved_set_gid = 0;
	sKernelTeam->real_gid = 0;
	sKernelTeam->effective_gid = 0;
	sKernelTeam->supplementary_groups = NULL;
	sKernelTeam->supplementary_group_count = 0;

	insert_team_into_group(group, sKernelTeam);

	sKernelTeam->io_context = vfs_new_io_context(NULL, false);
	if (sKernelTeam->io_context == NULL)
		panic("could not create io_context for kernel team!\n");

	// stick it in the team hash
	sTeamHash.InsertUnchecked(sKernelTeam);

	add_debugger_command_etc("team", &dump_team_info,
		"Dump info about a particular team",
		"[ <id> | <address> | <name> ]\n"
		"Prints information about the specified team. If no argument is given\n"
		"the current team is selected.\n"
		"  <id>       - The ID of the team.\n"
		"  <address>  - The address of the team structure.\n"
		"  <name>     - The team's name.\n", 0);
	add_debugger_command_etc("teams", &dump_teams, "List all teams",
		"\n"
		"Prints a list of all existing teams.\n", 0);

	new(&sNotificationService) TeamNotificationService();

	return B_OK;
}


int32
team_max_teams(void)
{
	return sMaxTeams;
}


int32
team_used_teams(void)
{
	return sUsedTeams;
}


/*!	Iterates through the list of teams. The team spinlock must be held.
*/
Team*
team_iterate_through_teams(team_iterator_callback callback, void* cookie)
{
	for (TeamHashTable::Iterator it = sTeamHash.GetIterator();
		Team* team = it.Next();) {
		if (callback(team, cookie))
			return team;
	}

	return NULL;
}


/*! Fills the provided death entry if it's in the team.
	You need to have the team lock held when calling this function.
*/
job_control_entry*
team_get_death_entry(Team* team, thread_id child, bool* _deleteEntry)
{
	if (child <= 0)
		return NULL;

	job_control_entry* entry = get_job_control_entry(team->dead_children,
		child);
	if (entry) {
		// remove the entry only, if the caller is the parent of the found team
		if (team_get_current_team_id() == entry->thread) {
			team->dead_children.entries.Remove(entry);
			team->dead_children.count--;
			*_deleteEntry = true;
		} else {
			*_deleteEntry = false;
		}
	}

	return entry;
}


/*! Quick check to see if we have a valid team ID. */
bool
team_is_valid(team_id id)
{
	Team* team;
	cpu_status state;

	if (id <= 0)
		return false;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return team != NULL;
}


Team*
team_get_team_struct_locked(team_id id)
{
	return sTeamHash.Lookup(id);
}


/*! This searches the session of the team for the specified group ID.
	You must hold the team lock when you call this function.
*/
struct process_group*
team_get_process_group_locked(struct process_session* session, pid_t id)
{
	struct process_group* group;
	struct team_key key;
	key.id = id;

	group = (struct process_group*)hash_lookup(sGroupHash, &key);
	if (group != NULL && (session == NULL || session == group->session))
		return group;

	return NULL;
}


void
team_delete_process_group(struct process_group* group)
{
	if (group == NULL)
		return;

	TRACE(("team_delete_process_group(id = %ld)\n", group->id));

	// remove_group_from_session() keeps this pointer around
	// only if the session can be freed as well
	if (group->session) {
		TRACE(("team_delete_process_group(): frees session %ld\n",
			group->session->id));
		free(group->session);
	}

	free(group);
}


void
team_set_controlling_tty(int32 ttyIndex)
{
	Team* team = thread_get_current_thread()->team;

	InterruptsSpinLocker _(gTeamSpinlock);

	team->group->session->controlling_tty = ttyIndex;
	team->group->session->foreground_group = -1;
}


int32
team_get_controlling_tty()
{
	Team* team = thread_get_current_thread()->team;

	InterruptsSpinLocker _(gTeamSpinlock);

	return team->group->session->controlling_tty;
}


status_t
team_set_foreground_process_group(int32 ttyIndex, pid_t processGroupID)
{
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;

	InterruptsSpinLocker locker(gTeamSpinlock);

	process_session* session = team->group->session;

	// must be the controlling tty of the calling process
	if (session->controlling_tty != ttyIndex)
		return ENOTTY;

	// check process group -- must belong to our session
	process_group* group = team_get_process_group_locked(session,
		processGroupID);
	if (group == NULL)
		return B_BAD_VALUE;

	// If we are a background group, we can't do that unharmed, only if we
	// ignore or block SIGTTOU. Otherwise the group gets a SIGTTOU.
	if (session->foreground_group != -1
		&& session->foreground_group != team->group_id
		&& thread->sig_action[SIGTTOU - 1].sa_handler != SIG_IGN
		&& !is_signal_blocked(SIGTTOU)) {
		pid_t groupID = team->group->id;
		locker.Unlock();
		send_signal(-groupID, SIGTTOU);
		return B_INTERRUPTED;
	}

	team->group->session->foreground_group = processGroupID;

	return B_OK;
}


/*!	Removes the specified team from the global team hash, and from its parent.
	It also moves all of its children up to the parent.
	You must hold the team lock when you call this function.
*/
void
team_remove_team(Team* team)
{
	Team* parent = team->parent;

	// remember how long this team lasted
	parent->dead_children.kernel_time += team->dead_threads_kernel_time
		+ team->dead_children.kernel_time;
	parent->dead_children.user_time += team->dead_threads_user_time
		+ team->dead_children.user_time;

	// Also grab the thread spinlock while removing the team from the hash.
	// This makes the following sequence safe: grab teams lock, lookup team,
	// grab threads lock, unlock teams lock,
	// mutex_lock_threads_lock(<team related lock>), as used in the VFS code to
	// lock another team's IO context.
	GRAB_THREAD_LOCK();
	sTeamHash.RemoveUnchecked(team);
	RELEASE_THREAD_LOCK();
	sUsedTeams--;

	team->state = TEAM_STATE_DEATH;

	// If we're a controlling process (i.e. a session leader with controlling
	// terminal), there's a bit of signalling we have to do.
	if (team->session_id == team->id
		&& team->group->session->controlling_tty >= 0) {
		process_session* session = team->group->session;

		session->controlling_tty = -1;

		// send SIGHUP to the foreground
		if (session->foreground_group >= 0) {
			send_signal_etc(-session->foreground_group, SIGHUP,
				SIGNAL_FLAG_TEAMS_LOCKED);
		}

		// send SIGHUP + SIGCONT to all newly-orphaned process groups with
		// stopped processes
		Team* child = team->children;
		while (child != NULL) {
			process_group* childGroup = child->group;
			if (!childGroup->orphaned
				&& update_orphaned_process_group(childGroup, team->id)
				&& process_group_has_stopped_processes(childGroup)) {
				send_signal_etc(-childGroup->id, SIGHUP,
					SIGNAL_FLAG_TEAMS_LOCKED);
				send_signal_etc(-childGroup->id, SIGCONT,
					SIGNAL_FLAG_TEAMS_LOCKED);
			}

			child = child->siblings_next;
		}
	} else {
		// update "orphaned" flags of all children's process groups
		Team* child = team->children;
		while (child != NULL) {
			process_group* childGroup = child->group;
			if (!childGroup->orphaned)
				update_orphaned_process_group(childGroup, team->id);

			child = child->siblings_next;
		}

		// update "orphaned" flag of this team's process group
		update_orphaned_process_group(team->group, team->id);
	}

	// reparent each of the team's children
	reparent_children(team);

	// remove us from our process group
	remove_team_from_group(team);

	// remove us from our parent
	remove_team_from_parent(parent, team);
}


/*!	Kills all threads but the main thread of the team.
	To be called on exit of the team's main thread. The teams spinlock must be
	held. The function may temporarily drop the spinlock, but will reacquire it
	before it returns.
	\param team The team in question.
	\param state The CPU state as returned by disable_interrupts(). Will be
		adjusted, if the function needs to unlock and relock.
	\return The port of the debugger for the team, -1 if none. To be passed to
		team_delete_team().
*/
port_id
team_shutdown_team(Team* team, cpu_status& state)
{
	ASSERT(thread_get_current_thread() == team->main_thread);

	// Make sure debugging changes won't happen anymore.
	port_id debuggerPort = -1;
	while (true) {
		// If a debugger change is in progress for the team, we'll have to
		// wait until it is done.
		ConditionVariableEntry waitForDebuggerEntry;
		bool waitForDebugger = false;

		GRAB_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		if (team->debug_info.debugger_changed_condition != NULL) {
			team->debug_info.debugger_changed_condition->Add(
				&waitForDebuggerEntry);
			waitForDebugger = true;
		} else if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// The team is being debugged. That will stop with the termination
			// of the nub thread. Since we won't let go of the team lock, unless
			// we set team::death_entry or until we have removed the tem from
			// the team hash, no-one can install a debugger anymore. We fetch
			// the debugger's port to send it a message at the bitter end.
			debuggerPort = team->debug_info.debugger_port;
		}

		RELEASE_TEAM_DEBUG_INFO_LOCK(team->debug_info);

		if (!waitForDebugger)
			break;

		// wait for the debugger change to be finished
		RELEASE_TEAM_LOCK();
		restore_interrupts(state);

		waitForDebuggerEntry.Wait();

		state = disable_interrupts();
		GRAB_TEAM_LOCK();
	}

	// kill all threads but the main thread
	team_death_entry deathEntry;
	deathEntry.condition.Init(team, "team death");

	while (true) {
		team->death_entry = &deathEntry;
		deathEntry.remaining_threads = 0;

		Thread* thread = team->thread_list;
		while (thread != NULL) {
			if (thread != team->main_thread) {
				send_signal_etc(thread->id, SIGKILLTHR,
					B_DO_NOT_RESCHEDULE | SIGNAL_FLAG_TEAMS_LOCKED);
				deathEntry.remaining_threads++;
			}

			thread = thread->team_next;
		}

		if (deathEntry.remaining_threads == 0)
			break;

		// there are threads to wait for
		ConditionVariableEntry entry;
		deathEntry.condition.Add(&entry);

		RELEASE_TEAM_LOCK();
		restore_interrupts(state);

		entry.Wait();

		state = disable_interrupts();
		GRAB_TEAM_LOCK();
	}

	team->death_entry = NULL;
		// That makes the team "undead" again, but we have the teams spinlock
		// and our caller won't drop it until after removing the team from the
		// teams hash table.

	return debuggerPort;
}


void
team_delete_team(Team* team, port_id debuggerPort)
{
	team_id teamID = team->id;

	ASSERT(team->num_threads == 0);

	// If someone is waiting for this team to be loaded, but it dies
	// unexpectedly before being done, we need to notify the waiting
	// thread now.

	cpu_status state = disable_interrupts();
	GRAB_TEAM_LOCK();

	if (team->loading_info) {
		// there's indeed someone waiting
		struct team_loading_info* loadingInfo = team->loading_info;
		team->loading_info = NULL;

		loadingInfo->result = B_ERROR;
		loadingInfo->done = true;

		GRAB_THREAD_LOCK();

		// wake up the waiting thread
		if (loadingInfo->thread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(loadingInfo->thread);

		RELEASE_THREAD_LOCK();
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	// notify team watchers

	{
		// we're not reachable from anyone anymore at this point, so we
		// can safely access the list without any locking
		struct team_watcher* watcher;
		while ((watcher = (struct team_watcher*)list_remove_head_item(
				&team->watcher_list)) != NULL) {
			watcher->hook(teamID, watcher->data);
			free(watcher);
		}
	}

	sNotificationService.Notify(TEAM_REMOVED, team);

	// free team resources

	vfs_put_io_context(team->io_context);
	delete_realtime_sem_context(team->realtime_sem_context);
	xsi_sem_undo(team);
	delete_owned_ports(team);
	sem_delete_owned_sems(team);
	remove_images(team);
	team->address_space->RemoveAndPut();

	delete_team_struct(team);

	// notify the debugger, that the team is gone
	user_debug_team_deleted(teamID, debuggerPort);
}


Team*
team_get_kernel_team(void)
{
	return sKernelTeam;
}


team_id
team_get_kernel_team_id(void)
{
	if (!sKernelTeam)
		return 0;

	return sKernelTeam->id;
}


team_id
team_get_current_team_id(void)
{
	return thread_get_current_thread()->team->id;
}


status_t
team_get_address_space(team_id id, VMAddressSpace** _addressSpace)
{
	cpu_status state;
	Team* team;
	status_t status;

	// ToDo: we need to do something about B_SYSTEM_TEAM vs. its real ID (1)
	if (id == 1) {
		// we're the kernel team, so we don't have to go through all
		// the hassle (locking and hash lookup)
		*_addressSpace = VMAddressSpace::GetKernel();
		return B_OK;
	}

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team != NULL) {
		team->address_space->Get();
		*_addressSpace = team->address_space;
		status = B_OK;
	} else
		status = B_BAD_VALUE;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
}


/*!	Sets the team's job control state.
	Interrupts must be disabled and the team lock be held.
	\a threadsLocked indicates whether the thread lock is being held, too.
*/
void
team_set_job_control_state(Team* team, job_control_state newState,
	int signal, bool threadsLocked)
{
	if (team == NULL || team->job_control_entry == NULL)
		return;

	// don't touch anything, if the state stays the same or the team is already
	// dead
	job_control_entry* entry = team->job_control_entry;
	if (entry->state == newState || entry->state == JOB_CONTROL_STATE_DEAD)
		return;

	T(SetJobControlState(team->id, newState, signal));

	// remove from the old list
	switch (entry->state) {
		case JOB_CONTROL_STATE_NONE:
			// entry is in no list ATM
			break;
		case JOB_CONTROL_STATE_DEAD:
			// can't get here
			break;
		case JOB_CONTROL_STATE_STOPPED:
			team->parent->stopped_children.entries.Remove(entry);
			break;
		case JOB_CONTROL_STATE_CONTINUED:
			team->parent->continued_children.entries.Remove(entry);
			break;
	}

	entry->state = newState;
	entry->signal = signal;

	// add to new list
	team_job_control_children* childList = NULL;
	switch (entry->state) {
		case JOB_CONTROL_STATE_NONE:
			// entry doesn't get into any list
			break;
		case JOB_CONTROL_STATE_DEAD:
			childList = &team->parent->dead_children;
			team->parent->dead_children.count++;
			break;
		case JOB_CONTROL_STATE_STOPPED:
			childList = &team->parent->stopped_children;
			break;
		case JOB_CONTROL_STATE_CONTINUED:
			childList = &team->parent->continued_children;
			break;
	}

	if (childList != NULL) {
		childList->entries.Add(entry);
		team->parent->dead_children.condition_variable.NotifyAll(
			threadsLocked);
	}
}


/*! Adds a hook to the team that is called as soon as this
	team goes away.
	This call might get public in the future.
*/
status_t
start_watching_team(team_id teamID, void (*hook)(team_id, void*), void* data)
{
	struct team_watcher* watcher;
	Team* team;
	cpu_status state;

	if (hook == NULL || teamID < B_OK)
		return B_BAD_VALUE;

	watcher = (struct team_watcher*)malloc(sizeof(struct team_watcher));
	if (watcher == NULL)
		return B_NO_MEMORY;

	watcher->hook = hook;
	watcher->data = data;

	// find team and add watcher

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team != NULL)
		list_add_item(&team->watcher_list, watcher);

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (team == NULL) {
		free(watcher);
		return B_BAD_TEAM_ID;
	}

	return B_OK;
}


status_t
stop_watching_team(team_id teamID, void (*hook)(team_id, void*), void* data)
{
	struct team_watcher* watcher = NULL;
	Team* team;
	cpu_status state;

	if (hook == NULL || teamID < B_OK)
		return B_BAD_VALUE;

	// find team and remove watcher (if present)

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(teamID);
	if (team != NULL) {
		// search for watcher
		while ((watcher = (struct team_watcher*)list_get_next_item(
				&team->watcher_list, watcher)) != NULL) {
			if (watcher->hook == hook && watcher->data == data) {
				// got it!
				list_remove_item(&team->watcher_list, watcher);
				break;
			}
		}
	}

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (watcher == NULL)
		return B_ENTRY_NOT_FOUND;

	free(watcher);
	return B_OK;
}


/*!	The team lock must be held or the team must still be single threaded.
*/
struct user_thread*
team_allocate_user_thread(Team* team)
{
	if (team->user_data == 0)
		return NULL;

	user_thread* thread = NULL;

	// take an entry from the free list, if any
	if (struct free_user_thread* entry = team->free_user_threads) {
		thread = entry->thread;
		team->free_user_threads = entry->next;
		deferred_free(entry);
		return thread;
	} else {
		// enough space left?
		size_t needed = _ALIGN(sizeof(user_thread));
		if (team->user_data_size - team->used_user_data < needed)
			return NULL;
		// TODO: This imposes a per team thread limit! We should resize the
		// area, if necessary. That's problematic at this point, though, since
		// we've got the team lock.

		thread = (user_thread*)(team->user_data + team->used_user_data);
		team->used_user_data += needed;
	}

	thread->defer_signals = 0;
	thread->pending_signals = 0;
	thread->wait_status = B_OK;

	return thread;
}


/*!	The team lock must not be held. \a thread must be the current thread.
*/
void
team_free_user_thread(Thread* thread)
{
	user_thread* userThread = thread->user_thread;
	if (userThread == NULL)
		return;

	// create a free list entry
	free_user_thread* entry
		= (free_user_thread*)malloc(sizeof(free_user_thread));
	if (entry == NULL) {
		// we have to leak the user thread :-/
		return;
	}

	InterruptsSpinLocker _(gTeamSpinlock);

	// detach from thread
	SpinLocker threadLocker(gThreadSpinlock);
	thread->user_thread = NULL;
	threadLocker.Unlock();

	entry->thread = userThread;
	entry->next = thread->team->free_user_threads;
	thread->team->free_user_threads = entry;
}


//	#pragma mark - Associated data interface


AssociatedData::AssociatedData()
	:
	fOwner(NULL)
{
}


AssociatedData::~AssociatedData()
{
}


void
AssociatedData::OwnerDeleted(AssociatedDataOwner* owner)
{
}


AssociatedDataOwner::AssociatedDataOwner()
{
	mutex_init(&fLock, "associated data owner");
}


AssociatedDataOwner::~AssociatedDataOwner()
{
	mutex_destroy(&fLock);
}


bool
AssociatedDataOwner::AddData(AssociatedData* data)
{
	MutexLocker locker(fLock);

	if (data->Owner() != NULL)
		return false;

	data->AcquireReference();
	fList.Add(data);
	data->SetOwner(this);

	return true;
}


bool
AssociatedDataOwner::RemoveData(AssociatedData* data)
{
	MutexLocker locker(fLock);

	if (data->Owner() != this)
		return false;

	data->SetOwner(NULL);
	fList.Remove(data);

	locker.Unlock();

	data->ReleaseReference();

	return true;
}


void
AssociatedDataOwner::PrepareForDeletion()
{
	MutexLocker locker(fLock);

	// move all data to a temporary list and unset the owner
	DataList list;
	list.MoveFrom(&fList);

	for (DataList::Iterator it = list.GetIterator();
		AssociatedData* data = it.Next();) {
		data->SetOwner(NULL);
	}

	locker.Unlock();

	// call the notification hooks and release our references
	while (AssociatedData* data = list.RemoveHead()) {
		data->OwnerDeleted(this);
		data->ReleaseReference();
	}
}


/*!	Associates data with the current team.
	When the team is deleted, the data object is notified.
	The team acquires a reference to the object.

	\param data The data object.
	\return \c true on success, \c false otherwise. Fails only when the supplied
		data object is already associated with another owner.
*/
bool
team_associate_data(AssociatedData* data)
{
	return thread_get_current_thread()->team->AddData(data);
}


/*!	Dissociates data from the current team.
	Balances an earlier call to team_associate_data().

	\param data The data object.
	\return \c true on success, \c false otherwise. Fails only when the data
		object is not associated with the current team.
*/
bool
team_dissociate_data(AssociatedData* data)
{
	return thread_get_current_thread()->team->RemoveData(data);
}


//	#pragma mark - Public kernel API


thread_id
load_image(int32 argCount, const char** args, const char** env)
{
	return load_image_etc(argCount, args, env, B_NORMAL_PRIORITY,
		B_CURRENT_TEAM, B_WAIT_TILL_LOADED);
}


thread_id
load_image_etc(int32 argCount, const char* const* args,
	const char* const* env, int32 priority, team_id parentID, uint32 flags)
{
	// we need to flatten the args and environment

	if (args == NULL)
		return B_BAD_VALUE;

	// determine total needed size
	int32 argSize = 0;
	for (int32 i = 0; i < argCount; i++)
		argSize += strlen(args[i]) + 1;

	int32 envCount = 0;
	int32 envSize = 0;
	while (env != NULL && env[envCount] != NULL)
		envSize += strlen(env[envCount++]) + 1;

	int32 size = (argCount + envCount + 2) * sizeof(char*) + argSize + envSize;
	if (size > MAX_PROCESS_ARGS_SIZE)
		return B_TOO_MANY_ARGS;

	// allocate space
	char** flatArgs = (char**)malloc(size);
	if (flatArgs == NULL)
		return B_NO_MEMORY;

	char** slot = flatArgs;
	char* stringSpace = (char*)(flatArgs + argCount + envCount + 2);

	// copy arguments and environment
	for (int32 i = 0; i < argCount; i++) {
		int32 argSize = strlen(args[i]) + 1;
		memcpy(stringSpace, args[i], argSize);
		*slot++ = stringSpace;
		stringSpace += argSize;
	}

	*slot++ = NULL;

	for (int32 i = 0; i < envCount; i++) {
		int32 envSize = strlen(env[i]) + 1;
		memcpy(stringSpace, env[i], envSize);
		*slot++ = stringSpace;
		stringSpace += envSize;
	}

	*slot++ = NULL;

	thread_id thread = load_image_internal(flatArgs, size, argCount, envCount,
		B_NORMAL_PRIORITY, parentID, B_WAIT_TILL_LOADED, -1, 0);

	free(flatArgs);
		// load_image_internal() unset our variable if it took over ownership

	return thread;
}


status_t
wait_for_team(team_id id, status_t* _returnCode)
{
	Team* team;
	thread_id thread;
	cpu_status state;

	// find main thread and wait for that

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team != NULL && team->main_thread != NULL)
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
	status_t status = B_OK;
	thread_id threadID = -1;
	Team* team;
	cpu_status state;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	team = team_get_team_struct_locked(id);
	if (team != NULL) {
		if (team != sKernelTeam) {
			threadID = team->id;
				// the team ID is the same as the ID of its main thread
		} else
			status = B_NOT_ALLOWED;
	} else
		status = B_BAD_THREAD_ID;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (status < B_OK)
		return status;

	// just kill the main thread in the team. The cleanup code there will
	// take care of the team
	return kill_thread(threadID);
}


status_t
_get_team_info(team_id id, team_info* info, size_t size)
{
	cpu_status state;
	status_t status = B_OK;
	Team* team;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	if (id == B_CURRENT_TEAM)
		team = thread_get_current_thread()->team;
	else
		team = team_get_team_struct_locked(id);

	if (team == NULL) {
		status = B_BAD_TEAM_ID;
		goto err;
	}

	status = fill_team_info(team, info, size);

err:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return status;
}


status_t
_get_next_team_info(int32* cookie, team_info* info, size_t size)
{
	status_t status = B_BAD_TEAM_ID;
	Team* team = NULL;
	int32 slot = *cookie;
	team_id lastTeamID;
	cpu_status state;

	if (slot < 1)
		slot = 1;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	lastTeamID = peek_next_thread_id();
	if (slot >= lastTeamID)
		goto err;

	// get next valid team
	while (slot < lastTeamID && !(team = team_get_team_struct_locked(slot)))
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


status_t
_get_team_usage_info(team_id id, int32 who, team_usage_info* info, size_t size)
{
	bigtime_t kernelTime = 0, userTime = 0;
	status_t status = B_OK;
	Team* team;
	cpu_status state;

	if (size != sizeof(team_usage_info)
		|| (who != B_TEAM_USAGE_SELF && who != B_TEAM_USAGE_CHILDREN))
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	if (id == B_CURRENT_TEAM)
		team = thread_get_current_thread()->team;
	else
		team = team_get_team_struct_locked(id);

	if (team == NULL) {
		status = B_BAD_TEAM_ID;
		goto out;
	}

	switch (who) {
		case B_TEAM_USAGE_SELF:
		{
			Thread* thread = team->thread_list;

			for (; thread != NULL; thread = thread->team_next) {
				kernelTime += thread->kernel_time;
				userTime += thread->user_time;
			}

			kernelTime += team->dead_threads_kernel_time;
			userTime += team->dead_threads_user_time;
			break;
		}

		case B_TEAM_USAGE_CHILDREN:
		{
			Team* child = team->children;
			for (; child != NULL; child = child->siblings_next) {
				Thread* thread = team->thread_list;

				for (; thread != NULL; thread = thread->team_next) {
					kernelTime += thread->kernel_time;
					userTime += thread->user_time;
				}

				kernelTime += child->dead_threads_kernel_time;
				userTime += child->dead_threads_user_time;
			}

			kernelTime += team->dead_children.kernel_time;
			userTime += team->dead_children.user_time;
			break;
		}
	}

out:
	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	if (status == B_OK) {
		info->kernel_time = kernelTime;
		info->user_time = userTime;
	}

	return status;
}


pid_t
getpid(void)
{
	return thread_get_current_thread()->team->id;
}


pid_t
getppid(void)
{
	Team* team = thread_get_current_thread()->team;
	cpu_status state;
	pid_t parent;

	state = disable_interrupts();
	GRAB_TEAM_LOCK();

	parent = team->parent->id;

	RELEASE_TEAM_LOCK();
	restore_interrupts(state);

	return parent;
}


pid_t
getpgid(pid_t process)
{
	Thread* thread;
	pid_t result = -1;
	cpu_status state;

	if (process == 0)
		process = thread_get_current_thread()->team->id;

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
	Thread* thread;
	pid_t result = -1;
	cpu_status state;

	if (process == 0)
		process = thread_get_current_thread()->team->id;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(process);
	if (thread != NULL)
		result = thread->team->session_id;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return thread != NULL ? result : B_BAD_VALUE;
}


//	#pragma mark - User syscalls


status_t
_user_exec(const char* userPath, const char* const* userFlatArgs,
	size_t flatArgsSize, int32 argCount, int32 envCount, mode_t umask)
{
	// NOTE: Since this function normally doesn't return, don't use automatic
	// variables that need destruction in the function scope.
	char path[B_PATH_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userPath) || !IS_USER_ADDRESS(userFlatArgs)
		|| user_strlcpy(path, userPath, sizeof(path)) < B_OK)
		return B_BAD_ADDRESS;

	// copy and relocate the flat arguments
	char** flatArgs;
	status_t error = copy_user_process_args(userFlatArgs, flatArgsSize,
		argCount, envCount, flatArgs);

	if (error == B_OK) {
		error = exec_team(path, flatArgs, _ALIGN(flatArgsSize), argCount,
			envCount, umask);
			// this one only returns in case of error
	}

	free(flatArgs);
	return error;
}


thread_id
_user_fork(void)
{
	return fork_team();
}


thread_id
_user_wait_for_child(thread_id child, uint32 flags, int32* _userReason,
	status_t* _userReturnCode)
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
		if ((_userReason != NULL
				&& user_memcpy(_userReason, &reason, sizeof(int32)) < B_OK)
			|| (_userReturnCode != NULL
				&& user_memcpy(_userReturnCode, &returnCode, sizeof(status_t))
					< B_OK)) {
			return B_BAD_ADDRESS;
		}

		return deadChild;
	}

	return syscall_restart_handle_post(deadChild);
}


pid_t
_user_process_info(pid_t process, int32 which)
{
	// we only allow to return the parent of the current process
	if (which == PARENT_ID
		&& process != 0 && process != thread_get_current_thread()->team->id)
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
	Thread* thread = thread_get_current_thread();
	Team* currentTeam = thread->team;
	Team* team;

	if (groupID < 0)
		return B_BAD_VALUE;

	if (processID == 0)
		processID = currentTeam->id;

	// if the group ID is not specified, use the target process' ID
	if (groupID == 0)
		groupID = processID;

	if (processID == currentTeam->id) {
		// we set our own group

		// we must not change our process group ID if we're a session leader
		if (is_session_leader(currentTeam))
			return B_NOT_ALLOWED;
	} else {
		// another team is the target of the call -- check it out
		InterruptsSpinLocker _(gTeamSpinlock);

		team = team_get_team_struct_locked(processID);
		if (team == NULL)
			return ESRCH;

		// The team must be a child of the calling team and in the same session.
		// (If that's the case it isn't a session leader either.)
		if (team->parent != currentTeam
			|| team->session_id != currentTeam->session_id) {
			return B_NOT_ALLOWED;
		}

		if (team->group_id == groupID)
			return groupID;

		// The call is also supposed to fail on a child, when the child already
		// has executed exec*() [EACCES].
		if ((team->flags & TEAM_FLAG_EXEC_DONE) != 0)
			return EACCES;
	}

	struct process_group* group = NULL;
	if (groupID == processID) {
		// A new process group might be needed.
		group = create_process_group(groupID);
		if (group == NULL)
			return B_NO_MEMORY;

		// Assume orphaned. We consider the situation of the team's parent
		// below.
		group->orphaned = true;
	}

	status_t status = B_OK;
	struct process_group* freeGroup = NULL;

	InterruptsSpinLocker locker(gTeamSpinlock);

	team = team_get_team_struct_locked(processID);
	if (team != NULL) {
		// check the conditions again -- they might have changed in the meantime
		if (is_session_leader(team)
			|| team->session_id != currentTeam->session_id) {
			status = B_NOT_ALLOWED;
		} else if (team != currentTeam
				&& (team->flags & TEAM_FLAG_EXEC_DONE) != 0) {
			status = EACCES;
		} else if (team->group_id == groupID) {
			// the team is already in the desired process group
			freeGroup = group;
		} else {
			// Check if a process group with the requested ID already exists.
			struct process_group* targetGroup
				= team_get_process_group_locked(team->group->session, groupID);
			if (targetGroup != NULL) {
				// In case of processID == groupID we have to free the
				// allocated group.
				freeGroup = group;
			} else if (processID == groupID) {
				// We created a new process group, let us insert it into the
				// team's session.
				insert_group_into_session(team->group->session, group);
				targetGroup = group;
			}

			if (targetGroup != NULL) {
				// we got a group, let's move the team there
				process_group* oldGroup = team->group;

				remove_team_from_group(team);
				insert_team_into_group(targetGroup, team);

				// Update the "orphaned" flag of all potentially affected
				// groups.

				// the team's old group
				if (oldGroup->teams != NULL) {
					oldGroup->orphaned = false;
					update_orphaned_process_group(oldGroup, -1);
				}

				// the team's new group
				Team* parent = team->parent;
				targetGroup->orphaned &= parent == NULL
					|| parent->group == targetGroup
					|| team->parent->session_id != team->session_id;

				// children's groups
				Team* child = team->children;
				while (child != NULL) {
					child->group->orphaned = false;
					update_orphaned_process_group(child->group, -1);

					child = child->siblings_next;
				}
			} else
				status = B_NOT_ALLOWED;
		}
	} else
		status = B_NOT_ALLOWED;

	// Changing the process group might have changed the situation for a parent
	// waiting in wait_for_child(). Hence we notify it.
	if (status == B_OK)
		team->parent->dead_children.condition_variable.NotifyAll(false);

	locker.Unlock();

	if (status != B_OK) {
		// in case of error, the group hasn't been added into the hash
		team_delete_process_group(group);
	}

	team_delete_process_group(freeGroup);

	return status == B_OK ? groupID : status;
}


pid_t
_user_setsid(void)
{
	Team* team = thread_get_current_thread()->team;
	struct process_session* session;
	struct process_group* group;
	cpu_status state;
	bool failed = false;

	// the team must not already be a process group leader
	if (is_process_group_leader(team))
		return B_NOT_ALLOWED;

	group = create_process_group(team->id);
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
		remove_team_from_group(team);

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
	}

	return team->group_id;
}


status_t
_user_wait_for_team(team_id id, status_t* _userReturnCode)
{
	status_t returnCode;
	status_t status;

	if (_userReturnCode != NULL && !IS_USER_ADDRESS(_userReturnCode))
		return B_BAD_ADDRESS;

	status = wait_for_team(id, &returnCode);
	if (status >= B_OK && _userReturnCode != NULL) {
		if (user_memcpy(_userReturnCode, &returnCode, sizeof(returnCode))
				!= B_OK)
			return B_BAD_ADDRESS;
		return B_OK;
	}

	return syscall_restart_handle_post(status);
}


thread_id
_user_load_image(const char* const* userFlatArgs, size_t flatArgsSize,
	int32 argCount, int32 envCount, int32 priority, uint32 flags,
	port_id errorPort, uint32 errorToken)
{
	TRACE(("_user_load_image: argc = %ld\n", argCount));

	if (argCount < 1)
		return B_BAD_VALUE;

	// copy and relocate the flat arguments
	char** flatArgs;
	status_t error = copy_user_process_args(userFlatArgs, flatArgsSize,
		argCount, envCount, flatArgs);
	if (error != B_OK)
		return error;

	thread_id thread = load_image_internal(flatArgs, _ALIGN(flatArgsSize),
		argCount, envCount, priority, B_CURRENT_TEAM, flags, errorPort,
		errorToken);

	free(flatArgs);
		// load_image_internal() unset our variable if it took over ownership

	return thread;
}


void
_user_exit_team(status_t returnValue)
{
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;
	Thread* mainThread = team->main_thread;

	mainThread->exit.status = returnValue;
	mainThread->exit.reason = THREAD_RETURN_EXIT;

	// Also set the exit code in the current thread for the sake of it
	if (thread != mainThread) {
		thread->exit.status = returnValue;
		thread->exit.reason = THREAD_RETURN_EXIT;
	}

	if ((atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_PREVENT_EXIT)
			!= 0) {
		// This team is currently being debugged, and requested that teams
		// should not be exited.
		user_debug_stop_thread();
	}

	send_signal(thread->id, SIGKILL);
}


status_t
_user_kill_team(team_id team)
{
	return kill_team(team);
}


status_t
_user_get_team_info(team_id id, team_info* userInfo)
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
_user_get_next_team_info(int32* userCookie, team_info* userInfo)
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


status_t
_user_get_team_usage_info(team_id team, int32 who, team_usage_info* userInfo,
	size_t size)
{
	team_usage_info info;
	status_t status;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_team_usage_info(team, who, &info, size);
	if (status != B_OK)
		return status;

	if (user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_extended_team_info(team_id teamID, uint32 flags, void* buffer,
	size_t size, size_t* _sizeNeeded)
{
	// check parameters
	if ((buffer != NULL && !IS_USER_ADDRESS(buffer))
		|| (buffer == NULL && size > 0)
		|| _sizeNeeded == NULL || !IS_USER_ADDRESS(_sizeNeeded)) {
		return B_BAD_ADDRESS;
	}

	KMessage info;

	if ((flags & B_TEAM_INFO_BASIC) != 0) {
		// allocate memory for a copy of the team struct
		Team* teamClone = new(std::nothrow) Team;
		if (teamClone == NULL)
			return B_NO_MEMORY;
		ObjectDeleter<Team> teamCloneDeleter(teamClone);

		io_context* ioContext;
		{
			// get the team structure
			InterruptsSpinLocker _(gTeamSpinlock);
			Team* team = teamID == B_CURRENT_TEAM
				? thread_get_current_thread()->team
				: team_get_team_struct_locked(teamID);
			if (team == NULL)
				return B_BAD_TEAM_ID;

			// copy it
			memcpy(teamClone, team, sizeof(*team));

			// also fetch a reference to the I/O context
			ioContext = team->io_context;
			vfs_get_io_context(ioContext);
		}
		CObjectDeleter<io_context> ioContextPutter(ioContext,
			&vfs_put_io_context);

		// add the basic data to the info message
		if (info.AddInt32("id", teamClone->id) != B_OK
			|| info.AddString("name", teamClone->name) != B_OK
			|| info.AddInt32("process group", teamClone->group_id) != B_OK
			|| info.AddInt32("session", teamClone->session_id) != B_OK
			|| info.AddInt32("uid", teamClone->real_uid) != B_OK
			|| info.AddInt32("gid", teamClone->real_gid) != B_OK
			|| info.AddInt32("euid", teamClone->effective_uid) != B_OK
			|| info.AddInt32("egid", teamClone->effective_gid) != B_OK) {
			return B_NO_MEMORY;
		}

		// get the current working directory from the I/O context
		dev_t cwdDevice;
		ino_t cwdDirectory;
		{
			MutexLocker ioContextLocker(ioContext->io_mutex);
			vfs_vnode_to_node_ref(ioContext->cwd, &cwdDevice, &cwdDirectory);
		}

		if (info.AddInt32("cwd device", cwdDevice) != B_OK
			|| info.AddInt64("cwd directory", cwdDirectory) != B_OK) {
			return B_NO_MEMORY;
		}
	}

	// TODO: Support the other flags!

	// copy the needed size and, if it fits, the message back to userland
	size_t sizeNeeded = info.ContentSize();
	if (user_memcpy(_sizeNeeded, &sizeNeeded, sizeof(sizeNeeded)) != B_OK)
		return B_BAD_ADDRESS;

	if (sizeNeeded > size)
		return B_BUFFER_OVERFLOW;

	if (user_memcpy(buffer, info.Buffer(), sizeNeeded) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}
