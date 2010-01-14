// ClientVolume.h

#ifndef NET_FS_CLIENT_VOLUME_H
#define NET_FS_CLIENT_VOLUME_H

#include <Locker.h>

#include "BlockingQueue.h"
#include "FSObject.h"
#include "HashString.h"
#include "Node.h"
#include "Permissions.h"
#include "ServerNodeID.h"

class AttrDirIterator;
class DirIterator;
class FileHandle;
class NodeInfo;
class NodeHandle;
class NodeHandleMap;
class NodeMonitoringEvent;
class QueryHandle;
class UserSecurityContext;
class Share;
class VolumeManager;

// ClientVolume
class ClientVolume : public FSObject, public BLocker {
public:
			class NodeMonitoringProcessor;

public:
								ClientVolume(Locker& securityContextLocker,
									NodeMonitoringProcessor*
										nodeMonitoringProcessor);
								~ClientVolume();

			status_t			Init();

			int32				GetID() const;

			status_t			Mount(UserSecurityContext* securityContext,
									Share* share);
			void				Unmount();
			bool				IsMounted() const;

			UserSecurityContext* GetSecurityContext() const;
			void				SetSecurityContext(
									UserSecurityContext* securityContext);

			Share*				GetShare() const;

			Directory*			GetRootDirectory() const;
			const NodeRef&		GetRootNodeRef() const;

			Permissions			GetSharePermissions() const;

			Permissions			GetNodePermissions(dev_t volumeID,
									ino_t nodeID);
			Permissions			GetNodePermissions(Node* node);

			Node*				GetNode(dev_t volumeID, ino_t nodeID);
			Node*				GetNode(NodeID nodeID);
			Node*				GetNode(const node_ref& nodeRef);

			Directory*			GetDirectory(dev_t volumeID, ino_t nodeID);
			Directory*			GetDirectory(NodeID nodeID);
			status_t			LoadDirectory(dev_t volumeID, ino_t nodeID,
									Directory** directory);

			Entry*				GetEntry(dev_t volumeID, ino_t dirID,
									const char* name);
			Entry*				GetEntry(Directory* directory,
									const char* name);
			status_t			LoadEntry(dev_t volumeID, ino_t dirID,
									const char* name, Entry** entry);
			status_t			LoadEntry(Directory* directory,
									const char* name, Entry** entry);

			status_t			Open(Node* node, int openMode,
									FileHandle** handle);
			status_t			OpenDir(Directory* directory,
									DirIterator** iterator);
			status_t			OpenAttrDir(Node* node,
									AttrDirIterator** iterator);
			status_t			Close(NodeHandle* handle);

			status_t			LockNodeHandle(int32 cookie,
									NodeHandle** handle);
			void				UnlockNodeHandle(NodeHandle* nodeHandle);

			void				ProcessNodeMonitoringEvent(
									NodeMonitoringEvent* event);

private:
	static	int32				_NextVolumeID();

private:
			struct NodeIDMap;
			struct NodeMap;

			int32				fID;
			UserSecurityContext* fSecurityContext;
			Locker&				fSecurityContextLock;
			NodeMonitoringProcessor* fNodeMonitoringProcessor;
			NodeHandleMap*		fNodeHandles;
			Share*				fShare;
			NodeRef				fRootNodeRef;
			Permissions			fSharePermissions;
			bool				fMounted;

	static	vint32				sNextVolumeID;
};

// NodeMonitoringProcessor
class ClientVolume::NodeMonitoringProcessor {
public:
								NodeMonitoringProcessor() {}
	virtual						~NodeMonitoringProcessor();

	virtual	void				ProcessNodeMonitoringEvent(int32 volumeID,
									NodeMonitoringEvent* event) = 0;
};

// NodeHandleUnlocker
struct NodeHandleUnlocker {
	NodeHandleUnlocker(ClientVolume* volume, NodeHandle* nodeHandle)
		: fVolume(volume),
		  fHandle(nodeHandle)
	{
	}

	~NodeHandleUnlocker()
	{
		if (fVolume && fHandle) {
			fVolume->UnlockNodeHandle(fHandle);
			fVolume = NULL;
			fHandle = NULL;
		}
	}

private:
	ClientVolume*	fVolume;
	NodeHandle*		fHandle;
};

#endif	// NET_FS_CLIENT_VOLUME_H
