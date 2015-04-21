/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_LAUNCH_H
#define _LIBROOT_LAUNCH_H


#include <LaunchDaemonDefs.h>
#include <OS.h>


#ifdef __cplusplus
namespace BPrivate {


class KMessage;


port_id		get_launch_daemon_port();
status_t	send_request_to_launch_daemon(KMessage& request, KMessage& reply);
status_t	get_launch_data(const char* signature, KMessage& data);


}	// namespace BPrivate
#endif	// __cplusplus


#endif	// _LIBROOT_LAUNCH_H
