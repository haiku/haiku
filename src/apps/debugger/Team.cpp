/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Team.h"

#include <new>


Team::Team(team_id teamID)
	:
	fID(teamID)
{
}


Team::~Team()
{
	while (Image* image = fImages.RemoveHead())
		delete image;

	while (Thread* thread = fThreads.RemoveHead())
		delete thread;
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


void
Team::RemoveThread(Thread* thread)
{
	fThreads.Remove(thread);
}


bool
Team::RemoveThread(thread_id threadID)
{
	Thread* thread = ThreadByID(threadID);
	if (thread == NULL)
		return false;

	RemoveThread(thread);
	delete thread;
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


void
Team::AddImage(Image* image)
{
	fImages.Add(image);
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
}


bool
Team::RemoveImage(image_id imageID)
{
	Image* image = ImageByID(imageID);
	if (image == NULL)
		return false;

	RemoveImage(image);
	delete image;
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
