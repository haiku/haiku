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

#include <errno.h>
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

#include "TeamThreadTables.h"


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


namespace {


class TeamNotificationService : public DefaultNotificationService {
public:
							TeamNotificationService();

			void			Notify(uint32 eventCode, Team* team);
};


// #pragma mark - TeamTable


typedef BKernel::TeamThreadTable<Team> TeamTable;


// #pragma mark - ProcessGroupHashDefinition


struct ProcessGroupHashDefinition {
	typedef pid_t			KeyType;
	typedef	ProcessGroup	ValueType;

	size_t HashKey(pid_t key) const
	{
		return key;
	}

	size_t Hash(ProcessGroup* value) const
	{
		return HashKey(value->id);
	}

	bool Compare(pid_t key, ProcessGroup* value) const
	{
		return value->id == key;
	}

	ProcessGroup*& GetLink(ProcessGroup* value) const
	{
		return value->next;
	}
};

typedef BOpenHashTable<ProcessGroupHashDefinition> ProcessGroupHashTable;


}	// unnamed namespace


// #pragma mark -


// the team_id -> Team hash table and the lock protecting it
static TeamTable sTeamHash;
static spinlock sTeamHashLock = B_SPINLOCK_INITIALIZER;

// the pid_t -> ProcessGroup hash table and the lock protecting it
static ProcessGroupHashTable sGroupHash;
static spinlock sGroupHashLock = B_SPINLOCK_INITIALIZER;

static Team* sKernelTeam = NULL;

// A list of process groups of children of dying session leaders that need to
// be signalled, if they have become orphaned and contain stopped processes.
static ProcessGroupList sOrphanedCheckProcessGroups;
static mutex sOrphanedCheckLock
	= MUTEX_INITIALIZER("orphaned process group check");

// some arbitrarily chosen limits -- should probably depend on the available
// memory (the limit is not yet enforced)
static int32 sMaxTeams = 2048;
static int32 sUsedTeams = 1;

static TeamNotificationService sNotificationService;


// #pragma mark - TeamListIterator


TeamListIterator::TeamListIterator()
{
	// queue the entry
	InterruptsSpinLocker locker(sTeamHashLock);
	sTeamHash.InsertIteratorEntry(&fEntry);
}


TeamListIterator::~TeamListIterator()
{
	// remove the entry
	InterruptsSpinLocker locker(sTeamHashLock);
	sTeamHash.RemoveIteratorEntry(&fEntry);
}


Team*
TeamListIterator::Next()
{
	// get the next team -- if there is one, get reference for it
	InterruptsSpinLocker locker(sTeamHashLock);
	Team* team = sTeamHash.NextElement(&fEntry);
	if (team != NULL)
		team->AcquireReference();

	return team;
}


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
	SetJobControlState(team_id team, job_control_state newState, Signal* signal)
		:
		fTeam(team),
		fNewState(newState),
		fSignal(signal != NULL ? signal->Number() : 0)
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


//	#pragma mark - Team


Team::Team(team_id id, bool kernel)
{
	// allocate an ID
	this->id = id;
	visible = true;
	serial_number = -1;

	// init mutex
	if (kernel) {
		mutex_init(&fLock, "Team:kernel");
	} else {
		char lockName[16];
		snprintf(lockName, sizeof(lockName), "Team:%" B_PRId32, id);
		mutex_init_etc(&fLock, lockName, MUTEX_FLAG_CLONE_NAME);
	}

	hash_next = siblings_next = children = parent = NULL;
	fName[0] = '\0';
	fArgs[0] = '\0';
	num_threads = 0;
	io_context = NULL;
	address_space = NULL;
	realtime_sem_context = NULL;
	xsi_sem_context = NULL;
	thread_list = NULL;
	main_thread = NULL;
	loading_info = NULL;
	state = TEAM_STATE_BIRTH;
	flags = 0;
	death_entry = NULL;
	user_data_area = -1;
	user_data = 0;
	used_user_data = 0;
	user_data_size = 0;
	free_user_threads = NULL;

	supplementary_groups = NULL;
	supplementary_group_count = 0;

	dead_threads_kernel_time = 0;
	dead_threads_user_time = 0;
	cpu_clock_offset = 0;

	// dead threads
	list_init(&dead_threads);
	dead_threads_count = 0;

	// dead children
	dead_children.count = 0;
	dead_children.kernel_time = 0;
	dead_children.user_time = 0;

	// job control entry
	job_control_entry = new(nothrow) ::job_control_entry;
	if (job_control_entry != NULL) {
		job_control_entry->state = JOB_CONTROL_STATE_NONE;
		job_control_entry->thread = id;
		job_control_entry->team = this;
	}

	// exit status -- setting initialized to false suffices
	exit.initialized = false;

	list_init(&sem_list);
	list_init(&port_list);
	list_init(&image_list);
	list_init(&watcher_list);

	clear_team_debug_info(&debug_info, true);

	// init dead/stopped/continued children condition vars
	dead_children.condition_variable.Init(&dead_children, "team children");

	fQueuedSignalsCounter = new(std::nothrow) BKernel::QueuedSignalsCounter(
		kernel ? -1 : MAX_QUEUED_SIGNALS);
	memset(fSignalActions, 0, sizeof(fSignalActions));

	fUserDefinedTimerCount = 0;
}


Team::~Team()
{
	// get rid of all associated data
	PrepareForDeletion();

	vfs_put_io_context(io_context);
	delete_owned_ports(this);
	sem_delete_owned_sems(this);

	DeleteUserTimers(false);

	fPendingSignals.Clear();

	if (fQueuedSignalsCounter != NULL)
		fQueuedSignalsCounter->ReleaseReference();

	while (thread_death_entry* threadDeathEntry
			= (thread_death_entry*)list_remove_head_item(&dead_threads)) {
		free(threadDeathEntry);
	}

	while (::job_control_entry* entry = dead_children.entries.RemoveHead())
		delete entry;

	while (free_user_thread* entry = free_user_threads) {
		free_user_threads = entry->next;
		free(entry);
	}

	malloc_referenced_release(supplementary_groups);

	delete job_control_entry;
		// usually already NULL and transferred to the parent

	mutex_destroy(&fLock);
}


/*static*/ Team*
Team::Create(team_id id, const char* name, bool kernel)
{
	// create the team object
	Team* team = new(std::nothrow) Team(id, kernel);
	if (team == NULL)
		return NULL;
	ObjectDeleter<Team> teamDeleter(team);

	if (name != NULL)
		team->SetName(name);

	// check initialization
	if (team->job_control_entry == NULL || team->fQueuedSignalsCounter == NULL)
		return NULL;

	// finish initialization (arch specifics)
	if (arch_team_init_team_struct(team, kernel) != B_OK)
		return NULL;

	if (!kernel) {
		status_t error = user_timer_create_team_timers(team);
		if (error != B_OK)
			return NULL;
	}

	// everything went fine
	return teamDeleter.Detach();
}


/*!	\brief Returns the team with the given ID.
	Returns a reference to the team.
	Team and thread spinlock must not be held.
*/
/*static*/ Team*
Team::Get(team_id id)
{
	if (id == B_CURRENT_TEAM) {
		Team* team = thread_get_current_thread()->team;
		team->AcquireReference();
		return team;
	}

	InterruptsSpinLocker locker(sTeamHashLock);
	Team* team = sTeamHash.Lookup(id);
	if (team != NULL)
		team->AcquireReference();
	return team;
}


/*!	\brief Returns the team with the given ID in a locked state.
	Returns a reference to the team.
	Team and thread spinlock must not be held.
*/
/*static*/ Team*
Team::GetAndLock(team_id id)
{
	// get the team
	Team* team = Get(id);
	if (team == NULL)
		return NULL;

	// lock it
	team->Lock();

	// only return the team, when it isn't already dying
	if (team->state >= TEAM_STATE_SHUTDOWN) {
		team->Unlock();
		team->ReleaseReference();
		return NULL;
	}

	return team;
}


/*!	Locks the team and its parent team (if any).
	The caller must hold a reference to the team or otherwise make sure that
	it won't be deleted.
	If the team doesn't have a parent, only the team itself is locked. If the
	team's parent is the kernel team and \a dontLockParentIfKernel is \c true,
	only the team itself is locked.

	\param dontLockParentIfKernel If \c true, the team's parent team is only
		locked, if it is not the kernel team.
*/
void
Team::LockTeamAndParent(bool dontLockParentIfKernel)
{
	// The locking order is parent -> child. Since the parent can change as long
	// as we don't lock the team, we need to do a trial and error loop.
	Lock();

	while (true) {
		// If the team doesn't have a parent, we're done. Otherwise try to lock
		// the parent.This will succeed in most cases, simplifying things.
		Team* parent = this->parent;
		if (parent == NULL || (dontLockParentIfKernel && parent == sKernelTeam)
			|| parent->TryLock()) {
			return;
		}

		// get a temporary reference to the parent, unlock this team, lock the
		// parent, and re-lock this team
		BReference<Team> parentReference(parent);

		Unlock();
		parent->Lock();
		Lock();

		// If the parent hasn't changed in the meantime, we're done.
		if (this->parent == parent)
			return;

		// The parent has changed -- unlock and retry.
		parent->Unlock();
	}
}


/*!	Unlocks the team and its parent team (if any).
*/
void
Team::UnlockTeamAndParent()
{
	if (parent != NULL)
		parent->Unlock();

	Unlock();
}


/*!	Locks the team, its parent team (if any), and the team's process group.
	The caller must hold a reference to the team or otherwise make sure that
	it won't be deleted.
	If the team doesn't have a parent, only the team itself is locked.
*/
void
Team::LockTeamParentAndProcessGroup()
{
	LockTeamAndProcessGroup();

	// We hold the group's and the team's lock, but not the parent team's lock.
	// If we have a parent, try to lock it.
	if (this->parent == NULL || this->parent->TryLock())
		return;

	// No success -- unlock the team and let LockTeamAndParent() do the rest of
	// the job.
	Unlock();
	LockTeamAndParent(false);
}


/*!	Unlocks the team, its parent team (if any), and the team's process group.
*/
void
Team::UnlockTeamParentAndProcessGroup()
{
	group->Unlock();

	if (parent != NULL)
		parent->Unlock();

	Unlock();
}


void
Team::LockTeamAndProcessGroup()
{
	// The locking order is process group -> child. Since the process group can
	// change as long as we don't lock the team, we need to do a trial and error
	// loop.
	Lock();

	while (true) {
		// Try to lock the group. This will succeed in most cases, simplifying
		// things.
		ProcessGroup* group = this->group;
		if (group->TryLock())
			return;

		// get a temporary reference to the group, unlock this team, lock the
		// group, and re-lock this team
		BReference<ProcessGroup> groupReference(group);

		Unlock();
		group->Lock();
		Lock();

		// If the group hasn't changed in the meantime, we're done.
		if (this->group == group)
			return;

		// The group has changed -- unlock and retry.
		group->Unlock();
	}
}


void
Team::UnlockTeamAndProcessGroup()
{
	group->Unlock();
	Unlock();
}


void
Team::SetName(const char* name)
{
	if (const char* lastSlash = strrchr(name, '/'))
		name = lastSlash + 1;

	strlcpy(fName, name, B_OS_NAME_LENGTH);
}


void
Team::SetArgs(const char* args)
{
	strlcpy(fArgs, args, sizeof(fArgs));
}


void
Team::SetArgs(const char* path, const char* const* otherArgs, int otherArgCount)
{
	fArgs[0] = '\0';
	strlcpy(fArgs, path, sizeof(fArgs));
	for (int i = 0; i < otherArgCount; i++) {
		strlcat(fArgs, " ", sizeof(fArgs));
		strlcat(fArgs, otherArgs[i], sizeof(fArgs));
	}
}


void
Team::ResetSignalsOnExec()
{
	// We are supposed to keep pending signals. Signal actions shall be reset
	// partially: SIG_IGN and SIG_DFL dispositions shall be kept as they are
	// (for SIGCHLD it's implementation-defined). Others shall be reset to
	// SIG_DFL. SA_ONSTACK shall be cleared. There's no mention of the other
	// flags, but since there aren't any handlers, they make little sense, so
	// we clear them.

	for (uint32 i = 1; i <= MAX_SIGNAL_NUMBER; i++) {
		struct sigaction& action = SignalActionFor(i);
		if (action.sa_handler != SIG_IGN && action.sa_handler != SIG_DFL)
			action.sa_handler = SIG_DFL;

		action.sa_mask = 0;
		action.sa_flags = 0;
		action.sa_userdata = NULL;
	}
}


void
Team::InheritSignalActions(Team* parent)
{
	memcpy(fSignalActions, parent->fSignalActions, sizeof(fSignalActions));
}


/*!	Adds the given user timer to the team and, if user-defined, assigns it an
	ID.

	The caller must hold the team's lock.

	\param timer The timer to be added. If it doesn't have an ID yet, it is
		considered user-defined and will be assigned an ID.
	\return \c B_OK, if the timer was added successfully, another error code
		otherwise.
*/
status_t
Team::AddUserTimer(UserTimer* timer)
{
	// don't allow addition of timers when already shutting the team down
	if (state >= TEAM_STATE_SHUTDOWN)
		return B_BAD_TEAM_ID;

	// If the timer is user-defined, check timer limit and increment
	// user-defined count.
	if (timer->ID() < 0 && !CheckAddUserDefinedTimer())
		return EAGAIN;

	fUserTimers.AddTimer(timer);

	return B_OK;
}


/*!	Removes the given user timer from the team.

	The caller must hold the team's lock.

	\param timer The timer to be removed.

*/
void
Team::RemoveUserTimer(UserTimer* timer)
{
	fUserTimers.RemoveTimer(timer);

	if (timer->ID() >= USER_TIMER_FIRST_USER_DEFINED_ID)
		UserDefinedTimersRemoved(1);
}


/*!	Deletes all (or all user-defined) user timers of the team.

	Timer's belonging to the team's threads are not affected.
	The caller must hold the team's lock.

	\param userDefinedOnly If \c true, only the user-defined timers are deleted,
		otherwise all timers are deleted.
*/
void
Team::DeleteUserTimers(bool userDefinedOnly)
{
	int32 count = fUserTimers.DeleteTimers(userDefinedOnly);
	UserDefinedTimersRemoved(count);
}


/*!	If not at the limit yet, increments the team's user-defined timer count.
	\return \c true, if the limit wasn't reached yet, \c false otherwise.
*/
bool
Team::CheckAddUserDefinedTimer()
{
	int32 oldCount = atomic_add(&fUserDefinedTimerCount, 1);
	if (oldCount >= MAX_USER_TIMERS_PER_TEAM) {
		atomic_add(&fUserDefinedTimerCount, -1);
		return false;
	}

	return true;
}


/*!	Subtracts the given count for the team's user-defined timer count.
	\param count The count to subtract.
*/
void
Team::UserDefinedTimersRemoved(int32 count)
{
	atomic_add(&fUserDefinedTimerCount, -count);
}


void
Team::DeactivateCPUTimeUserTimers()
{
	while (TeamTimeUserTimer* timer = fCPUTimeUserTimers.Head())
		timer->Deactivate();

	while (TeamUserTimeUserTimer* timer = fUserTimeUserTimers.Head())
		timer->Deactivate();
}


/*!	Returns the team's current total CPU time (kernel + user + offset).

	The caller must hold the scheduler lock.

	\param ignoreCurrentRun If \c true and the current thread is one team's
		threads, don't add the time since the last time \c last_time was
		updated. Should be used in "thread unscheduled" scheduler callbacks,
		since although the thread is still running at that time, its time has
		already been stopped.
	\return The team's current total CPU time.
*/
bigtime_t
Team::CPUTime(bool ignoreCurrentRun) const
{
	bigtime_t time = cpu_clock_offset + dead_threads_kernel_time
		+ dead_threads_user_time;

	Thread* currentThread = thread_get_current_thread();
	bigtime_t now = system_time();

	for (Thread* thread = thread_list; thread != NULL;
			thread = thread->team_next) {
		SpinLocker threadTimeLocker(thread->time_lock);
		time += thread->kernel_time + thread->user_time;

		if (thread->IsRunning()) {
			if (!ignoreCurrentRun || thread != currentThread)
				time += now - thread->last_time;
		}
	}

	return time;
}


/*!	Returns the team's current user CPU time.

	The caller must hold the scheduler lock.

	\return The team's current user CPU time.
*/
bigtime_t
Team::UserCPUTime() const
{
	bigtime_t time = dead_threads_user_time;

	bigtime_t now = system_time();

	for (Thread* thread = thread_list; thread != NULL;
			thread = thread->team_next) {
		SpinLocker threadTimeLocker(thread->time_lock);
		time += thread->user_time;

		if (thread->IsRunning() && !thread->in_kernel)
			time += now - thread->last_time;
	}

	return time;
}


//	#pragma mark - ProcessGroup


ProcessGroup::ProcessGroup(pid_t id)
	:
	id(id),
	teams(NULL),
	fSession(NULL),
	fInOrphanedCheckList(false)
{
	char lockName[32];
	snprintf(lockName, sizeof(lockName), "Group:%" B_PRId32, id);
	mutex_init_etc(&fLock, lockName, MUTEX_FLAG_CLONE_NAME);
}


ProcessGroup::~ProcessGroup()
{
	TRACE(("ProcessGroup::~ProcessGroup(): id = %ld\n", group->id));

	// If the group is in the orphaned check list, remove it.
	MutexLocker orphanedCheckLocker(sOrphanedCheckLock);

	if (fInOrphanedCheckList)
		sOrphanedCheckProcessGroups.Remove(this);

	orphanedCheckLocker.Unlock();

	// remove group from the hash table and from the session
	if (fSession != NULL) {
		InterruptsSpinLocker groupHashLocker(sGroupHashLock);
		sGroupHash.RemoveUnchecked(this);
		groupHashLocker.Unlock();

		fSession->ReleaseReference();
	}

	mutex_destroy(&fLock);
}


/*static*/ ProcessGroup*
ProcessGroup::Get(pid_t id)
{
	InterruptsSpinLocker groupHashLocker(sGroupHashLock);
	ProcessGroup* group = sGroupHash.Lookup(id);
	if (group != NULL)
		group->AcquireReference();
	return group;
}


/*!	Adds the group the given session and makes it publicly accessible.
	The caller must not hold the process group hash lock.
*/
void
ProcessGroup::Publish(ProcessSession* session)
{
	InterruptsSpinLocker groupHashLocker(sGroupHashLock);
	PublishLocked(session);
}


/*!	Adds the group to the given session and makes it publicly accessible.
	The caller must hold the process group hash lock.
*/
void
ProcessGroup::PublishLocked(ProcessSession* session)
{
	ASSERT(sGroupHash.Lookup(this->id) == NULL);

	fSession = session;
	fSession->AcquireReference();

	sGroupHash.InsertUnchecked(this);
}


/*!	Checks whether the process group is orphaned.
	The caller must hold the group's lock.
	\return \c true, if the group is orphaned, \c false otherwise.
*/
bool
ProcessGroup::IsOrphaned() const
{
	// Orphaned Process Group: "A process group in which the parent of every
	// member is either itself a member of the group or is not a member of the
	// group's session." (Open Group Base Specs Issue 7)
	bool orphaned = true;

	Team* team = teams;
	while (orphaned && team != NULL) {
		team->LockTeamAndParent(false);

		Team* parent = team->parent;
		if (parent != NULL && parent->group_id != id
			&& parent->session_id == fSession->id) {
			orphaned = false;
		}

		team->UnlockTeamAndParent();

		team = team->group_next;
	}

	return orphaned;
}


void
ProcessGroup::ScheduleOrphanedCheck()
{
	MutexLocker orphanedCheckLocker(sOrphanedCheckLock);

	if (!fInOrphanedCheckList) {
		sOrphanedCheckProcessGroups.Add(this);
		fInOrphanedCheckList = true;
	}
}


void
ProcessGroup::UnsetOrphanedCheck()
{
	fInOrphanedCheckList = false;
}


//	#pragma mark - ProcessSession


ProcessSession::ProcessSession(pid_t id)
	:
	id(id),
	controlling_tty(-1),
	foreground_group(-1)
{
	char lockName[32];
	snprintf(lockName, sizeof(lockName), "Session:%" B_PRId32, id);
	mutex_init_etc(&fLock, lockName, MUTEX_FLAG_CLONE_NAME);
}


ProcessSession::~ProcessSession()
{
	mutex_destroy(&fLock);
}


//	#pragma mark - KDL functions


static void
_dump_team_info(Team* team)
{
	kprintf("TEAM: %p\n", team);
	kprintf("id:               %" B_PRId32 " (%#" B_PRIx32 ")\n", team->id,
		team->id);
	kprintf("serial_number:    %" B_PRId64 "\n", team->serial_number);
	kprintf("name:             '%s'\n", team->Name());
	kprintf("args:             '%s'\n", team->Args());
	kprintf("hash_next:        %p\n", team->hash_next);
	kprintf("parent:           %p", team->parent);
	if (team->parent != NULL) {
		kprintf(" (id = %" B_PRId32 ")\n", team->parent->id);
	} else
		kprintf("\n");

	kprintf("children:         %p\n", team->children);
	kprintf("num_threads:      %d\n", team->num_threads);
	kprintf("state:            %d\n", team->state);
	kprintf("flags:            0x%" B_PRIx32 "\n", team->flags);
	kprintf("io_context:       %p\n", team->io_context);
	if (team->address_space)
		kprintf("address_space:    %p\n", team->address_space);
	kprintf("user data:        %p (area %" B_PRId32 ")\n",
		(void*)team->user_data, team->user_data_area);
	kprintf("free user thread: %p\n", team->free_user_threads);
	kprintf("main_thread:      %p\n", team->main_thread);
	kprintf("thread_list:      %p\n", team->thread_list);
	kprintf("group_id:         %" B_PRId32 "\n", team->group_id);
	kprintf("session_id:       %" B_PRId32 "\n", team->session_id);
}


static int
dump_team_info(int argc, char** argv)
{
	ulong arg;
	bool found = false;

	if (argc < 2) {
		Thread* thread = thread_get_current_thread();
		if (thread != NULL && thread->team != NULL)
			_dump_team_info(thread->team);
		else
			kprintf("No current team!\n");
		return 0;
	}

	arg = strtoul(argv[1], NULL, 0);
	if (IS_KERNEL_ADDRESS(arg)) {
		// semi-hack
		_dump_team_info((Team*)arg);
		return 0;
	}

	// walk through the thread list, trying to match name or id
	for (TeamTable::Iterator it = sTeamHash.GetIterator();
		Team* team = it.Next();) {
		if ((team->Name() && strcmp(argv[1], team->Name()) == 0)
			|| team->id == (team_id)arg) {
			_dump_team_info(team);
			found = true;
			break;
		}
	}

	if (!found)
		kprintf("team \"%s\" (%" B_PRId32 ") doesn't exist!\n", argv[1], (team_id)arg);
	return 0;
}


static int
dump_teams(int argc, char** argv)
{
	kprintf("team           id  parent      name\n");

	for (TeamTable::Iterator it = sTeamHash.GetIterator();
		Team* team = it.Next();) {
		kprintf("%p%7" B_PRId32 "  %p  %s\n", team, team->id, team->parent, team->Name());
	}

	return 0;
}


//	#pragma mark - Private functions


/*!	Inserts team \a team into the child list of team \a parent.

	The caller must hold the lock of both \a parent and \a team.

	\param parent The parent team.
	\param team The team to be inserted into \a parent's child list.
*/
static void
insert_team_into_parent(Team* parent, Team* team)
{
	ASSERT(parent != NULL);

	team->siblings_next = parent->children;
	parent->children = team;
	team->parent = parent;
}


/*!	Removes team \a team from the child list of team \a parent.

	The caller must hold the lock of both \a parent and \a team.

	\param parent The parent team.
	\param team The team to be removed from \a parent's child list.
*/
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


/*!	Returns whether the given team is a session leader.
	The caller must hold the team's lock or its process group's lock.
*/
static bool
is_session_leader(Team* team)
{
	return team->session_id == team->id;
}


/*!	Returns whether the given team is a process group leader.
	The caller must hold the team's lock or its process group's lock.
*/
static bool
is_process_group_leader(Team* team)
{
	return team->group_id == team->id;
}


/*!	Inserts the given team into the given process group.
	The caller must hold the process group's lock, the team's lock, and the
	team's parent's lock.
*/
static void
insert_team_into_group(ProcessGroup* group, Team* team)
{
	team->group = group;
	team->group_id = group->id;
	team->session_id = group->Session()->id;

	team->group_next = group->teams;
	group->teams = team;
	group->AcquireReference();
}


/*!	Removes the given team from its process group.

	The caller must hold the process group's lock, the team's lock, and the
	team's parent's lock. Interrupts must be enabled.

	\param team The team that'll be removed from its process group.
*/
static void
remove_team_from_group(Team* team)
{
	ProcessGroup* group = team->group;
	Team* current;
	Team* last = NULL;

	// the team must be in a process group to let this function have any effect
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

	group->ReleaseReference();
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
	char** flatArgs = (char**)malloc(_ALIGN(flatArgsSize));
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


static status_t
team_create_thread_start_internal(void* args)
{
	status_t err;
	Thread* thread;
	Team* team;
	struct team_arg* teamArgs = (struct team_arg*)args;
	const char* path;
	addr_t entry;
	char** userArgs;
	char** userEnv;
	struct user_space_program_args* programArgs;
	uint32 argCount, envCount;

	thread = thread_get_current_thread();
	team = thread->team;
	cache_node_launched(teamArgs->arg_count, teamArgs->flat_args);

	TRACE(("team_create_thread_start: entry thread %ld\n", thread->id));

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

	// set team args and update state
	team->Lock();
	team->SetArgs(path, teamArgs->flat_args + 1, argCount - 1);
	team->state = TEAM_STATE_NORMAL;
	team->Unlock();

	free_team_arg(teamArgs);
		// the arguments are already on the user stack, we no longer need
		// them in this form

	// NOTE: Normally arch_thread_enter_userspace() never returns, that is
	// automatic variables with function scope will never be destroyed.
	{
		// find runtime_loader path
		KPath runtimeLoaderPath;
		err = find_directory(B_SYSTEM_DIRECTORY, gBootDevice, false,
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

	// enter userspace -- returns only in case of error
	return thread_enter_userspace_new_team(thread, (addr_t)entry,
		programArgs, NULL);
}


static status_t
team_create_thread_start(void* args)
{
	team_create_thread_start_internal(args);
	team_init_exit_info_on_error(thread_get_current_thread()->team);
	thread_exit();
		// does not return
	return B_OK;
}


static thread_id
load_image_internal(char**& _flatArgs, size_t flatArgsSize, int32 argCount,
	int32 envCount, int32 priority, team_id parentID, uint32 flags,
	port_id errorPort, uint32 errorToken)
{
	char** flatArgs = _flatArgs;
	thread_id thread;
	status_t status;
	struct team_arg* teamArgs;
	struct team_loading_info loadingInfo;
	io_context* parentIOContext = NULL;
	team_id teamID;

	if (flatArgs == NULL || argCount == 0)
		return B_BAD_VALUE;

	const char* path = flatArgs[0];

	TRACE(("load_image_internal: name '%s', args = %p, argCount = %ld\n",
		path, flatArgs, argCount));

	// cut the path from the main thread name
	const char* threadName = strrchr(path, '/');
	if (threadName != NULL)
		threadName++;
	else
		threadName = path;

	// create the main thread object
	Thread* mainThread;
	status = Thread::Create(threadName, mainThread);
	if (status != B_OK)
		return status;
	BReference<Thread> mainThreadReference(mainThread, true);

	// create team object
	Team* team = Team::Create(mainThread->id, path, false);
	if (team == NULL)
		return B_NO_MEMORY;
	BReference<Team> teamReference(team, true);

	if (flags & B_WAIT_TILL_LOADED) {
		loadingInfo.thread = thread_get_current_thread();
		loadingInfo.result = B_ERROR;
		loadingInfo.done = false;
		team->loading_info = &loadingInfo;
	}

	// get the parent team
	Team* parent = Team::Get(parentID);
	if (parent == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> parentReference(parent, true);

	parent->LockTeamAndProcessGroup();
	team->Lock();

	// inherit the parent's user/group
	inherit_parent_user_and_group(team, parent);

 	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	sTeamHash.Insert(team);
	sUsedTeams++;

	teamsLocker.Unlock();

	insert_team_into_parent(parent, team);
	insert_team_into_group(parent->group, team);

	// get a reference to the parent's I/O context -- we need it to create ours
	parentIOContext = parent->io_context;
	vfs_get_io_context(parentIOContext);

	team->Unlock();
	parent->UnlockTeamAndProcessGroup();

	// notify team listeners
	sNotificationService.Notify(TEAM_ADDED, team);

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

	// create the user data area
	status = create_team_user_data(team);
	if (status != B_OK)
		goto err4;

	// In case we start the main thread, we shouldn't access the team object
	// afterwards, so cache the team's ID.
	teamID = team->id;

	// Create a kernel thread, but under the context of the new team
	// The new thread will take over ownership of teamArgs.
	{
		ThreadCreationAttributes threadAttributes(team_create_thread_start,
			threadName, B_NORMAL_PRIORITY, teamArgs, teamID, mainThread);
		threadAttributes.additional_stack_size = sizeof(user_space_program_args)
			+ teamArgs->flat_args_size;
		thread = thread_create_thread(threadAttributes, false);
		if (thread < 0) {
			status = thread;
			goto err5;
		}
	}

	// The team has been created successfully, so we keep the reference. Or
	// more precisely: It's owned by the team's main thread, now.
	teamReference.Detach();

	// wait for the loader of the new team to finish its work
	if ((flags & B_WAIT_TILL_LOADED) != 0) {
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		// resume the team's main thread
		if (mainThread != NULL && mainThread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(mainThread);

		// Now suspend ourselves until loading is finished. We will be woken
		// either by the thread, when it finished or aborted loading, or when
		// the team is going to die (e.g. is killed). In either case the one
		// setting `loadingInfo.done' is responsible for removing the info from
		// the team structure.
		while (!loadingInfo.done) {
			thread_get_current_thread()->next_state = B_THREAD_SUSPENDED;
			scheduler_reschedule();
		}

		schedulerLocker.Unlock();

		if (loadingInfo.result < B_OK)
			return loadingInfo.result;
	}

	// notify the debugger
	user_debug_team_created(teamID);

	return thread;

err5:
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

	// Remove the team structure from the process group, the parent team, and
	// the team hash table and delete the team structure.
	parent->LockTeamAndProcessGroup();
	team->Lock();

	remove_team_from_group(team);
	remove_team_from_parent(team->parent, team);

	team->Unlock();
	parent->UnlockTeamAndProcessGroup();

	teamsLocker.Lock();
	sTeamHash.Remove(team);
	teamsLocker.Unlock();

	sNotificationService.Notify(TEAM_REMOVED, team);

	return status;
}


/*!	Almost shuts down the current team and loads a new image into it.
	If successful, this function does not return and will takeover ownership of
	the arguments provided.
	This function may only be called in a userland team (caused by one of the
	exec*() syscalls).
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
	thread_id nubThreadID = -1;

	TRACE(("exec_team(path = \"%s\", argc = %ld, envCount = %ld): team %ld\n",
		path, argCount, envCount, team->id));

	T(ExecTeam(path, argCount, flatArgs, envCount, flatArgs + argCount + 1));

	// switching the kernel at run time is probably not a good idea :)
	if (team == team_get_kernel_team())
		return B_NOT_ALLOWED;

	// we currently need to be single threaded here
	// TODO: maybe we should just kill all other threads and
	//	make the current thread the team's main thread?
	Thread* currentThread = thread_get_current_thread();
	if (currentThread != team->main_thread)
		return B_NOT_ALLOWED;

	// The debug nub thread, a pure kernel thread, is allowed to survive.
	// We iterate through the thread list to make sure that there's no other
	// thread.
	TeamLocker teamLocker(team);
	InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

	if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED)
		nubThreadID = team->debug_info.nub_thread;

	debugInfoLocker.Unlock();

	for (Thread* thread = team->thread_list; thread != NULL;
			thread = thread->team_next) {
		if (thread != team->main_thread && thread->id != nubThreadID)
			return B_NOT_ALLOWED;
	}

	team->DeleteUserTimers(true);
	team->ResetSignalsOnExec();

	teamLocker.Unlock();

	status_t status = create_team_arg(&teamArgs, path, flatArgs, flatArgsSize,
		argCount, envCount, umask, -1, 0);
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

	team->Lock();
	team->SetName(path);
	team->Unlock();

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

	// get a user thread for the thread
	user_thread* userThread = team_allocate_user_thread(team);
		// cannot fail (the allocation for the team would have failed already)
	ThreadLocker currentThreadLocker(currentThread);
	currentThread->user_thread = userThread;
	currentThreadLocker.Unlock();

	// create the user stack for the thread
	status = thread_create_user_stack(currentThread->team, currentThread, NULL,
		0, sizeof(user_space_program_args) + teamArgs->flat_args_size);
	if (status == B_OK) {
		// prepare the stack, load the runtime loader, and enter userspace
		team_create_thread_start(teamArgs);
			// does never return
	} else
		free_team_arg(teamArgs);

	// Sorry, we have to kill ourselves, there is no way out anymore
	// (without any areas left and all that).
	exit_thread(status);

	// We return a status here since the signal that is sent by the
	// call above is not immediately handled.
	return B_ERROR;
}


static thread_id
fork_team(void)
{
	Thread* parentThread = thread_get_current_thread();
	Team* parentTeam = parentThread->team;
	Team* team;
	arch_fork_arg* forkArgs;
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

	// create the main thread object
	Thread* thread;
	status = Thread::Create(parentThread->name, thread);
	if (status != B_OK)
		return status;
	BReference<Thread> threadReference(thread, true);

	// create the team object
	team = Team::Create(thread->id, NULL, false);
	if (team == NULL)
		return B_NO_MEMORY;

	parentTeam->LockTeamAndProcessGroup();
	team->Lock();

	team->SetName(parentTeam->Name());
	team->SetArgs(parentTeam->Args());

	// Inherit the parent's user/group.
	inherit_parent_user_and_group(team, parentTeam);

	// inherit signal handlers
	team->InheritSignalActions(parentTeam);

	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	sTeamHash.Insert(team);
	sUsedTeams++;

	teamsLocker.Unlock();

	insert_team_into_parent(parentTeam, team);
	insert_team_into_group(parentTeam->group, team);

	team->Unlock();
	parentTeam->UnlockTeamAndProcessGroup();

	// notify team listeners
	sNotificationService.Notify(TEAM_ADDED, team);

	// inherit some team debug flags
	team->debug_info.flags |= atomic_get(&parentTeam->debug_info.flags)
		& B_TEAM_DEBUG_INHERITED_FLAGS;

	forkArgs = (arch_fork_arg*)malloc(sizeof(arch_fork_arg));
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

	cookie = 0;
	while (get_next_area_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.area == parentTeam->user_data_area) {
			// don't clone the user area; just create a new one
			status = create_team_user_data(team);
			if (status != B_OK)
				break;

			thread->user_thread = team_allocate_user_thread(team);
		} else {
			void* address;
			area_id area = vm_copy_area(team->address_space->ID(), info.name,
				&address, B_CLONE_ADDRESS, info.protection, info.area);
			if (area < B_OK) {
				status = area;
				break;
			}

			if (info.area == parentThread->user_stack_area)
				thread->user_stack_area = area;
		}
	}

	if (status < B_OK)
		goto err4;

	if (thread->user_thread == NULL) {
#if KDEBUG
		panic("user data area not found, parent area is %" B_PRId32,
			parentTeam->user_data_area);
#endif
		status = B_ERROR;
		goto err4;
	}

	thread->user_stack_base = parentThread->user_stack_base;
	thread->user_stack_size = parentThread->user_stack_size;
	thread->user_local_storage = parentThread->user_local_storage;
	thread->sig_block_mask = parentThread->sig_block_mask;
	thread->signal_stack_base = parentThread->signal_stack_base;
	thread->signal_stack_size = parentThread->signal_stack_size;
	thread->signal_stack_enabled = parentThread->signal_stack_enabled;

	arch_store_fork_frame(forkArgs);

	// copy image list
	image_info imageInfo;
	cookie = 0;
	while (get_next_image_info(parentTeam->id, &cookie, &imageInfo) == B_OK) {
		image_id image = register_image(team, &imageInfo, sizeof(imageInfo));
		if (image < 0)
			goto err5;
	}

	// create the main thread
	{
		ThreadCreationAttributes threadCreationAttributes(NULL,
			parentThread->name, parentThread->priority, NULL, team->id, thread);
		threadCreationAttributes.forkArgs = forkArgs;
		threadID = thread_create_thread(threadCreationAttributes, false);
		if (threadID < 0) {
			status = threadID;
			goto err5;
		}
	}

	// notify the debugger
	user_debug_team_created(team->id);

	T(TeamForked(threadID));

	resume_thread(threadID);
	return threadID;

err5:
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
	// Remove the team structure from the process group, the parent team, and
	// the team hash table and delete the team structure.
	parentTeam->LockTeamAndProcessGroup();
	team->Lock();

	remove_team_from_group(team);
	remove_team_from_parent(team->parent, team);

	team->Unlock();
	parentTeam->UnlockTeamAndProcessGroup();

	teamsLocker.Lock();
	sTeamHash.Remove(team);
	teamsLocker.Unlock();

	sNotificationService.Notify(TEAM_REMOVED, team);

	team->ReleaseReference();

	return status;
}


/*!	Returns if the specified team \a parent has any children belonging to the
	process group with the specified ID \a groupID.
	The caller must hold \a parent's lock.
*/
static bool
has_children_in_group(Team* parent, pid_t groupID)
{
	for (Team* child = parent->children; child != NULL;
			child = child->siblings_next) {
		TeamLocker childLocker(child);
		if (child->group_id == groupID)
			return true;
	}

	return false;
}


/*!	Returns the first job control entry from \a children, which matches \a id.
	\a id can be:
	- \code > 0 \endcode: Matching an entry with that team ID.
	- \code == -1 \endcode: Matching any entry.
	- \code < -1 \endcode: Matching any entry with a process group ID of \c -id.
	\c 0 is an invalid value for \a id.

	The caller must hold the lock of the team that \a children belongs to.

	\param children The job control entry list to check.
	\param id The match criterion.
	\return The first matching entry or \c NULL, if none matches.
*/
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


/*!	Returns the first job control entry from one of team's dead, continued, or
    stopped children which matches \a id.
	\a id can be:
	- \code > 0 \endcode: Matching an entry with that team ID.
	- \code == -1 \endcode: Matching any entry.
	- \code < -1 \endcode: Matching any entry with a process group ID of \c -id.
	\c 0 is an invalid value for \a id.

	The caller must hold \a team's lock.

	\param team The team whose dead, stopped, and continued child lists shall be
		checked.
	\param id The match criterion.
	\param flags Specifies which children shall be considered. Dead children
		always are. Stopped children are considered when \a flags is ORed
		bitwise with \c WUNTRACED, continued children when \a flags is ORed
		bitwise with \c WCONTINUED.
	\return The first matching entry or \c NULL, if none matches.
*/
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
		InterruptsSpinLocker groupHashLocker(sGroupHashLock);

		ProcessGroup* group = sGroupHash.Lookup(group_id);
		if (group == NULL) {
			panic("job_control_entry::~job_control_entry(): unknown group "
				"ID: %" B_PRId32, group_id);
			return;
		}

		groupHashLocker.Unlock();

		group->ReleaseReference();
	}
}


/*!	Invoked when the owning team is dying, initializing the entry according to
	the dead state.

	The caller must hold the owning team's lock and the scheduler lock.
*/
void
job_control_entry::InitDeadState()
{
	if (team != NULL) {
		ASSERT(team->exit.initialized);

		group_id = team->group_id;
		team->group->AcquireReference();
		has_group_ref = true;

		thread = team->id;
		status = team->exit.status;
		reason = team->exit.reason;
		signal = team->exit.signal;
		signaling_user = team->exit.signaling_user;

		team = NULL;
	}
}


job_control_entry&
job_control_entry::operator=(const job_control_entry& other)
{
	state = other.state;
	thread = other.thread;
	signal = other.signal;
	has_group_ref = false;
	signaling_user = other.signaling_user;
	team = other.team;
	group_id = other.group_id;
	status = other.status;
	reason = other.reason;

	return *this;
}


/*! This is the kernel backend for waitid().
*/
static thread_id
wait_for_child(pid_t child, uint32 flags, siginfo_t& _info)
{
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;
	struct job_control_entry foundEntry;
	struct job_control_entry* freeDeathEntry = NULL;
	status_t status = B_OK;

	TRACE(("wait_for_child(child = %ld, flags = %ld)\n", child, flags));

	T(WaitForChild(child, flags));

	pid_t originalChild = child;

	bool ignoreFoundEntries = false;
	bool ignoreFoundEntriesChecked = false;

	while (true) {
		// lock the team
		TeamLocker teamLocker(team);

		// A 0 child argument means to wait for all children in the process
		// group of the calling team.
		child = originalChild == 0 ? -team->group_id : originalChild;

		// check whether any condition holds
		job_control_entry* entry = get_job_control_entry(team, child, flags);

		// If we don't have an entry yet, check whether there are any children
		// complying to the process group specification at all.
		if (entry == NULL) {
			// No success yet -- check whether there are any children complying
			// to the process group specification at all.
			bool childrenExist = false;
			if (child == -1) {
				childrenExist = team->children != NULL;
			} else if (child < -1) {
				childrenExist = has_children_in_group(team, -child);
			} else {
				if (Team* childTeam = Team::Get(child)) {
					BReference<Team> childTeamReference(childTeam, true);
					TeamLocker childTeamLocker(childTeam);
					childrenExist = childTeam->parent == team;
				}
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

			// unless WNOWAIT has been specified, "consume" the wait state
			if ((flags & WNOWAIT) == 0 || ignoreFoundEntries) {
				if (entry->state == JOB_CONTROL_STATE_DEAD) {
					// The child is dead. Reap its death entry.
					freeDeathEntry = entry;
					team->dead_children.entries.Remove(entry);
					team->dead_children.count--;
				} else {
					// The child is well. Reset its job control state.
					team_set_job_control_state(entry->team,
						JOB_CONTROL_STATE_NONE, NULL, false);
				}
			}
		}

		// If we haven't got anything yet, prepare for waiting for the
		// condition variable.
		ConditionVariableEntry deadWaitEntry;

		if (status == B_WOULD_BLOCK && (flags & WNOHANG) == 0)
			team->dead_children.condition_variable.Add(&deadWaitEntry);

		teamLocker.Unlock();

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
			teamLocker.Lock();

			struct sigaction& handler = team->SignalActionFor(SIGCHLD);
			if ((handler.sa_flags & SA_NOCLDWAIT) != 0
				|| handler.sa_handler == SIG_IGN) {
				ignoreFoundEntries = true;
			}

			teamLocker.Unlock();

			ignoreFoundEntriesChecked = true;
		}
	}

	delete freeDeathEntry;

	// When we got here, we have a valid death entry, and already got
	// unregistered from the team or group. Fill in the returned info.
	memset(&_info, 0, sizeof(_info));
	_info.si_signo = SIGCHLD;
	_info.si_pid = foundEntry.thread;
	_info.si_uid = foundEntry.signaling_user;
	// TODO: Fill in si_errno?

	switch (foundEntry.state) {
		case JOB_CONTROL_STATE_DEAD:
			_info.si_code = foundEntry.reason;
			_info.si_status = foundEntry.reason == CLD_EXITED
				? foundEntry.status : foundEntry.signal;
			break;
		case JOB_CONTROL_STATE_STOPPED:
			_info.si_code = CLD_STOPPED;
			_info.si_status = foundEntry.signal;
			break;
		case JOB_CONTROL_STATE_CONTINUED:
			_info.si_code = CLD_CONTINUED;
			_info.si_status = 0;
			break;
		case JOB_CONTROL_STATE_NONE:
			// can't happen
			break;
	}

	// If SIGCHLD is blocked, we shall clear pending SIGCHLDs, if no other child
	// status is available.
	TeamLocker teamLocker(team);
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (is_team_signal_blocked(team, SIGCHLD)) {
		if (get_job_control_entry(team, child, flags) == NULL)
			team->RemovePendingSignals(SIGNAL_TO_MASK(SIGCHLD));
	}

	schedulerLocker.Unlock();
	teamLocker.Unlock();

	// When the team is dead, the main thread continues to live in the kernel
	// team for a very short time. To avoid surprises for the caller we rather
	// wait until the thread is really gone.
	if (foundEntry.state == JOB_CONTROL_STATE_DEAD)
		wait_for_thread(foundEntry.thread, NULL);

	T(WaitForChildDone(foundEntry));

	return foundEntry.thread;
}


/*! Fills the team_info structure with information from the specified team.
	Interrupts must be enabled. The team must not be locked.
*/
static status_t
fill_team_info(Team* team, team_info* info, size_t size)
{
	if (size != sizeof(team_info))
		return B_BAD_VALUE;

	// TODO: Set more informations for team_info
	memset(info, 0, size);

	info->team = team->id;
		// immutable
	info->image_count = count_images(team);
		// protected by sImageMutex

	TeamLocker teamLocker(team);
	InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

	info->thread_count = team->num_threads;
	//info->area_count =
	info->debugger_nub_thread = team->debug_info.nub_thread;
	info->debugger_nub_port = team->debug_info.nub_port;
	//info->uid =
	//info->gid =

	strlcpy(info->args, team->Args(), sizeof(info->args));
	info->argc = 1;

	return B_OK;
}


/*!	Returns whether the process group contains stopped processes.
	The caller must hold the process group's lock.
*/
static bool
process_group_has_stopped_processes(ProcessGroup* group)
{
	Team* team = group->teams;
	while (team != NULL) {
		// the parent team's lock guards the job control entry -- acquire it
		team->LockTeamAndParent(false);

		if (team->job_control_entry != NULL
			&& team->job_control_entry->state == JOB_CONTROL_STATE_STOPPED) {
			team->UnlockTeamAndParent();
			return true;
		}

		team->UnlockTeamAndParent();

		team = team->group_next;
	}

	return false;
}


/*!	Iterates through all process groups queued in team_remove_team() and signals
	those that are orphaned and have stopped processes.
	The caller must not hold any team or process group locks.
*/
static void
orphaned_process_group_check()
{
	// process as long as there are groups in the list
	while (true) {
		// remove the head from the list
		MutexLocker orphanedCheckLocker(sOrphanedCheckLock);

		ProcessGroup* group = sOrphanedCheckProcessGroups.RemoveHead();
		if (group == NULL)
			return;

		group->UnsetOrphanedCheck();
		BReference<ProcessGroup> groupReference(group);

		orphanedCheckLocker.Unlock();

		AutoLocker<ProcessGroup> groupLocker(group);

		// If the group is orphaned and contains stopped processes, we're
		// supposed to send SIGHUP + SIGCONT.
		if (group->IsOrphaned() && process_group_has_stopped_processes(group)) {
			Thread* currentThread = thread_get_current_thread();

			Signal signal(SIGHUP, SI_USER, B_OK, currentThread->team->id);
			send_signal_to_process_group_locked(group, signal, 0);

			signal.SetNumber(SIGCONT);
			send_signal_to_process_group_locked(group, signal, 0);
		}
	}
}


static status_t
common_get_team_usage_info(team_id id, int32 who, team_usage_info* info,
	uint32 flags)
{
	if (who != B_TEAM_USAGE_SELF && who != B_TEAM_USAGE_CHILDREN)
		return B_BAD_VALUE;

	// get the team
	Team* team = Team::GetAndLock(id);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);
	TeamLocker teamLocker(team, true);

	if ((flags & B_CHECK_PERMISSION) != 0) {
		uid_t uid = geteuid();
		if (uid != 0 && uid != team->effective_uid)
			return B_NOT_ALLOWED;
	}

	bigtime_t kernelTime = 0;
	bigtime_t userTime = 0;

	switch (who) {
		case B_TEAM_USAGE_SELF:
		{
			Thread* thread = team->thread_list;

			for (; thread != NULL; thread = thread->team_next) {
				InterruptsSpinLocker threadTimeLocker(thread->time_lock);
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
				TeamLocker childLocker(child);

				Thread* thread = team->thread_list;

				for (; thread != NULL; thread = thread->team_next) {
					InterruptsSpinLocker threadTimeLocker(thread->time_lock);
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

	info->kernel_time = kernelTime;
	info->user_time = userTime;

	return B_OK;
}


//	#pragma mark - Private kernel API


status_t
team_init(kernel_args* args)
{
	// create the team hash table
	new(&sTeamHash) TeamTable;
	if (sTeamHash.Init(64) != B_OK)
		panic("Failed to init team hash table!");

	new(&sGroupHash) ProcessGroupHashTable;
	if (sGroupHash.Init() != B_OK)
		panic("Failed to init process group hash table!");

	// create initial session and process groups

	ProcessSession* session = new(std::nothrow) ProcessSession(1);
	if (session == NULL)
		panic("Could not create initial session.\n");
	BReference<ProcessSession> sessionReference(session, true);

	ProcessGroup* group = new(std::nothrow) ProcessGroup(1);
	if (group == NULL)
		panic("Could not create initial process group.\n");
	BReference<ProcessGroup> groupReference(group, true);

	group->Publish(session);

	// create the kernel team
	sKernelTeam = Team::Create(1, "kernel_team", true);
	if (sKernelTeam == NULL)
		panic("could not create kernel team!\n");
	sKernelTeam->SetArgs(sKernelTeam->Name());
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
	sTeamHash.Insert(sKernelTeam);

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
	InterruptsSpinLocker teamsLocker(sTeamHashLock);
	return sUsedTeams;
}


/*! Returns a death entry of a child team specified by ID (if any).
	The caller must hold the team's lock.

	\param team The team whose dead children list to check.
	\param child The ID of the child for whose death entry to lock. Must be > 0.
	\param _deleteEntry Return variable, indicating whether the caller needs to
		delete the returned entry.
	\return The death entry of the matching team, or \c NULL, if no death entry
		for the team was found.
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
	if (id <= 0)
		return false;

	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	return team_get_team_struct_locked(id) != NULL;
}


Team*
team_get_team_struct_locked(team_id id)
{
	return sTeamHash.Lookup(id);
}


void
team_set_controlling_tty(int32 ttyIndex)
{
	// lock the team, so its session won't change while we're playing with it
	Team* team = thread_get_current_thread()->team;
	TeamLocker teamLocker(team);

	// get and lock the session
	ProcessSession* session = team->group->Session();
	AutoLocker<ProcessSession> sessionLocker(session);

	// set the session's fields
	session->controlling_tty = ttyIndex;
	session->foreground_group = -1;
}


int32
team_get_controlling_tty()
{
	// lock the team, so its session won't change while we're playing with it
	Team* team = thread_get_current_thread()->team;
	TeamLocker teamLocker(team);

	// get and lock the session
	ProcessSession* session = team->group->Session();
	AutoLocker<ProcessSession> sessionLocker(session);

	// get the session's field
	return session->controlling_tty;
}


status_t
team_set_foreground_process_group(int32 ttyIndex, pid_t processGroupID)
{
	// lock the team, so its session won't change while we're playing with it
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;
	TeamLocker teamLocker(team);

	// get and lock the session
	ProcessSession* session = team->group->Session();
	AutoLocker<ProcessSession> sessionLocker(session);

	// check given TTY -- must be the controlling tty of the calling process
	if (session->controlling_tty != ttyIndex)
		return ENOTTY;

	// check given process group -- must belong to our session
	{
		InterruptsSpinLocker groupHashLocker(sGroupHashLock);
		ProcessGroup* group = sGroupHash.Lookup(processGroupID);
		if (group == NULL || group->Session() != session)
			return B_BAD_VALUE;
	}

	// If we are a background group, we can do that unharmed only when we
	// ignore or block SIGTTOU. Otherwise the group gets a SIGTTOU.
	if (session->foreground_group != -1
		&& session->foreground_group != team->group_id
		&& team->SignalActionFor(SIGTTOU).sa_handler != SIG_IGN) {
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		if (!is_team_signal_blocked(team, SIGTTOU)) {
			pid_t groupID = team->group_id;

			schedulerLocker.Unlock();
			sessionLocker.Unlock();
			teamLocker.Unlock();

			Signal signal(SIGTTOU, SI_USER, B_OK, team->id);
			send_signal_to_process_group(groupID, signal, 0);
			return B_INTERRUPTED;
		}
	}

	session->foreground_group = processGroupID;

	return B_OK;
}


/*!	Removes the specified team from the global team hash, from its process
	group, and from its parent.
	It also moves all of its children to the kernel team.

	The caller must hold the following locks:
	- \a team's process group's lock,
	- the kernel team's lock,
	- \a team's parent team's lock (might be the kernel team), and
	- \a team's lock.
*/
void
team_remove_team(Team* team, pid_t& _signalGroup)
{
	Team* parent = team->parent;

	// remember how long this team lasted
	parent->dead_children.kernel_time += team->dead_threads_kernel_time
		+ team->dead_children.kernel_time;
	parent->dead_children.user_time += team->dead_threads_user_time
		+ team->dead_children.user_time;

	// remove the team from the hash table
	InterruptsSpinLocker teamsLocker(sTeamHashLock);
	sTeamHash.Remove(team);
	sUsedTeams--;
	teamsLocker.Unlock();

	// The team can no longer be accessed by ID. Navigation to it is still
	// possible from its process group and its parent and children, but that
	// will be rectified shortly.
	team->state = TEAM_STATE_DEATH;

	// If we're a controlling process (i.e. a session leader with controlling
	// terminal), there's a bit of signalling we have to do. We can't do any of
	// the signaling here due to the bunch of locks we're holding, but we need
	// to determine, whom to signal.
	_signalGroup = -1;
	bool isSessionLeader = false;
	if (team->session_id == team->id
		&& team->group->Session()->controlling_tty >= 0) {
		isSessionLeader = true;

		ProcessSession* session = team->group->Session();

		AutoLocker<ProcessSession> sessionLocker(session);

		session->controlling_tty = -1;
		_signalGroup = session->foreground_group;
	}

	// remove us from our process group
	remove_team_from_group(team);

	// move the team's children to the kernel team
	while (Team* child = team->children) {
		// remove the child from the current team and add it to the kernel team
		TeamLocker childLocker(child);

		remove_team_from_parent(team, child);
		insert_team_into_parent(sKernelTeam, child);

		// move job control entries too
		sKernelTeam->stopped_children.entries.MoveFrom(
			&team->stopped_children.entries);
		sKernelTeam->continued_children.entries.MoveFrom(
			&team->continued_children.entries);

		// If the team was a session leader with controlling terminal,
		// we need to send SIGHUP + SIGCONT to all newly-orphaned process
		// groups with stopped processes. Due to locking complications we can't
		// do that here, so we only check whether we were a reason for the
		// child's process group not being an orphan and, if so, schedule a
		// later check (cf. orphaned_process_group_check()).
		if (isSessionLeader) {
			ProcessGroup* childGroup = child->group;
			if (childGroup->Session()->id == team->session_id
				&& childGroup->id != team->group_id) {
				childGroup->ScheduleOrphanedCheck();
			}
		}

		// Note, we don't move the dead children entries. Those will be deleted
		// when the team structure is deleted.
	}

	// remove us from our parent
	remove_team_from_parent(parent, team);
}


/*!	Kills all threads but the main thread of the team and shuts down user
	debugging for it.
	To be called on exit of the team's main thread. No locks must be held.

	\param team The team in question.
	\return The port of the debugger for the team, -1 if none. To be passed to
		team_delete_team().
*/
port_id
team_shutdown_team(Team* team)
{
	ASSERT(thread_get_current_thread() == team->main_thread);

	TeamLocker teamLocker(team);

	// Make sure debugging changes won't happen anymore.
	port_id debuggerPort = -1;
	while (true) {
		// If a debugger change is in progress for the team, we'll have to
		// wait until it is done.
		ConditionVariableEntry waitForDebuggerEntry;
		bool waitForDebugger = false;

		InterruptsSpinLocker debugInfoLocker(team->debug_info.lock);

		if (team->debug_info.debugger_changed_condition != NULL) {
			team->debug_info.debugger_changed_condition->Add(
				&waitForDebuggerEntry);
			waitForDebugger = true;
		} else if (team->debug_info.flags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) {
			// The team is being debugged. That will stop with the termination
			// of the nub thread. Since we set the team state to death, no one
			// can install a debugger anymore. We fetch the debugger's port to
			// send it a message at the bitter end.
			debuggerPort = team->debug_info.debugger_port;
		}

		debugInfoLocker.Unlock();

		if (!waitForDebugger)
			break;

		// wait for the debugger change to be finished
		teamLocker.Unlock();

		waitForDebuggerEntry.Wait();

		teamLocker.Lock();
	}

	// Mark the team as shutting down. That will prevent new threads from being
	// created and debugger changes from taking place.
	team->state = TEAM_STATE_SHUTDOWN;

	// delete all timers
	team->DeleteUserTimers(false);

	// deactivate CPU time user timers for the team
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (team->HasActiveCPUTimeUserTimers())
		team->DeactivateCPUTimeUserTimers();

	schedulerLocker.Unlock();

	// kill all threads but the main thread
	team_death_entry deathEntry;
	deathEntry.condition.Init(team, "team death");

	while (true) {
		team->death_entry = &deathEntry;
		deathEntry.remaining_threads = 0;

		Thread* thread = team->thread_list;
		while (thread != NULL) {
			if (thread != team->main_thread) {
				Signal signal(SIGKILLTHR, SI_USER, B_OK, team->id);
				send_signal_to_thread(thread, signal, B_DO_NOT_RESCHEDULE);
				deathEntry.remaining_threads++;
			}

			thread = thread->team_next;
		}

		if (deathEntry.remaining_threads == 0)
			break;

		// there are threads to wait for
		ConditionVariableEntry entry;
		deathEntry.condition.Add(&entry);

		teamLocker.Unlock();

		entry.Wait();

		teamLocker.Lock();
	}

	team->death_entry = NULL;

	return debuggerPort;
}


/*!	Called on team exit to notify threads waiting on the team and free most
	resources associated with it.
	The caller shouldn't hold any locks.
*/
void
team_delete_team(Team* team, port_id debuggerPort)
{
	// Not quite in our job description, but work that has been left by
	// team_remove_team() and that can be done now that we're not holding any
	// locks.
	orphaned_process_group_check();

	team_id teamID = team->id;

	ASSERT(team->num_threads == 0);

	// If someone is waiting for this team to be loaded, but it dies
	// unexpectedly before being done, we need to notify the waiting
	// thread now.

	TeamLocker teamLocker(team);

	if (team->loading_info) {
		// there's indeed someone waiting
		struct team_loading_info* loadingInfo = team->loading_info;
		team->loading_info = NULL;

		loadingInfo->result = B_ERROR;
		loadingInfo->done = true;

		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		// wake up the waiting thread
		if (loadingInfo->thread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(loadingInfo->thread);
	}

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

	teamLocker.Unlock();

	sNotificationService.Notify(TEAM_REMOVED, team);

	// free team resources

	delete_realtime_sem_context(team->realtime_sem_context);
	xsi_sem_undo(team);
	remove_images(team);
	team->address_space->RemoveAndPut();

	team->ReleaseReference();

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
	if (id == sKernelTeam->id) {
		// we're the kernel team, so we don't have to go through all
		// the hassle (locking and hash lookup)
		*_addressSpace = VMAddressSpace::GetKernel();
		return B_OK;
	}

	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	Team* team = team_get_team_struct_locked(id);
	if (team == NULL)
		return B_BAD_VALUE;

	team->address_space->Get();
	*_addressSpace = team->address_space;
	return B_OK;
}


/*!	Sets the team's job control state.
	The caller must hold the parent team's lock. Interrupts are allowed to be
	enabled or disabled. In the latter case the scheduler lock may be held as
	well.
	\a team The team whose job control state shall be set.
	\a newState The new state to be set.
	\a signal The signal the new state was caused by. Can \c NULL, if none. Then
		the caller is responsible for filling in the following fields of the
		entry before releasing the parent team's lock, unless the new state is
		\c JOB_CONTROL_STATE_NONE:
		- \c signal: The number of the signal causing the state change.
		- \c signaling_user: The real UID of the user sending the signal.
	\a schedulerLocked indicates whether the scheduler lock is being held, too.
*/
void
team_set_job_control_state(Team* team, job_control_state newState,
	Signal* signal, bool schedulerLocked)
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

	if (signal != NULL) {
		entry->signal = signal->Number();
		entry->signaling_user = signal->SendingUser();
	}

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
			schedulerLocked);
	}
}


/*!	Inits the given team's exit information, if not yet initialized, to some
	generic "killed" status.
	The caller must not hold the team's lock. Interrupts must be enabled.

	\param team The team whose exit info shall be initialized.
*/
void
team_init_exit_info_on_error(Team* team)
{
	TeamLocker teamLocker(team);

	if (!team->exit.initialized) {
		team->exit.reason = CLD_KILLED;
		team->exit.signal = SIGKILL;
		team->exit.signaling_user = geteuid();
		team->exit.status = 0;
		team->exit.initialized = true;
	}
}


/*! Adds a hook to the team that is called as soon as this team goes away.
	This call might get public in the future.
*/
status_t
start_watching_team(team_id teamID, void (*hook)(team_id, void*), void* data)
{
	if (hook == NULL || teamID < B_OK)
		return B_BAD_VALUE;

	// create the watcher object
	team_watcher* watcher = (team_watcher*)malloc(sizeof(team_watcher));
	if (watcher == NULL)
		return B_NO_MEMORY;

	watcher->hook = hook;
	watcher->data = data;

	// add watcher, if the team isn't already dying
	// get the team
	Team* team = Team::GetAndLock(teamID);
	if (team == NULL) {
		free(watcher);
		return B_BAD_TEAM_ID;
	}

	list_add_item(&team->watcher_list, watcher);

	team->UnlockAndReleaseReference();

	return B_OK;
}


status_t
stop_watching_team(team_id teamID, void (*hook)(team_id, void*), void* data)
{
	if (hook == NULL || teamID < 0)
		return B_BAD_VALUE;

	// get team and remove watcher (if present)
	Team* team = Team::GetAndLock(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;

	// search for watcher
	team_watcher* watcher = NULL;
	while ((watcher = (team_watcher*)list_get_next_item(
			&team->watcher_list, watcher)) != NULL) {
		if (watcher->hook == hook && watcher->data == data) {
			// got it!
			list_remove_item(&team->watcher_list, watcher);
			break;
		}
	}

	team->UnlockAndReleaseReference();

	if (watcher == NULL)
		return B_ENTRY_NOT_FOUND;

	free(watcher);
	return B_OK;
}


/*!	Allocates a user_thread structure from the team.
	The team lock must be held, unless the function is called for the team's
	main thread. Interrupts must be enabled.
*/
struct user_thread*
team_allocate_user_thread(Team* team)
{
	if (team->user_data == 0)
		return NULL;

	// take an entry from the free list, if any
	if (struct free_user_thread* entry = team->free_user_threads) {
		user_thread* thread = entry->thread;
		team->free_user_threads = entry->next;
		free(entry);
		return thread;
	}

	while (true) {
		// enough space left?
		size_t needed = ROUNDUP(sizeof(user_thread), 8);
		if (team->user_data_size - team->used_user_data < needed) {
			// try to resize the area
			if (resize_area(team->user_data_area,
					team->user_data_size + B_PAGE_SIZE) != B_OK) {
				return NULL;
			}

			// resized user area successfully -- try to allocate the user_thread
			// again
			team->user_data_size += B_PAGE_SIZE;
			continue;
		}

		// allocate the user_thread
		user_thread* thread
			= (user_thread*)(team->user_data + team->used_user_data);
		team->used_user_data += needed;

		return thread;
	}
}


/*!	Frees the given user_thread structure.
	The team's lock must not be held. Interrupts must be enabled.
	\param team The team the user thread was allocated from.
	\param userThread The user thread to free.
*/
void
team_free_user_thread(Team* team, struct user_thread* userThread)
{
	if (userThread == NULL)
		return;

	// create a free list entry
	free_user_thread* entry
		= (free_user_thread*)malloc(sizeof(free_user_thread));
	if (entry == NULL) {
		// we have to leak the user thread :-/
		return;
	}

	// add to free list
	TeamLocker teamLocker(team);

	entry->thread = userThread;
	entry->next = team->free_user_threads;
	team->free_user_threads = entry;
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
	// check whether the team exists
	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	Team* team = team_get_team_struct_locked(id);
	if (team == NULL)
		return B_BAD_TEAM_ID;

	id = team->id;

	teamsLocker.Unlock();

	// wait for the main thread (it has the same ID as the team)
	return wait_for_thread(id, _returnCode);
}


status_t
kill_team(team_id id)
{
	InterruptsSpinLocker teamsLocker(sTeamHashLock);

	Team* team = team_get_team_struct_locked(id);
	if (team == NULL)
		return B_BAD_TEAM_ID;

	id = team->id;

	teamsLocker.Unlock();

	if (team == sKernelTeam)
		return B_NOT_ALLOWED;

	// Just kill the team's main thread (it has same ID as the team). The
	// cleanup code there will take care of the team.
	return kill_thread(id);
}


status_t
_get_team_info(team_id id, team_info* info, size_t size)
{
	// get the team
	Team* team = Team::Get(id);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// fill in the info
	return fill_team_info(team, info, size);
}


status_t
_get_next_team_info(int32* cookie, team_info* info, size_t size)
{
	int32 slot = *cookie;
	if (slot < 1)
		slot = 1;

	InterruptsSpinLocker locker(sTeamHashLock);

	team_id lastTeamID = peek_next_thread_id();
		// TODO: This is broken, since the id can wrap around!

	// get next valid team
	Team* team = NULL;
	while (slot < lastTeamID && !(team = team_get_team_struct_locked(slot)))
		slot++;

	if (team == NULL)
		return B_BAD_TEAM_ID;

	// get a reference to the team and unlock
	BReference<Team> teamReference(team);
	locker.Unlock();

	// fill in the info
	*cookie = ++slot;
	return fill_team_info(team, info, size);
}


status_t
_get_team_usage_info(team_id id, int32 who, team_usage_info* info, size_t size)
{
	if (size != sizeof(team_usage_info))
		return B_BAD_VALUE;

	return common_get_team_usage_info(id, who, info, 0);
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

	TeamLocker teamLocker(team);

	return team->parent->id;
}


pid_t
getpgid(pid_t id)
{
	if (id < 0) {
		errno = EINVAL;
		return -1;
	}

	if (id == 0) {
		// get process group of the calling process
		Team* team = thread_get_current_thread()->team;
		TeamLocker teamLocker(team);
		return team->group_id;
	}

	// get the team
	Team* team = Team::GetAndLock(id);
	if (team == NULL) {
		errno = ESRCH;
		return -1;
	}

	// get the team's process group ID
	pid_t groupID = team->group_id;

	team->UnlockAndReleaseReference();

	return groupID;
}


pid_t
getsid(pid_t id)
{
	if (id < 0) {
		errno = EINVAL;
		return -1;
	}

	if (id == 0) {
		// get session of the calling process
		Team* team = thread_get_current_thread()->team;
		TeamLocker teamLocker(team);
		return team->session_id;
	}

	// get the team
	Team* team = Team::GetAndLock(id);
	if (team == NULL) {
		errno = ESRCH;
		return -1;
	}

	// get the team's session ID
	pid_t sessionID = team->session_id;

	team->UnlockAndReleaseReference();

	return sessionID;
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


pid_t
_user_wait_for_child(thread_id child, uint32 flags, siginfo_t* userInfo)
{
	if (userInfo != NULL && !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	siginfo_t info;
	pid_t foundChild = wait_for_child(child, flags, info);
	if (foundChild < 0)
		return syscall_restart_handle_post(foundChild);

	// copy info back to userland
	if (userInfo != NULL && user_memcpy(userInfo, &info, sizeof(info)) != B_OK)
		return B_BAD_ADDRESS;

	return foundChild;
}


pid_t
_user_process_info(pid_t process, int32 which)
{
	// we only allow to return the parent of the current process
	if (which == PARENT_ID
		&& process != 0 && process != thread_get_current_thread()->team->id)
		return B_BAD_VALUE;

	pid_t result;
	switch (which) {
		case SESSION_ID:
			result = getsid(process);
			break;
		case GROUP_ID:
			result = getpgid(process);
			break;
		case PARENT_ID:
			result = getppid();
			break;
		default:
			return B_BAD_VALUE;
	}

	return result >= 0 ? result : errno;
}


pid_t
_user_setpgid(pid_t processID, pid_t groupID)
{
	// setpgid() can be called either by the parent of the target process or
	// by the process itself to do one of two things:
	// * Create a new process group with the target process' ID and the target
	//   process as group leader.
	// * Set the target process' process group to an already existing one in the
	//   same session.

	if (groupID < 0)
		return B_BAD_VALUE;

	Team* currentTeam = thread_get_current_thread()->team;
	if (processID == 0)
		processID = currentTeam->id;

	// if the group ID is not specified, use the target process' ID
	if (groupID == 0)
		groupID = processID;

	// We loop when running into the following race condition: We create a new
	// process group, because there isn't one with that ID yet, but later when
	// trying to publish it, we find that someone else created and published
	// a group with that ID in the meantime. In that case we just restart the
	// whole action.
	while (true) {
		// Look up the process group by ID. If it doesn't exist yet and we are
		// allowed to create a new one, do that.
		ProcessGroup* group = ProcessGroup::Get(groupID);
		bool newGroup = false;
		if (group == NULL) {
			if (groupID != processID)
				return B_NOT_ALLOWED;

			group = new(std::nothrow) ProcessGroup(groupID);
			if (group == NULL)
				return B_NO_MEMORY;

			newGroup = true;
		}
		BReference<ProcessGroup> groupReference(group, true);

		// get the target team
		Team* team = Team::Get(processID);
		if (team == NULL)
			return ESRCH;
		BReference<Team> teamReference(team, true);

		// lock the new process group and the team's current process group
		while (true) {
			// lock the team's current process group
			team->LockProcessGroup();

			ProcessGroup* oldGroup = team->group;
			if (oldGroup == group) {
				// it's the same as the target group, so just bail out
				oldGroup->Unlock();
				return group->id;
			}

			oldGroup->AcquireReference();

			// lock the target process group, if locking order allows it
			if (newGroup || group->id > oldGroup->id) {
				group->Lock();
				break;
			}

			// try to lock
			if (group->TryLock())
				break;

			// no dice -- unlock the team's current process group and relock in
			// the correct order
			oldGroup->Unlock();

			group->Lock();
			oldGroup->Lock();

			// check whether things are still the same
			TeamLocker teamLocker(team);
			if (team->group == oldGroup)
				break;

			// something changed -- unlock everything and retry
			teamLocker.Unlock();
			oldGroup->Unlock();
			group->Unlock();
			oldGroup->ReleaseReference();
		}

		// we now have references and locks of both new and old process group
		BReference<ProcessGroup> oldGroupReference(team->group, true);
		AutoLocker<ProcessGroup> oldGroupLocker(team->group, true);
		AutoLocker<ProcessGroup> groupLocker(group, true);

		// also lock the target team and its parent
		team->LockTeamAndParent(false);
		TeamLocker parentLocker(team->parent, true);
		TeamLocker teamLocker(team, true);

		// perform the checks
		if (team == currentTeam) {
			// we set our own group

			// we must not change our process group ID if we're a session leader
			if (is_session_leader(currentTeam))
				return B_NOT_ALLOWED;
		} else {
			// Calling team != target team. The target team must be a child of
			// the calling team and in the same session. (If that's the case it
			// isn't a session leader either.)
			if (team->parent != currentTeam
				|| team->session_id != currentTeam->session_id) {
				return B_NOT_ALLOWED;
			}

			// The call is also supposed to fail on a child, when the child has
			// already executed exec*() [EACCES].
			if ((team->flags & TEAM_FLAG_EXEC_DONE) != 0)
				return EACCES;
		}

		// If we created a new process group, publish it now.
		if (newGroup) {
			InterruptsSpinLocker groupHashLocker(sGroupHashLock);
			if (sGroupHash.Lookup(groupID)) {
				// A group with the group ID appeared since we first checked.
				// Back to square one.
				continue;
			}

			group->PublishLocked(team->group->Session());
		} else if (group->Session()->id != team->session_id) {
			// The existing target process group belongs to a different session.
			// That's not allowed.
			return B_NOT_ALLOWED;
		}

		// Everything is ready -- set the group.
		remove_team_from_group(team);
		insert_team_into_group(group, team);

		// Changing the process group might have changed the situation for a
		// parent waiting in wait_for_child(). Hence we notify it.
		team->parent->dead_children.condition_variable.NotifyAll(false);

		return group->id;
	}
}


pid_t
_user_setsid(void)
{
	Team* team = thread_get_current_thread()->team;

	// create a new process group and session
	ProcessGroup* group = new(std::nothrow) ProcessGroup(team->id);
	if (group == NULL)
		return B_NO_MEMORY;
	BReference<ProcessGroup> groupReference(group, true);
	AutoLocker<ProcessGroup> groupLocker(group);

	ProcessSession* session = new(std::nothrow) ProcessSession(group->id);
	if (session == NULL)
		return B_NO_MEMORY;
	BReference<ProcessSession> sessionReference(session, true);

	// lock the team's current process group, parent, and the team itself
	team->LockTeamParentAndProcessGroup();
	BReference<ProcessGroup> oldGroupReference(team->group);
	AutoLocker<ProcessGroup> oldGroupLocker(team->group, true);
	TeamLocker parentLocker(team->parent, true);
	TeamLocker teamLocker(team, true);

	// the team must not already be a process group leader
	if (is_process_group_leader(team))
		return B_NOT_ALLOWED;

	// remove the team from the old and add it to the new process group
	remove_team_from_group(team);
	group->Publish(session);
	insert_team_into_group(group, team);

	// Changing the process group might have changed the situation for a
	// parent waiting in wait_for_child(). Hence we notify it.
	team->parent->dead_children.condition_variable.NotifyAll(false);

	return group->id;
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

	// set this thread's exit status
	thread->exit.status = returnValue;

	// set the team exit status
	TeamLocker teamLocker(team);

	if (!team->exit.initialized) {
		team->exit.reason = CLD_EXITED;
		team->exit.signal = 0;
		team->exit.signaling_user = 0;
		team->exit.status = returnValue;
		team->exit.initialized = true;
	}

	teamLocker.Unlock();

	// Stop the thread, if the team is being debugged and that has been
	// requested.
	if ((atomic_get(&team->debug_info.flags) & B_TEAM_DEBUG_PREVENT_EXIT) != 0)
		user_debug_stop_thread();

	// Send this thread a SIGKILL. This makes sure the thread will not return to
	// userland. The signal handling code forwards the signal to the main
	// thread (if that's not already this one), which will take the team down.
	Signal signal(SIGKILL, SI_USER, B_OK, team->id);
	send_signal_to_thread(thread, signal, 0);
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
	if (size != sizeof(team_usage_info))
		return B_BAD_VALUE;

	team_usage_info info;
	status_t status = common_get_team_usage_info(team, who, &info,
		B_CHECK_PERMISSION);

	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(userInfo, &info, size) != B_OK) {
		return B_BAD_ADDRESS;
	}

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
		// allocate memory for a copy of the needed team data
		struct ExtendedTeamData {
			team_id	id;
			pid_t	group_id;
			pid_t	session_id;
			uid_t	real_uid;
			gid_t	real_gid;
			uid_t	effective_uid;
			gid_t	effective_gid;
			char	name[B_OS_NAME_LENGTH];
		};

		ExtendedTeamData* teamClone
			= (ExtendedTeamData*)malloc(sizeof(ExtendedTeamData));
			// It would be nicer to use new, but then we'd have to use
			// ObjectDeleter and declare the structure outside of the function
			// due to template parameter restrictions.
		if (teamClone == NULL)
			return B_NO_MEMORY;
		MemoryDeleter teamCloneDeleter(teamClone);

		io_context* ioContext;
		{
			// get the team structure
			Team* team = Team::GetAndLock(teamID);
			if (team == NULL)
				return B_BAD_TEAM_ID;
			BReference<Team> teamReference(team, true);
			TeamLocker teamLocker(team, true);

			// copy the data
			teamClone->id = team->id;
			strlcpy(teamClone->name, team->Name(), sizeof(teamClone->name));
			teamClone->group_id = team->group_id;
			teamClone->session_id = team->session_id;
			teamClone->real_uid = team->real_uid;
			teamClone->real_gid = team->real_gid;
			teamClone->effective_uid = team->effective_uid;
			teamClone->effective_gid = team->effective_gid;

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
