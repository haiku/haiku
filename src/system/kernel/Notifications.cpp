/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */


#include <Notifications.h>


NotificationManager NotificationManager::sManager;


// #pragma mark - UserMessagingListener


NotificationListener::~NotificationListener()
{
}


void
NotificationListener::EventOccured(NotificationService& service,
	const KMessage* event)
{
}


void
NotificationListener::AllListenersNotified()
{
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
	if (message != fMessage && fMessage != NULL
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
UserMessagingListener::AllListenersNotified()
{
	fSender.FlushMessage();
}


// #pragma mark - NotificationListenerUpdater


NotificationListenerUpdater::NotificationListenerUpdater(
		const KMessage* eventSpecifier)
	: fEventSpecifier(eventSpecifier)
{
}


NotificationListenerUpdater::~NotificationListenerUpdater()
{
}


status_t
NotificationListenerUpdater::UpdateListener(NotificationListener& listener,
	enum update_action& action)
{
	action = SKIP;
	return B_OK;
}


status_t
NotificationListenerUpdater::CreateListener(NotificationListener** _listener)
{
	return B_ERROR;
}


void
NotificationListenerUpdater::SetEventSpecifier(const KMessage* eventSpecifier)
{
	fEventSpecifier = eventSpecifier;
}


// #pragma mark - NotificationListenerUpdater


UserMessagingListenerUpdater::UserMessagingListenerUpdater(
		const KMessage* eventSpecifier, port_id port, int32 token)
	:
	NotificationListenerUpdater(eventSpecifier),
	fPort(port),
	fToken(token)
{
}


status_t
UserMessagingListenerUpdater::UpdateListener(NotificationListener& _listener,
	enum update_action& action)
{
	UserMessagingListener* listener
		= dynamic_cast<UserMessagingListener*>(&_listener);
	if (listener != NULL && listener->Port() == fPort
		&& listener->Token() == fToken) {
		return UpdateListener(*listener, action);
	}

	action = SKIP;
	return B_OK;
}


//	#pragma mark - NotificationManager

#if 0
NotificationService::~NotificationService()
{
}
#endif


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
	status_t status = mutex_init(&fLock, "notification manager");
	if (status < B_OK)
		return status;

	return fServiceHash.InitCheck();
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
NotificationManager::RemoveListener(const char* serviceName, uint32 eventMask,
	NotificationListener& listener)
{
	char buffer[96];
	KMessage specifier;
	specifier.SetTo(buffer, sizeof(buffer), 0);
	specifier.AddInt32("event mask", eventMask);

	return RemoveListener(serviceName, &specifier, listener);
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


status_t
NotificationManager::UpdateListener(const char* serviceName,
	NotificationListenerUpdater& updater)
{
	MutexLocker locker(fLock);
	NotificationService* service = _ServiceFor(serviceName);
	if (service == NULL)
		return B_NAME_NOT_FOUND;

	Reference<NotificationService> reference(service);
	locker.Unlock();

	return service->UpdateListener(updater);
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

