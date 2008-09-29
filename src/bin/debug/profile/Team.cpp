/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Team.h"

#include <new>

#include <image.h>

#include <debug_support.h>

#include "debug_utils.h"

#include "Options.h"


enum {
	SAMPLE_AREA_SIZE	= 128 * 1024,
};


Team::Team()
	:
	fNubPort(-1),
	fThreads(),
	fImages(20, false)
{
	fInfo.team = -1;
	fDebugContext.nub_port = -1;
}


Team::~Team()
{
	if (fDebugContext.nub_port >= 0)
		destroy_debug_context(&fDebugContext);

	if (fNubPort >= 0)
		remove_team_debugger(fInfo.team);

	for (int32 i = 0; Image* image = fImages.ItemAt(i); i++)
		image->RemoveReference();
}


status_t
Team::Init(team_id teamID, port_id debuggerPort)
{
	// get team info
	status_t error = get_team_info(teamID, &fInfo);
	if (error != B_OK)
		return error;

	// install ourselves as the team debugger
	fNubPort = install_team_debugger(teamID, debuggerPort);
	if (fNubPort < 0) {
		fprintf(stderr, "%s: Failed to install as debugger for team %ld: "
			"%s\n", kCommandName, teamID, strerror(fNubPort));
		return fNubPort;
	}

	// init debug context
	error = init_debug_context(&fDebugContext, teamID, fNubPort);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to init debug context for team %ld: "
			"%s\n", kCommandName, teamID, strerror(error));
		return error;
	}

	// create symbol lookup context
	debug_symbol_lookup_context* lookupContext;
	error = debug_create_symbol_lookup_context(&fDebugContext,
		&lookupContext);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to create symbol lookup context for "
			"team %ld: %s\n", kCommandName, teamID, strerror(error));
		return error;
	}

	// load the team's images and their symbols
	error = _LoadSymbols(lookupContext, ID());
	debug_delete_symbol_lookup_context(lookupContext);
	if (error != B_OK)
		return error;

	// also try to load the kernel images and symbols
	if (gOptions.profile_kernel) {
		// fake a debug context -- it's not really needed anyway
		debug_context debugContext;
		debugContext.team = B_SYSTEM_TEAM;
		debugContext.nub_port = -1;
		debugContext.reply_port = -1;

		// create symbol lookup context
		error = debug_create_symbol_lookup_context(&debugContext,
			&lookupContext);
		if (error != B_OK) {
			fprintf(stderr, "%s: Failed to create symbol lookup context "
				"for the kernel team: %s\n", kCommandName, strerror(error));
			return error;
		}

		// load the kernel's images and their symbols
		_LoadSymbols(lookupContext, B_SYSTEM_TEAM);
		debug_delete_symbol_lookup_context(lookupContext);
	}

	// set team debugging flags
	int32 teamDebugFlags = B_TEAM_DEBUG_THREADS
		| B_TEAM_DEBUG_TEAM_CREATION | B_TEAM_DEBUG_IMAGES;
	set_team_debugging_flags(fNubPort, teamDebugFlags);

	return B_OK;
}


status_t
Team::InitThread(Thread* thread)
{
	// create the sample area
	char areaName[B_OS_NAME_LENGTH];
	snprintf(areaName, sizeof(areaName), "profiling samples %ld",
		thread->ID());
	void* samples;
	area_id sampleArea = create_area(areaName, &samples, B_ANY_ADDRESS,
		SAMPLE_AREA_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (sampleArea < 0) {
		fprintf(stderr, "%s: Failed to create sample area for thread %ld: "
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

	// set thread debugging flags and start profiling
	int32 threadDebugFlags = 0;
//	if (!traceTeam) {
//		threadDebugFlags = B_THREAD_DEBUG_POST_SYSCALL
//			| (traceChildThreads
//				? B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS : 0);
//	}
	set_thread_debugging_flags(fNubPort, thread->ID(), threadDebugFlags);

	// start profiling
	debug_nub_start_profiler message;
	message.reply_port = fDebugContext.reply_port;
	message.thread = thread->ID();
	message.interval = gOptions.interval;
	message.sample_area = sampleArea;
	message.stack_depth = gOptions.stack_depth;
	message.variable_stack_depth = gOptions.analyze_full_stack;

	debug_nub_start_profiler_reply reply;
	status_t error = send_debug_message(&fDebugContext,
		B_DEBUG_START_PROFILER, &message, sizeof(message), &reply,
		sizeof(reply));
	if (error != B_OK || (error = reply.error) != B_OK) {
		fprintf(stderr, "%s: Failed to start profiler for thread %ld: %s\n",
			kCommandName, thread->ID(), strerror(error));
		return error;
	}

	thread->SetInterval(reply.interval);

	fThreads.Add(thread);

	// resume the target thread to be sure, it's running
	resume_thread(thread->ID());

	return B_OK;
}


void
Team::RemoveThread(Thread* thread)
{
	fThreads.Remove(thread);
}


void
Team::Exec(int32 event)
{
	// remove all non-kernel images
	int32 imageCount = fImages.CountItems();
	for (int32 i = imageCount - 1; i >= 0; i--) {
		Image* image = fImages.ItemAt(i);
		if (image->Owner() == ID())
			_RemoveImage(i, event);
	}

	// update the main thread
	ThreadList::Iterator it = fThreads.GetIterator();
	while (Thread* thread = it.Next()) {
		if (thread->ID() == ID()) {
			thread->UpdateInfo();
			break;
		}
	}
}


status_t
Team::AddImage(const image_info& imageInfo, int32 event)
{
	// create symbol lookup context
	debug_symbol_lookup_context* lookupContext;
	status_t error = debug_create_symbol_lookup_context(&fDebugContext,
		&lookupContext);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to create symbol lookup context for "
			"team %ld: %s\n", kCommandName, ID(), strerror(error));
		return error;
	}

	Image* image;
	error = _LoadImageSymbols(lookupContext, imageInfo, ID(), event,
		&image);
	debug_delete_symbol_lookup_context(lookupContext);

	// Although we generally synchronize the threads' images lazily, we have
	// to add new images at least, since otherwise images could be added
	// and removed again, and the hits inbetween could never be matched.
	if (error == B_OK) {
		ThreadList::Iterator it = fThreads.GetIterator();
		while (Thread* thread = it.Next())
			thread->AddImage(image);
	}

	return error;
}


status_t
Team::RemoveImage(const image_info& imageInfo, int32 event)
{
	for (int32 i = 0; Image* image = fImages.ItemAt(i); i++) {
		if (image->ID() == imageInfo.id) {
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


status_t
Team::_LoadSymbols(debug_symbol_lookup_context* lookupContext,
	team_id owner)
{
	// iterate through the team's images and collect the symbols
	image_info imageInfo;
	int32 cookie = 0;
	while (get_next_image_info(owner, &cookie, &imageInfo) == B_OK) {
		status_t error = _LoadImageSymbols(lookupContext, imageInfo,
			owner, 0);
		if (error == B_NO_MEMORY)
			return error;
	}

	return B_OK;
}


status_t
Team::_LoadImageSymbols(debug_symbol_lookup_context* lookupContext,
	const image_info& imageInfo, team_id owner, int32 event, Image** _image)
{
	Image* image = new(std::nothrow) Image(imageInfo, owner, event);
	if (image == NULL)
		return B_NO_MEMORY;

	status_t error = image->LoadSymbols(lookupContext);
	if (error != B_OK) {
		delete image;
		return error;
	}

	if (!fImages.AddItem(image)) {
		delete image;
		return B_NO_MEMORY;
	}

	if (_image != NULL)
		*_image = image;

	return B_OK;
}


void
Team::_RemoveImage(int32 index, int32 event)
{
	Image* image = fImages.RemoveItemAt(index);
	if (image == NULL)
		return;

	// Note: We don't tell the threads that the image has been removed. They
	// will be updated lazily when their next profiler update arrives. This
	// is necessary, since the update might contain samples hitting that
	// image.

	image->SetDeletionEvent(event);
	image->RemoveReference();
}
