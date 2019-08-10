/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


//! kernel-side implementation of the messaging service


#include <new>

#include <AutoDeleter.h>
#include <BytePointer.h>
#include <KernelExport.h>
#include <KMessage.h>
#include <messaging.h>
#include <MessagingServiceDefs.h>

#include "MessagingService.h"

//#define TRACE_MESSAGING_SERVICE
#ifdef TRACE_MESSAGING_SERVICE
#	define PRINT(x) dprintf x
#else
#	define PRINT(x) ;
#endif


using namespace std;

static MessagingService *sMessagingService = NULL;

static const int32 kMessagingAreaSize = B_PAGE_SIZE * 4;


// #pragma mark - MessagingArea


MessagingArea::MessagingArea()
{
}


MessagingArea::~MessagingArea()
{
	if (fID >= 0)
		delete_area(fID);
}


MessagingArea *
MessagingArea::Create(sem_id lockSem, sem_id counterSem)
{
	// allocate the object on the heap
	MessagingArea *area = new(nothrow) MessagingArea;
	if (!area)
		return NULL;

	// create the area
	area->fID = create_area("messaging", (void**)&area->fHeader,
		B_ANY_KERNEL_ADDRESS, kMessagingAreaSize, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (area->fID < 0) {
		delete area;
		return NULL;
	}

	// finish the initialization of the object
	area->fSize = kMessagingAreaSize;
	area->fLockSem = lockSem;
	area->fCounterSem = counterSem;
	area->fNextArea = NULL;
	area->InitHeader();

	return area;
}


void
MessagingArea::InitHeader()
{
	fHeader->lock_counter = 1;			// create locked
	fHeader->size = fSize;
	fHeader->kernel_area = fID;
	fHeader->next_kernel_area = (fNextArea ? fNextArea->ID() : -1);
	fHeader->command_count = 0;
	fHeader->first_command = 0;
	fHeader->last_command = 0;
}


bool
MessagingArea::CheckCommandSize(int32 dataSize)
{
	int32 size = sizeof(messaging_command) + dataSize;

	return (dataSize >= 0
		&& size <= kMessagingAreaSize - (int32)sizeof(messaging_area_header));
}


bool
MessagingArea::Lock()
{
	// benaphore-like locking
	if (atomic_add(&fHeader->lock_counter, 1) == 0)
		return true;

	return (acquire_sem(fLockSem) == B_OK);
}


void
MessagingArea::Unlock()
{
	if (atomic_add(&fHeader->lock_counter, -1) > 1)
		release_sem(fLockSem);
}


area_id
MessagingArea::ID() const
{
	return fID;
}


int32
MessagingArea::Size() const
{
	return fSize;
}


bool
MessagingArea::IsEmpty() const
{
	return fHeader->command_count == 0;
}


void *
MessagingArea::AllocateCommand(uint32 commandWhat, int32 dataSize,
	bool &wasEmpty)
{
	int32 size = sizeof(messaging_command) + dataSize;

	if (dataSize < 0 || size > fSize - (int32)sizeof(messaging_area_header))
		return NULL;

	// the area is used as a ring buffer
	int32 startOffset = sizeof(messaging_area_header);

	// the simple case first: the area is empty
	int32 commandOffset;
	wasEmpty = (fHeader->command_count == 0);
	if (wasEmpty) {
		commandOffset = startOffset;

		// update the header
		fHeader->command_count++;
		fHeader->first_command = fHeader->last_command = commandOffset;
	} else {
		int32 firstCommandOffset = fHeader->first_command;
		int32 lastCommandOffset = fHeader->last_command;
		int32 firstCommandSize;
		int32 lastCommandSize;
		messaging_command *firstCommand = _CheckCommand(firstCommandOffset,
			firstCommandSize);
		messaging_command *lastCommand = _CheckCommand(lastCommandOffset,
			lastCommandSize);
		if (!firstCommand || !lastCommand) {
			// something has been screwed up
			return NULL;
		}

		// find space for the command
		if (firstCommandOffset <= lastCommandOffset) {
			// not wrapped
			// try to allocate after the last command
			if (size <= fSize - (lastCommandOffset + lastCommandSize)) {
				commandOffset = (lastCommandOffset + lastCommandSize);
			} else {
				// is there enough space before the first command?
				if (size > firstCommandOffset - startOffset)
					return NULL;
				commandOffset = startOffset;
			}
		} else {
			// wrapped: we can only allocate between the last and the first
			// command
			commandOffset = lastCommandOffset + lastCommandSize;
			if (size > firstCommandOffset - commandOffset)
				return NULL;
		}

		// update the header and the last command
		fHeader->command_count++;
		lastCommand->next_command = fHeader->last_command = commandOffset;
	}

	// init the command
	BytePointer<messaging_command> command(fHeader);
	command += commandOffset;
	command->next_command = 0;
	command->command = commandWhat;
	command->size = size;

	return command->data;
}


void
MessagingArea::CommitCommand()
{
	// TODO: If invoked while locked, we should supply B_DO_NOT_RESCHEDULE.
	release_sem(fCounterSem);
}


void
MessagingArea::SetNextArea(MessagingArea *area)
{
	fNextArea = area;
	fHeader->next_kernel_area = (fNextArea ? fNextArea->ID() : -1);
}


MessagingArea *
MessagingArea::NextArea() const
{
	return fNextArea;
}


messaging_command *
MessagingArea::_CheckCommand(int32 offset, int32 &size)
{
	// check offset
	if (offset < (int32)sizeof(messaging_area_header)
		|| offset + (int32)sizeof(messaging_command) > fSize
		|| (offset & 0x3)) {
		return NULL;
	}

	// get and check size
	BytePointer<messaging_command> command(fHeader);
	command += offset;
	size = command->size;
	if (size < (int32)sizeof(messaging_command))
		return NULL;
	size = (size + 3) & ~0x3;	// align
	if (offset + size > fSize)
		return NULL;

	return &command;
}


// #pragma mark - MessagingService


MessagingService::MessagingService()
	:
	fFirstArea(NULL),
	fLastArea(NULL)
{
	recursive_lock_init(&fLock, "messaging service");
}


MessagingService::~MessagingService()
{
	// Should actually never be called. Once created the service stays till the
	// bitter end.
}


status_t
MessagingService::InitCheck() const
{
	return B_OK;
}


bool
MessagingService::Lock()
{
	return recursive_lock_lock(&fLock) == B_OK;
}


void
MessagingService::Unlock()
{
	recursive_lock_unlock(&fLock);
}


status_t
MessagingService::RegisterService(sem_id lockSem, sem_id counterSem,
	area_id &areaID)
{
	// check, if a service is already registered
	if (fFirstArea)
		return B_BAD_VALUE;

	status_t error = B_OK;

	// check, if the semaphores are valid and belong to the calling team
	thread_info threadInfo;
	error = get_thread_info(find_thread(NULL), &threadInfo);

	sem_info lockSemInfo;
	if (error == B_OK)
		error = get_sem_info(lockSem, &lockSemInfo);

	sem_info counterSemInfo;
	if (error == B_OK)
		error = get_sem_info(counterSem, &counterSemInfo);

	if (error != B_OK)
		return error;

	if (threadInfo.team != lockSemInfo.team
		|| threadInfo.team != counterSemInfo.team) {
		return B_BAD_VALUE;
	}

	// create an area
	fFirstArea = fLastArea = MessagingArea::Create(lockSem, counterSem);
	if (!fFirstArea)
		return B_NO_MEMORY;

	areaID = fFirstArea->ID();
	fFirstArea->Unlock();

	// store the server team and the semaphores
	fServerTeam = threadInfo.team;
	fLockSem = lockSem;
	fCounterSem = counterSem;

	return B_OK;
}


status_t
MessagingService::UnregisterService()
{
	// check, if the team calling this function is indeed the server team
	thread_info threadInfo;
	status_t error = get_thread_info(find_thread(NULL), &threadInfo);
	if (error != B_OK)
		return error;

	if (threadInfo.team != fServerTeam)
		return B_BAD_VALUE;

	// delete all areas
	while (fFirstArea) {
		MessagingArea *area = fFirstArea;
		fFirstArea = area->NextArea();
		delete area;
	}
	fLastArea = NULL;

	// unset the other members
	fLockSem = -1;
	fCounterSem = -1;
	fServerTeam = -1;

	return B_OK;
}


status_t
MessagingService::SendMessage(const void *message, int32 messageSize,
	const messaging_target *targets, int32 targetCount)
{
PRINT(("MessagingService::SendMessage(%p, %ld, %p, %ld)\n", message,
messageSize, targets, targetCount));
	if (!message || messageSize <= 0 || !targets || targetCount <= 0)
		return B_BAD_VALUE;

	int32 dataSize = sizeof(messaging_command_send_message)
		+ targetCount * sizeof(messaging_target) + messageSize;

	// allocate space for the command
	MessagingArea *area;
	void *data;
	bool wasEmpty;
	status_t error = _AllocateCommand(MESSAGING_COMMAND_SEND_MESSAGE, dataSize,
		area, data, wasEmpty);
	if (error != B_OK) {
		PRINT(("MessagingService::SendMessage(): Failed to allocate space for "
			"send message command.\n"));
		return error;
	}
PRINT(("  Allocated space for send message command: area: %p, data: %p, "
"wasEmpty: %d\n", area, data, wasEmpty));

	// prepare the command
	messaging_command_send_message *command
		= (messaging_command_send_message*)data;
	command->message_size = messageSize;
	command->target_count = targetCount;
	memcpy(command->targets, targets, sizeof(messaging_target) * targetCount);
	memcpy((char*)command + (dataSize - messageSize), message, messageSize);

	// shoot
	area->Unlock();
	if (wasEmpty)
		area->CommitCommand();

	return B_OK;
}


status_t
MessagingService::_AllocateCommand(int32 commandWhat, int32 size,
	MessagingArea *&area, void *&data, bool &wasEmpty)
{
	if (!fFirstArea)
		return B_NO_INIT;

	if (!MessagingArea::CheckCommandSize(size))
		return B_BAD_VALUE;

	// delete the discarded areas (save one)
	ObjectDeleter<MessagingArea> discardedAreaDeleter;
	MessagingArea *discardedArea = NULL;

	while (fFirstArea != fLastArea) {
		area = fFirstArea;
		area->Lock();
		if (!area->IsEmpty()) {
			area->Unlock();
			break;
		}

		PRINT(("MessagingService::_AllocateCommand(): Discarding area: %p\n",
			area));

		fFirstArea = area->NextArea();
		area->SetNextArea(NULL);
		discardedArea = area;
		discardedAreaDeleter.SetTo(area);
	}

	// allocate space for the command in the last area
	area = fLastArea;
	area->Lock();
	data = area->AllocateCommand(commandWhat, size, wasEmpty);

	if (!data) {
		// not enough space in the last area: create a new area or reuse a
		// discarded one
		if (discardedArea) {
			area = discardedAreaDeleter.Detach();
			area->InitHeader();
			PRINT(("MessagingService::_AllocateCommand(): Not enough space "
				"left in current area. Recycling discarded one: %p\n", area));
		} else {
			area = MessagingArea::Create(fLockSem, fCounterSem);
			PRINT(("MessagingService::_AllocateCommand(): Not enough space "
				"left in current area. Allocated new one: %p\n", area));
		}
		if (!area) {
			fLastArea->Unlock();
			return B_NO_MEMORY;
		}

		// add the new area
		fLastArea->SetNextArea(area);
		fLastArea->Unlock();
		fLastArea = area;

		// allocate space for the command
		data = area->AllocateCommand(commandWhat, size, wasEmpty);

		if (!data) {
			// that should never happen
			area->Unlock();
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


// #pragma mark - kernel private


status_t
send_message(const void *message, int32 messageSize,
	const messaging_target *targets, int32 targetCount)
{
	// check, if init_messaging_service() has been called yet
	if (!sMessagingService)
		return B_NO_INIT;

	if (!sMessagingService->Lock())
		return B_BAD_VALUE;

	status_t error = sMessagingService->SendMessage(message, messageSize,
		targets, targetCount);

	sMessagingService->Unlock();

	return error;
}


status_t
send_message(const KMessage *message, const messaging_target *targets,
	int32 targetCount)
{
	if (!message)
		return B_BAD_VALUE;

	return send_message(message->Buffer(), message->ContentSize(), targets,
		targetCount);
}


status_t
init_messaging_service()
{
	static char buffer[sizeof(MessagingService)];

	if (!sMessagingService)
		sMessagingService = new(buffer) MessagingService;

	status_t error = sMessagingService->InitCheck();

	// cleanup on error
	if (error != B_OK) {
		dprintf("ERROR: Failed to init messaging service: %s\n",
			strerror(error));
		sMessagingService->~MessagingService();
		sMessagingService = NULL;
	}

	return error;
}


// #pragma mark - syscalls


/** \brief Called by the userland server to register itself as a messaging
		   service for the kernel.
	\param lockingSem A semaphore used for locking the shared data. Semaphore
		   counter must be initialized to 0.
	\param counterSem A semaphore released every time the kernel pushes a
		   command into an empty area. Semaphore counter must be initialized
		   to 0.
	\return
	- The ID of the kernel area used for communication, if everything went fine,
	- an error code otherwise.
*/
area_id
_user_register_messaging_service(sem_id lockSem, sem_id counterSem)
{
	// check, if init_messaging_service() has been called yet
	if (!sMessagingService)
		return B_NO_INIT;

	if (!sMessagingService->Lock())
		return B_BAD_VALUE;

	area_id areaID = 0;
	status_t error = sMessagingService->RegisterService(lockSem, counterSem,
		areaID);

	sMessagingService->Unlock();

	return (error != B_OK ? error : areaID);
}


status_t
_user_unregister_messaging_service()
{
	// check, if init_messaging_service() has been called yet
	if (!sMessagingService)
		return B_NO_INIT;

	if (!sMessagingService->Lock())
		return B_BAD_VALUE;

	status_t error = sMessagingService->UnregisterService();

	sMessagingService->Unlock();

	return error;
}
