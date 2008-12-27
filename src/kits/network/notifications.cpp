/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!	The notifications API uses the generic syscall interface of the
	network's stack notification module.
*/

#include <net_notifications.h>

#include <MessengerPrivate.h>
#include <generic_syscall_defs.h>
#include <syscalls.h>


static status_t
check_for_notifications_syscall(void)
{
	uint32 version = 0;
	return _kern_generic_syscall(NET_NOTIFICATIONS_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
}


//	#pragma mark -


status_t
start_watching_network(uint32 flags, const BMessenger& target)
{
	if (check_for_notifications_syscall() != B_OK)
		return B_NOT_SUPPORTED;

	BMessenger::Private targetPrivate(const_cast<BMessenger&>(target));
	net_notifications_control control;
	control.flags = flags;
	control.port = targetPrivate.Port();
	control.token = targetPrivate.Token();

	return _kern_generic_syscall(NET_NOTIFICATIONS_SYSCALLS,
		NET_NOTIFICATIONS_CONTROL_WATCHING, &control,
		sizeof(net_notifications_control));
}


status_t
start_watching_network(uint32 flags, const BHandler* handler,
	const BLooper* looper)
{
	const BMessenger target(handler, looper);
	return start_watching_network(flags, target);
}

status_t
stop_watching_network(const BMessenger& target)
{
	return start_watching_network(0, target);
		// start_watching_network() without flags just stops everything
}


status_t
stop_watching_network(const BHandler* handler, const BLooper* looper)
{
	const BMessenger target(handler, looper);
	return stop_watching_network(target);
}
