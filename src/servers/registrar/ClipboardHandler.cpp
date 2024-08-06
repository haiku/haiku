/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Gabe Yoder
 */


#include "Clipboard.h"
#include "ClipboardHandler.h"

#include <Message.h>
#include <RegistrarDefs.h>

#include <map>
#include <string>


using std::map;
using std::string;
using namespace BPrivate;


/*!
	\class ClipboardHandler
	\brief Handles all clipboard related requests.
*/

struct ClipboardHandler::ClipboardMap : map<string, Clipboard*> {};


// constructor
/*!	\brief Creates and initializes a ClipboardHandler.
*/
ClipboardHandler::ClipboardHandler()
				: BHandler(),
				  fClipboards(new ClipboardMap)
{
}


// destructor
/*!	\brief Frees all resources associate with this object.
*/
ClipboardHandler::~ClipboardHandler()
{
	for (ClipboardMap::iterator it = fClipboards->begin();
		it != fClipboards->end();
		++it)
		delete it->second;
}


// MessageReceived
/*!	\brief Overrides the super class version to handle the clipboard specific
		   messages.
	\param message The message to be handled
*/
void
ClipboardHandler::MessageReceived(BMessage *message)
{
	const char *name;
	BMessage reply;
	switch (message->what) {
		case B_REG_ADD_CLIPBOARD:
		{
			status_t result = B_BAD_VALUE;

			if (message->FindString("name", &name) == B_OK) {
				if (_GetClipboard(name))
					result = B_OK;
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", result);
			message->SendReply(&reply);
			break;
		}

		case B_REG_GET_CLIPBOARD_COUNT:
		{
			status_t result = B_BAD_VALUE;

			if (message->FindString("name", &name) == B_OK) {
				if (Clipboard *clipboard = _GetClipboard(name)) {
					reply.AddInt32("count", clipboard->Count());
					result = B_OK;
				}
			}

			reply.AddInt32("result", result);
			reply.what = B_REG_RESULT;
			message->SendReply(&reply);
			break;
		}

		case B_REG_CLIPBOARD_START_WATCHING:
		{
			status_t result = B_BAD_VALUE;

			BMessenger target;
			if (message->FindString("name", &name) == B_OK
				&& message->FindMessenger("target", &target) == B_OK) {
				Clipboard *clipboard = _GetClipboard(name);
				if (clipboard && clipboard->AddWatcher(target))
					result = B_OK;
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", result);
			message->SendReply(&reply);
			break;
		}

		case B_REG_CLIPBOARD_STOP_WATCHING:
		{
			status_t result = B_BAD_VALUE;

			BMessenger target;
			if (message->FindString("name", &name) == B_OK
				&& message->FindMessenger("target", &target) == B_OK) {
				Clipboard *clipboard = _GetClipboard(name);
				if (clipboard && clipboard->RemoveWatcher(target))
					result = B_OK;
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", result);
			message->SendReply(&reply);
			break;
		}

		case B_REG_DOWNLOAD_CLIPBOARD:
		{
			status_t result = B_BAD_VALUE;

			if (message->FindString("name", &name) == B_OK) {
				Clipboard *clipboard = _GetClipboard(name);
				if (clipboard) {
					reply.AddMessage("data", clipboard->Data());
					reply.AddMessenger("data source", clipboard->DataSource());
					reply.AddInt32("count", clipboard->Count());
					result = B_OK;
				}
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", result);
			result = message->SendReply(&reply);
			if (result != B_OK) {
				reply = BMessage();
				reply.what = B_REG_RESULT;
				reply.AddInt32("result", result);
				message->SendReply(&reply);
			}
			break;
		}

		case B_REG_UPLOAD_CLIPBOARD:
		{
			status_t result = B_BAD_VALUE;

			BMessage data;
			BMessenger source;
			if (message->FindString("name", &name) == B_OK
				&& message->FindMessenger("data source", &source) == B_OK
				&& message->FindMessage("data", &data) == B_OK) {
				Clipboard *clipboard = _GetClipboard(name);
				if (clipboard) {
					int32 localCount;
					bool failIfChanged;
					if (message->FindInt32("count", &localCount) == B_OK
						&& message->FindBool("fail if changed", &failIfChanged)
							== B_OK
						&& failIfChanged
						&& localCount != clipboard->Count()) {
						// atomic support
						result = B_ERROR;
					} else {
						clipboard->SetData(&data, source);
						result = reply.AddInt32("count", clipboard->Count());
						result = B_OK;
					}
				}
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", result);
			message->SendReply(&reply);
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


/*!	\brief Gets the clipboard with the specified name, or adds it, if not yet
		   existent.

	\param name The name of the clipboard to be returned.
	\return The clipboard with the respective name.
*/
Clipboard*
ClipboardHandler::_GetClipboard(const char *name)
{
	if (!name)
		name = "system";

	Clipboard *clipboard = NULL;
	ClipboardMap::iterator it = fClipboards->find(name);
	if (it != fClipboards->end()) {
		clipboard = it->second;
	} else {
		clipboard = new Clipboard(name);
		(*fClipboards)[name] = clipboard;
	}

	return clipboard;
}

