/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
 
 
#ifndef KeyCommandMap_h
#define KeyCommandMap_h

#include <List.h>
#include <Locker.h>


#include <Looper.h>
#include <Messenger.h>
#include <Message.h>
#include <MessageFilter.h>

// Maps BMessages to ShortcutsSpecs!
// The thread here gets file update messages, and updates
// the fSpecs list to match them (asynchronously).
class KeyCommandMap : public BLooper {
public:
									KeyCommandMap(const char* watchFile);
									~KeyCommandMap();

	// Called when a key is pressed or depressed, in the input_server thread. 
	// (keyMsg) is the KEY_DOWN message given by the Input Server. (outlist) is
	// a BList that additional BMessages can be added to (to insert events into
	// the input stream) (sendMessagesTo) is the address of a queue to send 
	// command messages to, for async effects.
				filter_result		KeyEvent(const BMessage* keyMsg, 
										BList* outlist, 
										const BMessenger& sendMessagesTo);
 
	// Called whenever the a B_MOUSE_MOVED message is received.
				void 				MouseMoved(const BMessage* mouseMoveMsg);
				void 				MessageReceived(BMessage* msg);
				void 				DrainInjectedEvents(const BMessage* keyMsg,
										BList* outlist, 
										const BMessenger& sendMessagesTo);

private: 
				void 				_PutMessageToPort();
				void 				_DeleteHKSList(BList* list);
				
				port_id 			fPort;
				char*				fFileName;
				BLocker 			fSyncSpecs;	// locks the lists below
				BList 				fInjects;
				BList*	 			fSpecs; 
				BMessage 			fLastMouseMessage;
};

#endif
