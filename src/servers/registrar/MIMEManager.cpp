// MIMEManager.cpp

#include <Bitmap.h>
#include <ClassInfo.h>
#include <Message.h>
#include <RegistrarDefs.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <string>

#include "MIMEManager.h"

/*!
	\class MIMEManager
	\brief MIMEManager handles communication between BMimeType and the system-wide
	MimeDatabase object for BMimeType's write and non-atomic read functions.

*/

// constructor
/*!	\brief Creates and initializes a MIMEManager.
*/
MIMEManager::MIMEManager()
		   : BLooper("main_mime")
		   , fMimeDatabase()
{
}

// destructor
/*!	\brief Frees all resources associate with this object.
*/
MIMEManager::~MIMEManager()
{
}

// MessageReceived
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
					    ? fMimeDatabase.StartWatching(messenger)
					      : fMimeDatabase.StopWatching(messenger);
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
					    ? fMimeDatabase.Install(type)
					      : fMimeDatabase.Delete(type);
			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);				
			break;
		}
	
		default:
			printf("MIMEMan: msg->what == %.4s\n", &(message->what));
			BLooper::MessageReceived(message);
			break;
	}
}

// HandleSetParam 
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
					err = fMimeDatabase.SetAppHint(type, &ref);
				break;
			}
		
			case B_REG_MIME_ATTR_INFO:
			{
				BMessage info;
				err = message->FindMessage("attr info", &info);
				if (!err)
					err = fMimeDatabase.SetAttrInfo(type, &info);
				break;
			}
		
			case B_REG_MIME_DESCRIPTION:
			{
				bool isLong;
				const char *description;
				err = message->FindBool("long", &isLong);
				if (!err)
					err = message->FindString("description", &description);
				if (!err) 
					err = (isLong
						     ? fMimeDatabase.SetLongDescription(type, description)
						       : fMimeDatabase.SetShortDescription(type, description));
				break;
			}
			
			case B_REG_MIME_FILE_EXTENSIONS:
			{
				BMessage extensions;
				err = message->FindMessage("extensions", &extensions);
				if (!err)
					err = fMimeDatabase.SetFileExtensions(type, &extensions);
				break;
			}
		
				
			case B_REG_MIME_ICON:
			case B_REG_MIME_ICON_FOR_TYPE:
			{
				const void *data;
				ssize_t dataSize;
				int32 size;
				err = message->FindData("icon data", B_RAW_TYPE, &data, &dataSize);
				if (!err)
					err = message->FindInt32("icon size", &size);
				if (which == B_REG_MIME_ICON_FOR_TYPE) {
					const char *fileType;
					if (!err)
						err = message->FindString("file type", &fileType);
					if (!err)
						err = fMimeDatabase.SetIconForType(type, fileType, data,
								dataSize, (icon_size)size);				
				} else {
					if (!err) 
						err = fMimeDatabase.SetIcon(type, data, dataSize,
								(icon_size)size);
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
				if (!err)
					err = fMimeDatabase.SetPreferredApp(type, signature, (app_verb)verb);			
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

// HandleSetParam 
//! Handles all B_REG_MIME_SET_PARAM messages
void
MIMEManager::HandleDeleteParam(BMessage *message)
{
//	using BPrivate::MimeDatabase;

	status_t err;
	int32 which;
	const char *type;
	
	err = message->FindString("type", &type);
	if (!err)
		err = message->FindInt32("which", &which);
	if (!err) {
		switch (which) {
			case B_REG_MIME_APP_HINT:
				err = fMimeDatabase.DeleteAppHint(type);
				break;

			case B_REG_MIME_ATTR_INFO:
				err = fMimeDatabase.DeleteAttrInfo(type);
				break;
		
			case B_REG_MIME_DESCRIPTION:
			{
				bool isLong;
				err = message->FindBool("long", &isLong);
				if (!err) 
					err = isLong
						    ? fMimeDatabase.DeleteLongDescription(type)
						      : fMimeDatabase.DeleteShortDescription(type);
				break;
			}
			
			case B_REG_MIME_FILE_EXTENSIONS:
				err = fMimeDatabase.DeleteFileExtensions(type);
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
					if (!err)
						err = fMimeDatabase.DeleteIconForType(type, fileType, (icon_size)size);
				} else {
					if (!err) 
						err = fMimeDatabase.DeleteIcon(type, (icon_size)size);
				}
				break;
			}
				
			case B_REG_MIME_PREFERRED_APP:
			{
				int32 verb;
				err = message->FindInt32("app verb", &verb);
				if (!err)
					err = fMimeDatabase.DeletePreferredApp(type, (app_verb)verb);
				break;
			}
			
			case B_REG_MIME_SNIFFER_RULE:
				err = fMimeDatabase.DeleteSnifferRule(type);
				break;			

			default:
				err = B_BAD_VALUE;
				break;				
		}		
	}

	BMessage reply(B_REG_RESULT);
	reply.AddInt32("result", err);
	message->SendReply(&reply, this);				
}

