// ServerManager.h

#ifndef NET_FS_SERVER_MANAGER_H
#define NET_FS_SERVER_MANAGER_H

#include "Locker.h"

class ExtendedServerInfo;
class NetAddress;

// ServerManager
class ServerManager {
public:
			class Listener;

public:
								ServerManager(Listener* listener);
								~ServerManager();

			status_t			Init();
			void				Uninit();

			void				Run();

			ExtendedServerInfo*	GetServerInfo(const NetAddress& address);
			status_t			AddServer(const NetAddress& address);
			void				RemoveServer(const NetAddress& address);

private:
			struct ServerInfoMap;
			class ServerInfoTask;
			friend class ServerInfoTask;

	static	int32				_BroadcastListenerEntry(void* data);
			int32				_BroadcastListener();
			status_t			_InitBroadcastListener();
			void				_TerminateBroadcastListener();

			void				_ServerAdded(ExtendedServerInfo* serverInfo);
			void				_ServerUpdated(ExtendedServerInfo* serverInfo);
			void				_AddingServerFailed(
									ExtendedServerInfo* serverInfo);
			void				_UpdatingServerFailed(
									ExtendedServerInfo* serverInfo);
			void				_RemoveServer(ExtendedServerInfo* serverInfo);

private:
			Locker				fLock;
			ServerInfoMap*		fServerInfos;
			thread_id			fBroadcastListener;
			vint32				fBroadcastListenerSocket;
			Listener*			fListener;
			volatile bool		fTerminating;
};

// Listener
class ServerManager::Listener {
public:
								Listener()	{}
	virtual						~Listener();

	virtual	void				ServerAdded(ExtendedServerInfo* serverInfo) = 0;
	virtual	void				ServerUpdated(ExtendedServerInfo* oldInfo,
									ExtendedServerInfo* newInfo) = 0;
	virtual	void				ServerRemoved(
									ExtendedServerInfo* serverInfo) = 0;
};


#endif	// NET_FS_SERVER_MANAGER_H
