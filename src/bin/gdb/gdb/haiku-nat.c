/* Haiku native-dependent code common to multiple platforms.

   Copyright 2005 Ingo Weinhold <bonefish@cs.tu-berlin.de>.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <string.h>

#include <debugger.h>
#include <image.h>
#include <OS.h>

#include <debug_support.h>

#include "defs.h"	// include this first -- otherwise "command.h" will choke
#include "command.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "haiku-nat.h"
#include "inferior.h"
#include "observer.h"
#include "regcache.h"
#include "solib-haiku.h"
#include "symfile.h"
#include "target.h"

//#define TRACE_HAIKU_NAT
#ifdef TRACE_HAIKU_NAT
	#define TRACE(x)	printf x
#else
	#define TRACE(x)	while (false) {}
#endif

static struct target_ops *sHaikuTarget = NULL;

typedef struct debug_event {
	struct debug_event			*next;
	debug_debugger_message		message;
	debug_debugger_message_data	data;
} debug_event;

typedef struct debug_event_list {
	debug_event		*head;
	debug_event		*tail;
} debug_event_list;

typedef enum {
	SIGNAL_ARRIVED,		// the signal has actually arrived and is going to be
						// handled (in any way)
	SIGNAL_WILL_ARRIVE,	// the signal has not yet been sent, but will be sent
						// (e.g. an exception occurred and will cause the
						// signal when not being ignored)
	SIGNAL_FAKED,		// the signal didn't arrive and won't arrive, we faked
						// it (often in case of SIGTRAP)
} pending_signal_status;

typedef struct thread_debug_info {
	struct thread_debug_info	*next;
	thread_id					thread;
	bool						stopped;
	debug_event					*last_event;		// last debug message from
													// the thread; valid only,
													// if stopped
	int							reprocess_event;	// > 0, the event
													// shall be processed a
													// that many times, which
													// will probably trigger
													// another target event
	int							signal;				// only valid, if stopped
	pending_signal_status		signal_status;
} thread_debug_info;

typedef struct extended_image_info {
	haiku_image_info			info;
	struct extended_image_info	*next;
} extended_image_info;

typedef struct team_debug_info {
	team_id				team;
	port_id				debugger_port;
	thread_id			nub_thread;
	debug_context		context;
	thread_debug_info	*threads;
	extended_image_info	*images;
	debug_event_list	events;
} team_debug_info;

static team_debug_info sTeamDebugInfo;


typedef struct signal_map_entry {
	enum target_signal	target;
	int					haiku;
} signal_map_entry;

static const signal_map_entry sSignalMap[] = {
	{ TARGET_SIGNAL_0,			0 },
	{ TARGET_SIGNAL_HUP,		SIGHUP },
	{ TARGET_SIGNAL_INT,		SIGINT },
	{ TARGET_SIGNAL_QUIT,		SIGQUIT },
	{ TARGET_SIGNAL_ILL,		SIGILL },
	{ TARGET_SIGNAL_CHLD,		SIGCHLD },
	{ TARGET_SIGNAL_ABRT,		SIGABRT },
	{ TARGET_SIGNAL_PIPE,		SIGPIPE },
	{ TARGET_SIGNAL_FPE,		SIGFPE },
	{ TARGET_SIGNAL_KILL,		SIGKILL },
	{ TARGET_SIGNAL_STOP,		SIGSTOP },
	{ TARGET_SIGNAL_SEGV,		SIGSEGV },
	{ TARGET_SIGNAL_CONT,		SIGCONT },
	{ TARGET_SIGNAL_TSTP,		SIGTSTP },
	{ TARGET_SIGNAL_ALRM,		SIGALRM },
	{ TARGET_SIGNAL_TERM,		SIGTERM },
	{ TARGET_SIGNAL_TTIN,		SIGTTIN },
	{ TARGET_SIGNAL_TTOU,		SIGTTOU },
	{ TARGET_SIGNAL_USR1,		SIGUSR1 },
	{ TARGET_SIGNAL_USR2,		SIGUSR2 },
	{ TARGET_SIGNAL_WINCH,		SIGWINCH },
	{ TARGET_SIGNAL_TRAP,		SIGTRAP },
	{ TARGET_SIGNAL_0,			SIGKILLTHR },	// not debuggable anyway
	{ -1, -1 }
};


// #pragma mark -


static char *haiku_pid_to_str (ptid_t ptid);


static int
target_to_haiku_signal(enum target_signal targetSignal)
{
	int i;

	if (targetSignal < 0)
		return -1;

	for (i = 0; sSignalMap[i].target != -1 || sSignalMap[i].haiku != -1; i++) {
		if (targetSignal == sSignalMap[i].target)
			return sSignalMap[i].haiku;
	}

	return -1;
}


static enum target_signal
haiku_to_target_signal(int haikuSignal)
{
	int i;

	if (haikuSignal < 0)
		return TARGET_SIGNAL_0;

	for (i = 0; sSignalMap[i].target != -1 || sSignalMap[i].haiku != -1; i++) {
		if (haikuSignal == sSignalMap[i].haiku)
			return sSignalMap[i].target;
	}

	return TARGET_SIGNAL_UNKNOWN;
}


static thread_debug_info *
haiku_find_thread(team_debug_info *teamDebugInfo, thread_id threadID)
{
	thread_debug_info *info;
	for (info = teamDebugInfo->threads; info; info = info->next) {
		if (info->thread == threadID)
			return info;
	}

	return NULL;
}


static thread_debug_info *
haiku_add_thread(team_debug_info *teamDebugInfo, thread_id threadID)
{
	struct thread_info *gdbThreadInfo;
	thread_debug_info *threadDebugInfo;

	if (threadID == teamDebugInfo->nub_thread) {
		error("haiku_thread_added(): Trying to add debug nub thread (%ld)\n",
			threadID);
	}

	// find the thread first
	threadDebugInfo = haiku_find_thread(teamDebugInfo, threadID);
	if (threadDebugInfo)
		return threadDebugInfo;

	// allocate a new thread debug info
	threadDebugInfo = XMALLOC(thread_debug_info);
	if (!threadDebugInfo)
		error("haiku_thread_added(): Out of memory!\n");

	// init and add it
	threadDebugInfo->thread = threadID;
	threadDebugInfo->next = teamDebugInfo->threads;
	threadDebugInfo->stopped = false;
	threadDebugInfo->last_event = NULL;
	teamDebugInfo->threads = threadDebugInfo;

	// add it to gdb's thread DB
	gdbThreadInfo = add_thread(ptid_build(teamDebugInfo->team, 0, threadID));

	// Note: In theory we could spare us the whole thread list management, since
	// gdb's thread DB is doing exactly the same. We could put our data as
	// thread_info::private. The only catch is that when the thread_info is
	// freed, xfree() is invoked on the private data directly, but there's no
	// callback invoked before that would allow us to do cleanup (e.g. free
	// last_event).

	TRACE(("haiku_add_thread(): team %ld thread %ld added: "
		"gdb thread info: %p\n", teamDebugInfo->team, threadID, gdbThreadInfo));

	return threadDebugInfo;
}


static void
haiku_remove_thread(team_debug_info *teamDebugInfo, thread_id threadID)
{
	thread_debug_info **info;
	for (info = &teamDebugInfo->threads; *info; info = &(*info)->next) {
		if ((*info)->thread == threadID) {
			thread_debug_info *foundInfo = *info;
			*info = foundInfo->next;
			if (foundInfo->last_event)
				xfree(foundInfo->last_event);
			xfree(foundInfo);

			// remove it from gdb's thread DB
			delete_thread(ptid_build(teamDebugInfo->team, 0, threadID));

			return;
		}
	}
}


static void
haiku_init_thread_list(team_debug_info *teamDebugInfo)
{
	thread_info threadInfo;
	int32 cookie = 0;

	// init gdb's thread DB
	init_thread_list();

	while (get_next_thread_info(teamDebugInfo->team, &cookie, &threadInfo)
		== B_OK) {
		if (threadInfo.thread != teamDebugInfo->nub_thread)
			haiku_add_thread(teamDebugInfo, threadInfo.thread);
	}
}


static void
haiku_cleanup_thread_list(team_debug_info *teamDebugInfo)
{
	while (teamDebugInfo->threads) {
		thread_debug_info *thread = teamDebugInfo->threads;
		teamDebugInfo->threads = thread->next;
		xfree(thread);
	}

	// clear gdb's thread DB
	init_thread_list();
}


// #pragma mark -

static extended_image_info **
haiku_find_image_insert_location(team_debug_info *teamDebugInfo,
	image_id imageID)
{
	extended_image_info **image;

	for (image = &teamDebugInfo->images; *image; image = &(*image)->next) {
		if ((*image)->info.id >= imageID)
			return image;
	}

	return image;
}


static extended_image_info *
haiku_find_image(team_debug_info *teamDebugInfo, image_id imageID)
{
	extended_image_info *image = *haiku_find_image_insert_location(
		teamDebugInfo, imageID);

	return (image && image->info.id == imageID ? image : NULL);
}


static extended_image_info *
haiku_add_image(team_debug_info *teamDebugInfo, image_info *imageInfo)
{
	extended_image_info **imageP = haiku_find_image_insert_location(
		teamDebugInfo, imageInfo->id);
	extended_image_info *image = *imageP;

	// already known?
	if (image && image->info.id == imageInfo->id)
		return image;

	image = XMALLOC(extended_image_info);
	if (!image)
		error("haiku_add_image(): Out of memory!");

	image->info.id = imageInfo->id;
	strncpy(image->info.name, imageInfo->name, sizeof(image->info.name));
	image->info.name[sizeof(image->info.name) - 1] = '\0';
		// TODO: This should be the shared objects soname, not the path. We
		// probably need to extend the debugger API.
	strncpy(image->info.path, imageInfo->name, sizeof(image->info.path));
	image->info.path[sizeof(image->info.path) - 1] = '\0';
	image->info.text_address = (CORE_ADDR)imageInfo->text;
	image->info.text_size = imageInfo->text_size;
	image->info.data_address = (CORE_ADDR)imageInfo->data;
	image->info.data_size = imageInfo->data_size;
	image->info.is_app_image = (imageInfo->type == B_APP_IMAGE);

	image->next = *imageP;
	*imageP = image;

	return image;
}


static void
haiku_remove_image(team_debug_info *teamDebugInfo, image_id imageID)
{
	extended_image_info **imageP = haiku_find_image_insert_location(
		teamDebugInfo, imageID);
	extended_image_info *image = *imageP;

	if (image && image->info.id == imageID) {
		*imageP = image->next;
		xfree(image);
	}
}


static void
haiku_init_image_list(team_debug_info *teamDebugInfo)
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(teamDebugInfo->team, &cookie, &info) == B_OK)
		haiku_add_image(teamDebugInfo, &info);
}


static void
haiku_cleanup_image_list(team_debug_info *teamDebugInfo)
{
	while (teamDebugInfo->images) {
		extended_image_info *image = teamDebugInfo->images;
		teamDebugInfo->images = image->next;
		xfree(image);
	}
}


/*
 * Service function. Call with -1 the first time.
 */
struct haiku_image_info *
haiku_get_next_image_info(int lastID)
{
	extended_image_info *image = *haiku_find_image_insert_location(
		&sTeamDebugInfo, (lastID >= 0 ? lastID : 0));

	if (image && image->info.id == lastID)
		image = image->next;

	return (image ? &image->info : NULL);
}


// #pragma mark -

static debug_event *
haiku_enqueue_debug_event(debug_event_list *list, int32 message,
	debug_debugger_message_data *data, int32 size)
{
	debug_event *event = XMALLOC(debug_event);

	if (!event)
		error("haiku_enqueue_debug_event(): Out of memory!\n");

	// init the event
	event->next = NULL;
	event->message = message;
	memcpy(&event->data, data,
		(size >= 0 ? size : sizeof(debug_debugger_message_data)));

	// add it to the queue
	if (list->tail) {
		list->tail->next = event;
		list->tail = event;
	} else {
		list->head = list->tail = event;
	}

	return event;
}


static debug_event *
haiku_dequeue_next_debug_event(debug_event_list *list)
{
	debug_event *event = list->head;

	if (event) {
		// remove it from the queue
		list->head = event->next;
		if (list->tail == event)
			list->tail = NULL;

		event->next = NULL;	// just because we're paranoid
	}

	return event;
}


static debug_event *
haiku_remove_debug_event(debug_event_list *list, debug_event *eventToRemove)
{
	debug_event **event;

	if (eventToRemove) {
		for (event = &list->head; *event; event = &(*event)->next) {
			if (*event == eventToRemove) {
				*event = (*event)->next;
				if (eventToRemove == list->tail)
					list->tail = NULL;
				return eventToRemove;
			}
		}
	}

	error("haiku_remove_debug_event(): event %p not found in list %p\n",
		eventToRemove, list);

	return NULL;
}


static void
haiku_clear_debug_event_list(debug_event_list *list)
{
	debug_event *event;

	while ((event = haiku_dequeue_next_debug_event(list)))
		xfree(event);
}


static debug_event *
haiku_find_next_debug_event(debug_event_list *list,
	bool (*predicate)(void *closure, debug_event *event), void *closure)
{
	debug_event *event;

	for (event = list->head; event; event = event->next) {
		if ((*predicate)(closure, event))
			return event;
	}

	return NULL;
}


static void
haiku_read_pending_debug_events(team_debug_info *teamDebugInfo,
	bool block, bool (*block_predicate)(void *closure, debug_event *event),
	void *closure)
{
	while (true) {
		// read the next message from the debugger port
		debug_debugger_message_data message;
		int32 code;
		ssize_t bytesRead;
		debug_event *event;

//		TRACE(("haiku_read_pending_debug_events(): reading from debugger port "
//			"(%sblocking)...\n", (block ? "" : "non-")));

		do {
			bytesRead = read_port_etc(teamDebugInfo->debugger_port, &code,
				&message, sizeof(message), (block ? 0 : B_RELATIVE_TIMEOUT), 0);
		} while (bytesRead == B_INTERRUPTED);

		if (bytesRead < 0) {
			if (bytesRead == B_WOULD_BLOCK && !block)
				break;

			error("Failed to read from debugger port: %s\n",
				strerror(bytesRead));
		}

//		TRACE(("haiku_read_pending_debug_events(): got event: %lu, "
//			"thread: %ld, team: %ld, nub port: %ld\n", code,
//			message.origin.thread, message.origin.team,
//			message.origin.nub_port));

		// got a message: queue it
		event = haiku_enqueue_debug_event(&teamDebugInfo->events, code,
			&message, bytesRead);

		block = !(*block_predicate)(closure, event);
	}
}


typedef struct thread_event_closure {
	team_debug_info	*context;
	thread_id		thread;
	debug_event		*event;
} thread_event_closure;


static bool
haiku_thread_event_predicate(void *_closure, debug_event *event)
{
	thread_event_closure *closure = (thread_event_closure*)_closure;

	if (event->message == B_DEBUGGER_MESSAGE_TEAM_DELETED) {
		if (closure->context->team < 0
			|| event->data.origin.team == closure->context->team) {
			closure->event = event;
		}
	}

	if (!closure->event) {
		if (event->data.origin.team == closure->context->team
			&& (closure->thread < 0
				|| closure->thread == event->data.origin.thread)) {
			closure->event = event;
		}
	}

	return (closure->event != NULL);
}


// #pragma mark -

static void
haiku_cleanup_team_debug_info()
{
	destroy_debug_context(&sTeamDebugInfo.context);
	delete_port(sTeamDebugInfo.debugger_port);
	sTeamDebugInfo.debugger_port = -1;
	sTeamDebugInfo.team = -1;

	haiku_cleanup_thread_list(&sTeamDebugInfo);
	haiku_cleanup_image_list(&sTeamDebugInfo);
}


static status_t
haiku_send_debugger_message(team_debug_info *teamDebugInfo, int32 messageCode,
	const void *message, int32 messageSize, void *reply, int32 replySize)
{
	return send_debug_message(&teamDebugInfo->context, messageCode,
		message, messageSize, reply, replySize);
}


static void
haiku_init_child_debugging (thread_id threadID, bool debugThread)
{
	thread_info threadInfo;
	status_t result;
	port_id nubPort;

	// get a thread info
	result = get_thread_info(threadID, &threadInfo);
	if (result != B_OK)
		error("Thread with ID %ld not found: %s", threadID, strerror(result));

	// init our team debug structure
	sTeamDebugInfo.team = threadInfo.team;
	sTeamDebugInfo.debugger_port = -1;
	sTeamDebugInfo.context.nub_port = -1;
	sTeamDebugInfo.context.reply_port = -1;
	sTeamDebugInfo.threads = NULL;
	sTeamDebugInfo.events.head = NULL;
	sTeamDebugInfo.events.tail = NULL;

	// create the debugger port
	sTeamDebugInfo.debugger_port = create_port(10, "gdb debug");
	if (sTeamDebugInfo.debugger_port < 0) {
		error("Failed to create debugger port: %s",
			strerror(sTeamDebugInfo.debugger_port));
	}

	// install ourselves as the team debugger
	nubPort = install_team_debugger(sTeamDebugInfo.team,
		sTeamDebugInfo.debugger_port);
	if (nubPort < 0) {
		error("Failed to install ourselves as debugger for team %ld: %s",
			sTeamDebugInfo.team, strerror(nubPort));
	}

	// get the nub thread
	{
		team_info teamInfo;
		result = get_team_info(sTeamDebugInfo.team, &teamInfo);
		if (result != B_OK) {
			error("Failed to get info for team %ld: %s\n",
				sTeamDebugInfo.team, strerror(result));
		}

		sTeamDebugInfo.nub_thread = teamInfo.debugger_nub_thread;
	}

	// init the debug context
	result = init_debug_context(&sTeamDebugInfo.context, sTeamDebugInfo.team,
		nubPort);
	if (result != B_OK) {
		error("Failed to init debug context for team %ld: %s\n",
			sTeamDebugInfo.team, strerror(result));
	}

	// start debugging the thread
	if (debugThread) {
		result = debug_thread(threadID);
		if (result != B_OK) {
			error("Failed to start debugging thread %ld: %s", threadID,
				strerror(result));
		}
	}

	// set the team debug flags
	{
		debug_nub_set_team_flags message;
		message.flags = B_TEAM_DEBUG_SIGNALS /*| B_TEAM_DEBUG_PRE_SYSCALL
			| B_TEAM_DEBUG_POST_SYSCALL*/ | B_TEAM_DEBUG_TEAM_CREATION
			| B_TEAM_DEBUG_THREADS | B_TEAM_DEBUG_IMAGES;
			// TODO: We probably don't need all events

		haiku_send_debugger_message(&sTeamDebugInfo,
			B_DEBUG_MESSAGE_SET_TEAM_FLAGS, &message, sizeof(message), NULL, 0);
	}


	// the fun can start: push the target and init the rest
	push_target(sHaikuTarget);

	haiku_init_thread_list(&sTeamDebugInfo);
	haiku_init_image_list(&sTeamDebugInfo);

	disable_breakpoints_in_shlibs (1);

//	child_clear_solibs ();
		// TODO: Implement? Do we need this?

	clear_proceed_status ();
	init_wait_for_inferior ();

	target_terminal_init ();
	target_terminal_inferior ();
}


static void
haiku_continue_thread(team_debug_info *teamDebugInfo, thread_id threadID,
	uint32 handleEvent, bool step)
{
	debug_nub_continue_thread message;
	status_t err;

	message.thread = threadID;
	message.handle_event = handleEvent;
	message.single_step = step;

	err = haiku_send_debugger_message(teamDebugInfo,
		B_DEBUG_MESSAGE_CONTINUE_THREAD, &message, sizeof(message), NULL, 0);
	if (err != B_OK) {
		printf_unfiltered ("Failed to resume thread %ld: %s\n", threadID,
			strerror(err));
	}
}


static status_t
haiku_stop_thread(team_debug_info *teamDebugInfo, thread_id threadID)
{
	// check whether we know the thread
	status_t err;
	thread_event_closure threadEventClosure;

	thread_debug_info *threadInfo = haiku_find_thread(teamDebugInfo, threadID);
	if (!threadInfo)
		return B_BAD_THREAD_ID;

	// already stopped?
	if (threadInfo->stopped)
		return B_OK;

	// debug the thread
	err = debug_thread(threadID);
	if (err != B_OK) {
		TRACE(("haiku_stop_thread(): failed to debug thread %ld: %s\n",
			threadID, strerror(err)));
		return err;
	}

	// wait for the event to hit our port

	// TODO: debug_thread() doesn't guarantee that the thread will enter the
	// debug loop any time soon. E.g. a wait_for_thread() is (at the moment)
	// not interruptable, so we block here forever, if we already debug the
	// thread that is being waited for. We should probably limit the time
	// we're waiting, and return the info to the caller, which can then try
	// to deal with the situation in an appropriate way.

	// prepare closure for finding interesting events
	threadEventClosure.context = teamDebugInfo;
	threadEventClosure.thread = threadID;
	threadEventClosure.event = NULL;

	// find the first interesting queued event
	threadEventClosure.event = haiku_find_next_debug_event(
		&teamDebugInfo->events, haiku_thread_event_predicate,
		&threadEventClosure);

	// read all events pending on the port
	haiku_read_pending_debug_events(teamDebugInfo,
		(threadEventClosure.event == NULL), haiku_thread_event_predicate,
		&threadEventClosure);

	// We should check, whether we really got an event for the thread in
	// question, but the only possible other event is that the team has
	// been delete, which ends the game anyway.

	// TODO: That's actually not true. We also get messages when an add-on
	// has been loaded/unloaded, a signal arrives, or a thread calls the
	// debugger.

	return B_OK;
}


static status_t
haiku_get_cpu_state(team_debug_info *teamDebugInfo,
	debug_nub_get_cpu_state_reply *reply)
{
	status_t err;
	thread_id threadID = ptid_get_tid(inferior_ptid);
	debug_nub_get_cpu_state message;

	// make sure the thread is stopped
	err = haiku_stop_thread(teamDebugInfo, threadID);
	if (err != B_OK) {
		printf_unfiltered ("Failed to stop thread %ld: %s\n",
			threadID, strerror(err));
		return err;
	}

	message.reply_port = teamDebugInfo->context.reply_port;
	message.thread = threadID;

	err = haiku_send_debugger_message(teamDebugInfo,
		B_DEBUG_MESSAGE_GET_CPU_STATE, &message, sizeof(message), reply,
		sizeof(*reply));
	if (err == B_OK)
		err = reply->error;
	if (err != B_OK) {
		printf_unfiltered ("Failed to get status of thread %ld: %s\n",
			threadID, strerror(err));
	}

	return err;
}


// #pragma mark -

static void
haiku_child_open (char *arg, int from_tty)
{
	error ("Use the \"run\" command to start a child process.");
}

static void
haiku_child_close (int x)
{
	printf_unfiltered ("gdb: child_close, inferior_ptid=%d\n",
		PIDGET (inferior_ptid));
}


static void
haiku_child_attach (char *args, int from_tty)
{
	extern int stop_after_trap;
	thread_id threadID;
	thread_info threadInfo;
	status_t result;

	TRACE(("haiku_child_attach(`%s', %d)\n", args, from_tty));

	if (!args)
		error_no_arg ("thread-id to attach");

	// get the thread ID
	threadID = strtoul (args, 0, 0);
	if (threadID <= 0)
		error("The given thread-id %ld is invalid.", threadID);

	haiku_init_child_debugging(threadID, true);

	TRACE(("haiku_child_attach() done\n"));
}


static void
haiku_child_detach (char *args, int from_tty)
{
	int detached = 1;
	status_t result;

	TRACE(("haiku_child_detach(`%s', %d)\n", args, from_tty));

	result = remove_team_debugger(sTeamDebugInfo.team);
	if (result != B_OK) {
		error("Failed to detach process %ld: %s\n", sTeamDebugInfo.team,
			strerror(result));
	}

	delete_command (NULL, 0);

	if (from_tty) {
		char *exec_file = get_exec_file (0);
		if (exec_file == 0)
			exec_file = "";
		printf_unfiltered ("Detaching from program: %s, Pid %ld\n", exec_file,
			sTeamDebugInfo.team);
		gdb_flush (gdb_stdout);
	}

	haiku_cleanup_team_debug_info();

	inferior_ptid = null_ptid;
	unpush_target (sHaikuTarget);
}


static void
haiku_resume_thread(team_debug_info *teamDebugInfo, thread_debug_info *thread,
	int step, enum target_signal sig)
{
	thread_id threadID = (thread ? thread->thread : -1);
	int pendingSignal = -1;
	int signalToSend = target_to_haiku_signal(sig);
	uint32 handleEvent = B_THREAD_DEBUG_HANDLE_EVENT;

	if (!thread || !thread->stopped) {
		TRACE(("haiku_resume_thread(): thread %ld not stopped\n",
			threadID));
		return;
	}

	if (thread->reprocess_event > 0) {
		TRACE(("haiku_resume_thread(): thread %ld is expected to "
			"reprocess its current event\n", threadID));
		return;
	}

	// get the pending signal
	if (thread->last_event)
		pendingSignal = thread->signal;

	// Decide what to do with the event and the pending and passed signal.
	// We simply adjust signalToSend, pendingSignal and handleEvent.
	// If signalToSend is set afterwards, we need to send it. pendingSignal we
	// need to ignore. handleEvent specifies whether to handle/ignore the event.
	if (signalToSend > 0) {
		if (pendingSignal > 0) {
			if (signalToSend == pendingSignal) {
				// signal to send is the pending signal
				// we don't need to send it
				signalToSend = -1;
				if (thread->signal_status != SIGNAL_WILL_ARRIVE) {
					// the signal has not yet been sent, so we need to ignore
					// it only in this case, but not in the other ones
					pendingSignal = -1;
				}
			} else {
				// signal to send is not the pending signal
				// If the signal has been sent or will be sent, we need to
				// ignore the event.
				if (thread->signal_status != SIGNAL_FAKED)
					handleEvent = B_THREAD_DEBUG_IGNORE_EVENT;
				// At any rate we don't need to ignore the signal.
				pendingSignal = -1;
				// send the signal
			}
		} else {
			// there's a signal to send, but no pending signal
			// handle the event
			// ignore the signal
			// send the signal
		}
	} else {
		if (pendingSignal > 0) {
			// there's a pending signal, but no signal to send
			// Ignore the event, if the signal has been sent or will be sent.
			if (thread->signal_status != SIGNAL_FAKED)
				handleEvent = B_THREAD_DEBUG_IGNORE_EVENT;
		} else {
			// there're neither signal to send nor pending signal
			// handle the event
		}
	}

	// ignore the pending signal, if necessary
	if (pendingSignal > 0) {
		debug_nub_set_signal_masks message;
		status_t err;

		message.thread = threadID;
		message.ignore_mask = 0;
		message.ignore_once_mask = B_DEBUG_SIGNAL_TO_MASK(pendingSignal);
		message.ignore_op = B_DEBUG_SIGNAL_MASK_OR;
		message.ignore_once_op = B_DEBUG_SIGNAL_MASK_OR;

		err = haiku_send_debugger_message(teamDebugInfo,
			B_DEBUG_MESSAGE_SET_SIGNAL_MASKS, &message, sizeof(message),
			NULL, 0);
		if (err != B_OK) {
			printf_unfiltered ("Set signal ignore masks for thread %ld: %s\n",
				threadID, strerror(err));
		}
	}

	// send the signal
	if (signalToSend > 0) {
		if (send_signal(threadID, signalToSend) < 0) {
			printf_unfiltered ("Failed to send signal %d to thread %ld: %s\n",
				signalToSend, threadID, strerror(errno));
		}
	}

	// resume thread
	haiku_continue_thread(teamDebugInfo, threadID, handleEvent, step);
	thread->stopped = false;
}


static void
haiku_child_resume (ptid_t ptid, int step, enum target_signal sig)
{
	team_id teamID = ptid_get_pid(ptid);
	thread_id threadID = ptid_get_tid(ptid);
	thread_debug_info *thread;

	TRACE(("haiku_child_resume(`%s', step: %d, signal: %d)\n",
		haiku_pid_to_str(ptid), step, sig));

	if (teamID < 0) {
		// resume all stopped threads (at least all haiku_wait_child() has
		// reported to be stopped)
		for (thread = sTeamDebugInfo.threads; thread; thread = thread->next) {
			if (thread->stopped)
				haiku_resume_thread(&sTeamDebugInfo, thread, step, sig);
		}
	} else {
		// resume the thread in question
		thread = haiku_find_thread(&sTeamDebugInfo, threadID);
		if (thread) {
			haiku_resume_thread(&sTeamDebugInfo, thread, step, sig);
		} else {
			printf_unfiltered ("haiku_child_resume(): unknown thread %ld\n",
				threadID);
		}
	}

}


static ptid_t
haiku_child_wait_internal (team_debug_info *teamDebugInfo, ptid_t ptid,
	struct target_waitstatus *ourstatus, bool *ignore, bool *selectThread)
{
	team_id teamID = ptid_get_pid(ptid);
	team_id threadID = ptid_get_tid(ptid);
	debug_event *event = NULL;
	ptid_t retval = pid_to_ptid(-1);
	struct thread_debug_info *thread;
	int pendingSignal = -1;
	pending_signal_status pendingSignalStatus = SIGNAL_FAKED;
	int reprocessEvent = -1;

	*selectThread = false;

	if (teamID < 0 || threadID == 0)
		threadID = -1;

	// if we're waiting for any thread, search the thread list for already
	// stopped threads (needed for reprocessing events)
	if (threadID < 0) {
		for (thread = teamDebugInfo->threads; thread; thread = thread->next) {
			if (thread->stopped) {
				threadID = thread->thread;
				break;
			}
		}
	}

	// check, if the thread exists and is already stopped
	if (threadID >= 0
		&& (thread = haiku_find_thread(teamDebugInfo, threadID)) != NULL
		&& thread->stopped) {
		// remove the event that stopped the thread from the thread (we will
		// add it again, if it shall not be ignored)
		event = thread->last_event;
		thread->last_event = NULL;
		thread->stopped = false;
		reprocessEvent = thread->reprocess_event;

		TRACE(("haiku_child_wait_internal(): thread %ld already stopped. "
			"re-digesting the last event (%p).\n", threadID, event));
	} else {
		// prepare closure for finding interesting events
		thread_event_closure threadEventClosure;
		threadEventClosure.context = teamDebugInfo;
		threadEventClosure.thread = threadID;
		threadEventClosure.event = NULL;

		// find the first interesting queued event
		threadEventClosure.event = haiku_find_next_debug_event(
			&teamDebugInfo->events, haiku_thread_event_predicate,
			&threadEventClosure);

		// read all events pending on the port
		haiku_read_pending_debug_events(teamDebugInfo,
			(threadEventClosure.event == NULL), haiku_thread_event_predicate,
			&threadEventClosure);

		// get the event of interest
		event = haiku_remove_debug_event(&teamDebugInfo->events,
			threadEventClosure.event);

		if (!event) {
			TRACE(("haiku_child_wait_internal(): got no event!\n"));
			return retval;
		}

		TRACE(("haiku_child_wait_internal(): got event: %u, thread: %ld, "
			"team: %ld, nub port: %ld\n", event->message,
			event->data.origin.thread, event->data.origin.team,
			event->data.origin.nub_port));
	}

	if (event->data.origin.team != teamDebugInfo->team) {
		// Spurious debug message. Doesn't concern our team. Ignore.
		xfree(event);
		return retval;
	}

	retval = ptid_build(event->data.origin.team, 0, event->data.origin.thread);

	*ignore = false;

	switch (event->message) {
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
		{
			// print the debugger message
			char debuggerMessage[1024];
			ssize_t bytesRead = debug_read_string(&teamDebugInfo->context,
				event->data.debugger_call.message, debuggerMessage,
				sizeof(debuggerMessage));
			if (bytesRead > 0) {
				printf_unfiltered ("Thread %ld called debugger(): %s\n",
					event->data.origin.thread, debuggerMessage);
			} else {
				printf_unfiltered ("Thread %ld called debugger(), but failed"
					"to get the debugger message.\n",
					event->data.origin.thread);
			}

			// fall through...
		}
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			ourstatus->kind = TARGET_WAITKIND_STOPPED;
			ourstatus->value.sig = TARGET_SIGNAL_TRAP;
			pendingSignal = SIGTRAP;
			pendingSignalStatus = SIGNAL_FAKED;
			break;

		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
			ourstatus->kind = TARGET_WAITKIND_SYSCALL_ENTRY;
			ourstatus->value.syscall_id = event->data.pre_syscall.syscall;
			break;

		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
			ourstatus->kind = TARGET_WAITKIND_SYSCALL_RETURN;
			ourstatus->value.syscall_id = event->data.post_syscall.syscall;
			break;

		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
			pendingSignal = event->data.signal_received.signal;
			pendingSignalStatus = SIGNAL_ARRIVED;
			ourstatus->kind = TARGET_WAITKIND_STOPPED;
			ourstatus->value.sig = haiku_to_target_signal(pendingSignal);
			break;

		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
		{
			// print the exception message
			char exception[1024];
			get_debug_exception_string(event->data.exception_occurred.exception,
				exception, sizeof(exception));
			printf_unfiltered ("Thread %ld caused an exception: %s\n",
				event->data.origin.thread, exception);

			pendingSignal = event->data.exception_occurred.signal;
			pendingSignalStatus = SIGNAL_WILL_ARRIVE;
			ourstatus->kind = TARGET_WAITKIND_STOPPED;
			ourstatus->value.sig = haiku_to_target_signal(pendingSignal);
			break;
		}

		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
			// ignore
			*ignore = true;
			break;

		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			ourstatus->kind = TARGET_WAITKIND_EXITED;
			ourstatus->value.integer = 0;
				// TODO: Extend the debugger interface?
			break;

		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
		{
			// internal bookkeeping only
			thread_id newThread = event->data.thread_created.new_thread;
			if (newThread != teamDebugInfo->nub_thread)
				haiku_add_thread(teamDebugInfo, newThread);
			*ignore = true;
			break;
		}

		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			// internal bookkeeping
			haiku_remove_thread(teamDebugInfo,
				event->data.thread_deleted.origin.thread);
			*ignore = true;

			// TODO: What if this is the current thread?
			break;

		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			if (reprocessEvent < 0) {
				// first time we see the event: update our image list
//				haiku_add_image(teamDebugInfo,
//					&event->data.image_created.info);
haiku_cleanup_image_list(teamDebugInfo);
haiku_init_image_list(teamDebugInfo);
// TODO: We don't get events when images have been removed in preparation of
// an exec*() yet.
			}

			ourstatus->kind = TARGET_WAITKIND_LOADED;
			ourstatus->value.integer = 0;
			if (event->data.image_created.info.type == B_APP_IMAGE) {
				// An app image has been loaded, i.e. the application is now
				// fully loaded. We need to send a TARGET_WAITKIND_LOADED
				// first, so that GDB's shared object list is updated correctly.
				// But we also need to send an TARGET_WAITKIND_EXECD event.
				// We use the reprocessEvent mechanism here.

				if (reprocessEvent == 2) {
					// the second time the event is processed: send the `exec'
					// event
					if (inferior_ignoring_startup_exec_events > 0) {
						// we shall not send an exec event, so we skip that
						// and send the `trap' immediately
TRACE(("haiku_child_wait_internal(): ignoring exec (%d)\n",
inferior_ignoring_startup_exec_events));
						inferior_ignoring_startup_exec_events--;
						reprocessEvent = 1;
					}
				}

				if (reprocessEvent < 0) {
TRACE(("haiku_child_wait_internal(): B_APP_IMAGE created, reprocess < 0\n"));
					// the first time the event is processed: send the
					// `loaded' event
					reprocessEvent = 2;
				} else if (reprocessEvent == 2) {
TRACE(("haiku_child_wait_internal(): B_APP_IMAGE created, reprocess -> exec\n"));
					// the second time the event is processed: send the `exec'
					// event
					ourstatus->kind = TARGET_WAITKIND_EXECD;
					ourstatus->value.execd_pathname
						= event->data.image_created.info.name;

					reprocessEvent = 1;
				} else if (reprocessEvent <= 1) {
					// the third time the event is processed: send the `trap'
					// event
					ourstatus->kind = TARGET_WAITKIND_STOPPED;
					ourstatus->value.sig = TARGET_SIGNAL_TRAP;
					pendingSignal = SIGTRAP;
					pendingSignalStatus = SIGNAL_FAKED;

					reprocessEvent = 0;
				}
			}

			// re_enable_breakpoints_in_shlibs ();
				// TODO: Needed?
			break;

		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			haiku_remove_image(teamDebugInfo,
				event->data.image_deleted.info.id);

			// send TARGET_WAITKIND_LOADED here too, it causes the shared
			// object list to be updated
			ourstatus->kind = TARGET_WAITKIND_LOADED;
			ourstatus->value.integer = 0;
			break;

		case B_DEBUGGER_MESSAGE_HANDED_OVER:
		{
TRACE(("haiku_child_wait_internal(): B_DEBUGGER_MESSAGE_HANDED_OVER: causing "
"thread: %ld\n", event->data.handed_over.causing_thread));
			// The debugged team has been handed over to us by another debugger
			// (likely the debug server). This event also tells us, which
			// thread has caused the original debugger to be installed (if any).
			// So, if we're not looking for any particular thread, select that
			// thread.
			if (threadID < 0 && event->data.handed_over.causing_thread >= 0) {
				*selectThread = true;
				retval = ptid_build(event->data.origin.team, 0,
					event->data.handed_over.causing_thread);
			}
			*ignore = true;
		}

		default:
			// unknown message, ignore
			*ignore = true;
			break;
	}

	// make the event the thread's last event or delete it
	if (!*ignore && event->data.origin.thread >= 0) {
		struct thread_debug_info *thread = haiku_find_thread(teamDebugInfo,
			event->data.origin.thread);
		if (thread->last_event)
			xfree(thread->last_event);
			thread->last_event = event;
			thread->stopped = true;
			thread->signal = pendingSignal;
			thread->signal_status = pendingSignalStatus;
			thread->reprocess_event
				= (reprocessEvent >= 0 ? reprocessEvent : 0);
	} else {
		thread_id originThread = event->data.origin.thread;
		xfree(event);

		// continue the thread
		if (originThread >= 0) {
			haiku_continue_thread(teamDebugInfo, originThread,
				B_THREAD_DEBUG_HANDLE_EVENT, false);
		}

//		*ignore = true;
// TODO: This should indeed not be needed. It definitely eats the
// `team deleted' events.
	}

	return retval;
}


static ptid_t
haiku_child_wait (ptid_t ptid, struct target_waitstatus *ourstatus)
{
	ptid_t retval;
	bool ignore = true;
	bool selectThread = false;

	TRACE(("haiku_child_wait(`%s', %p)\n",
		haiku_pid_to_str(ptid), ourstatus));

	do {
		retval = haiku_child_wait_internal(&sTeamDebugInfo, ptid, ourstatus,
			&ignore, &selectThread);
		if (selectThread)
			ptid = retval;
	} while (ignore);

	TRACE(("haiku_child_wait() done: `%s'\n", haiku_pid_to_str(retval)));

	return retval;
}


static void
haiku_child_fetch_inferior_registers (int reg)
{
	debug_nub_get_cpu_state_reply reply;

	TRACE(("haiku_child_fetch_inferior_registers(%d)\n", reg));

	// get the CPU state
	haiku_get_cpu_state(&sTeamDebugInfo, &reply);

// printf("haiku_child_fetch_inferior_registers(): eip: %p, ebp: %p\n",
// (void*)reply.cpu_state.eip, (void*)reply.cpu_state.ebp);

	// supply the registers (architecture specific)
	haiku_supply_registers(reg, &reply.cpu_state);
}


static void
haiku_child_store_inferior_registers (int reg)
{
	status_t err;
	thread_id threadID = ptid_get_tid(inferior_ptid);
	debug_nub_get_cpu_state_reply reply;
	debug_nub_set_cpu_state message;

	TRACE(("haiku_child_store_inferior_registers(%d)\n", reg));

	// get the current CPU state
	haiku_get_cpu_state(&sTeamDebugInfo, &reply);

	// collect the registers (architecture specific)
	haiku_collect_registers(reg, &reply.cpu_state);

	// set the new CPU state
	message.thread = threadID;
	memcpy(&message.cpu_state, &reply.cpu_state, sizeof(debug_cpu_state));
	err = haiku_send_debugger_message(&sTeamDebugInfo,
		B_DEBUG_MESSAGE_SET_CPU_STATE, &message, sizeof(message), NULL, 0);
	if (err != B_OK) {
		printf_unfiltered ("Failed to set status of thread %ld: %s\n",
			threadID, strerror(err));
	}
}

static void
haiku_child_prepare_to_store (void)
{
	// Since we always fetching the current state in
	// haiku_child_store_inferior_registers(), this should be a no-op.
}

static int
haiku_child_deprecated_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len,
	int write, struct mem_attrib *attrib, struct target_ops *target)
{
	TRACE(("haiku_child_deprecated_xfer_memory(0x%8lx, %p, %d, %d, %p, %p)\n",
		(uint32)memaddr, myaddr, len, write, attrib, target));

	if (len <= 0)
		return 0;

	if (write) {
		// write
		debug_nub_write_memory message;
		debug_nub_write_memory_reply reply;
		status_t err;

		if (len > B_MAX_READ_WRITE_MEMORY_SIZE)
			len = B_MAX_READ_WRITE_MEMORY_SIZE;

		message.reply_port = sTeamDebugInfo.context.reply_port;
		message.address = (void*)memaddr;
		message.size = len;
		memcpy(message.data, myaddr, len);

		err = haiku_send_debugger_message(&sTeamDebugInfo,
			B_DEBUG_MESSAGE_WRITE_MEMORY, &message, sizeof(message), &reply,
			sizeof(reply));
		if (err != B_OK || reply.error != B_OK)
{
TRACE(("haiku_child_deprecated_xfer_memory() failed: %lx\n",
(err != B_OK ? err : reply.error)));
			return 0;
}

TRACE(("haiku_child_deprecated_xfer_memory(): -> %ld\n", reply.size));
		return reply.size;
	} else {
		// read
		ssize_t bytesRead = debug_read_memory_partial(&sTeamDebugInfo.context,
			(const void *)memaddr, myaddr, len);
		return (bytesRead < 0 ? 0 : bytesRead);
	}

	return -1;
}

LONGEST
haiku_child_xfer_partial (struct target_ops *ops, enum target_object object,
	const char *annex, void *readbuf, const void *writebuf, ULONGEST offset,
	LONGEST len)
{
	if (!readbuf && !writebuf)
		return -1;

	switch (object) {
		case TARGET_OBJECT_MEMORY:
		{
			int write = !readbuf;
			return haiku_child_deprecated_xfer_memory (offset,
				(write ? (void*)writebuf : readbuf), len, write, NULL, ops);
		}
	}

	return -1;
}

static void
haiku_child_files_info (struct target_ops *ignore)
{
  printf_unfiltered ("\tUsing the running image of %s %s.\n",
      attach_flag ? "attached" : "child", target_pid_to_str (inferior_ptid));
}

static void
haiku_child_kill_inferior (void)
{
	status_t err;
	thread_id teamID = ptid_get_pid(inferior_ptid);

	TRACE(("haiku_child_kill_inferior()\n"));

	err = kill_team(teamID);
	if (err != B_OK) {
		printf_unfiltered ("Failed to kill team %ld: %s\n", teamID,
			strerror(err));
		return;
	}

	target_mourn_inferior();
}

static void
haiku_child_stop_inferior (void)
{
	status_t err;
	thread_id threadID = ptid_get_tid(inferior_ptid);

	TRACE(("haiku_child_stop_inferior()\n"));

	err = debug_thread(threadID);
	if (err != B_OK) {
		printf_unfiltered ("Failed to stop thread %ld: %s\n", threadID,
			strerror(err));
		return;
	}
}

static void
haiku_init_debug_create_inferior(int pid)
{
	extern int stop_after_trap;

	// fix inferior_ptid -- fork_inferior() sets the process ID only
	inferior_ptid = ptid_build (pid, 0, pid);
		// team ID == team main thread ID under Haiku

	haiku_init_child_debugging(pid, false);

	// eat the initial `debugged' event caused by wait_for_inferior()
// TODO: Maybe we should just dequeue the event and continue the thread instead
// of using gdb's mechanism, since I don't know what undesired side-effects
// they may have.
	clear_proceed_status ();
	init_wait_for_inferior ();

	if (STARTUP_WITH_SHELL)
		inferior_ignoring_startup_exec_events = 2;
	else
		inferior_ignoring_startup_exec_events = 1;
	inferior_ignoring_leading_exec_events = 0;

	while (true) {
		thread_debug_info *thread;

		stop_soon = STOP_QUIETLY;
		wait_for_inferior();

		if (stop_signal == TARGET_SIGNAL_TRAP) {
			thread = haiku_find_thread(&sTeamDebugInfo, pid);

			if (thread && thread->stopped
				&& thread->last_event->message
					== B_DEBUGGER_MESSAGE_IMAGE_CREATED
				&& thread->last_event->data.image_created.info.type
					== B_APP_IMAGE
				&& thread->reprocess_event == 0
				&& inferior_ignoring_startup_exec_events <= 0) {
				// This is the trap for the last (second, if started via shell)
				// `load app image' event. Be done.
				break;
			}
		}

		resume(0, stop_signal);
	}
	stop_soon = NO_STOP_QUIETLY;

	// load shared library symbols
	target_terminal_ours_for_output ();
	SOLIB_ADD (NULL, 0, &current_target, auto_solib_add);
	target_terminal_inferior ();

//	startup_inferior (START_INFERIOR_TRAPS_EXPECTED);

//while (1) {
//	stop_after_trap = 1;
//	wait_for_inferior ();
//	if (debugThread && stop_signal != TARGET_SIGNAL_TRAP)
//		resume (0, stop_signal);
//	else
//		break;
//}
//stop_after_trap = 0;



//	while (1) {
//		thread_debug_info *thread;
//
//		stop_after_trap = 1;
//		wait_for_inferior ();
//// TODO: Catch deadly events, so that we won't block here.
//
//		thread = haiku_find_thread(&sTeamDebugInfo, pid);
//TRACE(("haiku_init_debug_create_inferior(): wait_for_inferior() returned: "
//"thread: %p (%ld)\n", thread, (thread ? thread->thread : -1)));
//		if (thread && thread->stopped
//			&& thread->last_event->message
//				== B_DEBUGGER_MESSAGE_IMAGE_CREATED
//			&& thread->last_event->data.image_created.info.type
//				== B_APP_IMAGE) {
//TRACE(("haiku_init_debug_create_inferior(): Got an `app image created' "
//"message\n"));
//			break;
//		}
//
//		resume (0, stop_signal);
//	}
}

static void
haiku_child_create_inferior (char *exec_file, char *allargs, char **env,
	int from_tty)
{
	TRACE(("haiku_child_create_inferior(`%s', `%s', %p, %d)\n", exec_file,
		allargs, env, from_tty));

	fork_inferior (exec_file, allargs, env, wait_for_debugger,
		haiku_init_debug_create_inferior, NULL, NULL);


observer_notify_inferior_created (&current_target, from_tty);
proceed ((CORE_ADDR) - 1, TARGET_SIGNAL_0, 0);


	// TODO: Anything more to do here?
}

static void
haiku_child_mourn_inferior (void)
{
	TRACE(("haiku_child_mourn_inferior()\n"));

	haiku_cleanup_team_debug_info();
	unpush_target (sHaikuTarget);
	generic_mourn_inferior ();
}

static int
haiku_child_can_run (void)
{
	return 1;
}

static int
haiku_child_thread_alive (ptid_t ptid)
{
	thread_info info;

	TRACE(("haiku_child_thread_alive(`%s')\n", haiku_pid_to_str(ptid)));

	return (get_thread_info(ptid_get_tid(ptid), &info) == B_OK);
}

static char *
haiku_pid_to_str (ptid_t ptid)
{
	static char buffer[B_OS_NAME_LENGTH + 64 + 64];
	team_info teamInfo;
	thread_info threadInfo;
	status_t error;

	// get the team info for the target team
	error = get_team_info(ptid_get_pid(ptid), &teamInfo);
	if (error != B_OK) {
		sprintf(buffer, "invalid team ID %d", ptid_get_pid(ptid));
		return buffer;
	}

	// get the thread info for the target thread
	error = get_thread_info(ptid_get_tid(ptid), &threadInfo);
	if (error != B_OK) {
		sprintf(buffer, "team %.*s (%ld) invalid thread ID %ld",
			(int)sizeof(teamInfo.args), teamInfo.args, teamInfo.team,
			ptid_get_tid(ptid));
		return buffer;
	}

	sprintf(buffer, "team %.*s (%ld) thread %s (%ld)",
		(int)sizeof(teamInfo.args), teamInfo.args, teamInfo.team,
		threadInfo.name, threadInfo.thread);

	return buffer;
}

char *
haiku_child_pid_to_exec_file (int pid)
{
	static char buffer[B_PATH_NAME_LENGTH];

	// The only way to get the path to the application's executable seems to
	// be to get an image_info of its image, which also contains a path.
	// Several images may belong to the team (libraries, add-ons), but only
	// the one in question should be typed B_APP_IMAGE.
	image_info info;
	int32 cookie = 0;

	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE) {
			strncpy(buffer, info.name, B_PATH_NAME_LENGTH - 1);
			buffer[B_PATH_NAME_LENGTH - 1] = 0;
			return buffer;
		}
	}

	return NULL;
}


void
_initialize_haiku_nat (void)
{
	// child operations
	struct target_ops *t = XZALLOC (struct target_ops);
	t->to_shortname = "child";
	t->to_longname = "Haiku child process";
	t->to_doc = "Haiku child process (started by the \"run\" command).";
	t->to_open = haiku_child_open;
	t->to_close = haiku_child_close;
	t->to_attach = haiku_child_attach;
	t->to_detach = haiku_child_detach;
	t->to_resume = haiku_child_resume;
	t->to_wait = haiku_child_wait;
	t->to_fetch_registers = haiku_child_fetch_inferior_registers;
	t->to_store_registers = haiku_child_store_inferior_registers;
	t->to_prepare_to_store = haiku_child_prepare_to_store;
	t->deprecated_xfer_memory = haiku_child_deprecated_xfer_memory;
	t->to_xfer_partial = haiku_child_xfer_partial;
	t->to_files_info = haiku_child_files_info;

	t->to_insert_breakpoint = memory_insert_breakpoint;
	t->to_remove_breakpoint = memory_remove_breakpoint;
		// TODO: We can do better. If we wanted to.

	// TODO: The following terminal calls are not documented or not yet
	// understood by me.
	t->to_terminal_init = terminal_init_inferior;
	t->to_terminal_inferior = terminal_inferior;
	t->to_terminal_ours_for_output = terminal_ours_for_output;
	t->to_terminal_ours = terminal_ours;
	t->to_terminal_save_ours = terminal_save_ours;
	t->to_terminal_info = child_terminal_info;

	t->to_kill = haiku_child_kill_inferior;
	t->to_stop = haiku_child_stop_inferior;
	t->to_create_inferior = haiku_child_create_inferior;
	t->to_mourn_inferior = haiku_child_mourn_inferior;
	t->to_can_run = haiku_child_can_run;
	t->to_thread_alive = haiku_child_thread_alive;
// How about to_find_new_threads? Perhaps not necessary, as we could be informed
// about thread creation/deletion.
	t->to_pid_to_str = haiku_pid_to_str;
	t->to_pid_to_exec_file = haiku_child_pid_to_exec_file;

	t->to_stratum = process_stratum;
	t->to_has_all_memory = 1;
	t->to_has_memory = 1;
	t->to_has_stack = 1;
	t->to_has_registers = 1;
	t->to_has_execution = 1;
	t->to_magic = OPS_MAGIC;

	sHaikuTarget = t;

	add_target (t);
}
