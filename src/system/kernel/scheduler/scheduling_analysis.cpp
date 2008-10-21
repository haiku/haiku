/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <scheduling_analysis.h>

#include <elf.h>
#include <kernel.h>
#include <scheduler_defs.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include "scheduler_tracing.h"


#if SCHEDULER_TRACING

namespace SchedulingAnalysis {

using namespace SchedulerTracing;

#if SCHEDULING_ANALYSIS_TRACING
using namespace SchedulingAnalysisTracing;
#endif

struct ThreadWaitObject;

struct HashObjectKey {
	virtual ~HashObjectKey()
	{
	}

	virtual uint32 HashKey() const = 0;
};


struct HashObject {
	HashObject*	next;

	virtual ~HashObject()
	{
	}

	virtual uint32 HashKey() const = 0;
	virtual bool Equals(const HashObjectKey* key) const = 0;
};


struct ThreadKey : HashObjectKey {
	thread_id	id;

	ThreadKey(thread_id id)
		:
		id(id)
	{
	}

	virtual uint32 HashKey() const
	{
		return id;
	}
};


struct Thread : HashObject, scheduling_analysis_thread {
	ScheduleState state;
	bigtime_t lastTime;

	ThreadWaitObject* waitObject;

	Thread(thread_id id)
		:
		state(UNKNOWN),
		lastTime(0),

		waitObject(NULL)
	{
		this->id = id;
		name[0] = '\0';

		runs = 0;
		total_run_time = 0;
		min_run_time = 1;
		max_run_time = -1;

		latencies = 0;
		total_latency = 0;
		min_latency = -1;
		max_latency = -1;

		reruns = 0;
		total_rerun_time = 0;
		min_rerun_time = -1;
		max_rerun_time = -1;

		unspecified_wait_time = 0;

		preemptions = 0;

		wait_objects = NULL;
	}

	virtual uint32 HashKey() const
	{
		return id;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const ThreadKey* key = dynamic_cast<const ThreadKey*>(_key);
		if (key == NULL)
			return false;
		return key->id == id;
	}
};


struct WaitObjectKey : HashObjectKey {
	uint32	type;
	void*	object;

	WaitObjectKey(uint32 type, void* object)
		:
		type(type),
		object(object)
	{
	}

	virtual uint32 HashKey() const
	{
		return type ^ (uint32)(addr_t)object;
	}
};


struct WaitObject : HashObject, scheduling_analysis_wait_object {
	WaitObject(uint32 type, void* object)
	{
		this->type = type;
		this->object = object;
		name[0] = '\0';
		referenced_object = NULL;
	}

	virtual uint32 HashKey() const
	{
		return type ^ (uint32)(addr_t)object;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const WaitObjectKey* key = dynamic_cast<const WaitObjectKey*>(_key);
		if (key == NULL)
			return false;
		return key->type == type && key->object == object;
	}
};


struct ThreadWaitObjectKey : HashObjectKey {
	thread_id				thread;
	uint32					type;
	void*					object;

	ThreadWaitObjectKey(thread_id thread, uint32 type, void* object)
		:
		thread(thread),
		type(type),
		object(object)
	{
	}

	virtual uint32 HashKey() const
	{
		return thread ^ type ^ (uint32)(addr_t)object;
	}
};


struct ThreadWaitObject : HashObject, scheduling_analysis_thread_wait_object {
	ThreadWaitObject(thread_id thread, WaitObject* waitObject)
	{
		this->thread = thread;
		wait_object = waitObject;
		wait_time = 0;
		waits = 0;
		next_in_list = NULL;
	}

	virtual uint32 HashKey() const
	{
		return thread ^ wait_object->type ^ (uint32)(addr_t)wait_object->object;
	}

	virtual bool Equals(const HashObjectKey* _key) const
	{
		const ThreadWaitObjectKey* key
			= dynamic_cast<const ThreadWaitObjectKey*>(_key);
		if (key == NULL)
			return false;
		return key->thread == thread && key->type == wait_object->type
			&& key->object == wait_object->object;
	}
};


class SchedulingAnalysisManager {
public:
	SchedulingAnalysisManager(void* buffer, size_t size)
		:
		fBuffer(buffer),
		fSize(size),
		fHashTable(),
		fHashTableSize(0)
	{
		fAnalysis.thread_count = 0;
		fAnalysis.threads = 0;
		fAnalysis.wait_object_count = 0;
		fAnalysis.thread_wait_object_count = 0;

		size_t maxObjectSize = max_c(max_c(sizeof(Thread), sizeof(WaitObject)),
			sizeof(ThreadWaitObject));
		fHashTableSize = size / (maxObjectSize + sizeof(HashObject*));
		fHashTable = (HashObject**)((uint8*)fBuffer + fSize) - fHashTableSize;
		fNextAllocation = (uint8*)fBuffer;
		fRemainingBytes = (addr_t)fHashTable - (addr_t)fBuffer;

		image_info info;
		if (elf_get_image_info_for_address((addr_t)&scheduler_init, &info)
				== B_OK) {
			fKernelStart = (addr_t)info.text;
			fKernelEnd = (addr_t)info.data + info.data_size;
		} else {
			fKernelStart = 0;
			fKernelEnd = 0;
		}
	}

	const scheduling_analysis* Analysis() const
	{
		return &fAnalysis;
	}

	void* Allocate(size_t size)
	{
		size = (size + 7) & ~(size_t)7;

		if (size > fRemainingBytes)
			return NULL;

		void* address = fNextAllocation;
		fNextAllocation += size;
		fRemainingBytes -= size;
		return address;
	}

	void Insert(HashObject* object)
	{
		uint32 index = object->HashKey() % fHashTableSize;
		object->next = fHashTable[index];
		fHashTable[index] = object;
	}

	void Remove(HashObject* object)
	{
		uint32 index = object->HashKey() % fHashTableSize;
		HashObject** slot = &fHashTable[index];
		while (*slot != object)
			slot = &(*slot)->next;

		*slot = object->next;
	}

	HashObject* Lookup(const HashObjectKey& key) const
	{
		uint32 index = key.HashKey() % fHashTableSize;
		HashObject* object = fHashTable[index];
		while (object != NULL && !object->Equals(&key))
			object = object->next;
		return object;
	}

	Thread* ThreadFor(thread_id id) const
	{
		return dynamic_cast<Thread*>(Lookup(ThreadKey(id)));
	}

	WaitObject* WaitObjectFor(uint32 type, void* object) const
	{
		return dynamic_cast<WaitObject*>(Lookup(WaitObjectKey(type, object)));
	}

	ThreadWaitObject* ThreadWaitObjectFor(thread_id thread, uint32 type,
		void* object) const
	{
		return dynamic_cast<ThreadWaitObject*>(
			Lookup(ThreadWaitObjectKey(thread, type, object)));
	}

	status_t AddThread(thread_id id, const char* name)
	{
		Thread* thread = ThreadFor(id);
		if (thread == NULL) {
			void* memory = Allocate(sizeof(Thread));
			if (memory == NULL)
				return B_NO_MEMORY;

			thread = new(memory) Thread(id);
			Insert(thread);
			fAnalysis.thread_count++;
		}

		if (name != NULL && thread->name[0] == '\0')
			strlcpy(thread->name, name, sizeof(thread->name));

		return B_OK;
	}

	status_t AddWaitObject(uint32 type, void* object,
		WaitObject** _waitObject = NULL)
	{
		if (WaitObjectFor(type, object) != NULL)
			return B_OK;

		void* memory = Allocate(sizeof(WaitObject));
		if (memory == NULL)
			return B_NO_MEMORY;

		WaitObject* waitObject = new(memory) WaitObject(type, object);
		Insert(waitObject);
		fAnalysis.wait_object_count++;

		// Set a dummy name for snooze() and waiting for signals, so we don't
		// try to update them later on.
		if (type == THREAD_BLOCK_TYPE_SNOOZE
			|| type == THREAD_BLOCK_TYPE_SIGNAL) {
			strcpy(waitObject->name, "?");
		}

		if (_waitObject != NULL)
			*_waitObject = waitObject;

		return B_OK;
	}

	status_t UpdateWaitObject(uint32 type, void* object, const char* name,
		void* referencedObject)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL)
			return B_OK;

		if (waitObject->name[0] != '\0') {
			// This is a new object at the same address. Replace the old one.
			Remove(waitObject);
			status_t error = AddWaitObject(type, object, &waitObject);
			if (error != B_OK)
				return error;
		}

		if (name == NULL)
			name = "?";

		strlcpy(waitObject->name, name, sizeof(waitObject->name));
		waitObject->referenced_object = referencedObject;

		return B_OK;
	}

	bool UpdateWaitObjectDontAdd(uint32 type, void* object, const char* name,
		void* referencedObject)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL || waitObject->name[0] != '\0')
			return false;

		if (name == NULL)
			name = "?";

		strlcpy(waitObject->name, name, sizeof(waitObject->name));
		waitObject->referenced_object = referencedObject;

		return B_OK;
	}

	status_t AddThreadWaitObject(Thread* thread, uint32 type, void* object)
	{
		WaitObject* waitObject = WaitObjectFor(type, object);
		if (waitObject == NULL) {
			// The algorithm should prevent this case.
			return B_ERROR;
		}

		ThreadWaitObject* threadWaitObject = ThreadWaitObjectFor(thread->id,
			type, object);
		if (threadWaitObject == NULL
			|| threadWaitObject->wait_object != waitObject) {
			if (threadWaitObject != NULL)
				Remove(threadWaitObject);

			void* memory = Allocate(sizeof(ThreadWaitObject));
			if (memory == NULL)
				return B_NO_MEMORY;

			threadWaitObject = new(memory) ThreadWaitObject(thread->id,
				waitObject);
			Insert(threadWaitObject);
			fAnalysis.thread_wait_object_count++;

			threadWaitObject->next_in_list = thread->wait_objects;
			thread->wait_objects = threadWaitObject;
		}

		thread->waitObject = threadWaitObject;

		return B_OK;
	}

	int32 MissingWaitObjects() const
	{
		// Iterate through the hash table and count the wait objects that don't
		// have a name yet.
		int32 count = 0;
		for (uint32 i = 0; i < fHashTableSize; i++) {
			HashObject* object = fHashTable[i];
			while (object != NULL) {
				WaitObject* waitObject = dynamic_cast<WaitObject*>(object);
				if (waitObject != NULL && waitObject->name[0] == '\0')
					count++;

				object = object->next;
			}
		}

		return count;
	}

	status_t FinishAnalysis()
	{
		// allocate the thread array
		scheduling_analysis_thread** threads
			= (scheduling_analysis_thread**)Allocate(
				sizeof(Thread*) * fAnalysis.thread_count);
		if (threads == NULL)
			return B_NO_MEMORY;

		// Iterate through the hash table and collect all threads. Also polish
		// all wait objects that haven't been update yet.
		int32 index = 0;
		for (uint32 i = 0; i < fHashTableSize; i++) {
			HashObject* object = fHashTable[i];
			while (object != NULL) {
				Thread* thread = dynamic_cast<Thread*>(object);
				if (thread != NULL) {
					threads[index++] = thread;
				} else if (WaitObject* waitObject
						= dynamic_cast<WaitObject*>(object)) {
					_PolishWaitObject(waitObject);
				}

				object = object->next;
			}
		}

		fAnalysis.threads = threads;
dprintf("scheduling analysis: free bytes: %lu/%lu\n", fRemainingBytes, fSize);
		return B_OK;
	}

private:
	void _PolishWaitObject(WaitObject* waitObject)
	{
		if (waitObject->name[0] != '\0')
			return;

		switch (waitObject->type) {
			case THREAD_BLOCK_TYPE_SEMAPHORE:
			{
				sem_info info;
				if (get_sem_info((sem_id)(addr_t)waitObject->object, &info)
						== B_OK) {
					strlcpy(waitObject->name, info.name,
						sizeof(waitObject->name));
				}
				break;
			}
			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			{
				// If the condition variable object is in the kernel image,
				// assume, it is still initialized.
				ConditionVariable* variable
					= (ConditionVariable*)waitObject->object;
				if (!_IsInKernelImage(variable))
					break;

				waitObject->referenced_object = (void*)variable->Object();
				strlcpy(waitObject->name, variable->ObjectType(),
					sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_MUTEX:
			{
				// If the mutex object is in the kernel image, assume, it is
				// still initialized.
				mutex* lock = (mutex*)waitObject->object;
				if (!_IsInKernelImage(lock))
					break;

				strlcpy(waitObject->name, lock->name, sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_RW_LOCK:
			{
				// If the mutex object is in the kernel image, assume, it is
				// still initialized.
				rw_lock* lock = (rw_lock*)waitObject->object;
				if (!_IsInKernelImage(lock))
					break;

				strlcpy(waitObject->name, lock->name, sizeof(waitObject->name));
				break;
			}

			case THREAD_BLOCK_TYPE_OTHER:
			{
				const char* name = (const char*)waitObject->object;
				if (name == NULL || _IsInKernelImage(name))
					return;

				strlcpy(waitObject->name, name, sizeof(waitObject->name));
			}

			case THREAD_BLOCK_TYPE_SNOOZE:
			case THREAD_BLOCK_TYPE_SIGNAL:
			default:
				break;
		}

		if (waitObject->name[0] != '\0')
			return;

		strcpy(waitObject->name, "?");
	}

	bool _IsInKernelImage(const void* _address)
	{
		addr_t address = (addr_t)_address;
		return address >= fKernelStart && address < fKernelEnd;
	}

private:
	scheduling_analysis	fAnalysis;
	void*				fBuffer;
	size_t				fSize;
	HashObject**		fHashTable;
	uint32				fHashTableSize;
	uint8*				fNextAllocation;
	size_t				fRemainingBytes;
	addr_t				fKernelStart;
	addr_t				fKernelEnd;
};


static status_t
analyze_scheduling(bigtime_t from, bigtime_t until,
	SchedulingAnalysisManager& manager)
{
	// analyze how much threads and locking primitives we're talking about
	TraceEntryIterator iterator;
	iterator.MoveTo(INT_MAX);
	while (TraceEntry* _entry = iterator.Previous()) {
		SchedulerTraceEntry* baseEntry
			= dynamic_cast<SchedulerTraceEntry*>(_entry);
		if (baseEntry == NULL || baseEntry->Time() >= until)
			continue;
		if (baseEntry->Time() < from)
			break;

		status_t error = manager.AddThread(baseEntry->ThreadID(),
			baseEntry->Name());
		if (error != B_OK)
			return error;

		if (ScheduleThread* entry = dynamic_cast<ScheduleThread*>(_entry)) {
			error = manager.AddThread(entry->PreviousThreadID(), NULL);
			if (error != B_OK)
				return error;

			if (entry->PreviousState() == B_THREAD_WAITING) {
				void* waitObject = (void*)entry->PreviousWaitObject();
				switch (entry->PreviousWaitObjectType()) {
					case THREAD_BLOCK_TYPE_SNOOZE:
					case THREAD_BLOCK_TYPE_SIGNAL:
						waitObject = NULL;
						break;
					case THREAD_BLOCK_TYPE_SEMAPHORE:
					case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
					case THREAD_BLOCK_TYPE_MUTEX:
					case THREAD_BLOCK_TYPE_RW_LOCK:
					case THREAD_BLOCK_TYPE_OTHER:
					default:
						break;
				}

				error = manager.AddWaitObject(entry->PreviousWaitObjectType(),
					waitObject);
				if (error != B_OK)
					return error;
			}
		}
	}

#if SCHEDULING_ANALYSIS_TRACING
	int32 startEntryIndex = iterator.Index();
#endif

	while (TraceEntry* _entry = iterator.Next()) {
#if SCHEDULING_ANALYSIS_TRACING
		// might be info on a wait object
		if (WaitObjectTraceEntry* waitObjectEntry
				= dynamic_cast<WaitObjectTraceEntry*>(_entry)) {
			status_t error = manager.UpdateWaitObject(waitObjectEntry->Type(),
				waitObjectEntry->Object(), waitObjectEntry->Name(),
				waitObjectEntry->ReferencedObject());
			if (error != B_OK)
				return error;
			continue;
		}
#endif

		SchedulerTraceEntry* baseEntry
			= dynamic_cast<SchedulerTraceEntry*>(_entry);
		if (baseEntry == NULL)
			continue;
		if (baseEntry->Time() >= until)
			break;

		if (ScheduleThread* entry = dynamic_cast<ScheduleThread*>(_entry)) {
			// scheduled thread
			Thread* thread = manager.ThreadFor(entry->ThreadID());

			bigtime_t diffTime = entry->Time() - thread->lastTime;

			if (thread->state == READY) {
				// thread scheduled after having been woken up
				thread->latencies++;
				thread->total_latency += diffTime;
				if (thread->min_latency < 0 || diffTime < thread->min_latency)
					thread->min_latency = diffTime;
				if (diffTime > thread->max_latency)
					thread->max_latency = diffTime;
			} else if (thread->state == PREEMPTED) {
				// thread scheduled after having been preempted before
				thread->reruns++;
				thread->total_rerun_time += diffTime;
				if (thread->min_rerun_time < 0
						|| diffTime < thread->min_rerun_time) {
					thread->min_rerun_time = diffTime;
				}
				if (diffTime > thread->max_rerun_time)
					thread->max_rerun_time = diffTime;
			}

			if (thread->state == STILL_RUNNING) {
				// Thread was running and continues to run.
				thread->state = RUNNING;
			}

			if (thread->state != RUNNING) {
				thread->lastTime = entry->Time();
				thread->state = RUNNING;
			}

			// unscheduled thread

			if (entry->ThreadID() == entry->PreviousThreadID())
				continue;

			thread = manager.ThreadFor(entry->PreviousThreadID());

			diffTime = entry->Time() - thread->lastTime;

			if (thread->state == STILL_RUNNING) {
				// thread preempted
				thread->runs++;
				thread->preemptions++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;

				thread->lastTime = entry->Time();
				thread->state = PREEMPTED;
			} else if (thread->state == RUNNING) {
				// thread starts waiting (it hadn't been added to the run
				// queue before being unscheduled)
				thread->runs++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;

				if (entry->PreviousState() == B_THREAD_WAITING) {
					void* waitObject = (void*)entry->PreviousWaitObject();
					switch (entry->PreviousWaitObjectType()) {
						case THREAD_BLOCK_TYPE_SNOOZE:
						case THREAD_BLOCK_TYPE_SIGNAL:
							waitObject = NULL;
							break;
						case THREAD_BLOCK_TYPE_SEMAPHORE:
						case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
						case THREAD_BLOCK_TYPE_MUTEX:
						case THREAD_BLOCK_TYPE_RW_LOCK:
						case THREAD_BLOCK_TYPE_OTHER:
						default:
							break;
					}

					status_t error = manager.AddThreadWaitObject(thread,
						entry->PreviousWaitObjectType(), waitObject);
					if (error != B_OK)
						return error;
				}

				thread->lastTime = entry->Time();
				thread->state = WAITING;
			} else if (thread->state == UNKNOWN) {
				uint32 threadState = entry->PreviousState();
				if (threadState == B_THREAD_WAITING
					|| threadState == B_THREAD_SUSPENDED) {
					thread->lastTime = entry->Time();
					thread->state = WAITING;
				} else if (threadState == B_THREAD_READY) {
					thread->lastTime = entry->Time();
					thread->state = PREEMPTED;
				}
			}
		} else if (EnqueueThread* entry
				= dynamic_cast<EnqueueThread*>(_entry)) {
			// thread enqueued in run queue

			Thread* thread = manager.ThreadFor(entry->ThreadID());

			if (thread->state == RUNNING || thread->state == STILL_RUNNING) {
				// Thread was running and is reentered into the run queue. This
				// is done by the scheduler, if the thread remains ready.
				thread->state = STILL_RUNNING;
			} else {
				// Thread was waiting and is ready now.
				bigtime_t diffTime = entry->Time() - thread->lastTime;
				if (thread->waitObject != NULL) {
					thread->waitObject->wait_time += diffTime;
					thread->waitObject->waits++;
					thread->waitObject = NULL;
				} else if (thread->state != UNKNOWN)
					thread->unspecified_wait_time += diffTime;

				thread->lastTime = entry->Time();
				thread->state = READY;
			}
		} else if (RemoveThread* entry = dynamic_cast<RemoveThread*>(_entry)) {
			// thread removed from run queue

			Thread* thread = manager.ThreadFor(entry->ThreadID());

			// This really only happens when the thread priority is changed
			// while the thread is ready.

			bigtime_t diffTime = entry->Time() - thread->lastTime;
			if (thread->state == RUNNING) {
				// This should never happen.
				thread->runs++;
				thread->total_run_time += diffTime;
				if (thread->min_run_time < 0 || diffTime < thread->min_run_time)
					thread->min_run_time = diffTime;
				if (diffTime > thread->max_run_time)
					thread->max_run_time = diffTime;
			} else if (thread->state == READY || thread->state == PREEMPTED) {
				// Not really correct, but the case is rare and we keep it
				// simple.
				thread->unspecified_wait_time += diffTime;
			}

			thread->lastTime = entry->Time();
			thread->state = WAITING;
		}
	}


#if SCHEDULING_ANALYSIS_TRACING
	int32 missingWaitObjects = manager.MissingWaitObjects();
	if (missingWaitObjects > 0) {
		iterator.MoveTo(startEntryIndex + 1);
		while (TraceEntry* _entry = iterator.Previous()) {
			if (WaitObjectTraceEntry* waitObjectEntry
					= dynamic_cast<WaitObjectTraceEntry*>(_entry)) {
				if (manager.UpdateWaitObjectDontAdd(
						waitObjectEntry->Type(), waitObjectEntry->Object(),
						waitObjectEntry->Name(),
						waitObjectEntry->ReferencedObject())) {
					if (--missingWaitObjects == 0)
						break;
				}
			}
		}
	}
#endif

	return B_OK;
}

}	// namespace SchedulingAnalysis

#endif	// SCHEDULER_TRACING


status_t
_user_analyze_scheduling(bigtime_t from, bigtime_t until, void* buffer,
	size_t size, scheduling_analysis* analysis)
{
#if SCHEDULER_TRACING
	using namespace SchedulingAnalysis;

	if ((addr_t)buffer & 0x7) {
		addr_t diff = (addr_t)buffer & 0x7;
		buffer = (void*)((addr_t)buffer + 8 - diff);
		size -= 8 - diff;
	}
	size &= ~(size_t)0x7;

	if (buffer == NULL || !IS_USER_ADDRESS(buffer) || size == 0)
		return B_BAD_VALUE;

	status_t error = lock_memory(buffer, size, B_READ_DEVICE);
	if (error != B_OK)
		return error;

	SchedulingAnalysisManager manager(buffer, size);

	InterruptsLocker locker;
	lock_tracing_buffer();

	error = analyze_scheduling(from, until, manager);

	unlock_tracing_buffer();
	locker.Unlock();

	if (error == B_OK)
		error = manager.FinishAnalysis();

	unlock_memory(buffer, size, B_READ_DEVICE);

	if (error == B_OK) {
		error = user_memcpy(analysis, manager.Analysis(),
			sizeof(scheduling_analysis));
	}

	return error;
#else
	return B_BAD_VALUE;
#endif
}
