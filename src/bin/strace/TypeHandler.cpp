/*
 * Copyright 2005-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		Hugo Santos <hugosantos@gmail.com>
 */

#include "TypeHandler.h"

// headers required for network structures
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <net_stack_driver.h>

#include "Context.h"
#include "MemoryReader.h"
#include "Syscall.h"

// TypeHandlerImpl
template<typename Type>
class TypeHandlerImpl : public TypeHandler {
public:
	virtual string GetParameterValue(Context &, Parameter *, const void *);
	virtual string GetReturnValue(Context &, uint64 value);
};

template<typename value_t>
static inline value_t
get_value(const void *address)
{
	if (sizeof(align_t) > sizeof(value_t))
		return value_t(*(align_t*)address);
	else
		return *(value_t*)address;
}

// #pragma mark -

// get_pointer_value
static inline
string
get_pointer_value(const void *address)
{
	char buffer[32];
	sprintf(buffer, "%p", *(void **)address);
	return buffer;
}

// get_pointer_value
static inline
string
get_pointer_value(uint64 value)
{
	char buffer[32];
	sprintf(buffer, "%p", (void*)value);
	return buffer;
}

// create_pointer_type_handler
TypeHandler *
create_pointer_type_handler()
{
	return new TypeHandlerImpl<const void*>();
}

// create_string_type_handler
TypeHandler *
create_string_type_handler()
{
	return new TypeHandlerImpl<const char*>();
}

// #pragma mark -

// complete specializations

// void
template<>
string
TypeHandlerImpl<void>::GetParameterValue(Context &, Parameter *, const void *)
{
	return "void";
}

template<>
string
TypeHandlerImpl<void>::GetReturnValue(Context &, uint64 value)
{
	return "";
}

template<>
TypeHandler *
TypeHandlerFactory<void>::Create()
{
	return new TypeHandlerImpl<void>();
}

// bool
template<>
string
TypeHandlerImpl<bool>::GetParameterValue(Context &, Parameter *,
					 const void *address)
{
	return (*(const align_t*)address ? "true" : "false");
}

template<>
string
TypeHandlerImpl<bool>::GetReturnValue(Context &, uint64 value)
{
	return (value ? "true" : "false");
}

template<>
TypeHandler *
TypeHandlerFactory<bool>::Create()
{
	return new TypeHandlerImpl<bool>();
}

// read_string
static
string
read_string(MemoryReader &reader, void *data)
{
	char buffer[256];
	int32 bytesRead;
	status_t error = reader.Read(data, buffer, sizeof(buffer), bytesRead);
	if (error == B_OK) {
//		return string("\"") + string(buffer, bytesRead) + "\"";
//string result("\"");
//result += string(buffer, bytesRead);
//result += "\"";
//return result;

// TODO: Unless I'm missing something obvious, our STL string class is broken.
// The appended "\"" doesn't appear in either of the above cases.

		int32 len = strnlen(buffer, sizeof(buffer));
		char largeBuffer[259];
		largeBuffer[0] = '"';
		memcpy(largeBuffer + 1, buffer, len);
		largeBuffer[len + 1] = '"';
		largeBuffer[len + 2] = '\0';
		return largeBuffer;
	}
	return get_pointer_value(&data) + " (" + strerror(error) + ")";
}

static string
format_number(uint32 value)
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "%u", (unsigned int)value);
	return tmp;
}

static string
read_fdset(Context &context, void *data)
{
	// default FD_SETSIZE is 1024
	unsigned long tmp[1024 / (sizeof(unsigned long) * 8)];
	int32 bytesRead;

	status_t err = context.Reader().Read(data, &tmp, sizeof(tmp), bytesRead);
	if (err != B_OK)
		return get_pointer_value(&data);

	/* implicitly align to unsigned long lower boundary */
	int count = bytesRead / sizeof(unsigned long);
	int added = 0;

	string r;
	r.reserve(16);

	r = "[";

	for (int i = 0; i < count && added < 8; i++) {
		for (int j = 0;
			 j < (int)(sizeof(unsigned long) * 8) && added < 8; j++) {
			if (tmp[i] & (1 << j)) {
				if (added > 0)
					r += " ";
				unsigned int fd = i * sizeof(unsigned long) * 8 + j;
				r += format_number(fd);
				added++;
			}
		}
	}

	if (added >= 8)
		r += " ...";

	r += "]";

	return r;
}

// const void*
template<>
string
TypeHandlerImpl<const void*>::GetParameterValue(Context &, Parameter *,
						const void *address)
{
	return get_pointer_value(address);
}

template<>
string
TypeHandlerImpl<const void*>::GetReturnValue(Context &, uint64 value)
{
	return get_pointer_value(value);
}

// const char*
template<>
string
TypeHandlerImpl<const char*>::GetParameterValue(Context &context, Parameter *,
						const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::STRINGS))
		return read_string(context.Reader(), data);

	return get_pointer_value(&data);
}

template<>
string
TypeHandlerImpl<const char*>::GetReturnValue(Context &context, uint64 value)
{
	void *ptr = (void *)value;
	return GetParameterValue(context, NULL, (const void *)&ptr);
}

// struct fd_set *
template<>
string
TypeHandlerImpl<struct fd_set *>::GetParameterValue(Context &context, Parameter *,
						    const void *address)
{
	void *data = *(void **)address;
	if (data != NULL && context.GetContents(Context::SIMPLE_STRUCTS))
		return read_fdset(context, data);
	return get_pointer_value(&data);
}

template<>
string
TypeHandlerImpl<struct fd_set *>::GetReturnValue(Context &, uint64 value)
{
	return get_pointer_value(value);
}

EnumTypeHandler::EnumTypeHandler(const EnumMap &m) : fMap(m) {}

string
EnumTypeHandler::GetParameterValue(Context &context, Parameter *,
				   const void *address)
{
	return RenderValue(context, get_value<unsigned int>(address));
}

string
EnumTypeHandler::GetReturnValue(Context &context, uint64 value)
{
	return RenderValue(context, value);
}

string
EnumTypeHandler::RenderValue(Context &context, unsigned int value) const
{
	if (context.GetContents(Context::ENUMERATIONS)) {
		EnumMap::const_iterator i = fMap.find(value);
		if (i != fMap.end() && i->second != NULL)
			return i->second;
	}

	return context.FormatUnsigned(value);
}

TypeHandlerSelector::TypeHandlerSelector(const SelectMap &m, int sibling,
					 TypeHandler *def)
	: fMap(m), fSibling(sibling), fDefault(def) {}

string
TypeHandlerSelector::GetParameterValue(Context &context, Parameter *param,
				       const void *address)
{
	TypeHandler *target = fDefault;

	int index = get_value<int>(context.GetValue(context.GetSibling(fSibling)));

	SelectMap::const_iterator i = fMap.find(index);
	if (i != fMap.end())
		target = i->second;

	return target->GetParameterValue(context, param, address);
}

string
TypeHandlerSelector::GetReturnValue(Context &context, uint64 value)
{
	return fDefault->GetReturnValue(context, value);
}

template<typename Base>
static string
format_pointer_deep(Context &context, void *address)
{
	if (address == NULL || !context.GetContents(Context::COMPLEX_STRUCTS))
		return get_pointer_value(&address);

	Base data;
	int32 bytesRead;

	status_t err = context.Reader().Read(address, &data, sizeof(Base), bytesRead);
	if (err != B_OK || bytesRead < (int32)sizeof(Base))
		return get_pointer_value(&address);

	return format_pointer(context, &data);
}

template<typename Base>
static string
format_pointer_value(Context &context, const void *pointer)
{
	return format_pointer_deep<Base>(context, *(void **)pointer);
}

template<typename Base>
static string
format_pointer_value(Context &context, uint64 value)
{
	return format_pointer_deep<Base>(context, (void *)value);
}

static string
get_ipv4_address(struct in_addr *addr)
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
format_pointer(Context &context, sockaddr *saddr)
{
	string r = "{";

	struct sockaddr_in *sin = (struct sockaddr_in *)saddr;

	switch (saddr->sa_family) {
	case AF_INET:
		r += "AF_INET, ";
		r += get_ipv4_address(&sin->sin_addr);
		r += "/";
		r += format_number(ntohs(sin->sin_port));
		break;

	default:
		r += "family = ";
		r += context.FormatUnsigned(saddr->sa_family);
		r += ", ...";
		break;
	}

	return r + "}";
}

static string
format_pointer(Context &context, sockaddr_args *args)
{
	string r = "{";

	r +=   "addr = " + format_pointer_deep<struct sockaddr>(context, args->address);
	r += ", len = " + context.FormatUnsigned(args->address_length);

	return r + "}";
}

static string
format_pointer(Context &context, transfer_args *args)
{
	string r = "{";

	r +=   "data = " + get_pointer_value(&args->data);
	r += ", len = " + context.FormatUnsigned(args->data_length);
	r += ", flags = " + context.FormatFlags(args->flags);
	r += ", addr = " + format_pointer_deep<struct sockaddr>(context, args->address);

	return r + "}";
}

static string
format_pointer(Context &context, sockopt_args *args)
{
	string r = "{";

	r +=   "level = " + context.FormatSigned(args->level);
	r += ", option = " + context.FormatSigned(args->option);
	r += ", value = " + get_pointer_value(args->value);
	r += ", len = " + context.FormatSigned(args->length);

	return r + "}";
}

static string
get_iovec(Context &context, struct iovec *iov, int iovlen)
{
	string r = "{";
	r += get_pointer_value(&iov);
	r += ", " + context.FormatSigned(iovlen);
	return r + "}";
}

static string
format_pointer(Context &context, msghdr *h)
{
	string r = "{";

	r +=   "name = " + format_pointer_deep<struct sockaddr>(context, h->msg_name);
	r += ", name_len = " + context.FormatUnsigned(h->msg_namelen);
	r += ", iov = " + get_iovec(context, h->msg_iov, h->msg_iovlen);
	r += ", control = " + get_pointer_value(&h->msg_control);
	r += ", control_len = " + context.FormatUnsigned(h->msg_controllen);
	r += ", flags = " + context.FormatFlags(h->msg_flags);

	return r + "}";
}

template<typename Type>
class SignedIntegerTypeHandler : public TypeHandler {
public:
	SignedIntegerTypeHandler(const char *modifier)
		: fModifier(modifier) {}

	string GetParameterValue(Context &context, Parameter *,
				 const void *address)
	{
		return context.FormatSigned(get_value<Type>(address), fModifier);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return context.FormatSigned(value, fModifier);
	}

private:
	const char *fModifier;
};

template<typename Type>
class UnsignedIntegerTypeHandler : public TypeHandler {
public:
	string GetParameterValue(Context &context, Parameter *,
				 const void *address)
	{
		return context.FormatUnsigned(get_value<Type>(address));
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return context.FormatUnsigned(value);
	}
};

template<typename Type>
class SpecializedPointerTypeHandler : public TypeHandler {
	string GetParameterValue(Context &context, Parameter *,
				 const void *address)
	{
		return format_pointer_value<Type>(context, address);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return format_pointer_value<Type>(context, value);
	}
};

#define DEFINE_TYPE(name, type) \
	TypeHandler *create_##name##_type_handler() \
	{ \
		return new TypeHandlerImpl<type>(); \
	}

#define SIGNED_INTEGER_TYPE(type, modifier) \
	template<> \
	TypeHandler * \
	TypeHandlerFactory<type>::Create() \
	{ \
		return new SignedIntegerTypeHandler<type>(modifier); \
	}

#define UNSIGNED_INTEGER_TYPE(type) \
	template<> \
	TypeHandler * \
	TypeHandlerFactory<type>::Create() \
	{ \
		return new UnsignedIntegerTypeHandler<type>(); \
	}

#define POINTER_TYPE(name, type) \
	TypeHandler *create_##name##_type_handler() \
	{ \
		return new SpecializedPointerTypeHandler<type>(); \
	}

SIGNED_INTEGER_TYPE(char, "hh");
SIGNED_INTEGER_TYPE(short, "h");
SIGNED_INTEGER_TYPE(int, "");
SIGNED_INTEGER_TYPE(long, "l");
SIGNED_INTEGER_TYPE(long long, "ll");

UNSIGNED_INTEGER_TYPE(unsigned char);
UNSIGNED_INTEGER_TYPE(unsigned short);
UNSIGNED_INTEGER_TYPE(unsigned int);
UNSIGNED_INTEGER_TYPE(unsigned long);
UNSIGNED_INTEGER_TYPE(unsigned long long);

DEFINE_TYPE(fdset_ptr, struct fd_set *);
POINTER_TYPE(sockaddr_args_ptr, struct sockaddr_args);
POINTER_TYPE(transfer_args_ptr, struct transfer_args);
POINTER_TYPE(sockopt_args_ptr, struct sockopt_args);
POINTER_TYPE(msghdr_ptr, struct msghdr);

