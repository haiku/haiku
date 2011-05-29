/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		DarkWyrm (bpmagic@columbus.rr.com)
 *		Ingo Weinhold, bonefish@@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!	BLooper class spawns a thread that runs a message loop. */

#include <AppMisc.h>
#include <AutoLocker.h>
#include <DirectMessageTarget.h>
#include <LooperList.h>
#include <MessagePrivate.h>
#include <TokenSpace.h>

#include <Autolock.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <PropertyInfo.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>


// debugging
//#define DBG(x) x
#define DBG(x)	;
#define PRINT(x)	DBG({ printf("[%6ld] ", find_thread(NULL)); printf x; })

/*
#include <Autolock.h>
#include <Locker.h>
static BLocker sDebugPrintLocker("BLooper debug print");
#define PRINT(x)	DBG({						\
	BAutolock _(sDebugPrintLocker);				\
	debug_printf("[%6ld] ", find_thread(NULL));	\
	debug_printf x;								\
})
*/


#define FILTER_LIST_BLOCK_SIZE	5
#define DATA_BLOCK_SIZE			5

// Globals ---------------------------------------------------------------------
using BPrivate::gDefaultTokens;
using BPrivate::gLooperList;
using BPrivate::BLooperList;

port_id _get_looper_port_(const BLooper* looper);

enum {
	BLOOPER_PROCESS_INTERNALLY = 0,
	BLOOPER_HANDLER_BY_INDEX
};

static property_info gLooperPropInfo[] = {
	{
		"Handler",
			{},
			{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
			NULL, BLOOPER_HANDLER_BY_INDEX,
			{},
			{},
			{}
	},
	{
		"Handlers",
			{B_GET_PROPERTY},
			{B_DIRECT_SPECIFIER},
			NULL, BLOOPER_PROCESS_INTERNALLY,
			{B_MESSENGER_TYPE},
			{},
			{}
	},
	{
		"Handler",
			{B_COUNT_PROPERTIES},
			{B_DIRECT_SPECIFIER},
			NULL, BLOOPER_PROCESS_INTERNALLY,
			{B_INT32_TYPE},
			{},
			{}
	},
	{}
};

struct _loop_data_ {
	BLooper*	looper;
	thread_id	thread;
};


//	#pragma mark -


BLooper::BLooper(const char* name, int32 priority, int32 portCapacity)
	: BHandler(name)
{
	_InitData(name, priority, portCapacity);
}


BLooper::~BLooper()
{
	if (fRunCalled && !fTerminating) {
		debugger("You can't call delete on a BLooper object "
			"once it is running.");
	}

	Lock();

	// In case the looper thread calls Quit() fLastMessage is not deleted.
	if (fLastMessage) {
		delete fLastMessage;
		fLastMessage = NULL;
	}

	// Close the message port and read and reply to the remaining messages.
	if (fMsgPort >= 0)
		close_port(fMsgPort);

	// Clear the queue so our call to IsMessageWaiting() below doesn't give
	// us bogus info
	fDirectTarget->Close();

	BMessage* message;
	while ((message = fDirectTarget->Queue()->NextMessage()) != NULL) {
		delete message;
			// msg will automagically post generic reply
	}

	do {
		delete ReadMessageFromPort(0);
			// msg will automagically post generic reply
	} while (IsMessageWaiting());

	fDirectTarget->Release();
	delete_port(fMsgPort);

	// Clean up our filters
	SetCommonFilterList(NULL);

	AutoLocker<BLooperList> ListLock(gLooperList);
	RemoveHandler(this);

	// Remove all the "child" handlers
	int32 count = fHandlers.CountItems();
	for (int32 i = 0; i < count; i++) {
		BHandler* handler = (BHandler*)fHandlers.ItemAtFast(i);
		handler->SetNextHandler(NULL);
		handler->SetLooper(NULL);
	}
	fHandlers.MakeEmpty();

	Unlock();
	gLooperList.RemoveLooper(this);
	delete_sem(fLockSem);
}


BLooper::BLooper(BMessage* data)
	: BHandler(data)
{
	int32 portCapacity;
	if (data->FindInt32("_port_cap", &portCapacity) != B_OK || portCapacity < 0)
		portCapacity = B_LOOPER_PORT_DEFAULT_CAPACITY;

	int32 priority;
	if (data->FindInt32("_prio", &priority) != B_OK)
		priority = B_NORMAL_PRIORITY;

	_InitData(Name(), priority, portCapacity);
}


BArchivable*
BLooper::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BLooper"))
		return new BLooper(data);

	return NULL;
}


status_t
BLooper::Archive(BMessage* data, bool deep) const
{
	status_t status = BHandler::Archive(data, deep);
	if (status < B_OK)
		return status;

	port_info info;
	status = get_port_info(fMsgPort, &info);
	if (status == B_OK)
		status = data->AddInt32("_port_cap", info.capacity);

	thread_info threadInfo;
	if (get_thread_info(Thread(), &threadInfo) == B_OK)
		status = data->AddInt32("_prio", threadInfo.priority);

	return status;
}


status_t
BLooper::PostMessage(uint32 command)
{
	BMessage message(command);
	return _PostMessage(&message, this, NULL);
}


status_t
BLooper::PostMessage(BMessage* message)
{
	return _PostMessage(message, this, NULL);
}


status_t
BLooper::PostMessage(uint32 command, BHandler* handler, BHandler* replyTo)
{
	BMessage message(command);
	return _PostMessage(&message, handler, replyTo);
}


status_t
BLooper::PostMessage(BMessage* message, BHandler* handler, BHandler* replyTo)
{
	return _PostMessage(message, handler, replyTo);
}


void
BLooper::DispatchMessage(BMessage* message, BHandler* handler)
{
	PRINT(("BLooper::DispatchMessage(%.4s)\n", (char*)&message->what));

	switch (message->what) {
		case _QUIT_:
			// Can't call Quit() to do this, because of the slight chance
			// another thread with have us locked between now and then.
			fTerminating = true;

			// After returning from DispatchMessage(), the looper will be
			// deleted in _task0_()
			break;

		case B_QUIT_REQUESTED:
			if (handler == this) {
				_QuitRequested(message);
				break;
			}

			// fall through

		default:
			handler->MessageReceived(message);
			break;
	}
	PRINT(("BLooper::DispatchMessage() done\n"));
}


void
BLooper::MessageReceived(BMessage* message)
{
	// TODO: implement scripting support
	BHandler::MessageReceived(message);
}


BMessage*
BLooper::CurrentMessage() const
{
	return fLastMessage;
}


BMessage*
BLooper::DetachCurrentMessage()
{
	BMessage* message = fLastMessage;
	fLastMessage = NULL;
	return message;
}


BMessageQueue*
BLooper::MessageQueue() const
{
	return fDirectTarget->Queue();
}


bool
BLooper::IsMessageWaiting() const
{
	AssertLocked();

	if (!fDirectTarget->Queue()->IsEmpty())
		return true;

	int32 count;
	do {
		count = port_buffer_size_etc(fMsgPort, B_RELATIVE_TIMEOUT, 0);
	} while (count == B_INTERRUPTED);

	return count > 0;
}


void
BLooper::AddHandler(BHandler* handler)
{
	if (handler == NULL)
		return;

	AssertLocked();

	if (handler->Looper() == NULL) {
		fHandlers.AddItem(handler);
		handler->SetLooper(this);
		if (handler != this)	// avoid a cycle
			handler->SetNextHandler(this);
	}
}


bool
BLooper::RemoveHandler(BHandler* handler)
{
	if (handler == NULL)
		return false;

	AssertLocked();

	if (handler->Looper() == this && fHandlers.RemoveItem(handler)) {
		if (handler == fPreferred)
			fPreferred = NULL;

		handler->SetNextHandler(NULL);
		handler->SetLooper(NULL);
		return true;
	}

	return false;
}


int32
BLooper::CountHandlers() const
{
	AssertLocked();

	return fHandlers.CountItems();
}


BHandler*
BLooper::HandlerAt(int32 index) const
{
	AssertLocked();

	return (BHandler*)fHandlers.ItemAt(index);
}


int32
BLooper::IndexOf(BHandler* handler) const
{
	AssertLocked();

	return fHandlers.IndexOf(handler);
}


BHandler*
BLooper::PreferredHandler() const
{
	return fPreferred;
}


void
BLooper::SetPreferredHandler(BHandler* handler)
{
	if (handler && handler->Looper() == this && IndexOf(handler) >= 0) {
		fPreferred = handler;
	} else {
		fPreferred = NULL;
	}
}


thread_id
BLooper::Run()
{
	AssertLocked();

	if (fRunCalled) {
		// Not allowed to call Run() more than once
		debugger("can't call BLooper::Run twice!");
		return fThread;
	}

	fThread = spawn_thread(_task0_, Name(), fInitPriority, this);
	if (fThread < B_OK)
		return fThread;

	if (fMsgPort < B_OK)
		return fMsgPort;

	fRunCalled = true;
	Unlock();

	status_t err = resume_thread(fThread);
	if (err < B_OK)
		return err;

	return fThread;
}


void
BLooper::Quit()
{
	PRINT(("BLooper::Quit()\n"));

	if (!IsLocked()) {
		printf("ERROR - you must Lock a looper before calling Quit(), "
			"team=%ld, looper=%s\n", Team(), Name() ? Name() : "unnamed");
	}

	// Try to lock
	if (!Lock()) {
		// We're toast already
		return;
	}

	PRINT(("  is locked\n"));

	if (!fRunCalled) {
		PRINT(("  Run() has not been called yet\n"));
		fTerminating = true;
		delete this;
	} else if (find_thread(NULL) == fThread) {
		PRINT(("  We are the looper thread\n"));
		fTerminating = true;
		delete this;
		exit_thread(0);
	} else {
		PRINT(("  Run() has already been called and we are not the looper thread\n"));

		// As with sem in _Lock(), we need to cache this here in case the looper
		// disappears before we get to the wait_for_thread() below
		thread_id thread = Thread();

		// We need to unlock here. Otherwise the looper thread can't
		// dispatch the _QUIT_ message we're going to post.
		UnlockFully();

		// As per the BeBook, if we've been called by a thread other than
		// our own, the rest of the message queue has to get processed.  So
		// we put this in the queue, and when it shows up, we'll call Quit()
		// from our own thread.
		// QuitRequested() will not be called in this case.
		PostMessage(_QUIT_);

		// We have to wait until the looper is done processing any remaining
		// messages.
		status_t status;
		while (wait_for_thread(thread, &status) == B_INTERRUPTED)
			;
	}

	PRINT(("BLooper::Quit() done\n"));
}


bool
BLooper::QuitRequested()
{
	return true;
}


bool
BLooper::Lock()
{
	// Defer to global _Lock(); see notes there
	return _Lock(this, -1, B_INFINITE_TIMEOUT) == B_OK;
}


void
BLooper::Unlock()
{
PRINT(("BLooper::Unlock()\n"));
	//	Make sure we're locked to begin with
	AssertLocked();

	//	Decrement fOwnerCount
	--fOwnerCount;
PRINT(("  fOwnerCount now: %ld\n", fOwnerCount));
	//	Check to see if the owner still wants a lock
	if (fOwnerCount == 0) {
		//	Set fOwner to invalid thread_id (< 0)
		fOwner = -1;
		fCachedStack = 0;

#if DEBUG < 1
		//	Decrement requested lock count (using fAtomicCount for this)
		int32 atomicCount = atomic_add(&fAtomicCount, -1);
PRINT(("  fAtomicCount now: %ld\n", fAtomicCount));

		// Check if anyone is waiting for a lock
		// and release if it's the case
		if (atomicCount > 1)
#endif
			release_sem(fLockSem);
	}
PRINT(("BLooper::Unlock() done\n"));
}


bool
BLooper::IsLocked() const
{
	if (!gLooperList.IsLooperValid(this)) {
		// The looper is gone, so of course it's not locked
		return false;
	}

	uint32 stack;
	return ((uint32)&stack & ~(B_PAGE_SIZE - 1)) == fCachedStack
		|| find_thread(NULL) == fOwner;
}


status_t
BLooper::LockWithTimeout(bigtime_t timeout)
{
	return _Lock(this, -1, timeout);
}


thread_id
BLooper::Thread() const
{
	return fThread;
}


team_id
BLooper::Team() const
{
	return BPrivate::current_team();
}


BLooper*
BLooper::LooperForThread(thread_id thread)
{
	return gLooperList.LooperForThread(thread);
}


thread_id
BLooper::LockingThread() const
{
	return fOwner;
}


int32
BLooper::CountLocks() const
{
	return fOwnerCount;
}


int32
BLooper::CountLockRequests() const
{
	return fAtomicCount;
}


sem_id
BLooper::Sem() const
{
	return fLockSem;
}


BHandler*
BLooper::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
/**
	@note	When I was first dumping the results of GetSupportedSuites() from
			various classes, the use of the extra_data field was quite
			mysterious to me.  Then I dumped BApplication and compared the
			result against the BeBook's docs for scripting BApplication.  A
			bunch of it isn't documented, but what is tipped me to the idea
			that the extra_data is being used as a quick and dirty way to tell
			what scripting "command" has been sent, e.g., for easy use in a
			switch statement.  Would certainly be a lot faster than a bunch of
			string comparisons -- which wouldn't tell the whole story anyway,
			because of the same name being used for multiple properties.
 */
 	BPropertyInfo propertyInfo(gLooperPropInfo);
	uint32 data;
	status_t err = B_OK;
	const char* errMsg = "";
	if (propertyInfo.FindMatch(msg, index, specifier, form, property, &data)
			>= 0) {
		switch (data) {
			case BLOOPER_PROCESS_INTERNALLY:
				return this;

			case BLOOPER_HANDLER_BY_INDEX:
			{
				int32 index = specifier->FindInt32("index");
				if (form == B_REVERSE_INDEX_SPECIFIER) {
					index = CountHandlers() - index;
				}
				BHandler* target = HandlerAt(index);
				if (target) {
					// Specifier has been fully handled
					msg->PopSpecifier();
					return target;
				} else {
					err = B_BAD_INDEX;
					errMsg = "handler index out of range";
				}
				break;
			}

			default:
				err = B_BAD_SCRIPT_SYNTAX;
				errMsg = "Didn't understand the specifier(s)";
				break;
		}
	} else {
		return BHandler::ResolveSpecifier(msg, index, specifier, form,
			property);
	}

	BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);
	reply.AddInt32("error", err);
	reply.AddString("message", errMsg);
	msg->SendReply(&reply);

	return NULL;
}


status_t
BLooper::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-looper");
	if (status == B_OK) {
		BPropertyInfo PropertyInfo(gLooperPropInfo);
		status = data->AddFlat("messages", &PropertyInfo);
		if (status == B_OK)
			status = BHandler::GetSupportedSuites(data);
	}

	return status;
}


void
BLooper::AddCommonFilter(BMessageFilter* filter)
{
	if (!filter)
		return;

	AssertLocked();

	if (filter->Looper()) {
		debugger("A MessageFilter can only be used once.");
		return;
	}

	if (!fCommonFilters)
		fCommonFilters = new BList(FILTER_LIST_BLOCK_SIZE);

	filter->SetLooper(this);
	fCommonFilters->AddItem(filter);
}


bool
BLooper::RemoveCommonFilter(BMessageFilter* filter)
{
	AssertLocked();

	if (!fCommonFilters)
		return false;

	bool result = fCommonFilters->RemoveItem(filter);
	if (result)
		filter->SetLooper(NULL);

	return result;
}


void
BLooper::SetCommonFilterList(BList* filters)
{
	AssertLocked();

	BMessageFilter* filter;
	if (filters) {
		// Check for ownership issues - a filter can only have one owner
		for (int32 i = 0; i < filters->CountItems(); ++i) {
			filter = (BMessageFilter*)filters->ItemAt(i);
			if (filter->Looper()) {
				debugger("A MessageFilter can only be used once.");
				return;
			}
		}
	}

	if (fCommonFilters) {
		for (int32 i = 0; i < fCommonFilters->CountItems(); ++i) {
			delete (BMessageFilter*)fCommonFilters->ItemAt(i);
		}

		delete fCommonFilters;
		fCommonFilters = NULL;
	}

	// Per the BeBook, we take ownership of the list
	fCommonFilters = filters;
	if (fCommonFilters) {
		for (int32 i = 0; i < fCommonFilters->CountItems(); ++i) {
			filter = (BMessageFilter*)fCommonFilters->ItemAt(i);
			filter->SetLooper(this);
		}
	}
}


BList*
BLooper::CommonFilterList() const
{
	return fCommonFilters;
}


status_t
BLooper::Perform(perform_code d, void* arg)
{
	// This is sort of what we're doing for this function everywhere
	return BHandler::Perform(d, arg);
}


BMessage*
BLooper::MessageFromPort(bigtime_t timeout)
{
	return ReadMessageFromPort(timeout);
}


void BLooper::_ReservedLooper1() {}
void BLooper::_ReservedLooper2() {}
void BLooper::_ReservedLooper3() {}
void BLooper::_ReservedLooper4() {}
void BLooper::_ReservedLooper5() {}
void BLooper::_ReservedLooper6() {}


BLooper::BLooper(const BLooper& other)
{
	// Copy construction not allowed
}


BLooper&
BLooper::operator=(const BLooper& other)
{
	// Looper copying not allowed
	return *this;
}


BLooper::BLooper(int32 priority, port_id port, const char* name)
{
	// This must be a legacy constructor
	fMsgPort = port;
	_InitData(name, priority, B_LOOPER_PORT_DEFAULT_CAPACITY);
}


status_t
BLooper::_PostMessage(BMessage *msg, BHandler *handler, BHandler *replyTo)
{
	AutoLocker<BLooperList> listLocker(gLooperList);
	if (!listLocker.IsLocked())
		return B_ERROR;

	if (!gLooperList.IsLooperValid(this))
		return B_BAD_VALUE;

	// Does handler belong to this looper?
	if (handler && handler->Looper() != this)
		return B_MISMATCHED_VALUES;

	status_t status;
	BMessenger messenger(handler, this, &status);
	listLocker.Unlock();
	if (status == B_OK)
		status = messenger.SendMessage(msg, replyTo, 0);

	return status;
}


/*!
	Locks a looper either by port or using a direct pointer to the looper.

	\param looper looper to lock, if not NULL
	\param port port to identify the looper in case \a looper is NULL
	\param timeout timeout for acquiring the lock
*/
status_t
BLooper::_Lock(BLooper* looper, port_id port, bigtime_t timeout)
{
	PRINT(("BLooper::_Lock(%p, %lx)\n", looper, port));

	//	Check params (loop, port)
	if (looper == NULL && port < 0) {
		PRINT(("BLooper::_Lock() done 1\n"));
		return B_BAD_VALUE;
	}

	thread_id currentThread = find_thread(NULL);
	int32 oldCount;
	sem_id sem;

	{
		AutoLocker<BLooperList> ListLock(gLooperList);
		if (!ListLock.IsLocked())
			return B_BAD_VALUE;

		// Look up looper by port_id, if necessary
		if (looper == NULL) {
			looper = gLooperList.LooperForPort(port);
			if (looper == NULL) {
				PRINT(("BLooper::_Lock() done 3\n"));
				return B_BAD_VALUE;
			}
		} else if (!gLooperList.IsLooperValid(looper)) {
			//	Check looper validity
			PRINT(("BLooper::_Lock() done 4\n"));
			return B_BAD_VALUE;
		}

		// Check for nested lock attempt
		if (currentThread == looper->fOwner) {
			++looper->fOwnerCount;
			PRINT(("BLooper::_Lock() done 5: fOwnerCount: %ld\n", loop->fOwnerCount));
			return B_OK;
		}

		// Cache the semaphore, so that we can safely access it after having
		// unlocked the looper list
		sem = looper->fLockSem;
		if (sem < 0) {
			PRINT(("BLooper::_Lock() done 6\n"));
			return B_BAD_VALUE;
		}

		// Bump the requested lock count (using fAtomicCount for this)
		oldCount = atomic_add(&looper->fAtomicCount, 1);
	}

	return _LockComplete(looper, oldCount, currentThread, sem, timeout);
}


status_t
BLooper::_LockComplete(BLooper *looper, int32 oldCount, thread_id thread,
	sem_id sem, bigtime_t timeout)
{
	status_t err = B_OK;

#if DEBUG < 1
	if (oldCount > 0) {
#endif
		do {
			err = acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
		} while (err == B_INTERRUPTED);
#if DEBUG < 1
	}
#endif
	if (err == B_OK) {
		looper->fOwner = thread;
		looper->fCachedStack = (addr_t)&err & ~(B_PAGE_SIZE - 1);
		looper->fOwnerCount = 1;
	}

	PRINT(("BLooper::_LockComplete() done: %lx\n", err));
	return err;
}


void
BLooper::_InitData(const char *name, int32 priority, int32 portCapacity)
{
	fOwner = B_ERROR;
	fCachedStack = 0;
	fRunCalled = false;
	fDirectTarget = new (std::nothrow) BPrivate::BDirectMessageTarget();
	fCommonFilters = NULL;
	fLastMessage = NULL;
	fPreferred = NULL;
	fThread = B_ERROR;
	fTerminating = false;
	fMsgPort = -1;
	fAtomicCount = 0;

	if (name == NULL)
		name = "anonymous looper";

#if DEBUG
	fLockSem = create_sem(1, name);
#else
	fLockSem = create_sem(0, name);
#endif

	if (portCapacity <= 0)
		portCapacity = B_LOOPER_PORT_DEFAULT_CAPACITY;

	fMsgPort = create_port(portCapacity, name);

	fInitPriority = priority;

	gLooperList.AddLooper(this);
		// this will also lock this looper

	AddHandler(this);
}


void
BLooper::AddMessage(BMessage* message)
{
	_AddMessagePriv(message);

	// wakeup looper when being called from other threads if necessary
	if (find_thread(NULL) != Thread()
		&& fDirectTarget->Queue()->IsNextMessage(message)
		&& port_count(fMsgPort) <= 0) {
		// there is currently no message waiting, and we need to wakeup the
		// looper
		write_port_etc(fMsgPort, 0, NULL, 0, B_RELATIVE_TIMEOUT, 0);
	}
}


void
BLooper::_AddMessagePriv(BMessage* message)
{
	// ToDo: if no target token is specified, set to preferred handler
	// Others may want to peek into our message queue, so the preferred
	// handler must be set correctly already if no token was given

	fDirectTarget->Queue()->AddMessage(message);
}


status_t
BLooper::_task0_(void* arg)
{
	BLooper* looper = (BLooper *)arg;

	PRINT(("LOOPER: _task0_()\n"));

	if (looper->Lock()) {
		PRINT(("LOOPER: looper locked\n"));
		looper->task_looper();

		delete looper;
	}

	PRINT(("LOOPER: _task0_() done: thread %ld\n", find_thread(NULL)));
	return B_OK;
}


void *
BLooper::ReadRawFromPort(int32* msgCode, bigtime_t timeout)
{
	PRINT(("BLooper::ReadRawFromPort()\n"));
	uint8 *buffer = NULL;
	ssize_t bufferSize;

	do {
		bufferSize = port_buffer_size_etc(fMsgPort, B_RELATIVE_TIMEOUT, timeout);
	} while (bufferSize == B_INTERRUPTED);

	if (bufferSize < B_OK) {
		PRINT(("BLooper::ReadRawFromPort(): failed: %ld\n", bufferSize));
		return NULL;
	}

	if (bufferSize > 0)
		buffer = (uint8 *)malloc(bufferSize);

	// we don't want to wait again here, since that can only mean
	// that someone else has read our message and our bufferSize
	// is now probably wrong
	PRINT(("read_port()...\n"));
	bufferSize = read_port_etc(fMsgPort, msgCode, buffer, bufferSize,
		B_RELATIVE_TIMEOUT, 0);

	if (bufferSize < B_OK) {
		free(buffer);
		return NULL;
	}

	PRINT(("BLooper::ReadRawFromPort() read: %.4s, %p (%d bytes)\n", (char *)msgCode, buffer, bufferSize));
	return buffer;
}


BMessage*
BLooper::ReadMessageFromPort(bigtime_t timeout)
{
	PRINT(("BLooper::ReadMessageFromPort()\n"));
	int32 msgCode;
	BMessage *message = NULL;

	void *buffer = ReadRawFromPort(&msgCode, timeout);
	if (!buffer)
		return NULL;

	message = ConvertToMessage(buffer, msgCode);
	free(buffer);

	PRINT(("BLooper::ReadMessageFromPort() done: %p\n", message));
	return message;
}


BMessage*
BLooper::ConvertToMessage(void* buffer, int32 code)
{
	PRINT(("BLooper::ConvertToMessage()\n"));
	if (!buffer)
		return NULL;

	BMessage* message = new BMessage();
	if (message->Unflatten((const char*)buffer) != B_OK) {
		PRINT(("BLooper::ConvertToMessage(): unflattening message failed\n"));
		delete message;
		message = NULL;
	}

	PRINT(("BLooper::ConvertToMessage(): %p\n", message));
	return message;
}


void
BLooper::task_looper()
{
	PRINT(("BLooper::task_looper()\n"));
	// Check that looper is locked (should be)
	AssertLocked();
	// Unlock the looper
	Unlock();

	if (IsLocked())
		debugger("looper must not be locked!");

	// loop: As long as we are not terminating.
	while (!fTerminating) {
		PRINT(("LOOPER: outer loop\n"));
		// TODO: timeout determination algo
		//	Read from message port (how do we determine what the timeout is?)
		PRINT(("LOOPER: MessageFromPort()...\n"));
		BMessage *msg = MessageFromPort();
		PRINT(("LOOPER: ...done\n"));

		//	Did we get a message?
		if (msg)
			_AddMessagePriv(msg);

		// Get message count from port
		int32 msgCount = port_count(fMsgPort);
		for (int32 i = 0; i < msgCount; ++i) {
			// Read 'count' messages from port (so we will not block)
			// We use zero as our timeout since we know there is stuff there
			msg = MessageFromPort(0);
			if (msg)
				_AddMessagePriv(msg);
		}

		// loop: As long as there are messages in the queue and the port is
		//		 empty... and we are not terminating, of course.
		bool dispatchNextMessage = true;
		while (!fTerminating && dispatchNextMessage) {
			PRINT(("LOOPER: inner loop\n"));
			// Get next message from queue (assign to fLastMessage)
			fLastMessage = fDirectTarget->Queue()->NextMessage();

			Lock();

			if (!fLastMessage) {
				// No more messages: Unlock the looper and terminate the
				// dispatch loop.
				dispatchNextMessage = false;
			} else {
				PRINT(("LOOPER: fLastMessage: 0x%lx: %.4s\n", fLastMessage->what,
					(char*)&fLastMessage->what));
				DBG(fLastMessage->PrintToStream());

				// Get the target handler
				BHandler *handler = NULL;
				BMessage::Private messagePrivate(fLastMessage);
				bool usePreferred = messagePrivate.UsePreferredTarget();

				if (usePreferred) {
					PRINT(("LOOPER: use preferred target\n"));
					handler = fPreferred;
					if (handler == NULL)
						handler = this;
				} else {
					gDefaultTokens.GetToken(messagePrivate.GetTarget(),
						B_HANDLER_TOKEN, (void **)&handler);

					// if this handler doesn't belong to us, we drop the message
					if (handler != NULL && handler->Looper() != this)
						handler = NULL;

					PRINT(("LOOPER: use %ld, handler: %p, this: %p\n",
						messagePrivate.GetTarget(), handler, this));
				}

				// Is this a scripting message? (BMessage::HasSpecifiers())
				if (handler != NULL && fLastMessage->HasSpecifiers()) {
					int32 index = 0;
					// Make sure the current specifier is kosher
					if (fLastMessage->GetCurrentSpecifier(&index) == B_OK)
						handler = resolve_specifier(handler, fLastMessage);
				}

				if (handler) {
					// Do filtering
					handler = _TopLevelFilter(fLastMessage, handler);
					PRINT(("LOOPER: _TopLevelFilter(): %p\n", handler));
					if (handler && handler->Looper() == this)
						DispatchMessage(fLastMessage, handler);
				}
			}

			if (fTerminating) {
				// we leave the looper locked when we quit
				return;
			}

			// Unlock the looper
			Unlock();

			// Delete the current message (fLastMessage)
			if (fLastMessage) {
				delete fLastMessage;
				fLastMessage = NULL;
			}

			// Are any messages on the port?
			if (port_count(fMsgPort) > 0) {
				// Do outer loop
				dispatchNextMessage = false;
			}
		}
	}
	PRINT(("BLooper::task_looper() done\n"));
}


void
BLooper::_QuitRequested(BMessage* message)
{
	bool isQuitting = QuitRequested();
	int32 thread = fThread;

	if (isQuitting)
		Quit();

	// We send a reply to the sender, when they're waiting for a reply or
	// if the request message contains a boolean "_shutdown_" field with value
	// true. In the latter case the message came from the registrar, asking
	// the application to shut down.
	bool shutdown;
	if (message->IsSourceWaiting()
		|| (message->FindBool("_shutdown_", &shutdown) == B_OK && shutdown)) {
		BMessage replyMsg(B_REPLY);
		replyMsg.AddBool("result", isQuitting);
		replyMsg.AddInt32("thread", thread);
		message->SendReply(&replyMsg);
	}
}


bool
BLooper::AssertLocked() const
{
	if (!IsLocked()) {
		debugger("looper must be locked before proceeding\n");
		return false;
	}

	return true;
}


BHandler*
BLooper::_TopLevelFilter(BMessage* message, BHandler* target)
{
	if (message == NULL)
		return target;

	// Apply the common filters first
	target = _ApplyFilters(CommonFilterList(), message, target);
	if (target) {
		if (target->Looper() != this) {
			debugger("Targeted handler does not belong to the looper.");
			target = NULL;
		} else {
			// Now apply handler-specific filters
			target = _HandlerFilter(message, target);
		}
	}

	return target;
}


BHandler*
BLooper::_HandlerFilter(BMessage* message, BHandler* target)
{
	// Keep running filters until our handler is NULL, or until the filtering
	// handler returns itself as the designated handler
	BHandler* previousTarget = NULL;
	while (target != NULL && target != previousTarget) {
		previousTarget = target;

		target = _ApplyFilters(target->FilterList(), message, target);
		if (target != NULL && target->Looper() != this) {
			debugger("Targeted handler does not belong to the looper.");
			target = NULL;
		}
	}

	return target;
}


BHandler*
BLooper::_ApplyFilters(BList* list, BMessage* message, BHandler* target)
{
	// This is where the action is!
	// Check the parameters
	if (!list || !message)
		return target;

	// For each filter in the provided list
	BMessageFilter* filter = NULL;
	for (int32 i = 0; i < list->CountItems(); ++i) {
		filter = (BMessageFilter*)list->ItemAt(i);

		// Check command conditions
		if (filter->FiltersAnyCommand() || filter->Command() == message->what) {
			// Check delivery conditions
			message_delivery delivery = filter->MessageDelivery();
			bool dropped = message->WasDropped();
			if (delivery == B_ANY_DELIVERY
				|| (delivery == B_DROPPED_DELIVERY && dropped)
				|| (delivery == B_PROGRAMMED_DELIVERY && !dropped)) {
				// Check source conditions
				message_source source = filter->MessageSource();
				bool remote = message->IsSourceRemote();
				if (source == B_ANY_SOURCE
					|| (source == B_REMOTE_SOURCE && remote)
					|| (source == B_LOCAL_SOURCE && !remote)) {
					// Are we using an "external" function?
					filter_result result;
					filter_hook func = filter->FilterFunction();
					if (func)
						result = func(message, &target, filter);
					else
						result = filter->Filter(message, &target);

					// Is further processing allowed?
					if (result == B_SKIP_MESSAGE) {
						// No; time to bail out
						return NULL;
					}
				}
			}
		}
	}

	return target;
}


void
BLooper::check_lock()
{
	// This is a cheap variant of AssertLocked()
	// It is used in situations where it's clear that the looper is valid,
	// ie. from handlers
	uint32 stack;
	if (((uint32)&stack & ~(B_PAGE_SIZE - 1)) == fCachedStack
		|| fOwner == find_thread(NULL))
		return;

	debugger("Looper must be locked.");
}


BHandler*
BLooper::resolve_specifier(BHandler* target, BMessage* message)
{
	// Check params
	if (!target || !message)
		return NULL;

	int32 index;
	BMessage specifier;
	int32 form;
	const char* property;
	status_t err = B_OK;
	BHandler* newTarget = target;
	// Loop to deal with nested specifiers
	// (e.g., the 3rd button on the 4th view)
	do {
		err = message->GetCurrentSpecifier(&index, &specifier, &form, &property);
		if (err) {
			BMessage reply(B_REPLY);
			reply.AddInt32("error", err);
			message->SendReply(&reply);
			return NULL;
		}
		// Current target gets what was the new target
		target = newTarget;
		newTarget = target->ResolveSpecifier(message, index, &specifier, form,
			property);
		// Check that new target is owned by looper; use IndexOf() to avoid
		// dereferencing newTarget (possible race condition with object
		// destruction by another looper)
		if (!newTarget || IndexOf(newTarget) < 0)
			return NULL;

		// Get current specifier index (may change in ResolveSpecifier())
		err = message->GetCurrentSpecifier(&index);
	} while (newTarget && newTarget != target && !err && index >= 0);

	return newTarget;
}


/*!	Releases all eventually nested locks. Must be called with the lock
	actually held.
*/
void
BLooper::UnlockFully()
{
	AssertLocked();

	// Clear the owner count
	fOwnerCount = 0;
	// Nobody owns the lock now
	fOwner = -1;
	fCachedStack = 0;
#if DEBUG < 1
	// There is now one less thread holding a lock on this looper
	int32 atomicCount = atomic_add(&fAtomicCount, -1);
	if (atomicCount > 1)
#endif
		release_sem(fLockSem);
}


//	#pragma mark -


port_id
_get_looper_port_(const BLooper* looper)
{
	return looper->fMsgPort;
}

