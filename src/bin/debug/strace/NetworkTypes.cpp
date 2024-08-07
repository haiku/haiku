/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Hugo Santos <hugosantos@gmail.com>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <arpa/inet.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <map>
#include <utility>

#include "Context.h"
#include "MemoryReader.h"
#include "TypeHandler.h"

using std::map;
using std::pair;


template<typename Type>
static bool
obtain_pointer_data(Context &context, Type *data, void *address, uint32 what)
{
	if (address == NULL || !context.GetContents(what))
		return false;

	int32 bytesRead;

	status_t err = context.Reader().Read(address, data, sizeof(Type), bytesRead);
	if (err != B_OK || bytesRead < (int32)sizeof(Type))
		return false;

	return true;
}


static string
format_number(uint32 value)
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "%u", (unsigned int)value);
	return tmp;
}


static string
format_iovecs(Context &context, const iovec *iov, int iovlen)
{
	if (iov == NULL && iovlen == 0)
		return "(empty)";

	iovec vecs[iovlen];
	int32 bytesRead;

	string r = "[";
	status_t err = context.Reader().Read((void*)iov, vecs, sizeof(vecs), bytesRead);
	if (err != B_OK) {
		r += context.FormatPointer(iov);
		r += ", " + context.FormatSigned(iovlen);
	} else {
		for (int i = 0; i < iovlen; i++) {
			if (i > 0)
				r += ", ";
			if (i >= 8) {
				r += "...";
				break;
			}
			r += "{iov_base=" + context.FormatPointer(vecs[i].iov_base);
			r += ", iov_len=" + context.FormatUnsigned(vecs[i].iov_len);
			r += "}";
		}
	}
	return r + "]";
}


template<>
string
TypeHandlerImpl<iovec *>::GetParameterValue(Context &context, Parameter *param,
	const void *address)
{
	Parameter *size = context.GetNextSibling(param);
	return format_iovecs(context, (const iovec*)*(void **)address,
		context.ReadValue<size_t>(size));
}


template<>
string
TypeHandlerImpl<iovec *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


static string
format_ltype(Context &context, int ltype)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
#define LTYPE(type) \
		case type: \
			return #type

		switch (ltype) {
			LTYPE(F_RDLCK);
			LTYPE(F_UNLCK);
			LTYPE(F_WRLCK);
		}
	}

	return context.FormatSigned(ltype);
}


static string
format_lwhence(Context &context, int lwhence)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
#define LWHENCE(whence) \
		case whence: \
			return #whence

		switch (lwhence) {
			LWHENCE(SEEK_SET);
			LWHENCE(SEEK_CUR);
			LWHENCE(SEEK_END);
			LWHENCE(SEEK_DATA);
			LWHENCE(SEEK_HOLE);
		}
	}

	return context.FormatSigned(lwhence);
}



static string
format_pointer(Context &context, flock *lock)
{
	string r;

	r = "l_type=" + format_ltype(context, lock->l_type) + ", ";
	r += "l_whence=" + format_lwhence(context, lock->l_whence) + ", ";
	r += "l_start=" + context.FormatSigned(lock->l_start) + ", ";
	r += "l_len=" + context.FormatSigned(lock->l_len);

	return r;
}



template<typename Type>
static string
format_pointer_value(Context &context, void *address)
{
	Type data;

	if (obtain_pointer_data(context, &data, address, Context::COMPLEX_STRUCTS))
		return "{" + format_pointer(context, &data) + "}";

	return context.FormatPointer(address);
}


static string
get_ipv4_address(in_addr *addr)
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u",
		 (unsigned int)(htonl(addr->s_addr) >> 24) & 0xff,
		 (unsigned int)(htonl(addr->s_addr) >> 16) & 0xff,
		 (unsigned int)(htonl(addr->s_addr) >>  8) & 0xff,
		 (unsigned int)(htonl(addr->s_addr) >>  0) & 0xff);
	return tmp;
}


static string
format_socket_family(Context &context, int family)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
#define SOCKET_FAMILY(family) \
		case family: \
			return #family

		switch (family) {
			SOCKET_FAMILY(AF_UNSPEC);
			SOCKET_FAMILY(AF_INET);
			SOCKET_FAMILY(AF_APPLETALK);
			SOCKET_FAMILY(AF_ROUTE);
			SOCKET_FAMILY(AF_LINK);
			SOCKET_FAMILY(AF_INET6);
			SOCKET_FAMILY(AF_LOCAL);
		}
	}

	return "family = " + context.FormatSigned(family);
}


#if 0
static string
format_socket_type(Context &context, int type)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
		switch (type) {
			case SOCK_RAW:
				return "SOCK_RAW";
			case SOCK_DGRAM:
				return "SOCK_DGRAM";
			case SOCK_STREAM:
				return "SOCK_STREAM";
		}
	}

	return "type = " + context.FormatSigned(type);
}


static string
format_socket_protocol(Context &context, int protocol)
{
	if (context.GetContents(Context::ENUMERATIONS)) {
		switch (protocol) {
			case IPPROTO_IP:
				return "IPPROTO_IP";
			case IPPROTO_RAW:
				return "IPPROTO_RAW";
			case IPPROTO_ICMP:
				return "IPPROTO_ICMP";
			case IPPROTO_UDP:
				return "IPPROTO_UDP";
			case IPPROTO_TCP:
				return "IPPROTO_TCP";
		}
	}

	return "protocol = " + context.FormatSigned(protocol);
}
#endif


static string
format_pointer(Context &context, sockaddr *saddr)
{
	string r;

	r = format_socket_family(context, saddr->sa_family) + ", ";

	switch (saddr->sa_family) {
		case AF_INET:
		{
			sockaddr_in *sin = (sockaddr_in *)saddr;
			r += get_ipv4_address(&sin->sin_addr);
			r += "/";
			r += format_number(ntohs(sin->sin_port));
			break;
		}
		case AF_UNIX:
		{
			sockaddr_un *sun = (sockaddr_un *)saddr;
			r += "path = \"" + string(sun->sun_path) + "\"";
			break;
		}
		default:
			r += "...";
			break;
	}

	return r;
}


static string
read_sockaddr(Context &context, Parameter *param, void *address)
{
	param = context.GetNextSibling(param);
	if (param == NULL)
		return context.FormatPointer(address);

	socklen_t addrlen = context.ReadValue<socklen_t>(param);

	sockaddr_storage data;

	if (addrlen > sizeof(data))
		return context.FormatPointer(address);

	int32 bytesRead;
	status_t err = context.Reader().Read(address, &data, addrlen, bytesRead);
	if (err != B_OK)
		return context.FormatPointer(address);

	return "{" + format_pointer(context, (sockaddr *)&data) + "}";
}


template<>
string
TypeHandlerImpl<sockaddr *>::GetParameterValue(Context &context,
	Parameter *param, const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_sockaddr(context, param, data);
	return context.FormatPointer(data);
}


template<>
string
TypeHandlerImpl<sockaddr *>::GetReturnValue(Context &context, uint64 value)
{
	return context.FormatPointer((void *)value);
}


#if 0
static string
format_pointer(Context &context, sockaddr_args *args)
{
	string r;

	r  =   "addr = " + format_pointer_value<sockaddr>(context, args->address);
	r += ", len = " + context.FormatUnsigned(args->address_length);

	return r;
}


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
	{ -1, -1, NULL, NULL }
};

class SocketOptionsMap {
public:
	typedef map<pair<int, int>, const socket_option_info *> ThisMap;

	SocketOptionsMap()
	{
		for (int i = 0; kSocketOptions[i].name != NULL; i++) {
			fMap.insert(make_pair(
					std::make_pair(kSocketOptions[i].level,
						  kSocketOptions[i].option),
					&kSocketOptions[i]));
		}
	}

	const socket_option_info *GetEntry(int level, int option) const
	{
		ThisMap::const_iterator i = fMap.find(std::make_pair(level, option));
		if (i == fMap.end())
			return NULL;

		return i->second;
	}

private:
	ThisMap fMap;
};

static const SocketOptionsMap kSocketOptionsMap;


static string
format_pointer(Context &context, sockopt_args *args)
{
	const socket_option_info *info =
		kSocketOptionsMap.GetEntry(args->level, args->option);

	string level, option, value;

	if (context.GetContents(Context::ENUMERATIONS)) {
		switch (args->level) {
		case SOL_SOCKET:
			level = "SOL_SOCKET";
			break;
		case IPPROTO_IP:
			level = "IPPROTO_IP";
			break;
		}

		if (info != NULL)
			option = info->name;
	}

	if (info != NULL && info->length == args->length)
		value = info->handler->GetParameterValue(context, NULL, &args->value);
	else {
		value  = "value = " + context.FormatPointer(args->value);
		value += ", len = " + context.FormatUnsigned(args->length);
	}

	if (level.empty())
		level = "level = " + context.FormatSigned(args->level, sizeof(int));

	if (option.empty())
		option = "option = " + context.FormatSigned(args->option, sizeof(int));

	return level + ", " + option + ", " + value;
}


static string
format_pointer(Context &context, socket_args *args)
{
	string r;

	r  = format_socket_family(context, args->family) + ", ";
	r += format_socket_type(context, args->type) + ", ";
	r += format_socket_protocol(context, args->protocol);

	return r;
}


static string
format_pointer(Context &context, message_args *msg)
{
	string r;

	r +=   "header = " + format_pointer_value<msghdr>(context, msg->header);
	r += ", flags = " + context.FormatFlags(msg->flags);
	r += ", data = " + context.FormatPointer(msg->data);
	r += ", length = " + context.FormatUnsigned(msg->length);

	return r;
}
#endif


static string
format_pointer(Context &context, msghdr *h)
{
	string r;

	r  =   "name = " + format_pointer_value<sockaddr>(context, h->msg_name);
	r += ", name_len = " + context.FormatUnsigned(h->msg_namelen);
	r += ", iov = " + format_iovecs(context, h->msg_iov, h->msg_iovlen);
	if (h->msg_control != NULL || h->msg_controllen != 0) {
		r += ", control = " + context.FormatPointer(h->msg_control);
		r += ", control_len = " + context.FormatUnsigned(h->msg_controllen);
	}
	r += ", flags = " + context.FormatFlags(h->msg_flags);

	return r;
}


static string
format_pointer(Context &context, ifreq *ifr)
{
	return string(ifr->ifr_name) + ", ...";
}


static string
format_pointer(Context &context, ifconf *conf)
{
	unsigned char buffer[sizeof(ifreq) * 8];
	int maxData = sizeof(buffer);
	int32 bytesRead;

	if (conf->ifc_len < maxData)
		maxData = conf->ifc_len;

	string r = "len = " + context.FormatSigned(conf->ifc_len);

	if (conf->ifc_len == 0)
		return r;

	status_t err = context.Reader().Read(conf->ifc_buf, buffer,
					     maxData, bytesRead);
	if (err != B_OK)
		return r + ", buf = " + context.FormatPointer(conf->ifc_buf);

	r += ", [";

	uint8 *current = buffer, *bufferEnd = buffer + bytesRead;
	for (int i = 0; i < 8; i++) {
		if ((bufferEnd - current) < (IF_NAMESIZE + 1))
			break;

		ifreq *ifr = (ifreq *)current;
		int size = IF_NAMESIZE + ifr->ifr_addr.sa_len;

		if ((bufferEnd - current) < size)
			break;

		if (i > 0)
			r += ", ";

		r += "{" + string(ifr->ifr_name) + ", {"
			 + format_pointer(context, &ifr->ifr_addr) + "}}";

		current += size;
	}

	if (current < bufferEnd)
		r += ", ...";

	return r + "]";
}


static string
format_pointer(Context &context, siginfo_t *info)
{
	string r;

	switch (info->si_code) {
		case CLD_EXITED:
			r = "WIFEXITED(s) && WEXITSTATUS(s) == " + context.FormatUnsigned(info->si_status & 0xff);
			break;
	}
	return r;
}



template<typename Type>
class SpecializedPointerTypeHandler : public TypeHandler {
	string GetParameterValue(Context &context, Parameter *,
		const void *address)
	{
		return format_pointer_value<Type>(context, *(void **)address);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return format_pointer_value<Type>(context, (void *)value);
	}
};

#define POINTER_TYPE(name, type) \
	TypeHandler *create_##name##_type_handler() \
	{ \
		return new SpecializedPointerTypeHandler<type>(); \
	}

DEFINE_TYPE(iovec_ptr, iovec *);
POINTER_TYPE(flock_ptr, flock);
POINTER_TYPE(ifconf_ptr, ifconf);
POINTER_TYPE(ifreq_ptr, ifreq);
POINTER_TYPE(siginfo_t_ptr, siginfo_t);
POINTER_TYPE(msghdr_ptr, msghdr);
DEFINE_TYPE(sockaddr_ptr, sockaddr *);
#if 0
POINTER_TYPE(message_args_ptr, message_args);
POINTER_TYPE(sockaddr_args_ptr, sockaddr_args);
POINTER_TYPE(sockopt_args_ptr, sockopt_args);
POINTER_TYPE(socket_args_ptr, socket_args);
#endif
