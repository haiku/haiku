//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku
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
//	File Name:		Handler.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BHandler defines the message-handling protocol.
//					MessageReceived() is its lynchpin.
//------------------------------------------------------------------------------
/**
	@note	Some musings on the member variable 'fToken'.  In searching a dump
			of libbe.so, I found a struct/class called 'TokenSpace', which has
			various member functions like 'new_token'.  More intriguing is a
			dump of Dano's version of libbe.so:  there is BPrivate::BTokenSpace,
			which also has functions like 'NewToken'.  There's more: one
			version of BTokenSpace::NewToken takes a pointer to another new
			class, BPrivate::BDirectMessageTarget.  Only a constructor,
			destructor and vtable are listed for it, so I'm guessing it's an
			abstract base class (or not very useful ;).  My guess is that
			BDirectMessageTarget facilitates sending messages straight to an
			associated BHandler.  Maybe from app_server?  Probably not, since
			you'd have to do IPC anyway.  But maybe within the same team, or
			even between teams?  It might save unnecessary trips to and from
			app_server for messages which otherwise are entirely client-side.
			
			Back to tokens. There are also these functions in R5:
				_safe_get_server_token_(const BLooper*, int32*)
				BWindow::find_token_and_handler(BMessage*, int32*, BHandler**)
				BWindow::get_server_token

			and Dano adds a few more provocative sounding functions:
				get_handler_token(short, void*) (listed 3 times, actually)
				new_handler_token(short, void*)
				remove_handler_token(short, void*)

			Taken all together, I think there's a sound argument to be made that
			each BHandler has an int32 token associated with it in app_server.

			Furthermore, there is, in R5, a function set_token_type(long, short)
			which leads me to think that although BHandler's have server tokens
			associated with them, the tokening facility is, in fact, generic.
			These functions would seem to support that theory:
				BBitmap::get_server_token
				BPicture::set_token

			An important question is whether tokens are generated on the client
			side and registered with the server or generated on the server and
			given back to the client.  The exported functions of TokenSpace in
			libbe's dump are:
				~TokenSpace
				TokenSpace
				adjust_free(long, long)
				dump()
				find_free_entry(long*, long*)
				full_search_adjust()
				get_token(void**)
				get_token(short*, void**)
				new_token(long, short, void*)
				new_token_array(long)
				remove_token(long)
				set_token_type(long, short)

			TokenSpace functions are also exported from app_server:
				~TokenSpace
				TokenSpace
				adjust_free(long, long)
				cleanup_dead(long)
				delete_atom(SAtom*)
				dump_tokens()
				find_free_entry(long*, long*)
				full_search_adjust()
				get_token(long, void**)
				get_token(long, short*, void**)
				get_token_by_type(int*, short, long, void**)
				grab_atom(long, SAtom**)
				iterate_tokens(long, short,
							   unsigned long(*)(long, long, short, void*, void*),
							   void*)
				new_token(long, short, void*)
				new_token_array(long)
				remove_token(long)
				set_token_type(long, short)

			While there are common functions in both locations, the fact that
			app_server exports TokenSpace functions which libbe.so does not
			leads me to believe that libbe.so and app_server each have their own
			versions of TokenSpace.  While it's possible that the libbe version
			simply acts as a proxy for the app_server version, it seems not
			only inefficient but unnecessary as well:  client-side objects
			exists in a different "namespace" than server-side objects, so over-
			lap between the two token sets shouldn't be an issue.

			Obviously, I can't be entirely sure this is how R5 does it, but it
			seems like a reasonable design to follow for our purposes.
 */

/**
	@note	Thought on "pseudo-atomic" operations in Lock(), LockWithTimeout(),
			and Unlock().  Seems like avoiding the possibility of a looper
			change during these functions would be the way to go, and having a
			semaphore that protects SetLooper() would do the job very nicely.
			Maybe that's too heavy-weight a solution, though.
 */

// Standard Includes -----------------------------------------------------------
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>

// System Includes -------------------------------------------------------------
#include <AppDefs.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <PropertyInfo.h>

// Project Includes ------------------------------------------------------------
#include <TokenSpace.h>

using std::map;
using std::vector;
using BPrivate::gDefaultTokens;


static const char *kArchiveNameField = "_name";

static property_info sHandlerPropInfo[] = {
	{
		"Suites",					// name
		{ B_GET_PROPERTY },			// commands
		{ B_DIRECT_SPECIFIER },		// specifiers
		NULL,						// usage
		0,							// extra data
		{ 0 },						// types
		{							// ctypes (compound_type)
			{						// ctypes[0]
				{					// pairs[0]
					{
						"suites",		// name
						B_STRING_TYPE	// type
					} 
				}
			},
			{						// ctypes[1]
				{					// pairs[0]
					{
						"messages",
						B_PROPERTY_INFO_TYPE
					}
				}
			}
		},
		{}		// reserved
	},
	{
		"Messenger",
			{ B_GET_PROPERTY },
			{ B_DIRECT_SPECIFIER },
			NULL, 0,
			{ B_MESSENGER_TYPE },
			{},
			{}
	},
	{
		"InternalName",
			{ B_GET_PROPERTY },
			{ B_DIRECT_SPECIFIER },
			NULL, 0,
			{ B_STRING_TYPE },
			{},
			{}
	},
	{}
};

bool FilterDeleter(void *filter);

typedef map<unsigned long, vector<BHandler *> >	THandlerObserverMap;
typedef map<unsigned long, vector<BMessenger> >	TMessengerObserverMap;

//------------------------------------------------------------------------------
// TODO: Change to BPrivate::BObserverList if possible
class _ObserverList {
	public:
		_ObserverList(void);
		~_ObserverList(void);
		status_t SendNotices(unsigned long, BMessage const *);
		status_t StartObserving(BHandler *, unsigned long);
		status_t StartObserving(const BMessenger&, unsigned long);
		status_t StopObserving(BHandler *, unsigned long);
		status_t StopObserving(const BMessenger&, unsigned long);
		bool IsEmpty();

	private:
		THandlerObserverMap		fHandlerMap;
		TMessengerObserverMap	fMessengerMap;
};


BHandler::BHandler(const char *name)
	: BArchivable(),
	fName(NULL)
{
	InitData(name);
}


BHandler::~BHandler()
{
	free(fName);
	gDefaultTokens.RemoveToken(fToken);
}


BHandler::BHandler(BMessage *data)
	: BArchivable(data),
	fName(NULL)
{
	const char *name = NULL;

	if (data)
		data->FindString(kArchiveNameField, &name);

	InitData(name);
}


BArchivable *
BHandler::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BHandler"))
		return NULL;

	return new BHandler(data);
}


status_t
BHandler::Archive(BMessage *data, bool deep) const
{
	status_t status = BArchivable::Archive(data, deep);
	if (status < B_OK)
		return status;

	return data->AddString(kArchiveNameField, fName);
}


void
BHandler::MessageReceived(BMessage *message)
{
	BMessage reply(B_REPLY);

	switch (message->what) {
		// ToDo: am I missing something or is the "observed" stuff handshake completely missing?

		case B_GET_PROPERTY:
		{
			int32 cur;
			BMessage specifier;
			int32 form;
			const char *prop;

			status_t err = message->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (err == B_OK) {
				bool known = false;
				if (strcmp(prop, "Suites") == 0) {
					err = GetSupportedSuites(&reply);
					known = true;
				} else if (strcmp(prop, "Messenger") == 0) {
					err = reply.AddMessenger("result", this);
					known = true;
				} else if (strcmp(prop, "InternalName") == 0) {
					err = reply.AddString("result", Name());
					known = true;
				}

				if (known) {
					reply.AddInt32("error", B_OK);
					message->SendReply(&reply);
					return;
				}
				// let's try next handler
			}
			break;
		}

		case B_GET_SUPPORTED_SUITES:
		{
			reply.AddInt32("error", GetSupportedSuites(&reply));
			message->SendReply(&reply);
			return;
		}
	}

	// ToDo: there is some more work needed here (someone in the know should fill in)!

	if (fNextHandler) {
		// we need to apply the next handler's filters here, too
		BHandler* target = Looper()->_HandlerFilter(message, fNextHandler);
		if (target != NULL && target != this) {
			// TODO: we also need to make sure that "target" is not before
			//	us in the handler chain - at least in case it wasn't before
			//	the handler actually targeted with this message - this could
			//	get ugly, though.
			target->MessageReceived(message);
		}
	} else if (message->what != B_MESSAGE_NOT_UNDERSTOOD
		&& (message->WasDropped() || message->HasSpecifiers())) {
		printf("BHandler::MessageReceived(): B_MESSAGE_NOT_UNDERSTOOD");
		message->PrintToStream();
		message->SendReply(B_MESSAGE_NOT_UNDERSTOOD);
	}
}


BLooper *
BHandler::Looper() const
{
	return fLooper;
}


void
BHandler::SetName(const char *name)
{
	if (fName != NULL) {
		free(fName);
		fName = NULL;
	}

	if (name != NULL)
		fName = strdup(name);
}


const char *
BHandler::Name() const
{
	return fName;
}


void
BHandler::SetNextHandler(BHandler *handler)
{
	if (!fLooper) {
		debugger("handler must belong to looper before setting NextHandler");
		fNextHandler = NULL;
		return;
	}

	if (!fLooper->IsLocked()) {
		debugger("The handler's looper must be locked before setting NextHandler");
		return;
	}

	if (handler && fLooper != handler->Looper()) {
		debugger("The handler and its NextHandler must have the same looper");
		return;
	}

	// NOTE: I'm sure some sort of threading protection should happen here,
	// hopefully the spec-mandated BLooper lock is sufficient.
	// TODO: implement correctly
	fNextHandler = handler;
}


BHandler *
BHandler::NextHandler() const
{
	return fNextHandler;
}


void
BHandler::AddFilter(BMessageFilter *filter)
{
	// NOTE:  Although the documentation states that the handler must belong to
	// a looper and the looper must be locked in order to use this method,
	// testing shows that this is not the case in the original implementation.
	// We may want to investigate enforcing these rules; it would be interesting
	// to see how many apps out there have violated the dictates of the docs.
	// For now, though, we'll play nicely.
#if 0
	if (!fLooper)
	{
		// TODO: error handling
		return false;
	}

	if (!fLooper->IsLocked())
	{
		// TODO: error handling
		return false;
	}
#endif

	if (fLooper != NULL)
		filter->SetLooper(fLooper);
		
	if (!fFilters)
		fFilters = new BList;
		
	fFilters->AddItem(filter);
}


bool
BHandler::RemoveFilter(BMessageFilter *filter)
{
	// NOTE:  Although the documentation states that the handler must belong to
	// a looper and the looper must be locked in order to use this method,
	// testing shows that this is not the case in the original implementation.
	// We may want to investigate enforcing these rules; it would be interesting
	// to see how many apps out there have violated the dictates of the docs.
	// For now, though, we'll play nicely.
#if 0
	if (!fLooper)
	{
		// TODO: error handling
		return false;
	}

	if (!fLooper->IsLocked())
	{
		// TODO: error handling
		return false;
	}
#endif

	if (fFilters != NULL && fFilters->RemoveItem((void *)filter)) {
		filter->SetLooper(NULL);
		return true;
	}

	return false;
}


void
BHandler::SetFilterList(BList* filters)
{
	/**
		@note	Although the documentation states that the handler must belong to
				a looper and the looper must be locked in order to use this method,
				testing shows that this is not the case in the original implementation.
	 */

#if 0
	if (!fLooper)
	{
		// TODO: error handling
		return;
	}
#endif

	if (fLooper && !fLooper->IsLocked()) {
		debugger("Owning Looper must be locked before calling SetFilterList");
		return;
	}

	/**
		@note	I would like to use BObjectList internally, but this function is
				spec'd such that fFilters would get deleted and then assigned
				'filters', which would obviously mess this up.  Wondering if
				anyone ever assigns a list of filters and then checks against
				FilterList() to see if they are the same.
	 */

	// TODO: Explore issues with using BObjectList
	if (fFilters) {
		fFilters->DoForEach(FilterDeleter);
		delete fFilters;
	}

	fFilters = filters;
	if (fFilters) {
		for (int32 i = 0; i < fFilters->CountItems(); ++i) {
			BMessageFilter *filter =
				static_cast<BMessageFilter *>(fFilters->ItemAt(i));
			if (filter != NULL)
				filter->SetLooper(fLooper);
		}
	}
}


BList *
BHandler::FilterList()
{
	return fFilters;
}


bool
BHandler::LockLooper()
{
	/**
		@note	BeBook says that this function "retrieves the handler's looper and
				unlocks it in a pseudo-atomic operation, thus avoiding a race
				condition."  How "pseudo-atomic" would look completely escapes me,
				so we'll go with the dumb version for now.  Maybe I should use a
				benaphore?

				BeBook mentions handling the case where the handler's looper
				changes during this call.  I've attempted a "pseudo-atomic"
				operation to check that.
	 */

	BLooper *looper = fLooper;
	if (looper) {
		bool result = looper->Lock();

		// Are we still assigned to the same looper?
		if (fLooper == looper)
			return result;

		if (result) {
			// Our looper is different, and the lock was successful on the old
			// one; undo the lock
			looper->Unlock();
		}
	}

	return false;
}


status_t
BHandler::LockLooperWithTimeout(bigtime_t timeout)
{
	/**
		@note	BeBook says that this function "retrieves the handler's looper and
				unlocks it in a pseudo-atomic operation, thus avoiding a race
				condition."  How "pseudo-atomic" would look completely escapes me,
				so we'll go with the dumb version for now.  Maybe I should use a
				benaphore?

				BeBook mentions handling the case where the handler's looper
				changes during this call.  I've attempted a "pseudo-atomic"
				operation to check for that.
	 */

	BLooper *looper = fLooper;
	if (looper) {
		status_t result = looper->LockWithTimeout(timeout);

		// Are we still assigned to the same looper?
		if (fLooper == looper)
			return result;

		// Our looper changed during the lock attempt
		if (result == B_OK) {
			// The lock was successful on the old looper; undo the lock
			looper->Unlock();
		}

		return B_MISMATCHED_VALUES;
	}

	return B_BAD_VALUE;
}


void
BHandler::UnlockLooper()
{
	/**
		@note	BeBook says that this function "retrieves the handler's looper and
				unlocks it in a pseudo-atomic operation, thus avoiding a race
				condition."  How "pseudo-atomic" would look completely escapes me,
				so we'll go with the dumb version for now.  Maybe I should use a
				benaphore?

				The solution I used for Lock() and LockWithTimeout() seems out of
				place here; if our looper does change while attempting to unlock it,
				re-Lock()ing the original looper just doesn't seem right.
	 */

	// TODO: implement correctly
	BLooper *looper = fLooper;
	if (looper)
		looper->Unlock();
}


BHandler *
BHandler::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *specifier, int32 form, const char *property)
{
	// Straight from the BeBook
	BPropertyInfo propertyInfo(sHandlerPropInfo);
	if (propertyInfo.FindMatch(msg, index, specifier, form, property) >= 0)
		return this;

	BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);
	reply.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
	reply.AddString("message", "Didn't understand the specifier(s)");
	msg->SendReply(&reply);

	return NULL;
}


status_t
BHandler::GetSupportedSuites(BMessage *data)
{
/**
	@note	This is the output from the original implementation (calling
			PrintToStream() on both data and the contained BPropertyInfo):

BMessage: what =  (0x0, or 0)
    entry         suites, type='CSTR', c=1, size=21, data[0]: "suite/vnd.Be-handler"
    entry       messages, type='SCTD', c=1, size= 0,
      property   commands                       types                specifiers
--------------------------------------------------------------------------------
        Suites   PGET                                               1
                 (RTSC,suites)
                 (DTCS,messages)

     Messenger   PGET                          GNSM                 1
  InternalName   PGET                          RTSC                 1

			With a good deal of trial and error, I determined that the
			parenthetical clauses are entries in the 'ctypes' field of
			property_info.  'ctypes' is an array of 'compound_type', which
			contains an array of 'field_pair's.  I haven't the foggiest what
			either 'compound_type' or 'field_pair' is for, being as the
			scripting docs are so bloody horrible.  The corresponding
			property_info array is declared in the globals section.
 */

	status_t err = B_OK;
	if (!data)
		err = B_BAD_VALUE;

	if (!err) {
		err = data->AddString("suites", "suite/vnd.Be-handler");
		if (!err) {
			BPropertyInfo propertyInfo(sHandlerPropInfo);
			err = data->AddFlat("message", &propertyInfo);
		}
	}

	return err;
}


status_t
BHandler::StartWatching(BMessenger messenger, uint32 what)
{
	if (fObserverList == NULL)
		fObserverList = new _ObserverList;
	return fObserverList->StartObserving(messenger, what);
}


status_t
BHandler::StartWatchingAll(BMessenger messenger)
{
	return StartWatching(messenger, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StopWatching(BMessenger messenger, uint32 what)
{
	if (fObserverList == NULL)
		fObserverList = new _ObserverList;
	return fObserverList->StopObserving(messenger, what);
}


status_t
BHandler::StopWatchingAll(BMessenger messenger)
{
	return StopWatching(messenger, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StartWatching(BHandler *handler, uint32 what)
{
	if (fObserverList == NULL)
		fObserverList = new _ObserverList;
	return fObserverList->StartObserving(handler, what);
}


status_t
BHandler::StartWatchingAll(BHandler *handler)
{
	return StartWatching(handler, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StopWatching(BHandler *handler, uint32 what)
{
	if (fObserverList == NULL)
		fObserverList = new _ObserverList;
	return fObserverList->StopObserving(handler, what);
}


status_t
BHandler::StopWatchingAll(BHandler *handler)
{
	return StopWatching(handler, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}


void
BHandler::SendNotices(uint32 what, const BMessage *msg)
{
	if (fObserverList != NULL)
		fObserverList->SendNotices(what, msg);
}


bool
BHandler::IsWatched() const
{
	return fObserverList && !fObserverList->IsEmpty();
}


void
BHandler::InitData(const char *name)
{
	SetName(name);

	fLooper = NULL;
	fNextHandler = NULL;
	fFilters = NULL;
	fObserverList = NULL;

	fToken = gDefaultTokens.NewToken(B_HANDLER_TOKEN, this);
}


BHandler::BHandler(const BHandler &)
{
	// No copy construction allowed.
}


BHandler &
BHandler::operator=(const BHandler &)
{
	// No assignments allowed.
	return *this;
}


void
BHandler::SetLooper(BLooper *looper)
{
	fLooper = looper;
	
	if (fFilters) {
		for (int32 i = 0; i < fFilters->CountItems(); i++)
			static_cast<BMessageFilter *>(fFilters->ItemAtFast(i))->SetLooper(looper);
	}
}


#ifdef __INTEL__
// binary compatibility with R4.5
extern "C" void _ReservedHandler1__8BHandler(void) {}
#endif

void BHandler::_ReservedHandler2() {}
void BHandler::_ReservedHandler3() {}
void BHandler::_ReservedHandler4() {}


//	#pragma mark -


_ObserverList::_ObserverList(void)
{
}


_ObserverList::~_ObserverList(void)
{
}


status_t
_ObserverList::SendNotices(unsigned long what, BMessage const *message)
{
	// Having to new a temporary is really irritating ...
	BMessage *copyMsg = NULL;
	if (message) {
		copyMsg = new BMessage(*message);
		copyMsg->what = B_OBSERVER_NOTICE_CHANGE;
		copyMsg->AddInt32(B_OBSERVE_ORIGINAL_WHAT, message->what);
	} else
		copyMsg = new BMessage(B_OBSERVER_NOTICE_CHANGE);

	copyMsg->AddInt32(B_OBSERVE_WHAT_CHANGE, what);

	vector<BHandler *> &handlers = fHandlerMap[what];
	for (uint32 i = 0; i < handlers.size(); ++i) {
		BMessenger msgr(handlers[i]);
		msgr.SendMessage(copyMsg);
	}

	vector<BMessenger> &messengers = fMessengerMap[what];
	for (uint32 i = 0; i < messengers.size(); ++i)
		messengers[i].SendMessage(copyMsg);

	// We have to send the message also to the handlers
	// and messengers which were subscribed to ALL events,
	// since they aren't caught by the above loops.
	// TODO: cleanup
	vector<BHandler *> &handlersAll = fHandlerMap[B_OBSERVER_OBSERVE_ALL];
	for (uint32 i = 0; i < handlersAll.size(); ++i) {
		BMessenger msgr(handlersAll[i]);
		msgr.SendMessage(copyMsg);
	}
	
	vector<BMessenger> &messengersAll = fMessengerMap[B_OBSERVER_OBSERVE_ALL];
	for (uint32 i = 0; i < messengersAll.size(); ++i)
		messengers[i].SendMessage(copyMsg);
	
	// Gotta make sure to clean up the annoying temporary ...
	delete copyMsg;

	return B_OK;
}


status_t
_ObserverList::StartObserving(BHandler *handler, unsigned long what)
{
	if (handler == NULL)
		return B_BAD_HANDLER;

	vector<BHandler*> &handlers = fHandlerMap[what];

	vector<BHandler*>::iterator iter;
	iter = find(handlers.begin(), handlers.end(), handler);
	if (iter != handlers.end()) {
		// TODO: verify
		return B_OK;
	}

	handlers.push_back(handler);
	return B_OK;
}


status_t
_ObserverList::StartObserving(const BMessenger &messenger,
	unsigned long what)
{
	vector<BMessenger> &messengers = fMessengerMap[what];

	vector<BMessenger>::iterator iter;
	iter = find(messengers.begin(), messengers.end(), messenger);
	if (iter != messengers.end()) {
		// TODO: verify
		return B_OK;
	}

	messengers.push_back(messenger);
	return B_OK;
}


status_t
_ObserverList::StopObserving(BHandler *handler, unsigned long what)
{
	if (handler == NULL)
		return B_BAD_HANDLER;

	vector<BHandler*> &handlers = fHandlerMap[what];

	vector<BHandler*>::iterator iter;
	iter = find(handlers.begin(), handlers.end(), handler);
	if (iter != handlers.end()) {
		handlers.erase(iter);
		if (handlers.empty())
			fHandlerMap.erase(what);

		return B_OK;
	}

	return B_BAD_HANDLER;
}


status_t
_ObserverList::StopObserving(const BMessenger &messenger,
	unsigned long what)
{
	// ???:	What if you call StartWatching(MyMsngr, aWhat) and then call
	//		StopWatchingAll(MyMsnger)?  Will MyMsnger be removed from the aWhat
	//		watcher list?  For now, we'll assume that they're discreet lists
	//		which do no cross checking; i.e., MyMsnger would *not* be removed in
	//		this scenario.
	vector<BMessenger> &messengers = fMessengerMap[what];

	vector<BMessenger>::iterator iter;
	iter = find(messengers.begin(), messengers.end(), messenger);
	if (iter != messengers.end()) {
		messengers.erase(iter);
		if (messengers.empty())
			fMessengerMap.erase(what);

		return B_OK;
	}

	return B_BAD_HANDLER;
}


bool
_ObserverList::IsEmpty()
{
	return fHandlerMap.empty() && fMessengerMap.empty();
}


//	#pragma mark -


bool
FilterDeleter(void *_filter)
{
	delete static_cast<BMessageFilter *>(_filter);
	return false;
}

