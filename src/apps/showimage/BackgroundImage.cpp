/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "BackgroundImage.h"

#include <Background.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Window.h>

#include <fs_attr.h>

#include <new>
#include <stdlib.h>


using std::nothrow;

const char *kBackgroundImageInfoSet = "be:bgndimginfoset";


/*static*/
status_t
BackgroundImage::_NotifyTracker(BDirectory& directory, bool desktop)
{
	BMessenger tracker("application/x-vnd.Be-TRAK");

	if (desktop) {
		tracker.SendMessage(new BMessage(B_RESTORE_BACKGROUND_IMAGE));
	} else {
		BEntry entry;
		status_t status = directory.GetEntry(&entry);
		if (status < B_OK)
			return status;

		BPath folderPath(&entry);
		int32 i = -1;

		while (true) {
			BMessage msg(B_GET_PROPERTY);
			BMessage reply;
			i++;

			// look at the "Poses" in every Tracker window
			msg.AddSpecifier("Poses");
			msg.AddSpecifier("Window", i);

			reply.MakeEmpty();
			tracker.SendMessage(&msg, &reply);

			// break out of the loop when we're at the end of
			// the windows
			int32 err;
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &err) == B_OK
				&& err == B_BAD_INDEX)
				break;

			// don't stop for windows that don't understand
			// a request for "Poses"; they're not displaying
			// folders
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &err) == B_OK
				&& err != B_BAD_SCRIPT_SYNTAX)
				continue;

			BMessenger trackerWindow;
			if (reply.FindMessenger("result", &trackerWindow) != B_OK)
				continue;

          		// found a window with poses, ask for its path
			msg.MakeEmpty();
			msg.what = B_GET_PROPERTY;
			msg.AddSpecifier("Path");
			msg.AddSpecifier("Poses");
			msg.AddSpecifier("Window", i);

			reply.MakeEmpty();
			tracker.SendMessage(&msg, &reply);

          		// go on with the next if this din't have a path
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD)
				continue;

			entry_ref ref;
			if (reply.FindRef("result", &ref) == B_OK) {
				BEntry entry(&ref);
				BPath path(&entry);

				// these are not the paths you're looking for
				if (folderPath != path)
					continue;
			}

			trackerWindow.SendMessage(B_RESTORE_BACKGROUND_IMAGE);
		}
	}

	return B_OK;
}


/*static*/
status_t
BackgroundImage::_GetImages(BDirectory& directory, BMessage& container)
{
	attr_info info;
	status_t status = directory.GetAttrInfo(B_BACKGROUND_INFO, &info);
	if (status != B_OK)
		return status;

	char *buffer = new (nothrow) char [info.size];
	if (buffer == NULL)
		return B_NO_MEMORY;

	status = directory.ReadAttr(B_BACKGROUND_INFO, info.type,
		0, buffer, (size_t)info.size);
	if (status == info.size)
		status = container.Unflatten(buffer);
	else
		status = B_ERROR;

	delete[] buffer;

	return status;
}


/*static*/
status_t
BackgroundImage::_SetImage(BDirectory& directory, bool desktop, uint32 workspaces,
	const char* path, Mode mode, BPoint offset, bool eraseIconBackground)
{
	BMessage images;
	if (desktop) {
		status_t status = _GetImages(directory, images);
		if (status != B_OK)
			return status;

		if (workspaces == B_CURRENT_WORKSPACE)
			workspaces = 1UL << current_workspace();

		// Find old image and replace it

		uint32 imageWorkspaces;
		int32 found = -1;
		int32 i = 0;
		while (images.FindInt32(B_BACKGROUND_WORKSPACES, i,
				(int32 *)&imageWorkspaces) == B_OK) {
			// we don't care about masks that are similar to ours
			if (imageWorkspaces == workspaces) {
				found = i;
				break;
			}

			i++;
		}

		// Remove old image, if any

		if (found == i) {
			images.RemoveData(B_BACKGROUND_ERASE_TEXT, i);
			images.RemoveData(B_BACKGROUND_IMAGE, i);
			images.RemoveData(B_BACKGROUND_WORKSPACES, i);
			images.RemoveData(B_BACKGROUND_ORIGIN, i);
			images.RemoveData(B_BACKGROUND_MODE, i);
			if (desktop)
				images.RemoveData(kBackgroundImageInfoSet, i);
		}
	}

	// Insert new image

	images.AddBool(B_BACKGROUND_ERASE_TEXT, eraseIconBackground);
	images.AddString(B_BACKGROUND_IMAGE, path);
	images.AddInt32(B_BACKGROUND_WORKSPACES, workspaces);
	images.AddPoint(B_BACKGROUND_ORIGIN, offset);
	images.AddInt32(B_BACKGROUND_MODE, mode);
	if (desktop)
		images.AddInt32(kBackgroundImageInfoSet, 0);

	// Write back new image info

	size_t flattenedSize = images.FlattenedSize();
	char* buffer = new (std::nothrow) char[flattenedSize];
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = images.Flatten(buffer, flattenedSize);
	if (status != B_OK)
		return status;

	ssize_t written = directory.WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE,
		0, buffer, flattenedSize);

	delete[] buffer;

	if (written < B_OK)
		return written;
	if ((size_t)written != flattenedSize)
		return B_ERROR;

	_NotifyTracker(directory, desktop);
	return B_OK;
}


/*static*/
status_t
BackgroundImage::SetImage(BDirectory& directory, const char* path, Mode mode,
	BPoint offset, bool eraseIconBackground)
{
	return _SetImage(directory, false, 0, path, mode, offset, eraseIconBackground);
}


/*static*/
status_t
BackgroundImage::SetDesktopImage(uint32 workspaces, const char* image,
	Mode mode, BPoint offset, bool eraseIconBackground)
{
	BPath path;
	status_t status = find_directory(B_DESKTOP_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	BDirectory directory;
	status = directory.SetTo(path.Path());
	if (status != B_OK)
		return status;

	return _SetImage(directory, true, workspaces, image, mode, offset,
		eraseIconBackground);
}

