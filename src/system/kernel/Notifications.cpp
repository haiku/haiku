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

