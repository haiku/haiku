// ExtendedServerInfo.h

#ifndef NET_FS_EXTENDED_SERVER_INFO_H
#define NET_FS_EXTENDED_SERVER_INFO_H

#include "NetAddress.h"
#include "String.h"
#include "Referencable.h"
#include "Vector.h"

class ServerInfo;
class ShareInfo;

// ExtendedShareInfo
class ExtendedShareInfo : public Referencable {
public:
								ExtendedShareInfo();

			status_t			SetTo(const ShareInfo* shareInfo);

			const char*			GetShareName() const;

private:
			String				fShareName;
};

// ExtendedServerInfo
class ExtendedServerInfo : public Referencable {
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
		String					fServerName;
		String					fConnectionMethod;
		Vector<ExtendedShareInfo*> fShareInfos;
		uint32					fState;
};

#endif	// NET_FS_EXTENDED_SERVER_INFO_H
