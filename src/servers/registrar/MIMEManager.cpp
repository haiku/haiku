// MIMEManager.cpp

#include <Message.h>

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
	switch (message->what) {
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

