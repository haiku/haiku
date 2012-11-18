/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 *		Tyler Dauwalder
 */


#include "MIMEManager.h"

#include <stdio.h>
#include <string>

#include <Bitmap.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <RegistrarDefs.h>
#include <String.h>
#include <TypeConstants.h>

#include "CreateAppMetaMimeThread.h"
#include "MimeSnifferAddonManager.h"
#include "TextSnifferAddon.h"
#include "UpdateMimeInfoThread.h"


using namespace std;
using namespace BPrivate;


/*!	\class MIMEManager
	\brief MIMEManager handles communication between BMimeType and the system-wide
	MimeDatabase object for BMimeType's write and non-atomic read functions.

*/


/*!	\brief Creates and initializes a MIMEManager.
*/
MIMEManager::MIMEManager()
	:
	BLooper("main_mime"),
	fDatabase(),
	fThreadManager()
{
	AddHandler(&fThreadManager);

	// prepare the MimeSnifferAddonManager and the built-in add-ons
	status_t error = MimeSnifferAddonManager::CreateDefault();
	if (error == B_OK) {
		MimeSnifferAddonManager* addonManager
			= MimeSnifferAddonManager::Default();
		addonManager->AddMimeSnifferAddon(new(nothrow) TextSnifferAddon());
	}
}


/*!	\brief Frees all resources associate with this object.
*/
MIMEManager::~MIMEManager()
{
}


/*!	\brief Overrides the super class version to handle the MIME specific
		   messages.
	\param message The message to be handled
*/
void
MIMEManager::MessageReceived(BMessage *message)
{
	BMessage reply;
	status_t err;

	switch (message->what) {
		case B_REG_MIME_SET_PARAM:
			HandleSetParam(message);
			break;

		case B_REG_MIME_DELETE_PARAM:
			HandleDeleteParam(message);
			break;

		case B_REG_MIME_START_WATCHING:
		case B_REG_MIME_STOP_WATCHING:
		{
			BMessenger messenger;
			err = message->FindMessenger("target", &messenger);
			if (!err) {
				err = message->what == B_REG_MIME_START_WATCHING
					? fDatabase.StartWatching(messenger)
					: fDatabase.StopWatching(messenger);
			}

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_INSTALL:
		case B_REG_MIME_DELETE:
		{
			const char *type;
			err = message->FindString("type", &type);
			if (!err)
				err = message->what == B_REG_MIME_INSTALL
					? fDatabase.Install(type) : fDatabase.Delete(type);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_GET_INSTALLED_TYPES:
		{
			const char *supertype;
			err = message->FindString("supertype", &supertype);
			if (err == B_NAME_NOT_FOUND)
				err = fDatabase.GetInstalledTypes(&reply);
			else if (!err)
				err = fDatabase.GetInstalledTypes(supertype, &reply);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_GET_INSTALLED_SUPERTYPES:
		{
			err = fDatabase.GetInstalledSupertypes(&reply);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_GET_SUPPORTING_APPS:
		{
			const char *type;
			err = message->FindString("type", &type);
			if (!err)
				err = fDatabase.GetSupportingApps(type, &reply);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_GET_ASSOCIATED_TYPES:
		{
			const char *extension;
			err = message->FindString("extension", &extension);
			if (!err)
				err = fDatabase.GetAssociatedTypes(extension, &reply);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_SNIFF:
		{
			BString str;
			entry_ref ref;
			const char *filename;
			err = message->FindString("filename", &filename);
			if (!err)
				err = fDatabase.GuessMimeType(filename, &str);
			else if (err == B_NAME_NOT_FOUND) {
				err = message->FindRef("file ref", &ref);
				if (!err)
					err = fDatabase.GuessMimeType(&ref, &str);
				else if (err == B_NAME_NOT_FOUND) {
					const void *data;
					ssize_t dataSize;
					err = message->FindData("data", B_RAW_TYPE, &data,
						&dataSize);
					if (!err)
						err = fDatabase.GuessMimeType(data, dataSize, &str);
				}
			}
			if (!err)
				err = reply.AddString("mime type", str);

			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);
			break;
		}

		case B_REG_MIME_CREATE_APP_META_MIME:
		case B_REG_MIME_UPDATE_MIME_INFO:
		{
			using BPrivate::Storage::Mime::MimeUpdateThread;
			using BPrivate::Storage::Mime::CreateAppMetaMimeThread;
			using BPrivate::Storage::Mime::UpdateMimeInfoThread;

			entry_ref root;
			bool recursive;
			bool synchronous = false;
			int32 force;

			MimeUpdateThread *thread = NULL;

			status_t threadStatus = B_NO_INIT;
			bool messageIsDetached = false;
			bool stillOwnsThread = true;

			// Gather our arguments
			err = message->FindRef("entry", &root);
			if (!err)
				err = message->FindBool("recursive", &recursive);
			if (!err)
				err = message->FindBool("synchronous", &synchronous);
			if (!err)
				err = message->FindInt32("force", &force);

			// Detach the message for synchronous calls
			if (!err && synchronous) {
				DetachCurrentMessage();
				messageIsDetached = true;
			}

			// Create the appropriate flavor of mime update thread
			if (!err) {
				switch (message->what) {
					case B_REG_MIME_CREATE_APP_META_MIME:
						thread = new(nothrow) CreateAppMetaMimeThread(
							synchronous ? "create_app_meta_mime (s)"
								: "create_app_meta_mime (a)",
							B_NORMAL_PRIORITY + 1, &fDatabase,
							BMessenger(&fThreadManager), &root, recursive,
							force, synchronous ? message : NULL);
						break;

					case B_REG_MIME_UPDATE_MIME_INFO:
						thread = new(nothrow) UpdateMimeInfoThread(synchronous
								? "update_mime_info (s)"
								: "update_mime_info (a)",
							B_NORMAL_PRIORITY + 1, &fDatabase,
							BMessenger(&fThreadManager), &root, recursive,
							force, synchronous ? message : NULL);
						break;

					default:
						err = B_BAD_VALUE;
						break;
				}
			}
			if (!err)
				err = thread ? B_OK : B_NO_MEMORY;
			if (!err)
				err = threadStatus = thread->InitCheck();

			// Launch the thread
			if (!err) {
				err = fThreadManager.LaunchThread(thread);
				if (!err) {
					stillOwnsThread = false;
				}
			}

			// If something went wrong, we need to notify the sender regardless. However,
			// if this is a synchronous call, we've already detached the message, and must
			// be careful that it gets deleted once and only once. Thus, if the MimeUpdateThread
			// object was created successfully, we don't need to delete the message, as that
			// object has assumed control of it. Otherwise, we are still responsible.
			if (err || !synchronous) {
				// Send the reply
				reply.what = B_REG_RESULT;
				reply.AddInt32("result", err);
				message->SendReply(&reply, this);
			}
			// Delete the message if necessary
			if (messageIsDetached && threadStatus != B_OK)
				delete message;
			// Delete the thread if necessary
			if (stillOwnsThread)
				delete thread;
			break;
		}

		default:
			printf("MIMEMan: msg->what == %" B_PRIx32 " (%.4s)\n",
				message->what, (char*)&(message->what));
			BLooper::MessageReceived(message);
			break;
	}
}


//! Handles all B_REG_MIME_SET_PARAM messages
void
MIMEManager::HandleSetParam(BMessage *message)
{
	status_t err;
	int32 which;
	const char *type;

	err = message->FindString("type", &type);
	if (!err)
		err = message->FindInt32("which", &which);
	if (!err) {
		switch (which) {
			case B_REG_MIME_APP_HINT:
			{
				entry_ref ref;
				err = message->FindRef("app hint", &ref);
				if (!err)
					err = fDatabase.SetAppHint(type, &ref);
				break;
			}

			case B_REG_MIME_ATTR_INFO:
			{
				BMessage info;
				err = message->FindMessage("attr info", &info);
				if (!err)
					err = fDatabase.SetAttrInfo(type, &info);
				break;
			}

			case B_REG_MIME_DESCRIPTION:
			{
				bool isLong;
				const char *description;
				err = message->FindBool("long", &isLong);
				if (!err)
					err = message->FindString("description", &description);
				if (!err) {
					err = isLong
						? fDatabase.SetLongDescription(type, description)
						: fDatabase.SetShortDescription(type, description);
				}
				break;
			}

			case B_REG_MIME_FILE_EXTENSIONS:
			{
				BMessage extensions;
				err = message->FindMessage("extensions", &extensions);
				if (!err)
					err = fDatabase.SetFileExtensions(type, &extensions);
				break;
			}

			case B_REG_MIME_ICON:
			case B_REG_MIME_ICON_FOR_TYPE:
			{
				const void *data;
				ssize_t dataSize;
				int32 size;
				err = message->FindData("icon data", B_RAW_TYPE, &data,
					&dataSize);
				if (!err)
					err = message->FindInt32("icon size", &size);
				if (which == B_REG_MIME_ICON_FOR_TYPE) {
					const char *fileType;
					if (!err)
						err = message->FindString("file type", &fileType);
					if (!err) {
						err = size == -1
							? fDatabase.SetIconForType(type, fileType, data,
								dataSize)
							: fDatabase.SetIconForType(type, fileType, data,
								dataSize, (icon_size)size);
					}
				} else {
					if (!err) {
						err = size == -1
							? fDatabase.SetIcon(type, data, dataSize)
							: fDatabase.SetIcon(type, data, dataSize,
								(icon_size)size);
					}
				}
				break;
				// End temporary fix code
			}

			case B_REG_MIME_PREFERRED_APP:
			{
				const char *signature;
				int32 verb;
				err = message->FindString("signature", &signature);
				if (!err)
					err = message->FindInt32("app verb", &verb);
				if (!err) {
					err = fDatabase.SetPreferredApp(type, signature,
						(app_verb)verb);
				}
				break;
			}

			case B_REG_MIME_SNIFFER_RULE:
			{
				const char *rule;
				err = message->FindString("sniffer rule", &rule);
				if (!err)
					err = fDatabase.SetSnifferRule(type, rule);
				break;
			}

			case B_REG_MIME_SUPPORTED_TYPES:
			{
				BMessage types;
				bool fullSync = true;
				err = message->FindMessage("types", &types);
				if (!err)
					err = message->FindBool("full sync", &fullSync);
				if (!err)
					err = fDatabase.SetSupportedTypes(type, &types, fullSync);
				break;
			}

			default:
				err = B_BAD_VALUE;
				break;
		}
	}

	BMessage reply(B_REG_RESULT);
	reply.AddInt32("result", err);
	message->SendReply(&reply, this);
}


//! Handles all B_REG_MIME_SET_PARAM messages
void
MIMEManager::HandleDeleteParam(BMessage *message)
{
	status_t err;
	int32 which;
	const char *type;

	err = message->FindString("type", &type);
	if (!err)
		err = message->FindInt32("which", &which);
	if (!err) {
		switch (which) {
			case B_REG_MIME_APP_HINT:
				err = fDatabase.DeleteAppHint(type);
				break;

			case B_REG_MIME_ATTR_INFO:
				err = fDatabase.DeleteAttrInfo(type);
				break;

			case B_REG_MIME_DESCRIPTION:
			{
				bool isLong;
				err = message->FindBool("long", &isLong);
				if (!err) {
					err = isLong
						? fDatabase.DeleteLongDescription(type)
						: fDatabase.DeleteShortDescription(type);
				}
				break;
			}

			case B_REG_MIME_FILE_EXTENSIONS:
				err = fDatabase.DeleteFileExtensions(type);
				break;

			case B_REG_MIME_ICON:
			case B_REG_MIME_ICON_FOR_TYPE:
			{
				int32 size;
				err = message->FindInt32("icon size", &size);
				if (which == B_REG_MIME_ICON_FOR_TYPE) {
					const char *fileType;
					if (!err)
						err = message->FindString("file type", &fileType);
					if (!err) {
						err = size == -1
							? fDatabase.DeleteIconForType(type, fileType)
							: fDatabase.DeleteIconForType(type, fileType,
								(icon_size)size);
					}
				} else {
					if (!err) {
						err = size == -1
							? fDatabase.DeleteIcon(type)
							: fDatabase.DeleteIcon(type, (icon_size)size);
					}
				}
				break;
			}

			case B_REG_MIME_PREFERRED_APP:
			{
				int32 verb;
				err = message->FindInt32("app verb", &verb);
				if (!err)
					err = fDatabase.DeletePreferredApp(type, (app_verb)verb);
				break;
			}

			case B_REG_MIME_SNIFFER_RULE:
				err = fDatabase.DeleteSnifferRule(type);
				break;

			case B_REG_MIME_SUPPORTED_TYPES:
			{
				bool fullSync;
				err = message->FindBool("full sync", &fullSync);
				if (!err)
					err = fDatabase.DeleteSupportedTypes(type, fullSync);
				break;
			}

			default:
				err = B_BAD_VALUE;
				break;
		}
	}

	BMessage reply(B_REG_RESULT);
	reply.AddInt32("result", err);
	message->SendReply(&reply, this);
}

