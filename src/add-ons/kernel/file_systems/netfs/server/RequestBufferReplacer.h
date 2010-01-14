// RequestBufferReplacer.h

#ifndef NET_FS_REQUEST_BUFFER_REPLACER_H
#define NET_FS_REQUEST_BUFFER_REPLACER_H

#include "Request.h"

class RequestBufferReplacer : private RequestMemberVisitor {
public:
								RequestBufferReplacer();
								~RequestBufferReplacer();

			status_t			ReplaceBuffer(Request* request);

private:
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

private:
			char*				fBuffer;
			int32				fBufferSize;
};

#endif	// NET_FS_REQUEST_BUFFER_REPLACER_H
