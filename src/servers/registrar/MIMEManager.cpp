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
		   , fDatabase()
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
					    ? fDatabase.Install(type)
					      : fDatabase.Delete(type);
					      
			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			message->SendReply(&reply, this);				
			break;
		}
		
		case B_REG_MIME_GET_INSTALLED_TYPES:
		{
			BMessage types;
			const char *supertype;
			err = message->FindString("supertype", &supertype);
			if (err == B_NAME_NOT_FOUND) 
				err = fDatabase.GetInstalledTypes(&types);
			else if (!err) 
				err = fDatabase.GetInstalledTypes(supertype, &types);
				
			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			reply.AddMessage("types", &types);
			message->SendReply(&reply, this);				
			break;
		}
		
		case B_REG_MIME_GET_INSTALLED_SUPERTYPES:
		{
			BMessage types;
			err = fDatabase.GetInstalledSupertypes(&types);
				
			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			reply.AddMessage("types", &types);
			message->SendReply(&reply, this);				
			break;
		}

		case B_REG_MIME_GET_SUPPORTING_APPS:
		{
			BMessage apps;
			const char *type;
			err = message->FindString("type", &type);
			if (!err) 
				err = fDatabase.GetSupportingApps(type, &apps);
				
			reply.what = B_REG_RESULT;
			reply.AddInt32("result", err);
			reply.AddMessage("signatures", &apps);
			message->SendReply(&reply, this);				
			break;
		}
		
		default:
			printf("MIMEMan: msg->what == %.4s\n", (char*)&(message->what));
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
				if (!err) 
					err = (isLong
						     ? fDatabase.SetLongDescription(type, description)
						       : fDatabase.SetShortDescription(type, description));
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
				err = message->FindData("icon data", B_RAW_TYPE, &data, &dataSize);
				if (!err)
					err = message->FindInt32("icon size", &size);
				if (which == B_REG_MIME_ICON_FOR_TYPE) {
					const char *fileType;
					if (!err)
						err = message->FindString("file type", &fileType);
					if (!err)
						err = fDatabase.SetIconForType(type, fileType, data,
								dataSize, (icon_size)size);				
				} else {
					if (!err) 
						err = fDatabase.SetIcon(type, data, dataSize,
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
					err = fDatabase.SetPreferredApp(type, signature, (app_verb)verb);			
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
				err = fDatabase.DeleteAppHint(type);
				break;

			case B_REG_MIME_ATTR_INFO:
				err = fDatabase.DeleteAttrInfo(type);
				break;
		
			case B_REG_MIME_DESCRIPTION:
			{
				bool isLong;
				err = message->FindBool("long", &isLong);
				if (!err) 
					err = isLong
						    ? fDatabase.DeleteLongDescription(type)
						      : fDatabase.DeleteShortDescription(type);
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
					if (!err)
						err = fDatabase.DeleteIconForType(type, fileType, (icon_size)size);
				} else {
					if (!err) 
						err = fDatabase.DeleteIcon(type, (icon_size)size);
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
				err = fDatabase.DeleteSupportedTypes(type);
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

