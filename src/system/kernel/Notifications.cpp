/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */


#include <Notifications.h>

#include <team.h>


NotificationManager NotificationManager::sManager;


// #pragma mark - NotificationListener


NotificationListener::~NotificationListener()
{
}


void
NotificationListener::EventOccured(NotificationService& service,
	const KMessage* event)
{
}


void
NotificationListener::AllListenersNotified(NotificationService& service)
{
}


bool
NotificationListener::operator==(const NotificationListener& other) const
{
	return &other == this;
}


// #pragma mark - UserMessagingMessageSender


UserMessagingMessageSender::UserMessagingMessageSender()
	:
	fMessage(NULL),
	fTargetCount(0)
{
}


void
UserMessagingMessageSender::SendMessage(const KMessage* message, port_id port,
	int32 token)
{
	if ((message != fMessage && fMessage != NULL)
		|| fTargetCount == MAX_MESSAGING_TARGET_COUNT) {
		FlushMessage();
	}

	fMessage = message;
	fTargets[fTargetCount].port = port;
	fTargets[fTargetCount].token = token;
	fTargetCount++;
}


void
UserMessagingMessageSender::FlushMessage()
{
	if (fMessage != NULL && fTargetCount > 0) {
		send_message(fMessage->Buffer(), fMessage->ContentSize(),
			fTargets, fTargetCount);
	}

	fMessage = NULL;
	fTargetCount = 0;
}


// #pragma mark - UserMessagingListener


UserMessagingListener::UserMessagingListener(UserMessagingMessageSender& sender,
		port_id port, int32 token)
	:
	fSender(sender),
	fPort(port),
	fToken(token)
{
}


UserMessagingListener::~UserMessagingListener()
{
}


void
UserMessagingListener::EventOccured(NotificationService& service,
	const KMessage* event)
{
	fSender.SendMessage(event, fPort, fToken);
}


void
UserMessagingListener::AllListenersNotified(NotificationService& service)
{
	fSender.FlushMessage();
}


//	#pragma mark - NotificationService


NotificationService::~NotificationService()
{
}


//	#pragma mark - default_listener


default_listener::~default_listener()
{
	// Only delete the listener if it's one of ours
	if (dynamic_cast<UserMessagingListener*>(listener) != NULL) {
		delete listener;
	}
}


//	#pragma mark - NotificationService


DefaultNotificationService::DefaultNotificationService(const char* name)
	:
	fName(name)
{
	recursive_lock_init(&fLock, name);
	NotificationManager::Manager().RegisterService(*this);
}


DefaultNotificationService::~DefaultNotificationService()
{
	NotificationManager::Manager().UnregisterService(*this);
	recursive_lock_destroy(&fLock);
}


/*!	\brief Notifies all registered listeners.
	\param event The message defining the event
	\param flags The flags that must be set in listeners to receive this event
*/
void
DefaultNotificationService::Notify(const KMessage& event, uint32 flags)
{
	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if ((flags & listener->flags) != 0)
			listener->listener->EventOccured(*this, &event);
	}

	iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if ((flags & listener->flags) != 0)
			listener->listener->AllListenersNotified(*this);
	}
}


status_t
DefaultNotificationService::AddListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	uint32 flags;
	status_t status = _ToFlags(*eventSpecifier, flags);
	if (status != B_OK)
		return status;

	default_listener* listener = new(std::nothrow) default_listener;
	if (listener == NULL)
		return B_NO_MEMORY;

	listener->flags = flags;
	listener->team = -1;
	listener->listener = &notificationListener;

	RecursiveLocker _(fLock);
	if (fListeners.IsEmpty())
		_FirstAdded();
	fListeners.Add(listener);

	return B_OK;
}


status_t
DefaultNotificationService::UpdateListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	return B_NOT_SUPPORTED;
}


status_t
DefaultNotificationService::RemoveListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if (listener->listener == &notificationListener) {
			iterator.Remove();
			delete listener;
			
			if (fListeners.IsEmpty())
				_LastRemoved();
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DefaultNotificationService::_ToFlags(const KMessage& eventSpecifier,
	uint32& flags)
{
	return eventSpecifier.FindInt32("flags", (int32*)&flags);
}


void
DefaultNotificationService::_FirstAdded()
{
}


void
DefaultNotificationService::_LastRemoved()
{
}


//	#pragma mark - DefaultUserNotificationService


DefaultUserNotificationService::DefaultUserNotificationService(const char* name)
	: DefaultNotificationService(name)
{
	NotificationManager::Manager().AddListener("teams", TEAM_REMOVED, *this);
}


DefaultUserNotificationService::~DefaultUserNotificationService()
{
	NotificationManager::Manager().RemoveListener("teams", NULL, *this);
}


status_t
DefaultUserNotificationService::AddListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	uint32 flags = eventSpecifier->GetInt32("flags", 0);

	return _AddListener(flags, listener);
}


status_t
DefaultUserNotificationService::UpdateListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	uint32 flags = eventSpecifier->GetInt32("flags", 0);
	bool addFlags = eventSpecifier->GetBool("add flags", false);

	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if (*listener->listener == notificationListener) {
			if (addFlags)
				listener->flags |= flags;
			else
				listener->flags = flags;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DefaultUserNotificationService::RemoveListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if (listener->listener == &notificationListener) {
			iterator.Remove();
			delete listener;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DefaultUserNotificationService::RemoveUserListeners(port_id port, uint32 token)
{
	UserMessagingListener userListener(fSender, port, token);

	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if (*listener->listener == userListener) {
			iterator.Remove();
			delete listener;
			
			if (fListeners.IsEmpty())
				_LastRemoved();
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DefaultUserNotificationService::UpdateUserListener(uint32 flags, port_id port,
	uint32 token)
{
	UserMessagingListener userListener(fSender, port, token);

	RecursiveLocker _(fLock);

	DefaultListenerList::Iterator iterator = fListeners.GetIterator();
	while (default_listener* listener = iterator.Next()) {
		if (*listener->listener == userListener) {
			listener->flags |= flags;
			return B_OK;
		}
	}

	UserMessagingListener* copiedListener
		= new(std::nothrow) UserMessagingListener(userListener);
	if (copiedListener == NULL)
		return B_NO_MEMORY;

	status_t status = _AddListener(flags, *copiedListener);
	if (status != B_OK)
		delete copiedListener;

	return status;
}


void
DefaultUserNotificationService::EventOccured(NotificationService& service,
	const KMessage* event)
{
	int32 opcode = event->GetInt32("opcode", -1);
	team_id team = event->GetInt32("team", -1);

	if (opcode == TEAM_REMOVED && team >= B_OK) {
		// check if we have any listeners from that team, and remove them
		RecursiveLocker _(fLock);
	
		DefaultListenerList::Iterator iterator = fListeners.GetIterator();
		while (default_listener* listener = iterator.Next()) {
			if (listener->team == team) {
				iterator.Remove();
				delete listener;
			}
		}
	}
}


void
DefaultUserNotificationService::AllListenersNotified(
	NotificationService& service)
{
}


status_t
DefaultUserNotificationService::_AddListener(uint32 flags,
	NotificationListener& notificationListener)
{
	default_listener* listener = new(std::nothrow) default_listener;
	if (listener == NULL)
		return B_NO_MEMORY;

	listener->flags = flags;
	listener->team = team_get_current_team_id();
	listener->listener = &notificationListener;

	RecursiveLocker _(fLock);
	if (fListeners.IsEmpty())
		_FirstAdded();
	fListeners.Add(listener);

	return B_OK;
}


//	#pragma mark - NotificationManager


/*static*/ NotificationManager&
NotificationManager::Manager()
{
	return sManager;
}


/*static*/ status_t
NotificationManager::CreateManager()
{
	new(&sManager) NotificationManager;
	return sManager._Init();
}


NotificationManager::NotificationManager()
{
}


NotificationManager::~NotificationManager()
{
}


status_t
NotificationManager::_Init()
{
	mutex_init(&fLock, "notification manager");

	return fServiceHash.Init();
}


NotificationService*
NotificationManager::_ServiceFor(const char* name)
{
	return fServiceHash.Lookup(name);
}


status_t
NotificationManager::RegisterService(NotificationService& service)
{
	MutexLocker _(fLock);

	if (_ServiceFor(service.Name()))
		return B_NAME_IN_USE;

	status_t status = fServiceHash.Insert(&service);
	if (status == B_OK)
		service.AddReference();

	return status;
}


void
NotificationManager::UnregisterService(NotificationService& service)
{
	MutexLocker _(fLock);
	fServiceHash.Remove(&service);
	service.RemoveReference();
}


status_t
NotificationManager::AddListener(const char* serviceName,
	uint32 eventMask, NotificationListener& listener)
{
	char buffer[96];
	KMessage specifier;
	specifier.SetTo(buffer, sizeof(buffer), 0);
	specifier.AddInt32("event mask", eventMask);

	return AddListener(serviceName, &specifier, listener);
}


status_t
NotificationManager::AddListener(const char* serviceName,
	const KMessage* eventSpecifier, NotificationListener& listener)
{
	MutexLocker locker(fLock);
	NotificationService* service = _ServiceFor(serviceName);
	if (service == NULL)
		return B_NAME_NOT_FOUND;

	Reference<NotificationService> reference(service);
	locker.Unlock();

	return service->AddListener(eventSpecifier, listener);
}


status_t
NotificationManager::UpdateListener(const char* serviceName,
	uint32 eventMask, NotificationListener& listener)
{
	char buffer[96];
	KMessage specifier;
	specifier.SetTo(buffer, sizeof(buffer), 0);
	specifier.AddInt32("event mask", eventMask);

	return UpdateListener(serviceName, &specifier, listener);
}


status_t
NotificationManager::UpdateListener(const char* serviceName,
	const KMessage* eventSpecifier, NotificationListener& listener)
{
	MutexLocker locker(fLock);
	NotificationService* service = _ServiceFor(serviceName);
	if (service == NULL)
		return B_NAME_NOT_FOUND;

	Reference<NotificationService> reference(service);
	locker.Unlock();

	return service->UpdateListener(eventSpecifier, listener);
}


status_t
NotificationManager::RemoveListener(const char* serviceName,
	const KMessage* eventSpecifier, NotificationListener& listener)
{
	MutexLocker locker(fLock);
	NotificationService* service = _ServiceFor(serviceName);
	if (service == NULL)
		return B_NAME_NOT_FOUND;

	Reference<NotificationService> reference(service);
	locker.Unlock();

	return service->RemoveListener(eventSpecifier, listener);
}


//	#pragma mark -


extern "C" void
notifications_init(void)
{
	status_t status = NotificationManager::CreateManager();
	if (status < B_OK) {
		panic("Creating the notification manager failed: %s\n",
			strerror(status));
	}
}

