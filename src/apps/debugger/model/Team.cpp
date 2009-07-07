/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Team.h"

#include <new>

#include <AutoLocker.h>

#include "FunctionInstance.h"
#include "ImageDebugInfo.h"
#include "SpecificImageDebugInfo.h"
#include "TeamDebugInfo.h"


// #pragma mark - Team


Team::Team(team_id teamID, TeamDebugInfo* debugInfo)
	:
	fID(teamID),
	fDebugInfo(debugInfo)
{
	fDebugInfo->AddReference();
}


Team::~Team()
{
	while (Image* image = fImages.RemoveHead())
		image->RemoveReference();

	while (Thread* thread = fThreads.RemoveHead())
		thread->RemoveReference();

	fDebugInfo->RemoveReference();
}


status_t
Team::Init()
{
	return BLocker::InitCheck();
}


void
Team::SetName(const BString& name)
{
	fName = name;
}


void
Team::AddThread(Thread* thread)
{
	fThreads.Add(thread);
	_NotifyThreadAdded(thread);
}



status_t
Team::AddThread(const ThreadInfo& threadInfo, Thread** _thread)
{
	Thread* thread = new(std::nothrow) Thread(this, threadInfo.ThreadID());
	if (thread == NULL)
		return B_NO_MEMORY;

	status_t error = thread->Init();
	if (error != B_OK) {
		delete thread;
		return error;
	}

	thread->SetName(threadInfo.Name());
	AddThread(thread);

	if (_thread != NULL)
		*_thread = thread;

	return B_OK;
}


void
Team::RemoveThread(Thread* thread)
{
	fThreads.Remove(thread);
	_NotifyThreadRemoved(thread);
}


bool
Team::RemoveThread(thread_id threadID)
{
	Thread* thread = ThreadByID(threadID);
	if (thread == NULL)
		return false;

	RemoveThread(thread);
	thread->RemoveReference();
	return true;
}


Thread*
Team::ThreadByID(thread_id threadID) const
{
	for (ThreadList::ConstIterator it = fThreads.GetIterator();
			Thread* thread = it.Next();) {
		if (thread->ID() == threadID)
			return thread;
	}

	return NULL;
}


const ThreadList&
Team::Threads() const
{
	return fThreads;
}


status_t
Team::AddImage(const ImageInfo& imageInfo, LocatableFile* imageFile,
	Image** _image)
{
	Image* image = new(std::nothrow) Image(this, imageInfo, imageFile);
	if (image == NULL)
		return B_NO_MEMORY;

	status_t error = image->Init();
	if (error != B_OK) {
		delete image;
		return error;
	}

	fImages.Add(image);
	_NotifyImageAdded(image);

	if (_image != NULL)
		*_image = image;

	return B_OK;
}


void
Team::RemoveImage(Image* image)
{
	fImages.Remove(image);
	_NotifyImageRemoved(image);
}


bool
Team::RemoveImage(image_id imageID)
{
	Image* image = ImageByID(imageID);
	if (image == NULL)
		return false;

	RemoveImage(image);
	image->RemoveReference();
	return true;
}


Image*
Team::ImageByID(image_id imageID) const
{
	for (ImageList::ConstIterator it = fImages.GetIterator();
			Image* image = it.Next();) {
		if (image->ID() == imageID)
			return image;
	}

	return NULL;
}


Image*
Team::ImageByAddress(target_addr_t address) const
{
	for (ImageList::ConstIterator it = fImages.GetIterator();
			Image* image = it.Next();) {
		if (image->ContainsAddress(address))
			return image;
	}

	return NULL;
}


const ImageList&
Team::Images() const
{
	return fImages;
}


status_t
Team::GetStatementAtAddress(target_addr_t address, FunctionInstance*& _function,
	Statement*& _statement)
{
	// get the image at the address
	Image* image = ImageByAddress(address);
	if (image == NULL)
		return B_ENTRY_NOT_FOUND;

	ImageDebugInfo* imageDebugInfo = image->GetImageDebugInfo();
	if (imageDebugInfo == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the function
	FunctionInstance* functionInstance
		= imageDebugInfo->FunctionAtAddress(address);
	if (functionInstance == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the statement from the image debug info
	FunctionDebugInfo* functionDebugInfo
		= functionInstance->GetFunctionDebugInfo();
	status_t error = functionDebugInfo->GetSpecificImageDebugInfo()
		->GetStatement(functionDebugInfo, address, _statement);
		// TODO: Provide the corresponding SourceCode, if available!
	if (error != B_OK)
		return error;

	_function = functionInstance;
	return B_OK;
}


void
Team::AddListener(Listener* listener)
{
	AutoLocker<Team> locker(this);
	fListeners.Add(listener);
}


void
Team::RemoveListener(Listener* listener)
{
	AutoLocker<Team> locker(this);
	fListeners.Remove(listener);
}


void
Team::NotifyThreadStateChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadStateChanged(
			ThreadEvent(TEAM_EVENT_THREAD_STATE_CHANGED, thread));
	}
}


void
Team::NotifyThreadCpuStateChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadCpuStateChanged(
			ThreadEvent(TEAM_EVENT_THREAD_CPU_STATE_CHANGED, thread));
	}
}


void
Team::NotifyThreadStackTraceChanged(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadStackTraceChanged(
			ThreadEvent(TEAM_EVENT_THREAD_STACK_TRACE_CHANGED, thread));
	}
}


void
Team::NotifyImageDebugInfoChanged(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageDebugInfoChanged(
			ImageEvent(TEAM_EVENT_IMAGE_DEBUG_INFO_CHANGED, image));
	}
}


void
Team::_NotifyThreadAdded(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadAdded(ThreadEvent(TEAM_EVENT_THREAD_ADDED, thread));
	}
}


void
Team::_NotifyThreadRemoved(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadRemoved(ThreadEvent(TEAM_EVENT_THREAD_REMOVED, thread));
	}
}


void
Team::_NotifyImageAdded(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageAdded(ImageEvent(TEAM_EVENT_IMAGE_ADDED, image));
	}
}


void
Team::_NotifyImageRemoved(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageRemoved(ImageEvent(TEAM_EVENT_IMAGE_REMOVED, image));
	}
}


// #pragma mark - Event


Team::Event::Event(uint32 type, Team* team)
	:
	fEventType(type),
	fTeam(team)
{
}


// #pragma mark - ThreadEvent


Team::ThreadEvent::ThreadEvent(uint32 type, Thread* thread)
	:
	Event(type, thread->GetTeam()),
	fThread(thread)
{
}


// #pragma mark - ImageEvent


Team::ImageEvent::ImageEvent(uint32 type, Image* image)
	:
	Event(type, image->GetTeam()),
	fImage(image)
{
}


// #pragma mark - Listener


Team::Listener::~Listener()
{
}


void
Team::Listener::ThreadAdded(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadRemoved(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ImageAdded(const Team::ImageEvent& event)
{
}


void
Team::Listener::ImageRemoved(const Team::ImageEvent& event)
{
}


void
Team::Listener::ThreadStateChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadCpuStateChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ThreadStackTraceChanged(const Team::ThreadEvent& event)
{
}


void
Team::Listener::ImageDebugInfoChanged(const Team::ImageEvent& event)
{
}
