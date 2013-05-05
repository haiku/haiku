// RequestDumper.cpp

#include "RequestDumper.h"

#include <string.h>

#include <typeinfo>

#include <ByteOrder.h>

#include "DebugSupport.h"

static const char*	kIndentation = "                                ";
static const int	kMaxIndentation = 32;

// constructor
RequestDumper::RequestDumper()
	: RequestMemberVisitor(),
	  fIndentationLevel(0)
{
}

// DumpRequest
void
RequestDumper::DumpRequest(Request* request)
{
	PRINT("request: %s\n", typeid(*request).name());
	fIndentationLevel++;
	request->ShowAround(this);
	fIndentationLevel--;
}

// Visit
void
RequestDumper::Visit(RequestMember* member, bool& data)
{
	PRINT("%sbool:   %s\n", _Indentation(), (data ? "true" : "false"));
}

// Visit
void
RequestDumper::Visit(RequestMember* member, int8& data)
{
	PRINT("%sint8:   %d\n", _Indentation(), (int)data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, uint8& data)
{
	PRINT("%suint8:  %d\n", _Indentation(), (int)data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, int16& data)
{
	PRINT("%sint16:  %d\n", _Indentation(), (int)data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, uint16& data)
{
	PRINT("%suint16: %d\n", _Indentation(), (int)data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, int32& data)
{
	PRINT("%sint32:  %ld\n", _Indentation(), data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, uint32& data)
{
	PRINT("%suint32: %lu\n", _Indentation(), data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, int64& data)
{
	PRINT("%sint64:  %lld\n", _Indentation(), data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, uint64& data)
{
	PRINT("%suint64: %llu\n", _Indentation(), data);
}

// Visit
void
RequestDumper::Visit(RequestMember* member, Data& data)
{
	PRINT("%sdata:    %p (%ld bytes)\n", _Indentation(), data.GetData(),
		data.GetSize());
}

// Visit
void
RequestDumper::Visit(RequestMember* member, StringData& data)
{
	PRINT("%sstring: \"%s\" (%p, %ld bytes)\n", _Indentation(),
		data.GetString(), data.GetString(), data.GetSize());
}

// Visit
void
RequestDumper::Visit(RequestMember* member, RequestMember& subMember)
{
	PRINT("%ssubmember:\n", _Indentation());

	fIndentationLevel++;
	subMember.ShowAround(this);
	fIndentationLevel--;
}

// Visit
void
RequestDumper::Visit(RequestMember* member,
	FlattenableRequestMember& subMember)
{
	PRINT("%sflattenable: %s\n", _Indentation(), typeid(subMember).name());
}

// _Indentation
const char*
RequestDumper::_Indentation() const
{
	int indentation = fIndentationLevel * 2;
	if (indentation >= kMaxIndentation)
		return kIndentation;
	return kIndentation + kMaxIndentation - indentation;
}
