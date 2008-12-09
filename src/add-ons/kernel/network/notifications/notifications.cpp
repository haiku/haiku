/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/*! Provides the networking stack notification service. */

#include <net_notifications.h>

//#include <messaging.h>
#include <Notifications.h>
#include <util/KMessage.h>


// TODO: add generic syscall interface


static UserMessagingMessageSender sNotificationSender;

struct net_listener : public DoublyLinkedListLinkImpl<net_listener> {
	~net_listener();

	uint32	flags;
	NotificationListener* listener;
};

typedef DoublyLinkedList<net_listener> ListenerList;

class UserNetListener : public UserMessagingListener {
public:
	UserNetListener(port_id port, int32 token)
		: UserMessagingListener(sNotificationSender, port, token)
	{
	}

	bool operator==(const NotificationListener& _other) const
	{
		const UserNetListener* other
			= dynamic_cast<const UserNetListener*>(&_other);
		return other != NULL && other->Port() == Port()
			&& other->Token() == Token();
	}
};

class NetNotificationService : public NotificationService {
public:
							NetNotificationService();
	virtual					~NetNotificationService();

			void			Notify(const KMessage& event);

			status_t		AddListener(const KMessage* eventSpecifier,
								NotificationListener& listener);
			status_t		UpdateListener(const KMessage* eventSpecifier,
								NotificationListener& listener);
			status_t		RemoveListener(const KMessage* eventSpecifier,
								NotificationListener& listener);

			status_t		RemoveUserListeners(port_id port, uint32 token);
			status_t		UpdateUserListener(uint32 flags,
								port_id port, uint32 token);

	virtual const char*		Name() { return "network"; }

private:
			status_t		_AddListener(uint32 flags,
								NotificationListener& listener);

			recursive_lock	fRecursiveLock;
			ListenerList	fListeners;
};

static NetNotificationService sNotificationService;


net_listener::~net_listener()
{
	if (dynamic_cast<UserNetListener*>(listener) != NULL)
		delete listener;
}


//	#pragma mark - NetNotificationService


NetNotificationService::NetNotificationService()
{
	recursive_lock_init(&fRecursiveLock, "net notifications");
}


NetNotificationService::~NetNotificationService()
{
	recursive_lock_destroy(&fRecursiveLock);
}


/*!	\brief Notifies all registered listeners.
	\param event The message defining the event
*/
void
NetNotificationService::Notify(const KMessage& event)
{
	uint32 flags = event.GetInt32("flags", 0);
	if (flags == 0)
		return;

	RecursiveLocker _(fRecursiveLock);

	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
		if ((listener->flags & flags) != 0)
			listener->listener->EventOccured(*this, &event);
	}

	iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
		if ((listener->flags & flags) != 0)
			listener->listener->AllListenersNotified(*this);
	}
}


status_t
NetNotificationService::AddListener(const KMessage* eventSpecifier,
	NotificationListener& listener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	uint32 flags = eventSpecifier->GetInt32("flags", 0);

	return _AddListener(flags, listener);
}


status_t
NetNotificationService::UpdateListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	if (eventSpecifier == NULL)
		return B_BAD_VALUE;

	uint32 flags = eventSpecifier->GetInt32("flags", 0);
	bool addFlags = eventSpecifier->GetBool("add flags", false);

	RecursiveLocker _(fRecursiveLock);

	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
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
NetNotificationService::RemoveListener(const KMessage* eventSpecifier,
	NotificationListener& notificationListener)
{
	RecursiveLocker _(fRecursiveLock);

	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
		if (listener->listener == &notificationListener) {
			iterator.Remove();
			delete listener;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


inline status_t
NetNotificationService::RemoveUserListeners(port_id port, uint32 token)
{
	UserNetListener userListener(port, token);

	RecursiveLocker _(fRecursiveLock);

	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
		if (*listener->listener == userListener) {
			iterator.Remove();
			delete listener;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
NetNotificationService::UpdateUserListener(uint32 flags, port_id port,
	uint32 token)
{
	UserNetListener userListener(port, token);

	RecursiveLocker _(fRecursiveLock);

	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (net_listener* listener = iterator.Next()) {
		if (*listener->listener == userListener) {
			listener->flags |= flags;
			return B_OK;
		}
	}

	UserNetListener* copiedListener = new(std::nothrow) UserNetListener(
		userListener);
	if (copiedListener == NULL)
		return B_NO_MEMORY;

	status_t status = _AddListener(flags, *copiedListener);
	if (status != B_OK)
		delete copiedListener;

	return status;
}


status_t
NetNotificationService::_AddListener(uint32 flags,
	NotificationListener& notificationListener)
{
	net_listener* listener = new(std::nothrow) net_listener;
	if (listener == NULL)
		return B_NO_MEMORY;

	listener->flags = flags;
	listener->listener = &notificationListener;

	RecursiveLocker _(fRecursiveLock);

	fListeners.Add(listener);
	return B_OK;
}


//	#pragma mark - User generic syscall


status_t
_user_start_watching_network(uint32 flags, port_id port, uint32 token)
{
	return sNotificationService.UpdateUserListener(flags, port, token);
}


status_t
_user_stop_watching_network(port_id port, uint32 token)
{
	return sNotificationService.RemoveUserListeners(port, token);
}


//	#pragma mark - exported module API


static void
send_notification(const KMessage* event)
{
	sNotificationService.Notify(*event);
}


static status_t
notifications_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			new(&sNotificationSender) UserMessagingMessageSender();
			new(&sNotificationService) NetNotificationService();
			return B_OK;
		case B_MODULE_UNINIT:
			sNotificationSender.~UserMessagingMessageSender();
			sNotificationService.~NetNotificationService();
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_notifications_module_info sNotificationsModule = {
	{
		NET_NOTIFICATIONS_MODULE_NAME,
		0,
		notifications_std_ops
	},

	send_notification
};

module_info* modules[] = {
	(module_info*)&sNotificationsModule,
	NULL
};
