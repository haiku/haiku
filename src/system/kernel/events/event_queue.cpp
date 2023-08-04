/*
 * Copyright 2015, Hamish Morrison, hamishm53@gmail.com.
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <event_queue.h>

#include <OS.h>

#include <AutoDeleter.h>

#include <fs/fd.h>
#include <port.h>
#include <sem.h>
#include <syscalls.h>
#include <syscall_restart.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <util/AVLTree.h>
#include <util/DoublyLinkedList.h>
#include <AutoDeleterDrivers.h>
#include <StackOrHeapArray.h>
#include <wait_for_objects.h>

#include "select_ops.h"
#include "select_sync.h"


enum {
	B_EVENT_QUEUED			= (1 << 28),
	B_EVENT_SELECTING		= (1 << 29),
	B_EVENT_DELETING		= (1 << 30),
	/* (signed) */
	B_EVENT_PRIVATE_MASK	= (0xf0000000)
};


#define EVENT_BEHAVIOR(events) ((events) & (B_EVENT_LEVEL_TRIGGERED | B_EVENT_ONE_SHOT))
#define USER_EVENTS(events) ((events) & ~B_EVENT_PRIVATE_MASK)

#define B_EVENT_NON_MASKABLE (B_EVENT_INVALID | B_EVENT_ERROR | B_EVENT_DISCONNECTED)



struct select_event : select_info, AVLTreeNode,
		DoublyLinkedListLinkImpl<select_event> {
	int32				object;
	uint16				type;
	uint32				behavior;
	void*				user_data;
};


struct EventQueueTreeDefinition {
	typedef struct {
		int32 object;
		uint16 type;
	} 						Key;
	typedef select_event	Value;

	AVLTreeNode* GetAVLTreeNode(Value* value) const
	{
		return value;
	}

	Value* GetValue(AVLTreeNode* node) const
	{
		return static_cast<Value*>(node);
	}

	int Compare(Key a, const Value* b) const
	{
		if (a.object != b->object)
			return a.object - b->object;
		else
			return a.type - b->type;
	}

	int Compare(const Value* a, const Value* b) const
	{
		if (a->object != b->object)
			return a->object - b->object;
		else
			return a->type - b->type;
	}
};


//	#pragma mark -- EventQueue implementation


class EventQueue : public select_sync {
public:
						EventQueue(bool kernel);
						~EventQueue();

	void				Closed();

	status_t			Select(int32 object, uint16 type, uint32 events, void* userData);
	status_t			Query(int32 object, uint16 type, uint32* selectedEvents, void** userData);
	status_t			Deselect(int32 object, uint16 type);

	status_t			Notify(select_info* info, uint16 events);

	ssize_t				Wait(event_wait_info* infos, int numInfos,
							int32 flags, bigtime_t timeout);

private:
	void				_Notify(select_event* event, uint16 events);
	status_t			_DeselectEvent(select_event* event);

	ssize_t				_DequeueEvents(event_wait_info* infos, int numInfos);

	select_event*		_GetEvent(int32 object, uint16 type);

private:
	typedef AVLTree<EventQueueTreeDefinition> EventTree;
	typedef DoublyLinkedList<select_event> EventList;

	bool				fKernel;
	bool				fClosing;

	/*
	 * This flag is set in _DequeueEvents when we have to drop the lock to
	 * deselect an object to prevent another _DequeueEvents call concurrently
	 * modifying the list.
	 */
	bool				fDequeueing;

	EventList			fEventList;
	EventTree			fEventTree;

	/*
	 * Protects the queue. We cannot call select or deselect while holding
	 * this, because it will invert the locking order with EventQueue::Notify.
	 */
	mutex				fQueueLock;

	/*
	 * Notified when events are available on the queue.
	 */
	ConditionVariable	fQueueCondition;

	/*
	 * Used to wait on a changing select_event while the queue lock is dropped
	 * during a call to select/deselect.
	 */
	ConditionVariable	fEventCondition;
};


EventQueue::EventQueue(bool kernel)
	:
	fKernel(kernel),
	fClosing(false),
	fDequeueing(false)
{
	mutex_init(&fQueueLock, "event_queue lock");
	fQueueCondition.Init(this, "evtq wait");
	fEventCondition.Init(this, "event_queue event change wait");
}


EventQueue::~EventQueue()
{
	mutex_lock(&fQueueLock);
	ASSERT(fClosing && !fDequeueing);

	EventTree::Iterator iter = fEventTree.GetIterator();
	while (iter.HasNext()) {
		select_event* event = iter.Next();
		event->events |= B_EVENT_DELETING;

		mutex_unlock(&fQueueLock);
		_DeselectEvent(event);
		mutex_lock(&fQueueLock);

		iter.Remove();
		if ((event->events & B_EVENT_QUEUED) != 0)
			fEventList.Remove(event);
		delete event;
	}

	EventList::Iterator listIter = fEventList.GetIterator();
	while (listIter.HasNext()) {
		select_event* event = listIter.Next();

		// We already removed all events in the tree from this list.
		// The only remaining events will be INVALID ones already deselected.
		delete event;
	}

	mutex_destroy(&fQueueLock);
}


void
EventQueue::Closed()
{
	MutexLocker locker(&fQueueLock);

	fClosing = true;
	locker.Unlock();

	// Wake up all waiters
	fQueueCondition.NotifyAll(B_FILE_ERROR);
}


status_t
EventQueue::Select(int32 object, uint16 type, uint32 events, void* userData)
{
	MutexLocker locker(&fQueueLock);

	select_event* event = _GetEvent(object, type);
	if (event != NULL) {
		if ((event->selected_events | event->behavior)
				== (USER_EVENTS(events) | B_EVENT_NON_MASKABLE))
			return B_OK;

		// Rather than try to reuse the event object, which would be complicated
		// and error-prone, perform a full de-selection and then re-selection.
		locker.Unlock();
		status_t status = Deselect(object, type);
		if (status != B_OK)
			return status;
		locker.Lock();

		// Make sure nothing else re-selected before we reacquired the lock.
		event = _GetEvent(object, type);
		if (event != NULL)
			return EEXIST;
	}

	event = new(std::nothrow) select_event;
	if (event == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<select_event> eventDeleter(event);

	event->sync = this;
	event->object = object;
	event->type = type;
	event->behavior = EVENT_BEHAVIOR(events);
	event->user_data = userData;
	event->events = 0;

	status_t result = fEventTree.Insert(event);
	if (result != B_OK)
		return result;

	// We drop the lock before calling select() to avoid inverting the
	// locking order with Notify(). Setting the B_EVENT_SELECTING flag prevents
	// the event from being used or even deleted before it is ready.
	event->events |= B_EVENT_SELECTING;
	event->selected_events = USER_EVENTS(events) | B_EVENT_NON_MASKABLE;

	locker.Unlock();

	status_t status = select_object(event->type, event->object, event, fKernel);
	if (status < 0) {
		locker.Lock();
		fEventTree.Remove(event);
		fEventCondition.NotifyAll();
		return status;
	}

	eventDeleter.Detach();

	atomic_and(&event->events, ~B_EVENT_SELECTING);
	fEventCondition.NotifyAll();

	return B_OK;
}


status_t
EventQueue::Query(int32 object, uint16 type, uint32* selectedEvents, void** userData)
{
	MutexLocker locker(&fQueueLock);

	select_event* event = _GetEvent(object, type);
	if (event == NULL)
		return B_ENTRY_NOT_FOUND;

	*selectedEvents = event->selected_events | event->behavior;
	*userData = event->user_data;

	return B_OK;
}


status_t
EventQueue::Deselect(int32 object, uint16 type)
{
	MutexLocker locker(&fQueueLock);

	select_event* event = _GetEvent(object, type);
	if (event == NULL)
		return B_ENTRY_NOT_FOUND;

	if ((atomic_or(&event->events, B_EVENT_DELETING) & B_EVENT_DELETING) != 0)
		return B_OK;

	locker.Unlock();
	_DeselectEvent(event);
	locker.Lock();

	if ((event->events & B_EVENT_INVALID) == 0)
		fEventTree.Remove(event);
	if ((event->events & B_EVENT_QUEUED) != 0)
		fEventList.Remove(event);

	delete event;

	locker.Unlock();
	fEventCondition.NotifyAll();
	return B_OK;
}


status_t
EventQueue::_DeselectEvent(select_event* event)
{
	return deselect_object(event->type, event->object, event, fKernel);
}


status_t
EventQueue::Notify(select_info* info, uint16 events)
{
	select_event* event = static_cast<select_event*>(info);
	_Notify(event, events);
	return B_OK;
}


void
EventQueue::_Notify(select_event* event, uint16 events)
{
	if ((events & event->selected_events) == 0)
		return;

	const int32 previousEvents = atomic_or(&event->events, (events & ~B_EVENT_INVALID));

	// If the event is already being deleted, we should ignore this notification.
	if ((previousEvents & B_EVENT_DELETING) != 0)
		return;

	// If the event is already queued, and it is not becoming invalid,
	// we don't need to do anything more.
	if ((previousEvents & B_EVENT_QUEUED) != 0 && (events & B_EVENT_INVALID) == 0)
		return;

	{
		MutexLocker _(&fQueueLock);

		// We need to recheck B_EVENT_DELETING now we have the lock.
		if ((event->events & B_EVENT_DELETING) != 0)
			return;

		// If we get B_EVENT_INVALID it means the object we were monitoring was
		// deleted. The object's ID may now be reused, so we must remove it
		// from the event tree.
		if ((events & B_EVENT_INVALID) != 0) {
			atomic_or(&event->events, B_EVENT_INVALID);
			fEventTree.Remove(event);
		}

		// If it's not already queued, it's our responsibility to queue it.
		if ((atomic_or(&event->events, B_EVENT_QUEUED) & B_EVENT_QUEUED) == 0) {
			fEventList.Add(event);
			fQueueCondition.NotifyAll();
		}
	}
}


ssize_t
EventQueue::Wait(event_wait_info* infos, int numInfos,
	int32 flags, bigtime_t timeout)
{
	ASSERT((flags & B_ABSOLUTE_TIMEOUT) != 0
		|| (timeout == B_INFINITE_TIMEOUT || timeout == 0));

	MutexLocker queueLocker(&fQueueLock);

	ssize_t count = 0;
	while (timeout == 0 || (system_time() < timeout)) {
		while ((fDequeueing || fEventList.IsEmpty()) && !fClosing) {
			status_t status = fQueueCondition.Wait(queueLocker.Get(),
				flags | B_CAN_INTERRUPT, timeout);
			if (status != B_OK)
				return status;
		}

		if (fClosing)
			return B_FILE_ERROR;

		if (numInfos == 0)
			return B_OK;

		fDequeueing = true;
		count = _DequeueEvents(infos, numInfos);
		fDequeueing = false;

		if (count != 0)
			break;

		// Due to level-triggered events, it is possible for the event list to have
		// been not empty and _DequeueEvents() still returns nothing. Hence, we loop.
	}

	return count;
}


ssize_t
EventQueue::_DequeueEvents(event_wait_info* infos, int numInfos)
{
	ssize_t count = 0;

	const int32 kMaxToDeselect = 8;
	select_event* deselect[kMaxToDeselect];
	int32 deselectCount = 0;

	// Add a marker element, so we don't loop forever after unlocking the list.
	// (There is only one invocation of _DequeueEvents() at a time.)
	select_event marker = {};
	fEventList.Add(&marker);

	for (select_event* event = NULL; count < numInfos; ) {
		if (fEventList.Head() == NULL || fEventList.Head() == &marker)
			break;

		event = fEventList.RemoveHead();
		int32 events = atomic_and(&event->events,
			~(event->selected_events | B_EVENT_QUEUED));

		if ((events & B_EVENT_DELETING) != 0)
			continue;

		if ((events & B_EVENT_INVALID) == 0
				&& (event->behavior & B_EVENT_LEVEL_TRIGGERED) != 0) {
			// This event is level-triggered. We need to deselect and reselect it,
			// as its state may have changed since we were notified.
			const select_event tmp = *event;

			mutex_unlock(&fQueueLock);
			status_t status = Deselect(tmp.object, tmp.type);
			if (status == B_OK) {
				event = NULL;
				status = Select(tmp.object, tmp.type,
					tmp.selected_events | tmp.behavior, tmp.user_data);
			}
			mutex_lock(&fQueueLock);

			if (status == B_OK) {
				// Is the event still queued?
				event = _GetEvent(tmp.object, tmp.type);
				if (event == NULL)
					continue;
				events = atomic_get(&event->events);
				if ((events & B_EVENT_QUEUED) == 0)
					continue;
			} else if (event == NULL) {
				continue;
			}
		}

		infos[count].object = event->object;
		infos[count].type = event->type;
		infos[count].user_data = event->user_data;
		infos[count].events = USER_EVENTS(events);
		count++;

		// All logic past this point has to do with deleting events.
		if ((events & B_EVENT_INVALID) == 0 && (event->behavior & B_EVENT_ONE_SHOT) == 0)
			continue;

		// Check if the event was requeued.
		if ((atomic_and(&event->events, ~B_EVENT_QUEUED) & B_EVENT_QUEUED) != 0)
			fEventList.Remove(event);

		if ((events & B_EVENT_INVALID) != 0) {
			// The event will already have been removed from the tree.
			delete event;
		} else if ((event->behavior & B_EVENT_ONE_SHOT) != 0) {
			// We already checked B_EVENT_INVALID above, so we don't need to again.
			fEventTree.Remove(event);
			event->events = B_EVENT_DELETING;

			deselect[deselectCount++] = event;
			if (deselectCount == kMaxToDeselect)
				break;
		}
	}

	fEventList.Remove(&marker);

	if (deselectCount != 0) {
		mutex_unlock(&fQueueLock);
		for (int32 i = 0; i < deselectCount; i++) {
			select_event* event = deselect[i];

			_DeselectEvent(event);
			delete event;
		}
		mutex_lock(&fQueueLock);

		// We don't need to notify waiters, as we removed the events
		// from anywhere they could be found before dropping the lock.
	}

	return count;
}


/*
 * Get the select_event for the given object and type. Must be called with the
 * queue lock held. This method will sleep if the event is undergoing selection
 * or deletion.
 */
select_event*
EventQueue::_GetEvent(int32 object, uint16 type)
{
	EventQueueTreeDefinition::Key key = { object, type };

	while (true) {
		select_event* event = fEventTree.Find(key);
		if (event == NULL)
			return NULL;

		if ((event->events & (B_EVENT_SELECTING | B_EVENT_DELETING)) == 0)
			return event;

		fEventCondition.Wait(&fQueueLock);

		// At this point the select_event might have been deleted, so we
		// need to refetch it.
	}
}


//	#pragma mark -- File descriptor ops



static status_t
event_queue_close(file_descriptor* descriptor)
{
	EventQueue* queue = (EventQueue*)descriptor->u.queue;
	queue->Closed();
	return B_OK;
}


static void
event_queue_free(file_descriptor* descriptor)
{
	EventQueue* queue = (EventQueue*)descriptor->u.queue;
	put_select_sync(queue);
}


static status_t
get_queue_descriptor(int fd, bool kernel, file_descriptor*& descriptor)
{
	if (fd < 0)
		return B_FILE_ERROR;

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->type != FDTYPE_EVENT_QUEUE) {
		put_fd(descriptor);
		return B_BAD_VALUE;
	}

	return B_OK;
}


#define GET_QUEUE_FD_OR_RETURN(fd, kernel, descriptor)	\
	do {												\
		status_t getError = get_queue_descriptor(fd, kernel, descriptor); \
		if (getError != B_OK)							\
			return getError;							\
	} while (false)


static struct fd_ops sEventQueueFDOps = {
	NULL,	// fd_read
	NULL,	// fd_write
	NULL,	// fd_seek
	NULL,	// fd_ioctl
	NULL,	// fd_set_flags
	NULL,	// fd_select
	NULL,	// fd_deselect
	NULL,	// fd_read_dir
	NULL,	// fd_rewind_dir
	NULL,	// fd_read_stat
	NULL,	// fd_write_stat
	&event_queue_close,
	&event_queue_free
};


//	#pragma mark - User syscalls


int
_user_event_queue_create(int openFlags)
{
	EventQueue* queue = new(std::nothrow) EventQueue(false);
	if (queue == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<EventQueue> deleter(queue);

	file_descriptor* descriptor = alloc_fd();
	if (descriptor == NULL)
		return B_NO_MEMORY;

	descriptor->type = FDTYPE_EVENT_QUEUE;
	descriptor->ops = &sEventQueueFDOps;
	descriptor->u.queue = (struct event_queue*)queue;
	descriptor->open_mode = O_RDWR | openFlags;

	io_context* context = get_current_io_context(false);
	int fd = new_fd(context, descriptor);
	if (fd < 0) {
		free(descriptor);
		return fd;
	}

	mutex_lock(&context->io_mutex);
	fd_set_close_on_exec(context, fd, (openFlags & O_CLOEXEC) != 0);
	mutex_unlock(&context->io_mutex);

	deleter.Detach();
	return fd;
}


status_t
_user_event_queue_select(int queue, event_wait_info* userInfos, int numInfos)
{
	if (numInfos <= 0)
		return B_BAD_VALUE;
	if (userInfos == NULL || !IS_USER_ADDRESS(userInfos))
		return B_BAD_ADDRESS;

	BStackOrHeapArray<event_wait_info, 16> infos(numInfos);
	if (!infos.IsValid())
		return B_NO_MEMORY;

	file_descriptor* descriptor;
	GET_QUEUE_FD_OR_RETURN(queue, false, descriptor);
	DescriptorPutter _(descriptor);

	EventQueue* eventQueue = (EventQueue*)descriptor->u.queue;

	if (user_memcpy(infos, userInfos, sizeof(event_wait_info) * numInfos) != B_OK)
		return B_BAD_ADDRESS;

	status_t result = B_OK;

	for (int i = 0; i < numInfos; i++) {
		status_t error;
		if (infos[i].events > 0) {
			error = eventQueue->Select(infos[i].object, infos[i].type,
				infos[i].events, infos[i].user_data);
		} else if (infos[i].events < 0) {
			uint32 selectedEvents = 0;
			error = eventQueue->Query(infos[i].object, infos[i].type,
				&selectedEvents, &infos[i].user_data);
			if (error == B_OK) {
				infos[i].events = selectedEvents;
				error = user_memcpy(&userInfos[i], &infos[i], sizeof(event_wait_info));
			}
		} else /* == 0 */ {
			error = eventQueue->Deselect(infos[i].object, infos[i].type);
		}

		if (error != B_OK) {
			user_memcpy(&userInfos[i].events, &error, sizeof(&userInfos[i].events));
			result = B_ERROR;
		}
	}

	return result;
}


ssize_t
_user_event_queue_wait(int queue, event_wait_info* userInfos, int numInfos,
	uint32 flags, bigtime_t timeout)
{
	syscall_restart_handle_timeout_pre(flags, timeout);

	if (numInfos < 0)
		return B_BAD_VALUE;
	if (numInfos > 0 && (userInfos == NULL || !IS_USER_ADDRESS(userInfos)))
		return B_BAD_ADDRESS;

	BStackOrHeapArray<event_wait_info, 16> infos(numInfos);
	if (!infos.IsValid())
		return B_NO_MEMORY;

	file_descriptor* descriptor;
	GET_QUEUE_FD_OR_RETURN(queue, false, descriptor);
	DescriptorPutter _(descriptor);

	EventQueue* eventQueue = (EventQueue*)descriptor->u.queue;

	ssize_t result = eventQueue->Wait(infos, numInfos, flags, timeout);
	if (result < 0)
		return syscall_restart_handle_timeout_post(result, timeout);

	status_t status = B_OK;
	if (numInfos != 0)
		status = user_memcpy(userInfos, infos, sizeof(event_wait_info) * numInfos);

	return status == B_OK ? result : status;
}
