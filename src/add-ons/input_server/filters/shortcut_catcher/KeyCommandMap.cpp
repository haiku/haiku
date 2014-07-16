/*
 * Copyright 1999-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Jessica Hamilton, jessica.l.hamilton@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


#include "KeyCommandMap.h"

#include <stdio.h>

#include <Beep.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageFilter.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <PathFinder.h>
#include <PathMonitor.h>
#include <StringList.h>
#include <WindowScreen.h>

#include "BitFieldTesters.h"
#include "CommandActuators.h"
#include "ShortcutsFilterConstants.h"


#define FILE_UPDATED 'fiUp'


//	#pragma mark - hks


class hks {
public:
	hks(int32 key, BitFieldTester* tester, CommandActuator* actuator,
		const BMessage& actuatorMessage)
		:
		fKey(key),
		fTester(tester),
		fActuator(actuator),
		fActuatorMessage(actuatorMessage)
	{
	}

	~hks()
	{
		delete fActuator;
		delete fTester;
	}

	int32 GetKey() const
		{ return fKey; }
	bool DoModifiersMatch(uint32 bits) const
		{ return fTester->IsMatching(bits); }
	const BMessage& GetActuatorMessage() const
		{ return fActuatorMessage; }
	CommandActuator* GetActuator()
		{ return fActuator; }

private:
	int32 				fKey;
	BitFieldTester* 	fTester;
	CommandActuator* 	fActuator;
	const BMessage		fActuatorMessage;
};


//	#pragma mark - KeyCommandMap


KeyCommandMap::KeyCommandMap(const char* file)
	:
	BLooper("Shortcuts map watcher"),
	fSpecs(NULL)
{
	fFileName = new char[strlen(file) + 1];
	strcpy(fFileName, file);

	BEntry fileEntry(fFileName);

	BPrivate::BPathMonitor::StartWatching(fFileName,
		B_WATCH_STAT | B_WATCH_FILES_ONLY, this);

	if (fileEntry.InitCheck() == B_OK) {
		BMessage message(FILE_UPDATED);
		PostMessage(&message);
	}

	fPort = create_port(1, SHORTCUTS_CATCHER_PORT_NAME);
	_PutMessageToPort();
		// advertise our BMessenger to the world
}


KeyCommandMap::~KeyCommandMap()
{
	if (fPort >= 0)
		close_port(fPort);

	for (int32 i = fInjects.CountItems() - 1; i >= 0; i--)
		delete (BMessage*)fInjects.ItemAt(i);

	BPrivate::BPathMonitor::StopWatching(BMessenger(this, this));
		// don't know if this is necessary, but it can't hurt

	_DeleteHKSList(fSpecs);

	delete[] fFileName;
}


void
KeyCommandMap::MouseMessageReceived(const BMessage* message)
{
	// Save the mouse state for later...
	fLastMouseMessage = *message;
}


filter_result
KeyCommandMap::KeyEvent(const BMessage* keyMessage, BList* outList,
	const BMessenger& sendTo)
{
	filter_result result = B_DISPATCH_MESSAGE;
	uint32 modifiers;
	int32 key;

	if (keyMessage->FindInt32("modifiers", (int32*)&modifiers) == B_OK
		&& keyMessage->FindInt32("key", &key) == B_OK
		&& fSpecs != NULL && fSyncSpecs.Lock()) {
		int32 count = fSpecs->CountItems();
		for (int32 i = 0; i < count; i++) {
			hks* next = (hks*)fSpecs->ItemAt(i);

			if (key == next->GetKey() && next->DoModifiersMatch(modifiers)) {
				void* asyncData = NULL;
				result = next->GetActuator()->KeyEvent(keyMessage, outList,
					&asyncData, &fLastMouseMessage);

				if (asyncData != NULL) {
					BMessage newMessage(*keyMessage);
					newMessage.AddMessage("act", &next->GetActuatorMessage());
					newMessage.AddPointer("adata", asyncData);
					sendTo.SendMessage(&newMessage);
				}
			}
		}
		fSyncSpecs.Unlock();
	}

	return result;
}


void
KeyCommandMap::DrainInjectedEvents(const BMessage* keyMessage, BList* outList,
	const BMessenger& sendTo)
{
	BList temp;
	if (fSyncSpecs.Lock()) {
		temp = fInjects;
		fInjects.MakeEmpty();
		fSyncSpecs.Unlock();
	}

	int32 count = temp.CountItems();
	for (int32 i = 0; i < count; i++) {
		BMessage* message = (BMessage*)temp.ItemAt(i);

		BArchivable* archive = instantiate_object(message);
		if (archive != NULL) {
			CommandActuator* actuator
				= dynamic_cast<CommandActuator*>(archive);
			if (actuator != NULL) {
				BMessage newMessage(*keyMessage);
				newMessage.what = B_KEY_DOWN;

				void* asyncData = NULL;
				actuator->KeyEvent(&newMessage, outList, &asyncData,
					&fLastMouseMessage);

				if (asyncData != NULL) {
					newMessage.AddMessage("act", message);
					newMessage.AddPointer("adata", asyncData);
					sendTo.SendMessage(&newMessage);
				}
			}
			delete archive;
		}
		delete message;
	}
}


void
KeyCommandMap::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case EXECUTE_COMMAND:
		{
			BMessage actuatorMessage;
			if (message->FindMessage("act", &actuatorMessage) == B_OK) {
				if (fSyncSpecs.Lock()) {
					fInjects.AddItem(new BMessage(actuatorMessage));
					fSyncSpecs.Unlock();

					// This evil hack forces input_server to call Filter() on
					// us so we can process the injected event.
					BPoint where;
					status_t err = fLastMouseMessage.FindPoint("where", &where);
					if (err == B_OK)
						set_mouse_position((int32)where.x, (int32)where.y);
				}
			}
			break;
		}

		case REPLENISH_MESSENGER:
			_PutMessageToPort();
			break;

		case B_PATH_MONITOR:
		{
			const char* path = "";
			// only fall through for appropriate file
			if (!(message->FindString("path", &path) == B_OK
					&& strcmp(path, fFileName) == 0)) {
				dev_t device;
				ino_t node;
				if (message->FindInt32("device", &device) != B_OK
					|| message->FindInt64("node", &node) != B_OK
					|| device != fNodeRef.device
					|| node != fNodeRef.node) {
					break;
				}
			}
		}
		// fall-through
		case FILE_UPDATED:
		{
			BMessage fileMessage;
			BFile file(fFileName, B_READ_ONLY);
			BList* newList = new BList;
			BList* oldList = NULL;
			if (file.InitCheck() == B_OK && fileMessage.Unflatten(&file)
					== B_OK) {
				file.GetNodeRef(&fNodeRef);
				int32 i = 0;
				BMessage message;
				while (fileMessage.FindMessage("spec", i++, &message) == B_OK) {
					uint32 key;
					BMessage testerMessage;
					BMessage actuatorMessage;

					if (message.FindInt32("key", (int32*)&key) == B_OK
						&& message.FindMessage("act", &actuatorMessage) == B_OK
						&& message.FindMessage("modtester", &testerMessage)
							== B_OK) {
						// Leave handling of add-ons shortcuts to Tracker
						BString command;
						if (message.FindString("command", &command) == B_OK) {
							BStringList paths;
							BPathFinder::FindPaths(
								B_FIND_PATH_ADD_ONS_DIRECTORY, "Tracker",
								paths);
							bool foundAddOn = false;
							int32 count = paths.CountStrings();
							for (int32 i = 0; i < count; i++) {
								if (command.StartsWith(paths.StringAt(i))) {
									foundAddOn = true;
									break;
								}
							}
							if (foundAddOn)
								continue;
						}

						BArchivable* archive
							= instantiate_object(&testerMessage);
						if (BitFieldTester* tester
								= dynamic_cast<BitFieldTester*>(archive)) {
							archive = instantiate_object(&actuatorMessage);
							CommandActuator* actuator
								= dynamic_cast<CommandActuator*>(archive);
							if (actuator != NULL) {
								newList->AddItem(new hks(key, tester, actuator,
									actuatorMessage));
							} else {
								delete archive;
								delete tester;
							}
						} else
							delete archive;
					}
				}
			} else {
				fNodeRef.device = -1;
				fNodeRef.node = -1;
			}

			if (fSyncSpecs.Lock()) {
				// swap in the new list
				oldList = fSpecs;
				fSpecs = newList;
				fSyncSpecs.Unlock();
			} else {
				// This should never happen... but clean up if it does.
				oldList = newList;
			}

			_DeleteHKSList(oldList);
			break;
		}
	}
}


//	#pragma mark - KeyCommandMap private methods


//! Deletes an HKS-filled BList and its contents.
void
KeyCommandMap::_DeleteHKSList(BList* list)
{
	if (list == NULL)
		return;

	int32 count = list->CountItems();
	for (int32 i = 0; i < count; i++)
		delete (hks*)list->ItemAt(i);

	delete list;
}


void
KeyCommandMap::_PutMessageToPort()
{
	if (fPort >= 0) {
		BMessage message;
		message.AddMessenger("target", this);

		char buffer[2048];
		ssize_t size = message.FlattenedSize();
		if (size <= (ssize_t)sizeof(buffer)
			&& message.Flatten(buffer, size) == B_OK) {
			write_port_etc(fPort, 0, buffer, size, B_TIMEOUT, 250000);
		}
	}
}
