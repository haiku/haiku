// MIMEManager.cpp

#include <Message.h>
#include <RegistrarDefs.h>
#include <stdio.h>
#include <string>

#include "MIMEManager.h"

/*!
	\class MIMEManager
	\brief MIMEManager is the master of the MIME data base.

	All write and non-atomic read accesses are carried out by this class.
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
		{
			int32 which;
			const char *type;
			
			err = message->FindString("type", &type);
			if (!err)
				err = message->FindInt32("which", &which);
			if (!err) {
				switch (which) {
					case B_REG_MIME_DESCRIPTION:
					{
						bool isLong;
						const char *description;
						err = message->FindBool("long", &isLong);
						if (!err)
							err = message->FindString("description", &description);
						if (!err) {
							err = (isLong
								     ? fMimeDatabase.SetLongDescription(type, description)
								       : fMimeDatabase.SetShortDescription(type, description));
						}						
						break;
					}
						
					default:
						err = B_BAD_VALUE;
						break;				
				}
				
				reply.what = B_SIMPLE_DATA;
				reply.AddInt32("result", err);
				message->SendReply(&reply, this);				
			}
			
			break;		
		}
		
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
			
			reply.what = B_SIMPLE_DATA;
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
			reply.what = B_SIMPLE_DATA;
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

