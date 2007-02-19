// Request.h

#ifndef NET_FS_REQUEST_H
#define NET_FS_REQUEST_H

#include <string.h>

#include <SupportDefs.h>

#include "SLList.h"
#include "String.h"

class RequestFlattener;
class RequestMemberVisitor;
class RequestUnflattener;
class RequestVisitor;

// Data
struct Data {
	const void*	address;
	int32		size;

	Data() : address(NULL), size(0) {}

	void SetTo(const void* address, int32 size)
	{
		this->address = (size > 0 ? address : NULL);
		this->size = size;
	}

	const void* GetData() const
	{
		return address;
	}

	int32 GetSize() const
	{
		return size;
	}
};

// StringData
struct StringData : Data {
	void SetTo(const char* string)
	{
		Data::SetTo(string, (string ? strlen(string) + 1 : 0));
	}

	void SetTo(const String& string)
	{
		SetTo(string.GetString());
	}

	const char* GetString() const
	{
		return (const char*)address;
	}
};

// RequestMember
class RequestMember {
public:
								RequestMember();
	virtual						~RequestMember();

	virtual	void				ShowAround(RequestMemberVisitor* visitor) = 0;
};

// FlattenableRequestMember
class FlattenableRequestMember : public RequestMember {
public:
								FlattenableRequestMember();
	virtual						~FlattenableRequestMember();

	virtual	status_t			Flatten(RequestFlattener* flattener) = 0;
	virtual	status_t			Unflatten(RequestUnflattener* unflattener) = 0;
};

// RequestBuffer
class RequestBuffer : public SLListLinkImpl<RequestBuffer> {
private:
								RequestBuffer() {}
								~RequestBuffer() {}

public:
	static	RequestBuffer*		Create(uint32 dataSize);
	static	void				Delete(RequestBuffer* buffer);

			void*				GetData();
			const void*			GetData() const;
};

// Request
class Request : public FlattenableRequestMember {
public:
								Request(uint32 type);
	virtual						~Request();

			uint32				GetType() const;

			void				AttachBuffer(RequestBuffer* buffer);

	virtual	status_t			Accept(RequestVisitor* visitor) = 0;

	virtual	status_t			Flatten(RequestFlattener* flattener);
	virtual	status_t			Unflatten(RequestUnflattener* unflattener);

private:
			uint32				fType;
			SLList<RequestBuffer> fBuffers;
};

// RequestMemberVisitor
class RequestMemberVisitor {
public:
								RequestMemberVisitor();
	virtual						~RequestMemberVisitor();

	virtual	void				Visit(RequestMember* member, bool& data) = 0;
	virtual	void				Visit(RequestMember* member, int8& data) = 0;
	virtual	void				Visit(RequestMember* member, uint8& data) = 0;
	virtual	void				Visit(RequestMember* member, int16& data) = 0;
	virtual	void				Visit(RequestMember* member, uint16& data) = 0;
	virtual	void				Visit(RequestMember* member, int32& data) = 0;
	virtual	void				Visit(RequestMember* member, uint32& data) = 0;
	virtual	void				Visit(RequestMember* member, int64& data) = 0;
	virtual	void				Visit(RequestMember* member, uint64& data) = 0;
	virtual	void				Visit(RequestMember* member, Data& data) = 0;
	virtual	void				Visit(RequestMember* member,
									StringData& data) = 0;
	virtual	void				Visit(RequestMember* member,
									RequestMember& subMember) = 0;
	virtual	void				Visit(RequestMember* member,
									FlattenableRequestMember& subMember) = 0;
};

#endif	// NET_FS_REQUEST_H
