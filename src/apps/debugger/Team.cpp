/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Team.h"

#include <new>


// #pragma mark - Team


Team::Team(team_id teamID)
	:
	fID(teamID)
{
}


Team::~Team()
{
	while (Image* image = fImages.RemoveHead())
		image->RemoveReference();

	while (Thread* thread = fThreads.RemoveHead())
		thread->RemoveReference();
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
Team::AddThread(const thread_info& threadInfo, Thread** _thread)
{
	Thread* thread = new(std::nothrow) Thread(threadInfo.thread);
	if (thread == NULL)
		return B_NO_MEMORY;

	status_t error = thread->Init();
	if (error != B_OK) {
		delete thread;
		return error;
	}

	thread->SetName(threadInfo.name);
	AddThread(thread);

	if (_thread != NULL)
		*_thread = thread;

	return B_OK;
}


status_t
Team::AddThread(thread_id threadID, Thread** _thread)
{
	thread_info threadInfo;
	status_t error = get_thread_info(threadID, &threadInfo);
	return error == B_OK ? AddThread(threadInfo, _thread) : error;
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


void
Team::AddImage(Image* image)
{
	fImages.Add(image);
	_NotifyImageAdded(image);
}


status_t
Team::AddImage(const image_info& imageInfo, Image** _image)
{
	Image* image = new(std::nothrow) Image(imageInfo);
	if (image == NULL)
		return B_NO_MEMORY;

	status_t error = image->Init();
	if (error != B_OK) {
		delete image;
		return error;
	}

	AddImage(image);

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


const ImageList&
Team::Images() const
{
	return fImages;
}


void
Team::AddListener(Listener* listener)
{
	fListeners.Add(listener);
}


void
Team::RemoveListener(Listener* listener)
{
	fListeners.Remove(listener);
}


void
Team::_NotifyThreadAdded(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadAdded(this, thread);
	}
}


void
Team::_NotifyThreadRemoved(Thread* thread)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ThreadRemoved(this, thread);
	}
}


void
Team::_NotifyImageAdded(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageAdded(this, image);
	}
}


void
Team::_NotifyImageRemoved(Image* image)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->ImageRemoved(this, image);
	}
}


// #pragma mark - Listener

Team::Listener::~Listener()
{
}


void
Team::Listener::ThreadAdded(Team* team, Thread* thread)
{
}


void
Team::Listener::ThreadRemoved(Team* team, Thread* thread)
{
}


void
Team::Listener::ImageAdded(Team* team, Image* image)
{
}


void
Team::Listener::ImageRemoved(Team* team, Image* image)
{
}
