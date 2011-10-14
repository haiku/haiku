/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <map>
#include <new>

#include <string.h>

#include <Autolock.h>

#include <syscalls.h>

#include "Debug.h"
#include "MessageDeliverer.h"
#include "MessagingService.h"

using std::map;
using std::nothrow;

// sService -- the singleton instance
MessagingService *MessagingService::sService = NULL;

/*!	\class MessagingArea
	\brief Represents an area of the messaging service shared between kernel
		   and registrar.

	The main purpose of the class is to retrieve (and remove) commands from
	the area.
*/

// constructor
MessagingArea::MessagingArea()
{
}

// destructor
MessagingArea::~MessagingArea()
{
	if (fID >= 0)
		delete_area(fID);
}

// Create
status_t
MessagingArea::Create(area_id kernelAreaID, sem_id lockSem, sem_id counterSem,
	MessagingArea *&_area)
{
	// allocate the object on the heap
	MessagingArea *area = new(nothrow) MessagingArea;
	if (!area)
		return B_NO_MEMORY;

	// clone the kernel area
	area_id areaID = clone_area("messaging", (void**)&area->fHeader,
		B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, kernelAreaID);
	if (areaID < 0) {
		delete area;
		return areaID;
	}

	// finish the initialization of the object
	area->fID = areaID;
	area->fSize = area->fHeader->size;
	area->fLockSem = lockSem;
	area->fCounterSem = counterSem;
	area->fNextArea = NULL;

	_area = area;
	return B_OK;
}

// Lock
bool
MessagingArea::Lock()
{
	// benaphore-like locking
	if (atomic_add(&fHeader->lock_counter, 1) == 0)
		return true;

	return (acquire_sem(fLockSem) == B_OK);
}

// Unlock
void
MessagingArea::Unlock()
{
	if (atomic_add(&fHeader->lock_counter, -1) > 1)
		release_sem(fLockSem);
}

// ID
area_id
MessagingArea::ID() const
{
	return fID;
}

// Size
int32
MessagingArea::Size() const
{
	return fSize;
}

// CountCommands
int32
MessagingArea::CountCommands() const
{
	return fHeader->command_count;
}

// PopCommand
const messaging_command *
MessagingArea::PopCommand()
{
	if (fHeader->command_count == 0)
		return NULL;

	// get the command
	messaging_command *command
		= (messaging_command*)((char*)fHeader + fHeader->first_command);

	// remove it from the area
	// (as long as the area is still locked, noone will overwrite the contents)
	if (--fHeader->command_count == 0)
		fHeader->first_command = fHeader->last_command = 0;
	else
		fHeader->first_command = command->next_command;

	return command;
}

// Discard
void
MessagingArea::Discard()
{
	fHeader->size = 0;
}

// NextKernelAreaID
area_id
MessagingArea::NextKernelAreaID() const
{
	return fHeader->next_kernel_area;
}

// SetNextArea
void
MessagingArea::SetNextArea(MessagingArea *area)
{
	fNextArea = area;
}

// NextArea
MessagingArea *
MessagingArea::NextArea() const
{
	return fNextArea;
}


// #pragma mark -

// constructor
MessagingCommandHandler::MessagingCommandHandler()
{
}

// destructor
MessagingCommandHandler::~MessagingCommandHandler()
{
}


// #pragma mark -

// DefaultSendCommandHandler
class MessagingService::DefaultSendCommandHandler
	: public MessagingCommandHandler {

	virtual void HandleMessagingCommand(uint32 _command, const void *data,
		int32 dataSize)
	{
		const messaging_command_send_message *sendData
			= (const messaging_command_send_message*)data;
		const void *messageData = (uint8*)data
			+ sizeof(messaging_command_send_message)
			+ sizeof(messaging_target) * sendData->target_count;

		DefaultMessagingTargetSet set(sendData->targets,
			sendData->target_count);
		MessageDeliverer::Default()->DeliverMessage(messageData,
			sendData->message_size, set);
	}
};

// CommandHandlerMap
struct MessagingService::CommandHandlerMap
	: map<uint32, MessagingCommandHandler*> {
};


/*! \class MessagingService
	\brief Userland implementation of the kernel -> userland messaging service.

	This service provides a way for the kernel to send BMessages (usually
	notification (e.g. node monitoring) messages) to userland applications.

	The kernel could write the messages directly to the respective target ports,
	but this has the disadvantage, that a message needs to be dropped, if the
	port is full at the moment of sending. By transferring the message to the
	registrar, it is possible to use the MessageDeliverer which retries sending
	messages on full ports.

	The message transfer is implemented via areas shared between kernel
	and registrar. By default one area is used as a ring buffer. The kernel
	adds messages to it, the registrar removes them. If the area is full, the
	kernel creates a new one and adds it to the area list.

	While the service is called `messaging service' and we were speaking of
	`messages' being passed through the areas, the service is actually more
	general. In fact `commands' are passed through the areas. Currently the
	only implemented command type is to send a message, but it is very easy
	to add further command types (e.g. one for alerting the user in case of
	errors).

	The MessagingService maintains a mapping of command types to command
	handlers (MessagingCommandHandler, which perform the actual processing
	of the commands), that can be altered via
	MessagingService::SetCommandHandler().
*/

// constructor
MessagingService::MessagingService()
	: fLock("messaging service"),
	  fLockSem(-1),
	  fCounterSem(-1),
	  fFirstArea(NULL),
	  fCommandHandlers(NULL),
	  fCommandProcessor(-1),
	  fTerminating(false)
{
}

// destructor
MessagingService::~MessagingService()
{
	fTerminating = true;

	if (fLockSem >= 0)
		delete_sem(fLockSem);
	if (fCounterSem >= 0)
		delete_sem(fCounterSem);

	if (fCommandProcessor >= 0) {
		int32 result;
		wait_for_thread(fCommandProcessor, &result);
	}

	delete fCommandHandlers;

	delete fFirstArea;
}

// Init
status_t
MessagingService::Init()
{
	// create the semaphores
	fLockSem = create_sem(0, "messaging lock");
	if (fLockSem < 0)
		return fLockSem;

	fCounterSem = create_sem(0, "messaging counter");
	if (fCounterSem < 0)
		return fCounterSem;

	// create the command handler map
	fCommandHandlers = new(nothrow) CommandHandlerMap;
	if (!fCommandHandlers)
		return B_NO_MEMORY;

	// spawn the command processor
	fCommandProcessor = spawn_thread(MessagingService::_CommandProcessorEntry,
		"messaging command processor", B_DISPLAY_PRIORITY, this);
	if (fCommandProcessor < 0)
		return fCommandProcessor;

	// register with the kernel
	area_id areaID = _kern_register_messaging_service(fLockSem, fCounterSem);
	if (areaID < 0)
		return areaID;

	// create the area
	status_t error = MessagingArea::Create(areaID, fLockSem, fCounterSem,
		fFirstArea);
	if (error != B_OK) {
		_kern_unregister_messaging_service();
		return error;
	}

	// resume the command processor
	resume_thread(fCommandProcessor);

	// install the default send message command handler
	MessagingCommandHandler *handler = new(nothrow) DefaultSendCommandHandler;
	if (!handler)
		return B_NO_MEMORY;
	SetCommandHandler(MESSAGING_COMMAND_SEND_MESSAGE, handler);

	return B_OK;
}

// CreateDefault
status_t
MessagingService::CreateDefault()
{
	if (sService)
		return B_OK;

	// create the service
	MessagingService *service = new(nothrow) MessagingService;
	if (!service)
		return B_NO_MEMORY;

	// init it
	status_t error = service->Init();
	if (error != B_OK) {
		delete service;
		return error;
	}

	sService = service;
	return B_OK;
}

// DeleteDefault
void
MessagingService::DeleteDefault()
{
	if (sService) {
		delete sService;
		sService = NULL;
	}
}

// Default
MessagingService *
MessagingService::Default()
{
	return sService;
}

// SetCommandHandler
void
MessagingService::SetCommandHandler(uint32 command,
	MessagingCommandHandler *handler)
{
	BAutolock _(fLock);

	if (handler) {
		(*fCommandHandlers)[command] = handler;
	} else {
		// no handler: remove and existing entry
		CommandHandlerMap::iterator it = fCommandHandlers->find(command);
		if (it != fCommandHandlers->end())
			fCommandHandlers->erase(it);
	}
}

// _GetCommandHandler
MessagingCommandHandler *
MessagingService::_GetCommandHandler(uint32 command) const
{
	BAutolock _(fLock);

	CommandHandlerMap::iterator it = fCommandHandlers->find(command);
	return (it != fCommandHandlers->end() ? it->second : NULL);
}

// _CommandProcessorEntry
int32
MessagingService::_CommandProcessorEntry(void *data)
{
	return ((MessagingService*)data)->_CommandProcessor();
}

// _CommandProcessor
int32
MessagingService::_CommandProcessor()
{
	bool commandWaiting = false;
	while (!fTerminating) {
		// wait for the next command
		if (!commandWaiting) {
			status_t error = acquire_sem(fCounterSem);
			if (error != B_OK)
				continue;
		} else
			commandWaiting = false;

		// get it from the first area
		MessagingArea *area = fFirstArea;
		area->Lock();
		while (area->CountCommands() > 0) {
			const messaging_command *command = area->PopCommand();
			if (!command) {
				// something's seriously wrong
				ERROR("MessagingService::_CommandProcessor(): area %p (%ld) "
					"has command count %ld, but doesn't return any more "
					"commands.", area, area->ID(), area->CountCommands());
				break;
			}
PRINT("MessagingService::_CommandProcessor(): got command %lu\n",
command->command);

			// dispatch the command
			MessagingCommandHandler *handler
				= _GetCommandHandler(command->command);
			if (handler) {
				handler->HandleMessagingCommand(command->command, command->data,
					command->size - sizeof(messaging_command));
			} else {
				WARNING("MessagingService::_CommandProcessor(): No handler "
					"found for command %lu\n", command->command);
			}
		}

		// there is a new area we don't know yet
		if (!area->NextArea() && area->NextKernelAreaID() >= 0) {
			// create it
			MessagingArea *nextArea;
			status_t error = MessagingArea::Create(area->NextKernelAreaID(),
				fLockSem, fCounterSem, nextArea);
			if (error == B_OK) {
				area->SetNextArea(nextArea);
				commandWaiting = true;
			} else {
				// Bad, but what can we do?
				ERROR("MessagingService::_CommandProcessor(): Failed to clone "
					"kernel area %ld: %s\n", area->NextKernelAreaID(),
					strerror(error));
			}

		}

		// if the current area is empty and there is a next one, we discard the
		// current one
		if (area->NextArea() && area->CountCommands() == 0) {
			fFirstArea = area->NextArea();
			area->Discard();
			area->Unlock();
			delete area;
		} else {
			area->Unlock();
		}
	}

	return 0;
}

