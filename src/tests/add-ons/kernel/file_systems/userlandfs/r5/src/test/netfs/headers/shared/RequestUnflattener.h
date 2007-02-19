// RequestUnflattener.h

#ifndef NET_FS_REQUEST_UNFLATTENER_H
#define NET_FS_REQUEST_UNFLATTENER_H

#include <SupportDefs.h>

#include "Request.h"
#include "String.h"

// Reader
class Reader {
public:
								Reader();
	virtual						~Reader();

	virtual	status_t			Read(void* buffer, int32 size) = 0;
	virtual	status_t			Read(int32 size, void** buffer,
									bool* mustFree);
	virtual	status_t			Skip(int32 size);
};

// RequestUnflattener
class RequestUnflattener : public RequestMemberVisitor {
public:
								RequestUnflattener(Reader* reader);

			status_t			GetStatus() const;
			int32				GetBytesRead() const;

	virtual	void				Visit(RequestMember* member, bool& data);
	virtual	void				Visit(RequestMember* member, int8& data);
	virtual	void				Visit(RequestMember* member, uint8& data);
	virtual	void				Visit(RequestMember* member, int16& data);
	virtual	void				Visit(RequestMember* member, uint16& data);
	virtual	void				Visit(RequestMember* member, int32& data);
	virtual	void				Visit(RequestMember* member, uint32& data);
	virtual	void				Visit(RequestMember* member, int64& data);
	virtual	void				Visit(RequestMember* member, uint64& data);
	virtual	void				Visit(RequestMember* member, Data& data);
	virtual	void				Visit(RequestMember* member, StringData& data);
	virtual	void				Visit(RequestMember* member,
									RequestMember& subMember);
	virtual	void				Visit(RequestMember* member,
									FlattenableRequestMember& subMember);

			status_t			Read(void* buffer, int32 size);
			status_t			Read(int32 size, void*& buffer, bool& mustFree);
			status_t			Align(int32 align);

			status_t			ReadBool(bool& data);
			status_t			ReadInt32(int32& data);
			status_t			ReadData(void*& buffer, int32& size,
									bool& mustFree);
			status_t			ReadString(String& string);

private:
			Reader*				fReader;
			status_t			fStatus;
			int32				fBytesRead;
};

#endif	// NET_FS_REQUEST_UNFLATTENER_H
