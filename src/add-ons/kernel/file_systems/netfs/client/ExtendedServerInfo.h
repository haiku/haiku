// ExtendedServerInfo.h

#ifndef NET_FS_EXTENDED_SERVER_INFO_H
#define NET_FS_EXTENDED_SERVER_INFO_H

#include <HashString.h>
#include <Referenceable.h>

#include "NetAddress.h"
#include "Vector.h"

class ServerInfo;
class ShareInfo;

// ExtendedShareInfo
class ExtendedShareInfo : public BReferenceable {
public:
								ExtendedShareInfo();

			status_t			SetTo(const ShareInfo* shareInfo);

			const char*			GetShareName() const;

private:
			HashString			fShareName;
};

// ExtendedServerInfo
class ExtendedServerInfo : public BReferenceable {
public:
								ExtendedServerInfo(const NetAddress& address);
								~ExtendedServerInfo();

			const NetAddress&	GetAddress() const;
			const char*			GetServerName() const;
			const char*			GetConnectionMethod() const;

			int32				CountShares() const;
			ExtendedShareInfo*	ShareInfoAt(int32 index) const;
			ExtendedShareInfo*	GetShareInfo(const char* name);

			status_t			SetTo(ServerInfo* serverInfo);

			void				SetState(uint32 state);
			uint32				GetState() const;
				// used by the ServerManager only

private:
			status_t			_AddShare(const ShareInfo* info);

private:
		NetAddress				fAddress;
		HashString				fServerName;
		HashString				fConnectionMethod;
		Vector<ExtendedShareInfo*> fShareInfos;
		uint32					fState;
};

#endif	// NET_FS_EXTENDED_SERVER_INFO_H
