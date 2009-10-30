/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
 

#include "KeyCommandMap.h"


#include <stdio.h>


#include <OS.h>
#include <File.h>
#include <NodeMonitor.h>
#include <Entry.h>
#include <WindowScreen.h>
#include <MessageFilter.h>
#include <Beep.h>


#include "ShortcutsFilterConstants.h"
#include "BitFieldTesters.h"
#include "CommandActuators.h"

#define FILE_UPDATED 'fiUp'

class hks {
public:
	hks(int32 key, BitFieldTester* t, CommandActuator* act, const BMessage& a)
		:
		fKey(key),
		fTester(t),
		fActuator(act),
		fActuatorMessage(a)
	{
		// empty
	}


	~hks() 
	{
		delete fActuator;
		delete fTester;
	}

			int32 GetKey() const {return fKey;}
			bool DoModifiersMatch(uint32 bits) const 
			{
				return fTester->IsMatching(bits);
			}
			const BMessage& GetActuatorMsg() const {return fActuatorMessage;}
			CommandActuator* GetActuator() {return fActuator;}

private:
			int32 				fKey;
			BitFieldTester* 	fTester;
			CommandActuator* 	fActuator;
			const BMessage 		fActuatorMessage;
};


KeyCommandMap::KeyCommandMap(const char* file)
	:
	fSpecs(NULL)
{
	fFileName = new char[strlen(file) + 1];
	strcpy(fFileName, file);

	BEntry fileEntry(fFileName);
	if (fileEntry.InitCheck() == B_NO_ERROR) {
		node_ref nref;
		
		if (fileEntry.GetNodeRef(&nref) == B_NO_ERROR)
			watch_node(&nref, B_WATCH_STAT, this);
	}

	BMessage msg(FILE_UPDATED);
	PostMessage(&msg);

	fPort = create_port(1, SHORTCUTS_CATCHER_PORT_NAME);
	_PutMessageToPort(); // advertise our BMessenger to the world
}


KeyCommandMap::~KeyCommandMap()
{
	if (fPort >= 0)
		close_port(fPort);
	
	for (int i = fInjects.CountItems() - 1; i >= 0; i--) 
		delete (BMessage*)fInjects.ItemAt(i);
	
	stop_watching(this); // don't know if this is necessary, but it can't hurt
	_DeleteHKSList(fSpecs);
	delete [] fFileName;
}


void
KeyCommandMap::MouseMoved(const BMessage* mm)
{
	// Save the mouse state for later...
	fLastMouseMessage = *mm;
}


filter_result
KeyCommandMap::KeyEvent(const BMessage* keyMsg, BList* outlist, 
	const BMessenger& sendTo) 
{
	uint32 modifiers;
	filter_result ret = B_DISPATCH_MESSAGE; // default: pass it on

	if (keyMsg->FindInt32("modifiers", (int32*) &modifiers) == B_NO_ERROR) {
		int32 key;
		if (keyMsg->FindInt32("key", &key) == B_NO_ERROR) {
			if (fSyncSpecs.Lock()) {
				if (fSpecs != NULL) {
					int num = fSpecs->CountItems();
					
					for (int i = 0; i < num; i++) {
						hks* next = (hks*) fSpecs->ItemAt(i);
						
						if ((key == next->GetKey()) 
							&& (next->DoModifiersMatch(modifiers))) {
							void* asyncData = NULL;
							ret = next->GetActuator()->
								KeyEvent(keyMsg, outlist, &asyncData, &fLastMouseMessage);
							
							if (asyncData) {
								BMessage newMsg(*keyMsg);
								newMsg.AddMessage("act", &next->GetActuatorMsg());
								newMsg.AddPointer("adata", asyncData);
								sendTo.SendMessage(&newMsg);
							}
						}
					}
				}
				fSyncSpecs.Unlock();
			}
		}
	}
	return ret;
}


void
KeyCommandMap::DrainInjectedEvents(const BMessage* keyMsg, BList* outlist, 
	const BMessenger& sendTo)
{
	BList temp;
	if (fSyncSpecs.Lock()) {
		temp = fInjects;
		fInjects.MakeEmpty();
		fSyncSpecs.Unlock();
	}

	int is = temp.CountItems();
	for (int i = 0; i < is; i++) {
		BMessage* msg = (BMessage*)temp.ItemAt(i);
		BArchivable* arc = instantiate_object(msg);
		
		if (arc) {
			CommandActuator* act = dynamic_cast<CommandActuator*>(arc);
			
			if (act) {
				BMessage newMsg(*keyMsg);
				newMsg.what = B_KEY_DOWN;
				void* asyncData = NULL;
				(void) act->KeyEvent(&newMsg, outlist, &asyncData, 
					&fLastMouseMessage);
				
				if (asyncData) {
					newMsg.AddMessage("act", msg);
					newMsg.AddPointer("adata", asyncData);
					sendTo.SendMessage(&newMsg);
				}
			}
			delete arc;
		}
		delete msg;
	}
}


void
KeyCommandMap::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case EXECUTE_COMMAND:
		{
			BMessage subMsg;
			
			if (msg->FindMessage("act", &subMsg) == B_NO_ERROR) {
				if (fSyncSpecs.Lock()) {
					fInjects.AddItem(new BMessage(subMsg));
					fSyncSpecs.Unlock();
					
					// This evil hack forces input_server to call Filter() on 
					// us so we can process the injected event.
					BPoint lmp;
					status_t err = fLastMouseMessage.FindPoint("where", &lmp);
					if (err == B_NO_ERROR) 
						set_mouse_position((int32)lmp.x, (int32)lmp.y);
				}
			}
		break;
		}

		case REPLENISH_MESSENGER:
			_PutMessageToPort();
		break;

		case B_NODE_MONITOR:
		case FILE_UPDATED:
		{
			BMessage fileMsg;
			BFile file(fFileName, B_READ_ONLY);
			if ((file.InitCheck() == B_NO_ERROR)
				&& (fileMsg.Unflatten(&file) == B_NO_ERROR)) {
				BList* newList = new BList;
				
				// whatever this is set to will be deleted below.
				// defaults to no deletion
				BList* oldList = NULL;

				int i = 0;
				BMessage msg;
				while (fileMsg.FindMessage("spec", i++, &msg) == B_NO_ERROR) {
					uint32 key;
					BMessage testerMsg;
					BMessage actMsg;

					if ((msg.FindInt32("key", (int32*) &key) == B_NO_ERROR)
						&& ((msg.FindMessage("act", &actMsg)) == B_NO_ERROR)
						&& ((msg.FindMessage("modtester", &testerMsg)) 
							== B_NO_ERROR)) {
						
						BArchivable* arcObj = instantiate_object(&testerMsg);
						if (arcObj) {
							BitFieldTester* tester = 
								dynamic_cast<BitFieldTester*>(arcObj);
							
							if (tester) {
								BArchivable* barcObj = 
									instantiate_object(&actMsg);
								
								if (barcObj) {
									CommandActuator* act = 
										dynamic_cast<CommandActuator*>(barcObj);
									
									if (act) 
										newList->AddItem(
											new hks(key, tester, act, actMsg));
									else {
										delete barcObj;
										delete tester;
									}
								} else 
									delete tester;
							} else
								delete arcObj;
						}
					}
				}

				if (fSyncSpecs.Lock()) {
					// swap in the new list
					oldList = fSpecs;
					fSpecs = newList;
					fSyncSpecs.Unlock();
				} else {
					// wtf? This shouldn't happen...
					oldList = newList; // but clean up if it does
				}
				_DeleteHKSList(oldList);
			}
		}
		break;
	}
}


// Deletes an HKS-filled BList and its contents.
void KeyCommandMap::_DeleteHKSList(BList* l)
{
	if (l != NULL) {
		int num = l->CountItems();
		for (int i = 0; i < num; i++) 
			delete ((hks*) l->ItemAt(i));
		delete l;
	}
}


void KeyCommandMap::_PutMessageToPort()
{
	if (fPort >= 0) {
		BMessenger toMe(this);
		BMessage m;
		m.AddMessenger("target", toMe);

		char buf[2048];
		ssize_t fs = m.FlattenedSize();
		if ((fs <= sizeof(buf)) && (m.Flatten(buf, fs) == B_NO_ERROR)) 
			write_port_etc(fPort, 0, buf, fs, B_TIMEOUT, 250000);
	}
}
