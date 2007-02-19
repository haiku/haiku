// RequestFlattener.h

#ifndef NET_FS_REQUEST_FLATTENER_H
#define NET_FS_REQUEST_FLATTENER_H

#include <DataIO.h>

#include "Request.h"

// Writer
class Writer {
public:
								Writer();
	virtual						~Writer();

	virtual	status_t			Write(const void* buffer, int32 size) = 0;
	virtual	status_t			Pad(int32 size);
};

// DataIOWriter
class DataIOWriter : public Writer {
public:
								DataIOWriter(BDataIO* dataIO);
	virtual						~DataIOWriter();

	virtual	status_t			Write(const void* buffer, int32 size);

private:
			BDataIO*			fDataIO;
};

// DummyWriter
class DummyWriter : public Writer {
public:
								DummyWriter();
	virtual						~DummyWriter();

	virtual	status_t			Write(const void* buffer, int32 size);
};

// RequestFlattener
class RequestFlattener : public RequestMemberVisitor {
public:
								RequestFlattener(Writer* writer);

			status_t			GetStatus() const;
			int32				GetBytesWritten() const;

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

			status_t			Write(const void* buffer, int32 size);
			status_t			Align(int32 align);

			status_t			WriteBool(bool data);
			status_t			WriteInt32(int32 data);
			status_t			WriteData(const void* buffer, int32 size);
			status_t			WriteString(const char* string);

private:
			Writer*				fWriter;
			status_t			fStatus;
			int32				fBytesWritten;
};


#endif	// NET_FS_REQUEST_FLATTENER_H
