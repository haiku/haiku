// RequestDumper.h

#ifndef NET_FS_REQUEST_DUMPER_H
#define NET_FS_REQUEST_DUMPER_H

#include <SupportDefs.h>

#include "Request.h"

// RequestDumper
class RequestDumper : public RequestMemberVisitor {
public:
								RequestDumper();

			void				DumpRequest(Request* request);

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
			const char*			_Indentation() const;

private:
			int					fIndentationLevel;
};

#endif	// NET_FS_REQUEST_DUMPER_H
