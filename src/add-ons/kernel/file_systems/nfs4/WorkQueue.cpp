/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "WorkQueue.h"

#include <io_requests.h>


#define	MAX_BUFFER_SIZE			(1024 * 1024)

WorkQueue*		gWorkQueue		= NULL;


WorkQueue::WorkQueue()
	:
	fQueueSemaphore(create_sem(0, NULL)),
	fThreadCancel(create_sem(0, NULL))
{
	mutex_init(&fQueueLock, NULL);

	fThread = spawn_kernel_thread(&WorkQueue::LaunchWorkingThread,
		"NFSv4 Work Queue", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK) {
		fInitError = fThread;
		return;
	}

	status_t result = resume_thread(fThread);
	if (result != B_OK) {
		kill_thread(fThread);
		fInitError = result;
		return;
	}

	fInitError = B_OK;
}


WorkQueue::~WorkQueue()
{
	release_sem(fThreadCancel);

	status_t result;
	wait_for_thread(fThread, &result);

	mutex_destroy(&fQueueLock);
	delete_sem(fThreadCancel);
	delete_sem(fQueueSemaphore);
}


status_t
WorkQueue::EnqueueJob(JobType type, void* args)
{
	WorkQueueEntry* entry = new(std::nothrow) WorkQueueEntry;
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->fType = type;
	entry->fArguments = args;
	if (type == IORequest)
		reinterpret_cast<IORequestArgs*>(args)->fInode->BeginAIOOp();

	MutexLocker locker(fQueueLock);
	fQueue.InsertAfter(fQueue.Tail(), entry);
	locker.Unlock();

	release_sem(fQueueSemaphore);
	return B_OK;
}


status_t
WorkQueue::LaunchWorkingThread(void* object)
{
	ASSERT(object != NULL);

	WorkQueue* queue = reinterpret_cast<WorkQueue*>(object);
	return queue->WorkingThread();
}


status_t
WorkQueue::WorkingThread()
{
	while (true) {
		object_wait_info object[2];
		object[0].object = fThreadCancel;
		object[0].type = B_OBJECT_TYPE_SEMAPHORE;
		object[0].events = B_EVENT_ACQUIRE_SEMAPHORE;

		object[1].object = fQueueSemaphore;
		object[1].type = B_OBJECT_TYPE_SEMAPHORE;
		object[1].events = B_EVENT_ACQUIRE_SEMAPHORE;

		status_t result = wait_for_objects(object, 2);

		if (result < B_OK
			|| (object[0].events & B_EVENT_ACQUIRE_SEMAPHORE) != 0) {
			return result;
		} else if ((object[1].events & B_EVENT_ACQUIRE_SEMAPHORE) == 0)
			continue;

		acquire_sem(fQueueSemaphore);

		DequeueJob();
	}

	return B_OK;
}


void
WorkQueue::DequeueJob()
{
	MutexLocker locker(fQueueLock);
	WorkQueueEntry* entry = fQueue.RemoveHead();
	locker.Unlock();
	ASSERT(entry != NULL);

	void* args = entry->fArguments;
	switch (entry->fType) {
		case DelegationRecall:
			JobRecall(reinterpret_cast<DelegationRecallArgs*>(args));
			break;
		case IORequest:
			JobIO(reinterpret_cast<IORequestArgs*>(args));
			break;
	}

	delete entry;
}


void
WorkQueue::JobRecall(DelegationRecallArgs* args)
{
	ASSERT(args != NULL);
	args->fDelegation->GetInode()->RecallDelegation(args->fTruncate);
}


void
WorkQueue::JobIO(IORequestArgs* args)
{
	ASSERT(args != NULL);

	uint64 offset = io_request_offset(args->fRequest);
	uint64 length = io_request_length(args->fRequest);

	size_t bufferLength = min_c(MAX_BUFFER_SIZE, length);
	char* buffer = reinterpret_cast<char*>(malloc(bufferLength));
	if (buffer == NULL) {
		notify_io_request(args->fRequest, B_NO_MEMORY);
		args->fInode->EndAIOOp();
		return;
	}

	status_t result;
	if (io_request_is_write(args->fRequest)) {
		if (offset + length > args->fInode->MaxFileSize())
				length = args->fInode->MaxFileSize() - offset;

		uint64 position = 0;
		do {
			size_t size = 0;
			size_t thisBufferLength = min_c(bufferLength, length - position);

			result = read_from_io_request(args->fRequest, buffer,
				thisBufferLength);

			while (size < thisBufferLength && result == B_OK) {
				size_t bytesWritten = thisBufferLength - size;
				result = args->fInode->WriteDirect(NULL,
					offset + position + size, buffer + size, &bytesWritten);
				size += bytesWritten;
			}

			position += thisBufferLength;
		} while (position < length && result == B_OK);
	} else {
		bool eof = false;
		uint64 position = 0;
		do {
			size_t size = 0;
			size_t thisBufferLength = min_c(bufferLength, length - position);

			do {
				size_t bytesRead = thisBufferLength - size;
				result = args->fInode->ReadDirect(NULL,
					offset + position + size, buffer + size, &bytesRead, &eof);
				if (result != B_OK)
					break;

				result = write_to_io_request(args->fRequest, buffer + size,
					bytesRead);
				if (result != B_OK)
					break;

				size += bytesRead;
			} while (size < length && result == B_OK && !eof);

			position += thisBufferLength;
		} while (position < length && result == B_OK && !eof);
	}

	free(buffer);

	notify_io_request(args->fRequest, result);
	args->fInode->EndAIOOp();
}

