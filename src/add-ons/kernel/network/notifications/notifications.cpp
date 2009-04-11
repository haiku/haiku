/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/*! Provides the networking stack notification service. */

#include <net_notifications.h>

#include <generic_syscall.h>
#include <Notifications.h>
#include <util/KMessage.h>

//#define TRACE_NOTIFICATIONS
#ifdef TRACE_NOTIFICATIONS
#	define TRACE(x...) dprintf("\33[32mnet_notifications:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


class NetNotificationService : public DefaultUserNotificationService {
public:
							NetNotificationService();
	virtual					~NetNotificationService();

			void			Notify(const KMessage& event);

protected:
	virtual	void			FirstAdded();
	virtual	void			LastRemoved();
};

static NetNotificationService sNotificationService;


//	#pragma mark - NetNotificationService


NetNotificationService::NetNotificationService()
	: DefaultUserNotificationService("network")
{
}


NetNotificationService::~NetNotificationService()
{
}


void
NetNotificationService::Notify(const KMessage& event)
{
	uint32 opcode = event.GetInt32("opcode", 0);
	if (opcode == 0)
		return;

	TRACE("notify for %lx\n", opcode);

	DefaultUserNotificationService::Notify(event, opcode);
}


void
NetNotificationService::FirstAdded()
{
	// The reference counting doesn't work for us, as we'll have to
	// ensure our module stays loaded.
	module_info* dummy;
	get_module(NET_NOTIFICATIONS_MODULE_NAME, &dummy);
}


void
NetNotificationService::LastRemoved()
{
	// Give up the reference _AddListener()
	put_module(NET_NOTIFICATIONS_MODULE_NAME);
}


//	#pragma mark - User generic syscall


static status_t
net_notifications_control(const char *subsystem, uint32 function, void *buffer,
	size_t bufferSize)
{
	struct net_notifications_control control;
	if (bufferSize != sizeof(struct net_notifications_control)
		|| function != NET_NOTIFICATIONS_CONTROL_WATCHING)
		return B_BAD_VALUE;
	if (user_memcpy(&control, buffer,
			sizeof(struct net_notifications_control)) < B_OK)
		return B_BAD_ADDRESS;

	if (control.flags != 0) {
		return sNotificationService.UpdateUserListener(control.flags,
			control.port, control.token);
	}

	return sNotificationService.RemoveUserListeners(control.port,
		control.token);
}


//	#pragma mark - exported module API


static status_t
send_notification(const KMessage* event)
{
	sNotificationService.Notify(*event);
	return B_OK;
}


static status_t
notifications_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE("init\n");

			new(&sNotificationService) NetNotificationService();

			register_generic_syscall(NET_NOTIFICATIONS_SYSCALLS,
				net_notifications_control, 1, 0);
			return B_OK;

		case B_MODULE_UNINIT:
			TRACE("uninit\n");

			unregister_generic_syscall(NET_NOTIFICATIONS_SYSCALLS, 1);

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
