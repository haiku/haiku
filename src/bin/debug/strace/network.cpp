/*
 * Copyright 2023, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


//#include <OS.h>
#include <sys/socket.h>

#include "strace.h"
#include "Syscall.h"
#include "TypeHandler.h"


struct enum_info {
	unsigned int index;
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


static FlagsTypeHandler::FlagsList kRecvFlags;
static EnumTypeHandler::EnumMap kSocketFamilyMap;
static EnumTypeHandler::EnumMap kSocketTypeMap;
static EnumTypeHandler::EnumMap kShutdownHowMap;
static FlagsTypeHandler::FlagsList kSocketFlags;


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
}
