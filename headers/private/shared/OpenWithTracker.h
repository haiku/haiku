/*
 * Copyright 2009 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _OPEN_WITH_TRACKER_H
#define _OPEN_WITH_TRACKER_H


#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>


status_t
OpenWithTracker(const entry_ref* ref)
{
	status_t status;
	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", ref);

	BMessenger tracker("application/x-vnd.Be-TRAK");
	status = tracker.SendMessage(&message);
	return status;
}


status_t
OpenWithTracker(const char* path)
{
	status_t status;
	entry_ref ref;
	status = get_ref_for_path(path, &ref);
	if (status != B_OK)
		return status;

	return OpenWithTracker(&ref);
}


status_t
OpenWithTracker(directory_which which, const char* relativePath = NULL,
	bool createDirectory = false, BVolume* volume = NULL)
{
	status_t status;
	BPath path;
	find_directory(which, &path, createDirectory, volume);

	if (relativePath)
		path.Append(relativePath);

	entry_ref ref;
	BEntry entry(path.Path());

	if (!entry.Exists())
		return B_NAME_NOT_FOUND;
	
	status = entry.GetRef(&ref);
	if (status != B_OK)
		return status;

	return OpenWithTracker(&ref);
}


#endif	// _OPEN_WITH_TRACKER_H

