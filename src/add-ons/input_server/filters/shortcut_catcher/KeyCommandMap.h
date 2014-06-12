/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef _KEY_COMMAND_MAP_H
#define _KEY_COMMAND_MAP_H


#include <List.h>
#include <Locker.h>


#include <Looper.h>
#include <Messenger.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Node.h>


// Maps BMessages to ShortcutsSpecs.
// The thread here gets file update messages, and updates
// the fSpecs list to match them (asynchronously).

class KeyCommandMap : public BLooper {
public:
								KeyCommandMap(const char* watchFile);
	virtual						~KeyCommandMap();

	// Called when a key is pressed or depressed, in the input_server thread.
	// * keyMessage is the KEY_DOWN message given by the Input Server.
	// * outList is a BList that additional BMessages can be added to (to insert
	//   events into the input stream.)
	// * sendMessagesTo is the address of a queue to send command messages to,
	//   for async effects.
			filter_result		KeyEvent(const BMessage* keyMessage,
									BList* outList,
									const BMessenger& sendMessagesTo);

	// Called whenever the a B_MOUSE_MOVED message is received.
			void				MouseMessageReceived(const BMessage* message);
	virtual	void				MessageReceived(BMessage* message);
			void				DrainInjectedEvents(const BMessage* keyMessage,
									BList* outList,
									const BMessenger& sendMessagesTo);

private:
			void 				_PutMessageToPort();
			void 				_DeleteHKSList(BList* list);

			port_id 			fPort;
			char*				fFileName;
			node_ref			fNodeRef;
			BLocker 			fSyncSpecs;
	// locks the lists below
			BList				fInjects;
			BList*	 			fSpecs;
			BMessage 			fLastMouseMessage;
};


#endif	// _KEY_COMMAND_MAP_H
