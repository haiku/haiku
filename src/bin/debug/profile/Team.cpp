/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Team.h"

#include <new>

#include <image.h>

#include <debug_support.h>
#include <system_profiler_defs.h>

#include "debug_utils.h"

#include "Image.h"
#include "Options.h"


//#define TRACE_PROFILE_TEAM
#ifdef TRACE_PROFILE_TEAM
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) do {} while(false)
#endif


enum {
	SAMPLE_AREA_SIZE	= 128 * 1024,
};


Team::Team()
	:
	fID(-1),
	fNubPort(-1),
	fThreads(),
	fImages(20, false)
{
	fDebugContext.nub_port = -1;
}


Team::~Team()
{
	if (fDebugContext.nub_port >= 0)
		destroy_debug_context(&fDebugContext);

	if (fNubPort >= 0)
		remove_team_debugger(fID);

	for (int32 i = 0; Image* image = fImages.ItemAt(i); i++)
		image->ReleaseReference();
}


status_t
Team::Init(team_id teamID, port_id debuggerPort)
{
	// get team info
	team_info teamInfo;
	status_t error = get_team_info(teamID, &teamInfo);
	if (error != B_OK)
		return error;

	fID = teamID;
	fArgs = teamInfo.args;

	// install ourselves as the team debugger
	fNubPort = install_team_debugger(teamID, debuggerPort);
	if (fNubPort < 0) {
		fprintf(stderr,
			"%s: Failed to install as debugger for team %" B_PRId32 ": "
			"%s\n", kCommandName, teamID, strerror(fNubPort));
		return fNubPort;
	}

	// init debug context
	error = init_debug_context(&fDebugContext, teamID, fNubPort);
	if (error != B_OK) {
		fprintf(stderr,
			"%s: Failed to init debug context for team %" B_PRId32 ": "
			"%s\n", kCommandName, teamID, strerror(error));
		return error;
	}

	// set team debugging flags
	int32 teamDebugFlags = B_TEAM_DEBUG_THREADS
		| B_TEAM_DEBUG_TEAM_CREATION | B_TEAM_DEBUG_IMAGES;
	error = set_team_debugging_flags(fNubPort, teamDebugFlags);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
Team::Init(system_profiler_team_added* addedInfo)
{
	fID = addedInfo->team;
	fArgs = addedInfo->name + addedInfo->args_offset;
	return B_OK;
}


status_t
Team::InitThread(Thread* thread)
{
	// The thread
	thread->SetLazyImages(!_SynchronousProfiling());

	// create the sample area
	char areaName[B_OS_NAME_LENGTH];
	snprintf(areaName, sizeof(areaName), "profiling samples %" B_PRId32,
		thread->ID());
	void* samples;
	area_id sampleArea = create_area(areaName, &samples, B_ANY_ADDRESS,
		SAMPLE_AREA_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (sampleArea < 0) {
		fprintf(stderr,
			"%s: Failed to create sample area for thread %" B_PRId32 ": "
			"%s\n", kCommandName, thread->ID(), strerror(sampleArea));
		return sampleArea;
	}

	thread->SetSampleArea(sampleArea, (addr_t*)samples);

	// add the current images to the thread
	int32 imageCount = fImages.CountItems();
	for (int32 i = 0; i < imageCount; i++) {
		status_t error = thread->AddImage(fImages.ItemAt(i));
		if (error != B_OK)
			return error;
	}

	if (!_SynchronousProfiling()) {
		// set thread debugging flags and start profiling
		int32 threadDebugFlags = 0;
//		if (!traceTeam) {
//			threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
//				| (traceChildThreads
//					? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
//		}
		status_t error = set_thread_debugging_flags(fNubPort, thread->ID(),
			threadDebugFlags);
		if (error != B_OK)
			return error;

		// start profiling
		debug_nub_start_profiler message;
		message.reply_port = fDebugContext.reply_port;
		message.thread = thread->ID();
		message.interval = gOptions.interval;
		message.sample_area = sampleArea;
		message.stack_depth = gOptions.stack_depth;
		message.variable_stack_depth = gOptions.analyze_full_stack;

		debug_nub_start_profiler_reply reply;
		error = send_debug_message(&fDebugContext,
			B_DEBUG_START_PROFILER, &message, sizeof(message), &reply,
			sizeof(reply));
		if (error != B_OK || (error = reply.error) != B_OK) {
			fprintf(stderr,
				"%s: Failed to start profiler for thread %" B_PRId32 ": %s\n",
				kCommandName, thread->ID(), strerror(error));
			return error;
		}

		thread->SetInterval(reply.interval);

		fThreads.Add(thread);

		// resume the target thread to be sure, it's running
		resume_thread(thread->ID());
	} else {
		// debugger-less profiling
		thread->SetInterval(gOptions.interval);
		fThreads.Add(thread);
	}

	return B_OK;
}


void
Team::RemoveThread(Thread* thread)
{
	fThreads.Remove(thread);
}


void
Team::Exec(int32 event, const char* args, const char* threadName)
{
	// remove all non-kernel images
	int32 imageCount = fImages.CountItems();
	for (int32 i = imageCount - 1; i >= 0; i--) {
		Image* image = fImages.ItemAt(i);
		if (image->Owner() == ID())
			_RemoveImage(i, event);
	}

	fArgs = args;

	// update the main thread
	ThreadList::Iterator it = fThreads.GetIterator();
	while (Thread* thread = it.Next()) {
		if (thread->ID() == ID()) {
			thread->UpdateInfo(threadName);
			break;
		}
	}
}


status_t
Team::AddImage(SharedImage* sharedImage, const image_info& imageInfo,
	team_id owner, int32 event)
{
	// create the image
	Image* image = new(std::nothrow) Image(sharedImage, imageInfo, owner,
		event);
	if (image == NULL)
		return B_NO_MEMORY;

	if (!fImages.AddItem(image)) {
		delete image;
		return B_NO_MEMORY;
	}

	// Although we generally synchronize the threads' images lazily, we have
	// to add new images at least, since otherwise images could be added
	// and removed again, and the hits inbetween could never be matched.
	ThreadList::Iterator it = fThreads.GetIterator();
	while (Thread* thread = it.Next())
		thread->AddImage(image);

	return B_OK;
}


status_t
Team::RemoveImage(image_id imageID, int32 event)
{
	for (int32 i = 0; Image* image = fImages.ItemAt(i); i++) {
		if (image->ID() == imageID) {
			_RemoveImage(i, event);
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


Image*
Team::FindImage(image_id id) const
{
	for (int32 i = 0; Image* image = fImages.ItemAt(i); i++) {
		if (image->ID() == id)
			return image;
	}

	return NULL;
}


void
Team::_RemoveImage(int32 index, int32 event)
{
	Image* image = fImages.RemoveItemAt(index);
	if (image == NULL)
		return;

	if (_SynchronousProfiling()) {
		ThreadList::Iterator it = fThreads.GetIterator();
		while (Thread* thread = it.Next())
			thread->RemoveImage(image);
	} else {
		// Note: We don't tell the threads that the image has been removed. They
		// will be updated lazily when their next profiler update arrives. This
		// is necessary, since the update might contain samples hitting that
		// image.
	}

	image->SetDeletionEvent(event);
	image->ReleaseReference();
}
