//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Looper.cpp
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//					DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	BLooper class spawns a thread that runs a message loop.
//------------------------------------------------------------------------------

/**
	@note	Although I'm implementing "by the book" for now, I would like to
			refactor sLooperList and all of the functions that operate on it
			into their own class in the BPrivate namespace.

			Also considering adding the thread priority when archiving.

			
 */

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Autolock.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <PropertyInfo.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include <LooperList.h>
#include <ObjectLocker.h>
#include <TokenSpace.h>

// Local Defines ---------------------------------------------------------------
#define FILTER_LIST_BLOCK_SIZE	5
#define DATA_BLOCK_SIZE			5

// Globals ---------------------------------------------------------------------
using BPrivate::gDefaultTokens;
using BPrivate::gLooperList;
using BPrivate::BObjectLocker;
using BPrivate::BLooperList;

port_id _get_looper_port_(const BLooper* looper);
bool _use_preferred_target_(BMessage* msg) { return msg->fPreferred; }
int32 _get_message_target_(BMessage* msg) { return msg->fTarget; }

uint32			BLooper::sLooperID = (uint32)B_ERROR;
team_id			BLooper::sTeamID = (team_id)B_ERROR;

enum
{
	BLOOPER_PROCESS_INTERNALLY = 0,
	BLOOPER_HANDLER_BY_INDEX
};

static property_info gLooperPropInfo[] =
{
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

struct _loop_data_
{
	BLooper*	looper;
	thread_id	thread;
};

//------------------------------------------------------------------------------
BLooper::BLooper(const char* name, int32 priority, int32 port_capacity)
	:	BHandler(name)
{
	InitData(name, priority, port_capacity);
}
//------------------------------------------------------------------------------
BLooper::~BLooper()
{
	if (fRunCalled && !fTerminating)
	{
		debugger("You can't call delete on a BLooper object "
				 "once it is running.");
	}

	Lock();

	// In case the looper thread calls Quit() fLastMessage is not deleted.
	if (fLastMessage)
	{
		delete fLastMessage;
		fLastMessage = NULL;
	}

	// Close the message port and read and reply to the remaining messages.
	if (fMsgPort > 0)
	{
		close_port(fMsgPort);
	}

	BMessage* msg;
	// Clear the queue so our call to IsMessageWaiting() below doesn't give
	// us bogus info
	while ((msg = fQueue->NextMessage()))
	{
		delete msg;			// msg will automagically post generic reply
	}

	do
	{
		msg = ReadMessageFromPort(0);
		if (msg)
		{
			delete msg;		// msg will automagically post generic reply
		}
	} while (IsMessageWaiting());

// bonefish: Killing the looper thread doesn't work very well with
// BApplication. In fact here it doesn't work too well either. When the
// looper thread encounters a _QUIT_ message it deletes the BLooper object
// and thus commits suicide at this point. That is, the following cleanup
// isn't done.
//	kill_thread(fTaskID);
	delete fQueue;
	delete_port(fMsgPort);

	// Clean up our filters
	SetCommonFilterList(NULL);

	BObjectLocker<BLooperList> ListLock(gLooperList);
	RemoveHandler(this);

	// Remove all the "child" handlers
	BHandler* child;
	while (CountHandlers())
	{
		child = HandlerAt(0);
		if (child)
		{
			RemoveHandler(child);
		}
	}

	UnlockFully();
	RemoveLooper(this);
	delete_sem(fLockSem);
}
//------------------------------------------------------------------------------
BLooper::BLooper(BMessage* data)
	:	BHandler(data)
{
	int32 portCap;
	if (data->FindInt32("_port_cap", &portCap) != B_OK)
	{
		portCap = B_LOOPER_PORT_DEFAULT_CAPACITY;
	}

	InitData(Name(), B_NORMAL_PRIORITY, portCap);
}
//------------------------------------------------------------------------------
BArchivable* BLooper::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BLooper"))
	{
		return new BLooper(data);
	}

	return NULL;
}
//------------------------------------------------------------------------------
status_t BLooper::Archive(BMessage* data, bool deep) const
{
	status_t err = BHandler::Archive(data, deep);
	if (!err)
	{
		port_info info;
		err = get_port_info(fMsgPort, &info);
		if (!err)
		{
			err = data->AddInt32("_port_cap", info.capacity);
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BLooper::PostMessage(uint32 command)
{
	BMessage Message(command);
	return _PostMessage(&Message, this, NULL);
}
//------------------------------------------------------------------------------
status_t BLooper::PostMessage(BMessage* message)
{
	return _PostMessage(message, this, NULL);
}
//------------------------------------------------------------------------------
status_t BLooper::PostMessage(uint32 command, BHandler* handler,
							  BHandler* reply_to)
{
	BMessage Message(command);
	return _PostMessage(&Message, handler, reply_to);
}
//------------------------------------------------------------------------------
status_t BLooper::PostMessage(BMessage* message, BHandler* handler,
							  BHandler* reply_to)
{
	return _PostMessage(message, handler, reply_to);
}
//------------------------------------------------------------------------------
void BLooper::DispatchMessage(BMessage* message, BHandler* handler)
{
DBG(OUT("BLooper::DispatchMessage(%.4s)\n", (char*)&message->what));
/**
	@note	Initially, DispatchMessage() was locking the looper, calling the
			filtering API, determining whether to use fPreferred or not, and
			deleting the message.  A look at the BeBook, however, reveals that
			all this function does is handle its own B_QUIT_REQUESTED messages
			and pass everything else to handler->MessageReceived().  Clearly the
			rest must be happening in task_looper().  This makes a lot of sense
			because otherwise every derived class would have to figure out when
			to use fPreferred, handle the locking and filtering and delete the
			message.  Even if the BeBook didn't say as much, it would make total
			sense to hoist that functionality out of here and into task_looper().
 */
	switch (message->what)
	{
		case _QUIT_:
		{
			// Can't call Quit() to do this, because of the slight chance
			// another thread with have us locked between now and then.
			fTerminating = true;
// bonefish: Now it works straight forward: The thread running here is
// the looper thread, so after returning from DispatchMessage() it will
// quit the dispatching loop, and delete the object in _task0_(), directly
// before dying. Even better, this solution is BApplication friendly as
// Quit() doesn't delete the application object.
// TODO: Remove this.
//			delete this;
			break;
		}

		case B_QUIT_REQUESTED:
		{
			if (handler == this)
			{
				do_quit_requested(message);
				break;
			}
			else
			{
				// fall through
			}
		}

		default:
		{
			handler->MessageReceived(message);
			break;
		}
	}
DBG(OUT("BLooper::DispatchMessage() done\n"));
}
//------------------------------------------------------------------------------
void BLooper::MessageReceived(BMessage* msg)
{
	// TODO: verify
	// The BeBook says this "simply calls the inherited function. ...the BLooper
	// implementation does nothing of importance."  Which is not the same as
	// saying it does nothing.  Investigate.
	BHandler::MessageReceived(msg);
}
//------------------------------------------------------------------------------
BMessage* BLooper::CurrentMessage() const
{
	return fLastMessage;
}
//------------------------------------------------------------------------------
BMessage* BLooper::DetachCurrentMessage()
{
	Lock();
	BMessage* msg = fLastMessage;
	fLastMessage = NULL;
	fQueue->RemoveMessage(msg);
	Unlock();
	return msg;
}
//------------------------------------------------------------------------------
BMessageQueue* BLooper::MessageQueue() const
{
	return fQueue;
}
//------------------------------------------------------------------------------
bool BLooper::IsMessageWaiting() const
{
	if (!IsLocked())
	{
		debugger("The Looper must be locked before calling IsMsgWaiting");
		return false;
	}

	if (!fQueue->IsEmpty())
	{
		return true;
	}

/**
	@note:	What we're doing here differs slightly from the R5 implementation.
			It appears that they probably return count != 0, which gives an
			incorrect true result when port_buffer_size_etc() would block --
			which indicates that the port's buffer is empty, so we should return
			false.  Since we don't actually care about what the error is, we
			just return count > 0.  This has some interesting consequences in
			that we will correctly return 'false' if the port is empty
			(B_WOULD_BLOCK), whereas R5 will return true.  We call that a bug
			where I come from. ;)
 */
	int32 count;
	do
	{
		count = port_buffer_size_etc(fMsgPort, B_RELATIVE_TIMEOUT, 0);
	} while (count == B_INTERRUPTED);

	return count > 0;
}
//------------------------------------------------------------------------------
void BLooper::AddHandler(BHandler* handler)
{
	if (!handler)
	{
		return;
	}

	if (!IsLocked())
	{
		debugger("Looper must be locked before calling AddHandler.");
	}

	if (handler->Looper() == NULL)
	{
		fHandlers.AddItem(handler);
		handler->SetLooper(this);
		if (handler != this)	// avoid a cycle
			handler->SetNextHandler(this);
		BList* Filters = handler->FilterList();
		if (Filters)
		{
			for (int32 i = 0; i < Filters->CountItems(); ++i)
			{
				((BMessageFilter*)Filters->ItemAt(i))->SetLooper(this);
			}
		}
	}
}
//------------------------------------------------------------------------------
bool BLooper::RemoveHandler(BHandler* handler)
{
	// R5 implementation didn't bother to check its params, thus NULL handlers
	// will seg fault.  Bad form, you know.
	if (!handler)
	{
		return false;
	}

	// Correction; testing shows the looper *does* need to be locked for this;
	// it just doesn't use AssertLocked() for that.
	if (!IsLocked())
	{
		debugger("Looper must be locked before calling RemoveHandler.");
	}

	if (handler->Looper() == this && fHandlers.RemoveItem(handler))
	{
		if (handler == fPreferred)
		{
			fPreferred = NULL;
		}

		handler->SetNextHandler(NULL);
		handler->SetLooper(NULL);
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
int32 BLooper::CountHandlers() const
{
	AssertLocked();

	return fHandlers.CountItems();
}
//------------------------------------------------------------------------------
BHandler* BLooper::HandlerAt(int32 index) const
{
	if (!IsLocked())
	{
		debugger("Looper must be locked before calling HandlerAt.");
	}

	return (BHandler*)fHandlers.ItemAt(index);
}
//------------------------------------------------------------------------------
int32 BLooper::IndexOf(BHandler* handler) const
{
	if (!IsLocked())
	{
		debugger("Looper must be locked before calling IndexOf.");
	}

	return fHandlers.IndexOf(handler);
}
//------------------------------------------------------------------------------
BHandler* BLooper::PreferredHandler() const
{
	return fPreferred;
}
//------------------------------------------------------------------------------
void BLooper::SetPreferredHandler(BHandler* handler)
{
	if (handler && handler->Looper() == this && IndexOf(handler) >= 0)
	{
		fPreferred = handler;
	}
	else
	{
		fPreferred = NULL;
	}
}
//------------------------------------------------------------------------------
thread_id BLooper::Run()
{
	AssertLocked();

	if (fRunCalled)
	{
		// Not allowed to call Run() more than once
		debugger("can't call BLooper::Run twice!");
	}

	fTaskID = spawn_thread(_task0_, Name(), fInitPriority, this);

	if (fTaskID == B_NO_MORE_THREADS || fTaskID == B_NO_MEMORY)
	{
		return fTaskID;
	}

	if (fMsgPort == B_NO_MORE_PORTS || fMsgPort == B_BAD_VALUE)
	{
		return fMsgPort;
	}

	fRunCalled = true;
	Unlock();
	status_t err = resume_thread(fTaskID);
	if (err)
	{
		return err;
	}

	return fTaskID;
}
//------------------------------------------------------------------------------
void BLooper::Quit()
{
DBG(OUT("BLooper::Quit()\n"));
	if (!IsLocked())
	{
		const char* name = Name();
		if (!name)
		{
			name = "no-name";
		}
		printf("ERROR - you must Lock a looper before calling Quit(), "
			   "team=%ld, looper=%s\n", Team(), name);
	}

	// Try to lock
	if (!Lock())
	{
		// We're toast already
		return;
	}

DBG(OUT("  is locked\n"));

	if (!fRunCalled)
	{
DBG(OUT("  Run() has not been called yet\n"));
		fTerminating = true;
		delete this;
	} 
	else if (find_thread(NULL) == fTaskID)
	{
DBG(OUT("  We are the looper thread\n"));
		fTerminating = true;
		delete this;
		exit_thread(0);
	}
	else
	{
DBG(OUT("  Run() has already been called and we are not the looper thread\n"));
		// As with sem in _Lock(), we need to cache this here in case the looper
		// disappears before we get to the wait_for_thread() below
		thread_id tid = Thread();

		// bonefish: We need to unlock here. Otherwise the looper thread can't
		// dispatch the _QUIT_ message we're going to post.
//		do {
//			Unlock();
//		} while (IsLocked());
		UnlockFully();

		// As per the BeBook, if we've been called by a thread other than
		// our own, the rest of the message queue has to get processed.  So
		// we put this in the queue, and when it shows up, we'll call Quit()
		// from our own thread.
		// A little testing with BMessageFilter shows _QUIT_ is being used here.
		// I got suspicious when my test QuitRequested() wasn't getting called
		// when Quit() was invoked from another thread.  Makes a nice proof that
		// this is how it's handled, too.
		status_t err;
DBG(OUT("  PostMessage(_QUIT_)...\n"));
//		err = PostMessage(_QUIT_);

BMessage message(_QUIT_);
message.AddInt32("testfield", 42);
err = PostMessage(&message);
DBG(OUT("  ... done: %lx\n", err));

		// There's a possibility that PostMessage() will return B_WOULD_BLOCK
		// because the port is full, so we'll wait a bit and re-post until
		// we won't block.
		while (err == B_WOULD_BLOCK)
		{
			// TODO: test this value; it may be too short
			snooze(10000);
			err = PostMessage(_QUIT_);
		}

		// Also as per the BeBook, we have to wait until the looper is done
		// processing any remaining messages.
		int32 temp;
		do
		{
DBG(OUT("  wait_for_thread(%ld)...\n", tid));
			err = wait_for_thread(tid, &temp);
		} while (err == B_INTERRUPTED);
	}
DBG(OUT("BLooper::Quit() done\n"));
}
//------------------------------------------------------------------------------
bool BLooper::QuitRequested()
{
	return true;
}
//------------------------------------------------------------------------------
bool BLooper::Lock()
{
	// Defer to global _Lock(); see notes there
	return _Lock(this, -1, B_INFINITE_TIMEOUT) == B_OK;
}
//------------------------------------------------------------------------------
void BLooper::Unlock()
{
DBG(OUT("BLooper::Unlock()\n"));
	//	Make sure we're locked to begin with
	AssertLocked();

	//	Decrement fOwnerCount
	--fOwnerCount;
DBG(OUT("  fOwnerCount now: %ld\n", fOwnerCount));
	//	Check to see if the owner still wants a lock
	if (fOwnerCount == 0)
	{
		//	Set fOwner to invalid thread_id (< 0)
		fOwner = -1;

		//	Decrement requested lock count (using fAtomicCount for this)
/*		int32 atomicCount =*/ atomic_add(&fAtomicCount, -1);
DBG(OUT("  fAtomicCount now: %ld\n", fAtomicCount));

		//	Check if anyone is waiting for a lock
// bonefish: Currently _Lock() always acquires the semaphore.
//		if (atomicCount > 0)
		{
			//	release the lock
			release_sem(fLockSem);
		}
	}
DBG(OUT("BLooper::Unlock() done\n"));
}
//------------------------------------------------------------------------------
bool BLooper::IsLocked() const
{
	// We have to lock the list for the call to IsLooperValid().  Has the side
	// effect of not letting the looper get deleted while we're here.
	BObjectLocker<BLooperList> ListLock(gLooperList);

	if (!ListLock.IsLocked())
	{
		// If we can't lock the list, our semaphore is probably toast
		return false;
	}

	if (!IsLooperValid(this))
	{
		// The looper is gone, so of course it's not locked
		return false;
	}

	// Got this from Jeremy's BLocker implementation
	return find_thread(NULL) == fOwner;
}
//------------------------------------------------------------------------------
status_t BLooper::LockWithTimeout(bigtime_t timeout)
{
	return _Lock(this, -1, timeout);
}
//------------------------------------------------------------------------------
thread_id BLooper::Thread() const
{
	return fTaskID;
}
//------------------------------------------------------------------------------
team_id BLooper::Team() const
{
	return sTeamID;
}
//------------------------------------------------------------------------------
BLooper* BLooper::LooperForThread(thread_id tid)
{
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		return gLooperList.LooperForThread(tid);
	}

	return NULL;
}
//------------------------------------------------------------------------------
thread_id BLooper::LockingThread() const
{
	return fOwner;
}
//------------------------------------------------------------------------------
int32 BLooper::CountLocks() const
{
	return fOwnerCount;
}
//------------------------------------------------------------------------------
int32 BLooper::CountLockRequests() const
{
	return fAtomicCount;
}
//------------------------------------------------------------------------------
sem_id BLooper::Sem() const
{
	return fLockSem;
}
//------------------------------------------------------------------------------
BHandler* BLooper::ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* property)
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
	BPropertyInfo PropertyInfo(gLooperPropInfo);
	uint32 data;
	status_t err = B_OK;
	const char* errMsg = "";
	if (PropertyInfo.FindMatch(msg, index, specifier, form, property, &data) >= 0)
	{
		switch (data)
		{
			case BLOOPER_PROCESS_INTERNALLY:
				return this;

			case BLOOPER_HANDLER_BY_INDEX:
			{
				int32 index = specifier->FindInt32("index");
				if (form == B_REVERSE_INDEX_SPECIFIER)
				{
					index = CountHandlers() - index;
				}
				BHandler* target = HandlerAt(index);
				if (target)
				{
					// Specifier has been fully handled
					msg->PopSpecifier();
					return target;
				}
				else
				{
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
	}
	else
	{
		return BHandler::ResolveSpecifier(msg, index, specifier, form,
										  property);
	}

	BMessage Reply(B_MESSAGE_NOT_UNDERSTOOD);
	Reply.AddInt32("error", err);
	Reply.AddString("message", errMsg);
	msg->SendReply(&Reply);

	return NULL;
}
//------------------------------------------------------------------------------
status_t BLooper::GetSupportedSuites(BMessage* data)
{
	status_t err = B_OK;
	
	if (!data)
	{
		err = B_BAD_VALUE;
	}

	if (!err)
	{
		err = data->AddString("Suites", "suite/vnd.Be-handler");
		if (!err)
		{
			BPropertyInfo PropertyInfo(gLooperPropInfo);
			err = data->AddFlat("message", &PropertyInfo);
			if (!err)
			{
				err = BHandler::GetSupportedSuites(data);
			}
		}
	}

	return err;
}
//------------------------------------------------------------------------------
void BLooper::AddCommonFilter(BMessageFilter* filter)
{
	if (!filter)
	{
		return;
	}

	if (!IsLocked())
	{
		debugger("Owning Looper must be locked before calling AddCommonFilter");
		return;
	}

	if (filter->Looper())
	{
		debugger("A MessageFilter can only be used once.");
		return;
	}

	if (!fCommonFilters)
	{
		fCommonFilters = new BList(FILTER_LIST_BLOCK_SIZE);
	}
	filter->SetLooper(this);
	fCommonFilters->AddItem(filter);
}
//------------------------------------------------------------------------------
bool BLooper::RemoveCommonFilter(BMessageFilter* filter)
{
	if (!IsLocked())
	{
		debugger("Owning Looper must be locked before calling "
				 "RemoveCommonFilter");
		return false;
	}

	if (!fCommonFilters)
	{
		return false;
	}

	bool result = fCommonFilters->RemoveItem(filter);
	if (result)
	{
		filter->SetLooper(NULL);
	}

	return result;
}
//------------------------------------------------------------------------------
void BLooper::SetCommonFilterList(BList* filters)
{
	// We have a somewhat serious problem here.  It is entirely possible in R5
	// to assign a given list of filters to *two* BLoopers simultaneously.  This
	// becomes problematic when the loopers are destroyed: the last looper
	// destroyed will have a problem when it tries to delete a filter list that
	// has already been deleted.  In R5, this results in a general protection
	// fault.  We fix this by checking the filter list for ownership issues.

	if (!IsLocked())
	{
		debugger("Owning Looper must be locked before calling "
				 "SetCommonFilterList");
		return;
	}

	BMessageFilter* filter;
	if (filters)
	{
		// Check for ownership issues
		for (int32 i = 0; i < filters->CountItems(); ++i)
		{
			filter = (BMessageFilter*)filters->ItemAt(i);
			if (filter->Looper())
			{
				debugger("A MessageFilter can only be used once.");
				return;
			}
		}
	}
	
	if (fCommonFilters)
	{
		for (int32 i = 0; i < fCommonFilters->CountItems(); ++i)
		{
			delete (BMessageFilter*)fCommonFilters->ItemAt(i);
		}

		delete fCommonFilters;
		fCommonFilters = NULL;
	}

	// Per the BeBook, we take ownership of the list
	fCommonFilters = filters;
	if (fCommonFilters)
	{
		for (int32 i = 0; i < fCommonFilters->CountItems(); ++i)
		{
			filter = (BMessageFilter*)fCommonFilters->ItemAt(i);
			filter->SetLooper(this);
		}
	}
}
//------------------------------------------------------------------------------
BList* BLooper::CommonFilterList() const
{
	return fCommonFilters;
}
//------------------------------------------------------------------------------
status_t BLooper::Perform(perform_code d, void* arg)
{
	// This is sort of what we're doing for this function everywhere
	return BHandler::Perform(d, arg);
}
//------------------------------------------------------------------------------
BMessage* BLooper::MessageFromPort(bigtime_t timeout)
{
	return ReadMessageFromPort(timeout);
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper1()
{
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper2()
{
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper3()
{
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper4()
{
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper5()
{
}
//------------------------------------------------------------------------------
void BLooper::_ReservedLooper6()
{
}
//------------------------------------------------------------------------------
BLooper::BLooper(const BLooper&)
{
	// Copy construction not allowed
}
//------------------------------------------------------------------------------
BLooper& BLooper::operator=(const BLooper& )
{
	// Looper copying not allowed
	return *this;
}
//------------------------------------------------------------------------------
BLooper::BLooper(int32 priority, port_id port, const char* name)
{
	// This must be a legacy constructor
	fMsgPort = port;
	InitData(name, priority, B_LOOPER_PORT_DEFAULT_CAPACITY);
}
//------------------------------------------------------------------------------
status_t BLooper::_PostMessage(BMessage* msg, BHandler* handler,
							   BHandler* reply_to)
{
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (!ListLock.IsLocked())
	{
		return B_BAD_VALUE;
	}

	if (!IsLooperValid(this))
	{
		return B_BAD_VALUE;
	}

	// Does handler belong to this looper?
	if (handler && handler->Looper() != this)
	{
		return B_MISMATCHED_VALUES;
	}

	status_t err = B_OK;

	BMessenger Messenger(handler, this, &err);

	if (!err)
	{
		err = Messenger.SendMessage(msg, reply_to, 0);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BLooper::_Lock(BLooper* loop, port_id port, bigtime_t timeout)
{
DBG(OUT("BLooper::_Lock(%p, %lx)\n", loop, port));
/**
	@note	The assumption I'm under here is that since we can get the port of
			the BLooper directly from the BLooper itself, the port parameter is
			for identifying BLoopers by port_id when a pointer to the BLooper in
			question is not available.  So this function has two modes:
				o When loop != NULL, use it directly
				o When loop == NULL and port is valid, use the port_id to get
				  the looper
			I scoured the docs to find out what constitutes a valid port_id to
			no avail.  Since create_port uses the standard error values in its
			returned port_id, I'll assume that anything less than zero is a safe
			bet as an *invalid* port_id.  I'm guessing that, like thread and
			semaphore ids, anything >= zero is valid.  So, the short version of
			this reads: if you don't want to find by port_id, make port = -1.

			Another assumption I'm making is that Lock() and LockWithTimeout()
			are covers for this function.  If it turns out that we don't really
			need this function, I may refactor this code into LockWithTimeout()
			and have Lock() call it instead.  This function could then be
			removed.
 */

	//	Check params (loop, port)
	if (!loop && port < 0)
	{
DBG(OUT("BLooper::_Lock() done 1\n"));
		return B_BAD_VALUE;
	}

	// forward declared so I can use BAutolock on sLooperListLock
	thread_id curThread;
	sem_id sem;

/**
	@note	We lock the looper list at the start of the lock operation to
			prevent the looper getting removed from the list while we're
			doing list operations.  Also ensures that the looper doesn't
			get deleted here (since ~BLooper() has to lock the list as
			well to remove itself).
 */
	{
		BObjectLocker<BLooperList> ListLock(gLooperList);
		if (!ListLock.IsLocked())
		{
			// If we can't lock, the semaphore is probably
			// gone, which leaves us in no-man's land
DBG(OUT("BLooper::_Lock() done 2\n"));
			return B_BAD_VALUE;
		}
	
		//	Look up looper by port_id, if necessary
		if (!loop)
		{
			loop = LooperForPort(port);
			if (!loop)
			{
DBG(OUT("BLooper::_Lock() done 3\n"));
				return B_BAD_VALUE;
			}
		}
		else
		{
			//	Check looper validity
			if (!IsLooperValid(loop))
			{
DBG(OUT("BLooper::_Lock() done 4\n"));
				return B_BAD_VALUE;
			}
		}
	
		//	Is the looper trying to lock itself?
		//	Check for nested lock attempt
		curThread = find_thread(NULL);
		if (curThread == loop->fOwner)
		{
			//	Bump fOwnerCount
			++loop->fOwnerCount;
DBG(OUT("BLooper::_Lock() done 5: fOwnerCount: %ld\n", loop->fOwnerCount));
			return B_OK;
		}
	
		//	Something external to the looper is attempting to lock
		//	Cache the semaphore
		sem = loop->fLockSem;
	
		//	Validate the semaphore
		if (sem < 0)
		{
DBG(OUT("BLooper::_Lock() done 6\n"));
			return B_BAD_VALUE;
		}
	
		//	Bump the requested lock count (using fAtomicCount for this)
		atomic_add(&loop->fAtomicCount, 1);

		// sLooperListLock automatically released here
	}

/**
	@note	We have to operate with the looper list unlocked during semaphore
			acquisition so that the rest of the application doesn't have to
			wait for this lock to happen.  This is why we cached fLockSem
			earlier -- with the list unlocked, the looper might get deleted
			right out from under us.  This is also why we use a raw semaphore
			instead of the easier-to-deal-with BLocker; you can't cache a
			BLocker.
 */
	//	acquire the lock
	status_t err;
	do
	{
		err = acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
	} while (err == B_INTERRUPTED);

	if (!err)
	{
		//		Assign current thread to fOwner
		loop->fOwner = curThread;
		//		Reset fOwnerCount to 1
		loop->fOwnerCount = 1;
	}

DBG(OUT("BLooper::_Lock() done: %lx\n", err));
	return err;
}
//------------------------------------------------------------------------------
status_t BLooper::_LockComplete(BLooper* loop, int32 old, thread_id this_tid,
								sem_id sem, bigtime_t timeout)
{
	// What is this for?  Hope I'm not missing something conceptually here ...
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BLooper::InitData()
{
	fOwner = B_ERROR;
	fRunCalled = false;
	fQueue = new BMessageQueue();
	fCommonFilters = NULL;
	fLastMessage = NULL;
	fPreferred = NULL;
	fTaskID = B_ERROR;
	fTerminating = false;
	fMsgPort = -1;

	if (sTeamID == -1)
	{
		thread_info info;
		get_thread_info(find_thread(NULL), &info);
		sTeamID = info.team;
	}

}
//------------------------------------------------------------------------------
void BLooper::InitData(const char* name, int32 priority, int32 port_capacity)
{
	InitData();

	fLockSem = create_sem(1, name);

	if (port_capacity <= 0)
	{
		port_capacity = B_LOOPER_PORT_DEFAULT_CAPACITY;
	}

	fMsgPort = create_port(port_capacity, name ? name : "LooperPort");

	fInitPriority = priority;

	BObjectLocker<BLooperList> ListLock(gLooperList);
	AddLooper(this);
	AddHandler(this);
}
//------------------------------------------------------------------------------
void BLooper::AddMessage(BMessage* msg)
{
	// NOTE: Why is this here?
}
//------------------------------------------------------------------------------
void BLooper::_AddMessagePriv(BMessage* msg)
{
	// NOTE: No, really; why the hell is this here??
}
//------------------------------------------------------------------------------
status_t BLooper::_task0_(void* arg)
{
DBG(OUT("LOOPER: _task0_()\n"));
	BLooper* obj = (BLooper*)arg;

DBG(OUT("LOOPER: locking looper...\n"));
	if (obj->Lock())
	{
DBG(OUT("LOOPER: looper locked\n"));
		obj->task_looper();
		delete obj;
	}

DBG(OUT("LOOPER: _task0_() done: thread %ld\n", find_thread(NULL)));
	return B_OK;
}
//------------------------------------------------------------------------------

void *
BLooper::ReadRawFromPort(int32 *msgCode, bigtime_t timeout)
{
	DBG(OUT("BLooper::ReadRawFromPort()\n"));
	int8 *msgBuffer = NULL;
	ssize_t bufferSize;

	do {
		bufferSize = port_buffer_size_etc(fMsgPort, B_RELATIVE_TIMEOUT, timeout);
	} while (bufferSize == B_INTERRUPTED);

	if (bufferSize < B_OK) {
		DBG(OUT("BLooper::ReadRawFromPort(): failed: %ld\n", bufferSize));
		return NULL;
	}

	if (bufferSize > 0)
		msgBuffer = new int8[bufferSize];

	// we don't want to wait again here, since that can only mean
	// that someone else has read our message and our bufferSize
	// is now probably wrong
	DBG(OUT("read_port()...\n"));
	bufferSize = read_port_etc(fMsgPort, msgCode, msgBuffer, bufferSize,
					  B_RELATIVE_TIMEOUT, 0);
	if (bufferSize < B_OK) {
		delete[] msgBuffer;
		return NULL;
	}
	DBG(OUT("BLooper::ReadRawFromPort() read: %.4s\n", (char *)msgCode));
	DBG(OUT("BLooper::ReadRawFromPort() done: %p\n", msgBuffer));
	return msgBuffer;
}

//------------------------------------------------------------------------------
BMessage* BLooper::ReadMessageFromPort(bigtime_t tout)
{
DBG(OUT("BLooper::ReadMessageFromPort()\n"));
	int32 msgcode;
	BMessage* bmsg;

	void* msgbuffer = ReadRawFromPort(&msgcode, tout);

	bmsg = ConvertToMessage(msgbuffer, msgcode);

	if (msgbuffer)
	{
		delete[] (int8*)msgbuffer;
	}

DBG(OUT("BLooper::ReadMessageFromPort() done: %p\n", bmsg));
	return bmsg;
}
//------------------------------------------------------------------------------
BMessage* BLooper::ConvertToMessage(void* raw, int32 code)
{
DBG(OUT("BLooper::ConvertToMessage()\n"));
	BMessage* bmsg = new BMessage(code);

	if (raw != NULL)
	{
		if (bmsg->Unflatten((const char*)raw) != B_OK)
		{
DBG(OUT("BLooper::ConvertToMessage(): unflattening message failed\n"));
			delete bmsg;
			bmsg = NULL;
		}
	}

DBG(OUT("BLooper::ConvertToMessage(): %p\n", bmsg));
	return bmsg;
}
//------------------------------------------------------------------------------
void BLooper::task_looper()
{
DBG(OUT("BLooper::task_looper()\n"));
	//	Check that looper is locked (should be)
	AssertLocked();
	//	Unlock the looper
	Unlock();

	//	loop: As long as we are not terminating.
	while (!fTerminating)
	{
DBG(OUT("LOOPER: outer loop\n"));
		// TODO: timeout determination algo
		//	Read from message port (how do we determine what the timeout is?)
DBG(OUT("LOOPER: MessageFromPort()...\n"));
		BMessage* msg = MessageFromPort();
DBG(OUT("LOOPER: ...done\n"));

		//	Did we get a message?
		if (msg)
		{
DBG(OUT("LOOPER: got message\n"));
			//	Add to queue
			fQueue->AddMessage(msg);
		}
else
DBG(OUT("LOOPER: got no message\n"));

		//	Get message count from port
		int32 msgCount = port_count(fMsgPort);
		for (int32 i = 0; i < msgCount; ++i)
		{
			//	Read 'count' messages from port (so we will not block)
			//	We use zero as our timeout since we know there is stuff there
			msg = MessageFromPort(0);
			//	Add messages to queue
			if (msg)
			{
				fQueue->AddMessage(msg);
			}
		}

		//	loop: As long as there are messages in the queue and the port is
		//		  empty... and we are not terminating, of course.
		bool dispatchNextMessage = true;
		while (!fTerminating && dispatchNextMessage)
		{
DBG(OUT("LOOPER: inner loop\n"));
			//	Get next message from queue (assign to fLastMessage)
			fLastMessage = fQueue->NextMessage();

			//	Lock the looper
			Lock();
			if (!fLastMessage)
			{
				// No more messages: Unlock the looper and terminate the
				// dispatch loop.
				dispatchNextMessage = false;
			}
			else
			{
DBG(OUT("LOOPER: fLastMessage: 0x%lx: %.4s\n", fLastMessage->what,
(char*)&fLastMessage->what));
DBG(fLastMessage->PrintToStream());
				//	Get the target handler
				//	Use BMessage friend functions to determine if we are using the
				//	preferred handler, or if a target has been specified
				BHandler* handler;
				if (_use_preferred_target_(fLastMessage))
				{
DBG(OUT("LOOPER: use preferred target\n"));
					handler = fPreferred;
				}
				else
				{
DBG(OUT("LOOPER: don't use preferred target\n"));
					/**
						@note	Here is where all the token stuff starts to
								make sense.  How, exactly, do we determine
								what the target BHandler is?  If we look at
								BMessage, we see an int32 field, fTarget.
								Amazingly, we happen to have a global mapping
								of BHandler pointers to int32s!
					 */
DBG(OUT("LOOPER: use: %ld\n", _get_message_target_(fLastMessage)));
					 gDefaultTokens.GetToken(_get_message_target_(fLastMessage),
					 						 B_HANDLER_TOKEN,
					 						 (void**)&handler);
DBG(OUT("LOOPER: handler: %p, this: %p\n", handler, this));
				}

				if (!handler)
				{
DBG(OUT("LOOPER: no target handler, use this\n"));
					handler = this;
				}

				//	Is this a scripting message? (BMessage::HasSpecifiers())
				if (fLastMessage->HasSpecifiers())
				{
					int32 index = 0;
					// Make sure the current specifier is kosher
					if (fLastMessage->GetCurrentSpecifier(&index) == B_OK)
					{
						handler = resolve_specifier(handler, fLastMessage);
					}
				}
				else
				{
					DBG(OUT("LOOPER: no scripting message\n"));
				}
				
				if (handler)
				{
					//	Do filtering
					handler = top_level_filter(fLastMessage, handler);
DBG(OUT("LOOPER: top_level_filter(): %p\n", handler));
					if (handler && handler->Looper() == this)
					{
						DispatchMessage(fLastMessage, handler);
					}
				}
			}

			//	Unlock the looper
			Unlock();

			//	Delete the current message (fLastMessage)
			if (fLastMessage)
			{
				delete fLastMessage;
				fLastMessage = NULL;
			}

			//	Are any messages on the port?
			if (port_count(fMsgPort) > 0)
			{
				//	Do outer loop
				dispatchNextMessage = false;
			}
		}
	}
DBG(OUT("BLooper::task_looper() done\n"));
}
//------------------------------------------------------------------------------
void BLooper::do_quit_requested(BMessage* msg)
{
/**
	@note	I couldn't figure out why do_quit_requested() was necessary; why not
			just call Quit()?  Then, while writing the PostMessage() code, I
			realized that the sender of the B_QUIT_REQUESTED message just might
			be waiting for a reply.  A quick test, and yes, we get a reply
			which consists of:
				what: B_REPLY
				"result" (bool) return of QuitRequested()
				"thread" (int32) the looper's thread id

			While Quit() could use fLastMessage, it makes more sense that
			do_quit_requested() would handle it since it gets passed the
			message.
 */

	bool isQuitting = QuitRequested();

	if (msg->IsSourceWaiting())
	{
		BMessage ReplyMsg(B_REPLY);
		ReplyMsg.AddBool("result", isQuitting);
		ReplyMsg.AddInt32("thread", fTaskID);
		msg->SendReply(&ReplyMsg);
	}

	if (isQuitting)
	{
		Quit();
	}
}
//------------------------------------------------------------------------------
bool BLooper::AssertLocked() const
{
	if (!IsLocked())
	{
		debugger("looper must be locked before proceeding\n");
		return false;
	}

	return true;
}
//------------------------------------------------------------------------------
BHandler* BLooper::top_level_filter(BMessage* msg, BHandler* target)
{
	if (msg)
	{
		// Apply the common filters first
		target = apply_filters(CommonFilterList(), msg, target);
		if (target)
		{
			if (target->Looper() != this)
			{
				debugger("Targeted handler does not belong to the looper.");
				target = NULL;
			}
			else
			{
				// Now apply handler-specific filters
				target = handler_only_filter(msg, target);
			}
		}
	}

	return target;
}
//------------------------------------------------------------------------------
BHandler* BLooper::handler_only_filter(BMessage* msg, BHandler* target)
{
	// Keep running filters until our handler is NULL, or until the filtering
	// handler returns itself as the designated handler
	BHandler* oldTarget = NULL;
	while (target && (target != oldTarget))
	{
		oldTarget = target;
		target = apply_filters(oldTarget->FilterList(), msg, oldTarget);
		if (target && (target->Looper() != this))
		{
			debugger("Targeted handler does not belong to the looper.");
			target = NULL;
		}
	}

	return target;
}
//------------------------------------------------------------------------------
BHandler* BLooper::apply_filters(BList* list, BMessage* msg, BHandler* target)
{
	// This is where the action is!
	// Check the parameters
	if (!list || !msg)
	{
		return target;
	}

	// For each filter in the provided list
	BMessageFilter* filter = NULL;
	for (int32 i = 0; i < list->CountItems(); ++i)
	{
		filter = (BMessageFilter*)list->ItemAt(i);

		// Check command conditions
		if (filter->FiltersAnyCommand() || (filter->Command() == msg->what))
		{
			// Check delivery conditions
			message_delivery delivery = filter->MessageDelivery();
			bool dropped = msg->WasDropped();
			if (delivery == B_ANY_DELIVERY ||
				((delivery == B_DROPPED_DELIVERY) && dropped) ||
				((delivery == B_PROGRAMMED_DELIVERY) && !dropped))
			{
				// Check source conditions
				message_source source = filter->MessageSource();
				bool remote = msg->IsSourceRemote();
				if (source == B_ANY_SOURCE ||
					((source == B_REMOTE_SOURCE) && remote) ||
					((source == B_LOCAL_SOURCE) && !remote))
				{
					filter_result result;
					// Are we using an "external" function?
					filter_hook func = filter->FilterFunction();
					if (func)
					{
						result = func(msg, &target, filter);
					}
					else
					{
						result = filter->Filter(msg, &target);
					}

					// Is further processing allowed?
					if (result == B_SKIP_MESSAGE)
					{
						// No; time to bail out
						return NULL;
					}
				}
			}
		}
	}

	return target;
}
//------------------------------------------------------------------------------
void BLooper::check_lock()
{
	// NOTE: any use for this?
}
//------------------------------------------------------------------------------
BHandler* BLooper::resolve_specifier(BHandler* target, BMessage* msg)
{
	// Check params
	if (!target || !msg)
	{
		return NULL;
	}

	int32 index;
	BMessage specifier;
	int32 form;
	const char* property;
	status_t err = B_OK;
	BHandler* newTarget = target;
	//	Loop to deal with nested specifiers
	//	(e.g., the 3rd button on the 4th view)
	do
	{
		err = msg->GetCurrentSpecifier(&index, &specifier, &form, &property);
		if (err)
		{
			BMessage reply(B_REPLY);
			reply.AddInt32("error", err);
			msg->SendReply(&reply);
			return NULL;
		}
		//	Current target gets what was the new target
		target = newTarget;
		newTarget = target->ResolveSpecifier(msg, index, &specifier, form,
											 property);
		//	Check that new target is owned by looper;
		//	use IndexOf() to avoid dereferencing newTarget
		//	(possible race condition with object destruction
		//	by another looper)
		if (!newTarget || IndexOf(newTarget) < 0)
		{
			return NULL;
		}

		//	Get current specifier index (may change in ResolveSpecifier())
		msg->GetCurrentSpecifier(&index);
	} while (newTarget && newTarget != target && index >= 0);

	return newTarget;
}
//------------------------------------------------------------------------------
void BLooper::UnlockFully()
{
	AssertLocked();

/**
	@note	What we're doing here is completely undoing the current owner's lock
			on the looper.  This is actually pretty easy, since the owner only
			has a single aquisition on the semaphore; every subsequent "lock"
			is just an increment to the owner count.  The whole thing is quite
			similar to Unlock(), except that we clear the ownership variables,
			rather than merely decrementing them.
 */
	// Clear the owner count
	fOwnerCount = 0;
	// Nobody owns the lock now
	fOwner = -1;
	// There is now one less thread holding a lock on this looper
// bonefish: Currently _Lock() always acquires the semaphore.
/*	long atomicCount = */atomic_add(&fAtomicCount, -1);
//	if (atomicCount > 0)
	{
		release_sem(fLockSem);
	}
}
//------------------------------------------------------------------------------
void BLooper::AddLooper(BLooper* loop)
{
	if (gLooperList.IsLocked())
	{
		gLooperList.AddLooper(loop);
	}
}
//------------------------------------------------------------------------------
bool BLooper::IsLooperValid(const BLooper* l)
{
	if (gLooperList.IsLocked())
	{
		return gLooperList.IsLooperValid(l);
	}

	return false;
}
//------------------------------------------------------------------------------
void BLooper::RemoveLooper(BLooper* l)
{
	if (gLooperList.IsLocked())
	{
		gLooperList.RemoveLooper(l);
	}
}
//------------------------------------------------------------------------------
void BLooper::GetLooperList(BList* list)
{
	BObjectLocker<BLooperList> ListLock(gLooperList);
	if (ListLock.IsLocked())
	{
		gLooperList.GetLooperList(list);
	}
}
//------------------------------------------------------------------------------
BLooper* BLooper::LooperForName(const char* name)
{
	if (gLooperList.IsLocked())
	{
		return gLooperList.LooperForName(name);
	}

	return NULL;
}
//------------------------------------------------------------------------------
BLooper* BLooper::LooperForPort(port_id port)
{
	if (gLooperList.IsLocked())
	{
		return gLooperList.LooperForPort(port);
	}

	return NULL;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
port_id _get_looper_port_(const BLooper* looper)
{
	return looper->fMsgPort;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

