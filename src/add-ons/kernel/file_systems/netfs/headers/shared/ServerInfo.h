// ServerInfo.h

#ifndef NET_FS_SERVER_INFO_H
#define NET_FS_SERVER_INFO_H

#include "HashString.h"
#include "Request.h"
#include "Vector.h"

// ShareInfo
class ShareInfo : public FlattenableRequestMember {
public:
								ShareInfo();

			bool				IsValid() const;

	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	virtual	status_t			Flatten(RequestFlattener* flattener);
	virtual	status_t			Unflatten(RequestUnflattener* unflattener);

			status_t			SetShareName(const char* shareName);
			const char*			GetShareName() const;

private:
			HashString			fShareName;
};


// ServerInfo
class ServerInfo : public FlattenableRequestMember {
public:
								ServerInfo();
								ServerInfo(const ServerInfo& other);

	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	virtual	status_t			Flatten(RequestFlattener* flattener);
	virtual	status_t			Unflatten(RequestUnflattener* unflattener);

			status_t			SetServerName(const char* serverName);
			const char*			GetServerName() const;

			status_t			SetConnectionMethod(
									const char* connectionMethod);
			const char*			GetConnectionMethod() const;

			status_t			AddShare(const char* shareName);
			int32				CountShares() const;
			const ShareInfo&	ShareInfoAt(int32 index) const;

			ServerInfo&			operator=(const ServerInfo& other);

private:
			HashString			fServerName;
			HashString			fConnectionMethod;
			Vector<ShareInfo>	fShareInfos;
};

#endif	// NET_FS_SERVER_INFO_H
