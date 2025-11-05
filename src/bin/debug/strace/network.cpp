/*
 * Copyright 2023-2025, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <netinet/in.h>
#include <sys/socket.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"


struct enum_info {
	int index;
	const char *name;
};

#define ENUM_INFO_ENTRY(name) \
	{ name, #name }


#define FLAG_INFO_ENTRY(name) \
	{ name, #name }


static const FlagsTypeHandler::FlagInfo kRecvFlagInfos[] = {
	FLAG_INFO_ENTRY(MSG_PEEK),
	FLAG_INFO_ENTRY(MSG_OOB),
	FLAG_INFO_ENTRY(MSG_DONTROUTE),
	FLAG_INFO_ENTRY(MSG_EOR),
	FLAG_INFO_ENTRY(MSG_TRUNC),
	FLAG_INFO_ENTRY(MSG_CTRUNC),
	FLAG_INFO_ENTRY(MSG_WAITALL),
	FLAG_INFO_ENTRY(MSG_DONTWAIT),
	FLAG_INFO_ENTRY(MSG_BCAST),
	FLAG_INFO_ENTRY(MSG_MCAST),
	FLAG_INFO_ENTRY(MSG_EOF),
	FLAG_INFO_ENTRY(MSG_NOSIGNAL),
	FLAG_INFO_ENTRY(MSG_CMSG_CLOEXEC),
	FLAG_INFO_ENTRY(MSG_CMSG_CLOFORK),
	{ 0, NULL }
};


static const enum_info kSocketFamily[] = {
	ENUM_INFO_ENTRY(AF_UNSPEC),
	ENUM_INFO_ENTRY(AF_INET),
	ENUM_INFO_ENTRY(AF_APPLETALK),
	ENUM_INFO_ENTRY(AF_ROUTE),
	ENUM_INFO_ENTRY(AF_LINK),
	ENUM_INFO_ENTRY(AF_INET6),
	ENUM_INFO_ENTRY(AF_DLI),
	ENUM_INFO_ENTRY(AF_IPX),
	ENUM_INFO_ENTRY(AF_NOTIFY),
	ENUM_INFO_ENTRY(AF_UNIX),
	ENUM_INFO_ENTRY(AF_BLUETOOTH),

	{ 0, NULL }
};


static const enum_info kSocketType[] = {
	ENUM_INFO_ENTRY(SOCK_STREAM),
	ENUM_INFO_ENTRY(SOCK_DGRAM),
	ENUM_INFO_ENTRY(SOCK_RAW),
	ENUM_INFO_ENTRY(SOCK_SEQPACKET),
	ENUM_INFO_ENTRY(SOCK_MISC),

	{ 0, NULL }
};


static const enum_info kShutdownHow[] = {
	ENUM_INFO_ENTRY(SHUT_RD),
	ENUM_INFO_ENTRY(SHUT_WR),
	ENUM_INFO_ENTRY(SHUT_RDWR),

	{ 0, NULL }
};


static const FlagsTypeHandler::FlagInfo kSocketFlagInfos[] = {
	FLAG_INFO_ENTRY(SOCK_NONBLOCK),
	FLAG_INFO_ENTRY(SOCK_CLOEXEC),
	FLAG_INFO_ENTRY(SOCK_CLOFORK),

	{ 0, NULL }
};


static const enum_info kProtocolLevels[] = {
	ENUM_INFO_ENTRY(SOL_SOCKET),
	ENUM_INFO_ENTRY(IPPROTO_IP),
	ENUM_INFO_ENTRY(IPPROTO_IPV6),

	{ 0, NULL }
};


struct socket_option_info {
	int level;
	int option;
	const char *name;
	TypeHandler *handler;
	int length;
};


#define SOCKET_OPTION_INFO_ENTRY(level, option) \
	{ level, option, #option, NULL, 0 }

#define SOCKET_OPTION_INFO_ENTRY_TYPE(level, option, type) \
	{ level, option, #option, TypeHandlerFactory<type *>::Create(), sizeof(type) }

static const socket_option_info kSocketOptions[] = {
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_ACCEPTCONN),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_BROADCAST, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_DEBUG, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_DONTROUTE, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_KEEPALIVE, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_OOBINLINE, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_REUSEADDR, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_REUSEPORT, int32),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_USELOOPBACK, int32),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_LINGER),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_SNDBUF, uint32),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_SNDLOWAT),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_SNDTIMEO),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_RCVBUF, uint32),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_RCVLOWAT),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_RCVTIMEO),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_ERROR),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_TYPE),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_NONBLOCK, int32),
	SOCKET_OPTION_INFO_ENTRY(SOL_SOCKET, SO_BINDTODEVICE),
	SOCKET_OPTION_INFO_ENTRY_TYPE(SOL_SOCKET, SO_PEERCRED, struct ucred),

	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_OPTIONS),
	SOCKET_OPTION_INFO_ENTRY_TYPE(IPPROTO_IP, IP_HDRINCL, int),
	SOCKET_OPTION_INFO_ENTRY_TYPE(IPPROTO_IP, IP_TOS, int),
	SOCKET_OPTION_INFO_ENTRY_TYPE(IPPROTO_IP, IP_TTL, int),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_RECVOPTS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_RECVRETOPTS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_RECVDSTADDR),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_RETOPTS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_MULTICAST_IF),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_MULTICAST_TTL),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_MULTICAST_LOOP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_ADD_MEMBERSHIP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_DROP_MEMBERSHIP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_BLOCK_SOURCE),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_UNBLOCK_SOURCE),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_JOIN_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_BLOCK_SOURCE),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_UNBLOCK_SOURCE),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_LEAVE_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_JOIN_SOURCE_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, MCAST_LEAVE_SOURCE_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IP, IP_DONTFRAG),

	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_MULTICAST_IF),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_MULTICAST_HOPS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_MULTICAST_LOOP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_UNICAST_HOPS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_JOIN_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_LEAVE_GROUP),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_V6ONLY),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_PKTINFO),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_RECVPKTINFO),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_HOPLIMIT),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_RECVHOPLIMIT),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_HOPOPTS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_DSTOPTS),
	SOCKET_OPTION_INFO_ENTRY(IPPROTO_IPV6, IPV6_RTHDR),

	{ -1, -1, NULL, NULL }
};


static FlagsTypeHandler::FlagsList kRecvFlags;
static EnumTypeHandler::EnumMap kSocketFamilyMap;
static EnumTypeHandler::EnumMap kSocketTypeMap;
static EnumTypeHandler::EnumMap kShutdownHowMap;
static FlagsTypeHandler::FlagsList kSocketFlags;
static EnumTypeHandler::EnumMap kProtocolLevelMap;
static TypeHandlerSelector::SelectMap kLevelTypeHandlers;
static EnumTypeHandler::EnumMap kSocketLevelOptionMap;
static EnumTypeHandler::EnumMap kIPProtoLevelOptionMap;
static EnumTypeHandler::EnumMap kIPv6ProtoLevelOptionMap;
static TypeHandlerSelector::SelectMap kSocketLevelOptionTypeHandlers;
static TypeHandlerSelector::SelectMap kIPProtoLevelOptionTypeHandlers;
static TypeHandlerSelector::SelectMap kIPv6ProtoLevelOptionTypeHandlers;
static TypeHandlerSelector::SelectMap kLevelOptionTypeHandlers;


void
patch_network()
{
	for (int i = 0; kRecvFlagInfos[i].name != NULL; i++) {
		kRecvFlags.push_back(kRecvFlagInfos[i]);
	}
	for (int i = 0; kSocketFamily[i].name != NULL; i++) {
		kSocketFamilyMap[kSocketFamily[i].index] = kSocketFamily[i].name;
	}
	for (int i = 0; kSocketType[i].name != NULL; i++) {
		kSocketTypeMap[kSocketType[i].index] = kSocketType[i].name;
	}
	for (int i = 0; kShutdownHow[i].name != NULL; i++) {
		kShutdownHowMap[kShutdownHow[i].index] = kShutdownHow[i].name;
	}
	for (int i = 0; kSocketFlagInfos[i].name != NULL; i++) {
		kSocketFlags.push_back(kSocketFlagInfos[i]);
	}
	for (int i = 0; kProtocolLevels[i].name != NULL; i++) {
		kProtocolLevelMap[kProtocolLevels[i].index] = kProtocolLevels[i].name;
	}
	for (int i = 0; kSocketOptions[i].name != NULL; i++) {
		EnumTypeHandler::EnumMap* map = NULL;
		TypeHandlerSelector::SelectMap* selectMap = NULL;
		if (kSocketOptions[i].level == SOL_SOCKET) {
			map = &kSocketLevelOptionMap;
			selectMap = &kSocketLevelOptionTypeHandlers;
		} else if (kSocketOptions[i].level == IPPROTO_IP) {
			map = &kIPProtoLevelOptionMap;
			selectMap = &kIPProtoLevelOptionTypeHandlers;
		} else if (kSocketOptions[i].level == IPPROTO_IPV6) {
			map = &kIPv6ProtoLevelOptionMap;
			selectMap = &kIPv6ProtoLevelOptionTypeHandlers;
		}
		if (map != NULL) {
			(*map)[kSocketOptions[i].option] = kSocketOptions[i].name;
			if (kSocketOptions[i].handler == NULL)
				continue;
			(*selectMap)[kSocketOptions[i].option] = kSocketOptions[i].handler;
		}
	}
	kLevelTypeHandlers[SOL_SOCKET] = new EnumTypeHandler(kSocketLevelOptionMap);
	kLevelOptionTypeHandlers[SOL_SOCKET] = new TypeHandlerSelector(
		kSocketLevelOptionTypeHandlers, 2, TypeHandlerFactory<void *>::Create());
	kLevelOptionTypeHandlers[IPPROTO_IP] = new TypeHandlerSelector(
		kIPProtoLevelOptionTypeHandlers, 2, TypeHandlerFactory<void *>::Create());
	kLevelOptionTypeHandlers[IPPROTO_IPV6] = new TypeHandlerSelector(
		kIPv6ProtoLevelOptionTypeHandlers, 2, TypeHandlerFactory<void *>::Create());

	Syscall *recv = get_syscall("_kern_recv");
	recv->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));
	Syscall *recvfrom = get_syscall("_kern_recvfrom");
	recvfrom->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));
	Syscall *recvmsg = get_syscall("_kern_recvmsg");
	recvmsg->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));
	Syscall *send = get_syscall("_kern_send");
	send->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));
	Syscall *sendmsg = get_syscall("_kern_sendmsg");
	sendmsg->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));
	Syscall *sendto = get_syscall("_kern_sendto");
	sendto->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kRecvFlags));

	Syscall *socket = get_syscall("_kern_socket");
	socket->GetParameter("family")->SetHandler(
		new EnumTypeHandler(kSocketFamilyMap));
	socket->GetParameter("type")->SetHandler(
		new EnumFlagsTypeHandler(kSocketTypeMap, kSocketFlags));

	Syscall *shutdown = get_syscall("_kern_shutdown_socket");
	shutdown->GetParameter("how")->SetHandler(
		new EnumTypeHandler(kShutdownHowMap));

	Syscall *socketPair = get_syscall("_kern_socketpair");
	socketPair->ParameterAt(3)->SetOut(true);
	socketPair->ParameterAt(3)->SetCount(2);
	socketPair->GetParameter("family")->SetHandler(
		new EnumTypeHandler(kSocketFamilyMap));
	socketPair->GetParameter("type")->SetHandler(
		new EnumFlagsTypeHandler(kSocketTypeMap, kSocketFlags));

	Syscall *accept = get_syscall("_kern_accept");
	accept->GetParameter("flags")->SetHandler(new FlagsTypeHandler(kSocketFlags));

	Syscall *getsockopt = get_syscall("_kern_getsockopt");
	getsockopt->GetParameter("level")->SetHandler(new EnumTypeHandler(kProtocolLevelMap));
	getsockopt->GetParameter("option")->SetHandler(
		new TypeHandlerSelector(kLevelTypeHandlers, 1, TypeHandlerFactory<void *>::Create()));
	getsockopt->GetParameter("value")->SetHandler(
		new TypeHandlerSelector(kLevelOptionTypeHandlers, 1, TypeHandlerFactory<void *>::Create()));
	getsockopt->GetParameter("value")->SetOut(true);
	Syscall *setsockopt = get_syscall("_kern_setsockopt");
	setsockopt->GetParameter("level")->SetHandler(new EnumTypeHandler(kProtocolLevelMap));
	setsockopt->GetParameter("option")->SetHandler(
		new TypeHandlerSelector(kLevelTypeHandlers, 1, TypeHandlerFactory<void *>::Create()));
	setsockopt->GetParameter("value")->SetHandler(
		new TypeHandlerSelector(kLevelOptionTypeHandlers, 1, TypeHandlerFactory<void *>::Create()));

}
