/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos <hugosantos@gmail.com>
 */

#include <net_stack_driver.h>
#include <sys/sockio.h>
#include <termios.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"

struct ioctl_info {
	int index;
	const char *name;
	TypeHandler *handler;
};

#define IOCTL_INFO_ENTRY(name) \
	{ name, #name, NULL }

#define IOCTL_INFO_ENTRY_TYPE(name, type) \
	{ name, #name, TypeHandlerFactory<type>::Create() }

static const ioctl_info kIOCtls[] = {
	// network stack ioctls
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_SOCKET, struct socket_args *),
	IOCTL_INFO_ENTRY(NET_STACK_GET_COOKIE),
	IOCTL_INFO_ENTRY(NET_STACK_CONTROL_NET_MODULE),
	IOCTL_INFO_ENTRY(NET_STACK_GET_NEXT_STAT),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_BIND, struct sockaddr_args *),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_RECEIVE, struct message_args *),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_SEND, struct message_args *),
	IOCTL_INFO_ENTRY(NET_STACK_LISTEN),
	IOCTL_INFO_ENTRY(NET_STACK_ACCEPT),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_CONNECT, struct sockaddr_args *),
	IOCTL_INFO_ENTRY(NET_STACK_SHUTDOWN),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_GETSOCKOPT, struct sockopt_args *),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_SETSOCKOPT, struct sockopt_args *),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_GETSOCKNAME, struct sockaddr_args *),
	IOCTL_INFO_ENTRY_TYPE(NET_STACK_GETPEERNAME, struct sockaddr_args *),
	IOCTL_INFO_ENTRY(NET_STACK_SOCKETPAIR),
	IOCTL_INFO_ENTRY(NET_STACK_NOTIFY_SOCKET_EVENT),

	// <sys/sockio.h>
	IOCTL_INFO_ENTRY(SIOCADDRT),
	IOCTL_INFO_ENTRY(SIOCDELRT),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFDSTADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFDSTADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFFLAGS, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFFLAGS, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFBRDADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFBRDADDR, struct ifreq *),
	IOCTL_INFO_ENTRY(SIOCGIFCOUNT),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFCONF, struct ifconf *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFINDEX, struct ifreq *),
	IOCTL_INFO_ENTRY(SIOCGIFNAME),
	IOCTL_INFO_ENTRY(SIOCGIFNETMASK),
	IOCTL_INFO_ENTRY(SIOCSIFNETMASK),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFMETRIC, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFMETRIC, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCDIFADDR, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCAIFADDR, struct ifreq *),
	IOCTL_INFO_ENTRY(SIOCADDMULTI),
	IOCTL_INFO_ENTRY(SIOCDELMULTI),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFMTU, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCSIFMTU, struct ifreq *),
	IOCTL_INFO_ENTRY(SIOCSIFMEDIA),
	IOCTL_INFO_ENTRY(SIOCGIFMEDIA),
	IOCTL_INFO_ENTRY(SIOCGRTSIZE),
	IOCTL_INFO_ENTRY(SIOCGRTTABLE),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFSTATS, struct ifreq *),
	IOCTL_INFO_ENTRY_TYPE(SIOCGIFPARAM, struct ifreq *),
	IOCTL_INFO_ENTRY(SIOCGIFTYPE),
	IOCTL_INFO_ENTRY(SIOCSPACKETCAP),
	IOCTL_INFO_ENTRY(SIOCCPACKETCAP),
	IOCTL_INFO_ENTRY(SIOCSHIWAT),
	IOCTL_INFO_ENTRY(SIOCGHIWAT),
	IOCTL_INFO_ENTRY(SIOCSLOWAT),
	IOCTL_INFO_ENTRY(SIOCGLOWAT),
	IOCTL_INFO_ENTRY(SIOCATMARK),
	IOCTL_INFO_ENTRY(SIOCSPGRP),

	// termios ioctls
	IOCTL_INFO_ENTRY(TCGETA),
	IOCTL_INFO_ENTRY(TCSETA),
	IOCTL_INFO_ENTRY(TCSETAF),
	IOCTL_INFO_ENTRY(TCSETAW),
	IOCTL_INFO_ENTRY(TCWAITEVENT),
	IOCTL_INFO_ENTRY(TCSBRK),
	IOCTL_INFO_ENTRY(TCFLSH),
	IOCTL_INFO_ENTRY(TCXONC),
	IOCTL_INFO_ENTRY(TCQUERYCONNECTED),
	IOCTL_INFO_ENTRY(TCGETBITS),
	IOCTL_INFO_ENTRY(TCSETDTR),
	IOCTL_INFO_ENTRY(TCSETRTS),
	IOCTL_INFO_ENTRY(TIOCGWINSZ),
	IOCTL_INFO_ENTRY(TIOCSWINSZ),
	IOCTL_INFO_ENTRY(TCVTIME),
	IOCTL_INFO_ENTRY(TIOCGPGRP),
	IOCTL_INFO_ENTRY(TIOCSPGRP),

	{ -1, NULL, NULL }
};

static EnumTypeHandler::EnumMap kIoctlNames;
static TypeHandlerSelector::SelectMap kIoctlTypeHandlers;

void
patch_ioctl()
{
	for (int i = 0; kIOCtls[i].name != NULL; i++) {
		kIoctlNames[kIOCtls[i].index] = kIOCtls[i].name;
		if (kIOCtls[i].handler != NULL)
			kIoctlTypeHandlers[kIOCtls[i].index] = kIOCtls[i].handler;
	}

	Syscall *ioctl = get_syscall("_kern_ioctl");

	ioctl->GetParameter("cmd")->SetHandler(
			new EnumTypeHandler(kIoctlNames));
	ioctl->GetParameter("data")->SetHandler(
			new TypeHandlerSelector(kIoctlTypeHandlers,
					1, TypeHandlerFactory<void *>::Create()));
}

