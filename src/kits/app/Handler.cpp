/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <TokenSpace.h>

#include <AppDefs.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <PropertyInfo.h>

#include <algorithm>
#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using std::map;
using std::vector;
using BPrivate::gDefaultTokens;


static const char* kArchiveNameField = "_name";

static const uint32 kMsgStartObserving = '_OBS';
static const uint32 kMsgStopObserving = '_OBP';
static const char* kObserveTarget = "be:observe_target";


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

namespace BPrivate {

class ObserverList {
	public:
		ObserverList();
		~ObserverList();

		status_t SendNotices(uint32 what, const BMessage* notice);
		status_t Add(const BHandler* handler, uint32 what);
		status_t Add(const BMessenger& messenger, uint32 what);
		status_t Remove(const BHandler* handler, uint32 what);
		status_t Remove(const BMessenger& messenger, uint32 what);
		bool IsEmpty();

	private:
		typedef map<uint32, vector<const BHandler *> > HandlerObserverMap;
		typedef map<uint32, vector<BMessenger> > MessengerObserverMap;

		void _ValidateHandlers(uint32 what);
		void _SendNotices(uint32 what, BMessage* notice);

		HandlerObserverMap		fHandlerMap;
		MessengerObserverMap	fMessengerMap;
};

}	// namespace BPrivate

using namespace BPrivate;


//	#pragma mark -


BHandler::BHandler(const char *name)
	: BArchivable(),
	fName(NULL)
{
	_InitData(name);
}


BHandler::~BHandler()
{
	if (LockLooper()) {
		BLooper* looper = Looper();
		looper->RemoveHandler(this);
		looper->Unlock();
	}

	// remove all filters
	if (fFilters) {
		int32 count = fFilters->CountItems();
		for (int32 i = 0; i < count; i++)
			delete (BMessageFilter*)fFilters->ItemAtFast(i);
		delete fFilters;
	}

	// remove all observers (the observer list manages itself)
	delete fObserverList;

	// free rest
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

	_InitData(name);
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

	if (!fName)
		return B_OK;
	return data->AddString(kArchiveNameField, fName);
}


void
BHandler::MessageReceived(BMessage *message)
{
	BMessage reply(B_REPLY);

	switch (message->what) {
		case kMsgStartObserving:
		case kMsgStopObserving:
		{
			BMessenger target;
			uint32 what;
			if (message->FindMessenger(kObserveTarget, &target) != B_OK
				|| message->FindInt32(B_OBSERVE_WHAT_CHANGE, (int32*)&what) != B_OK)
				break;

			ObserverList* list = _ObserverList();
			if (list != NULL) {
				if (message->what == kMsgStartObserving)
					list->Add(target, what);
				else
					list->Remove(target, what);
			}
			break;
		}

		case B_GET_PROPERTY:
		{
			int32 cur;
			BMessage specifier;
			int32 form;
			const char *prop;

			status_t err = message->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (err != B_OK)
				break;
			bool known = false;
			if (cur < 0 || (strcmp(prop, "Messenger") == 0)) {
				err = reply.AddMessenger("result", this);
				known = true;
			} else if (strcmp(prop, "Suites") == 0) {
				err = GetSupportedSuites(&reply);
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
		printf("BHandler %s: MessageReceived() couldn't understand the message:\n", Name());
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
	BLooper* looper = fLooper;
	if (looper && !looper->IsLocked()) {
		debugger("Owning Looper must be locked before calling SetFilterList");
		return;
	}

	if (looper != NULL)
		filter->SetLooper(looper);

	if (!fFilters)
		fFilters = new BList;

	fFilters->AddItem(filter);
}


bool
BHandler::RemoveFilter(BMessageFilter *filter)
{
	BLooper* looper = fLooper;
	if (looper && !looper->IsLocked()) {
		debugger("Owning Looper must be locked before calling SetFilterList");
		return false;
	}

	if (fFilters != NULL && fFilters->RemoveItem((void *)filter)) {
		filter->SetLooper(NULL);
		return true;
	}

	return false;
}


void
BHandler::SetFilterList(BList* filters)
{
	BLooper* looper = fLooper;
	if (looper && !looper->IsLocked()) {
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
				filter->SetLooper(looper);
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
	BLooper *looper = fLooper;
	// Locking the looper also makes sure that the looper is valid
	if (looper != NULL && looper->Lock()) {
		// Have we locked the right looper? That's as far as the
		// "pseudo-atomic" operation mentioned in the BeBook.
		if (fLooper == looper)
			return true;

		// we locked the wrong looper, bail out
		looper->Unlock();
	}

	return false;
}


status_t
BHandler::LockLooperWithTimeout(bigtime_t timeout)
{
	BLooper *looper = fLooper;
	if (looper == NULL)
		return B_BAD_VALUE;

	status_t status = looper->LockWithTimeout(timeout);
	if (status != B_OK)
		return status;

	if (fLooper != looper) {
		// we locked the wrong looper, bail out
		looper->Unlock();
		return B_MISMATCHED_VALUES;
	}

	return B_OK;
}


void
BHandler::UnlockLooper()
{
	fLooper->Unlock();
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
			err = data->AddFlat("messages", &propertyInfo);
		}
	}

	return err;
}


status_t
BHandler::StartWatching(BMessenger target, uint32 what)
{
	BMessage message(kMsgStartObserving);
	message.AddMessenger(kObserveTarget, this);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, what);

	return target.SendMessage(&message);
}


status_t
BHandler::StartWatchingAll(BMessenger target)
{
	return StartWatching(target, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StopWatching(BMessenger target, uint32 what)
{
	BMessage message(kMsgStopObserving);
	message.AddMessenger(kObserveTarget, this);
	message.AddInt32(B_OBSERVE_WHAT_CHANGE, what);

	return target.SendMessage(&message);
}


status_t
BHandler::StopWatchingAll(BMessenger target)
{
	return StopWatching(target, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StartWatching(BHandler* handler, uint32 what)
{
	ObserverList* list = _ObserverList();
	if (list == NULL)
		return B_NO_MEMORY;

	return list->Add(handler, what);
}


status_t
BHandler::StartWatchingAll(BHandler* handler)
{
	return StartWatching(handler, B_OBSERVER_OBSERVE_ALL);
}


status_t
BHandler::StopWatching(BHandler* handler, uint32 what)
{
	ObserverList* list = _ObserverList();
	if (list == NULL)
		return B_NO_MEMORY;

	return list->Remove(handler, what);
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
BHandler::_InitData(const char *name)
{
	SetName(name);

	fLooper = NULL;
	fNextHandler = NULL;
	fFilters = NULL;
	fObserverList = NULL;

	fToken = gDefaultTokens.NewToken(B_HANDLER_TOKEN, this);
}


ObserverList*
BHandler::_ObserverList()
{
	if (fObserverList == NULL)
		fObserverList = new (std::nothrow) BPrivate::ObserverList();

	return fObserverList;
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
	gDefaultTokens.SetHandlerTarget(fToken, looper ? looper->fDirectTarget : NULL);

	if (fFilters) {
		for (int32 i = 0; i < fFilters->CountItems(); i++)
			static_cast<BMessageFilter *>(fFilters->ItemAtFast(i))->SetLooper(looper);
	}
}


#if __GNUC__ < 3
// binary compatibility with R4.5

extern "C" void
_ReservedHandler1__8BHandler(BHandler* handler, uint32 what,
	const BMessage* notice)
{
	handler->BHandler::SendNotices(what, notice);
}

#endif

void BHandler::_ReservedHandler2() {}
void BHandler::_ReservedHandler3() {}
void BHandler::_ReservedHandler4() {}


//	#pragma mark -


ObserverList::ObserverList()
{
}


ObserverList::~ObserverList()
{
}


void
ObserverList::_ValidateHandlers(uint32 what)
{
	vector<const BHandler *>& handlers = fHandlerMap[what];
	vector<const BHandler *>::iterator iterator = handlers.begin();

	while (iterator != handlers.end()) {
		BMessenger target(*iterator);
		if (!target.IsValid()) {
			iterator++;
			continue;
		}

		Add(target, what);
		iterator = handlers.erase(iterator);
	}
}


void
ObserverList::_SendNotices(uint32 what, BMessage* message)
{
	// first iterate over the list of handlers and try to make valid messengers out of them
	_ValidateHandlers(what);

	// now send it to all messengers we know
	vector<BMessenger>& messengers = fMessengerMap[what];
	vector<BMessenger>::iterator iterator = messengers.begin();

	while (iterator != messengers.end()) {
		if (!(*iterator).IsValid()) {
			iterator = messengers.erase(iterator);
			continue;
		}

		(*iterator).SendMessage(message);
		iterator++;
	}
}


status_t
ObserverList::SendNotices(uint32 what, const BMessage* message)
{
	BMessage *copy = NULL;
	if (message) {
		copy = new BMessage(*message);
		copy->what = B_OBSERVER_NOTICE_CHANGE;
		copy->AddInt32(B_OBSERVE_ORIGINAL_WHAT, message->what);
	} else
		copy = new BMessage(B_OBSERVER_NOTICE_CHANGE);

	copy->AddInt32(B_OBSERVE_WHAT_CHANGE, what);

	_SendNotices(what, copy);
	_SendNotices(B_OBSERVER_OBSERVE_ALL, copy);

	delete copy;
	return B_OK;
}


status_t
ObserverList::Add(const BHandler *handler, uint32 what)
{
	if (handler == NULL)
		return B_BAD_HANDLER;

	// if this handler already represents a valid target, add its messenger
	BMessenger target(handler);
	if (target.IsValid())
		return Add(target, what);

	vector<const BHandler*> &handlers = fHandlerMap[what];

	vector<const BHandler*>::iterator iter;
	iter = find(handlers.begin(), handlers.end(), handler);
	if (iter != handlers.end()) {
		// TODO: do we want to have a reference count for this?
		return B_OK;
	}

	handlers.push_back(handler);
	return B_OK;
}


status_t
ObserverList::Add(const BMessenger &messenger, uint32 what)
{
	vector<BMessenger> &messengers = fMessengerMap[what];

	vector<BMessenger>::iterator iter;
	iter = find(messengers.begin(), messengers.end(), messenger);
	if (iter != messengers.end()) {
		// TODO: do we want to have a reference count for this?
		return B_OK;
	}

	messengers.push_back(messenger);
	return B_OK;
}


status_t
ObserverList::Remove(const BHandler *handler, uint32 what)
{
	if (handler == NULL)
		return B_BAD_HANDLER;

	// look into the list of messengers
	BMessenger target(handler);
	if (target.IsValid() && Remove(target, what) == B_OK)
		return B_OK;

	vector<const BHandler*> &handlers = fHandlerMap[what];

	vector<const BHandler*>::iterator iterator = find(handlers.begin(),
		handlers.end(), handler);
	if (iterator != handlers.end()) {
		handlers.erase(iterator);
		if (handlers.empty())
			fHandlerMap.erase(what);

		return B_OK;
	}

	return B_BAD_HANDLER;
}


status_t
ObserverList::Remove(const BMessenger &messenger, uint32 what)
{
	vector<BMessenger> &messengers = fMessengerMap[what];

	vector<BMessenger>::iterator iterator = find(messengers.begin(),
		messengers.end(), messenger);
	if (iterator != messengers.end()) {
		messengers.erase(iterator);
		if (messengers.empty())
			fMessengerMap.erase(what);

		return B_OK;
	}

	return B_BAD_HANDLER;
}


bool
ObserverList::IsEmpty()
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

