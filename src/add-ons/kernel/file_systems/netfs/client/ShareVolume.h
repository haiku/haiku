// ShareVolume.h

#ifndef NET_FS_SHARE_VOLUME_H
#define NET_FS_SHARE_VOLUME_H

#include <fsproto.h>

#include <util/DoublyLinkedList.h>

#include "EntryInfo.h"
#include "FSObject.h"
#include "Locker.h"
#include "RequestHandler.h"
#include "RequestMemberArray.h"
#include "ServerNodeID.h"
#include "Volume.h"

class AttrDirInfo;
class ExtendedServerInfo;
class ExtendedShareInfo;
class Node;
class ReadQueryReply;
class RemoteShareDirIterator;
class RequestConnection;
class RootShareVolume;
class ServerConnection;
class ServerConnectionProvider;
class ShareAttrDirIterator;
class ShareDir;
class ShareDirEntry;
class ShareNode;
class WalkReply;

class ShareVolume : public Volume {
public:
								ShareVolume(VolumeManager* volumeManager,
									ServerConnectionProvider*
										connectionProvider,
									ExtendedServerInfo* serverInfo,
									ExtendedShareInfo* shareInfo);
								~ShareVolume();

			int32				GetID() const;

			bool				IsReadOnly() const;
			bool				SupportsQueries() const;

			status_t			Init(const char* name);
			void				Uninit();

	virtual	Node*				GetRootNode() const;

	virtual	void				PrepareToUnmount();

	virtual	void				RemoveChildVolume(Volume* volume);

			// FS
	virtual	status_t			Unmount();
	virtual	status_t			Sync();

			// vnodes
	virtual	status_t			ReadVNode(vnode_id vnid, char reenter,
									Node** node);
	virtual	status_t			WriteVNode(Node* node, char reenter);
	virtual	status_t			RemoveVNode(Node* node, char reenter);

			// nodes
	virtual	status_t			FSync(Node* node);
	virtual	status_t			ReadStat(Node* node, struct stat* st);
	virtual	status_t			WriteStat(Node* node, struct stat *st,
									uint32 mask);
	virtual	status_t			Access(Node* node, int mode);

			// files
	virtual	status_t			Create(Node* dir, const char* name,
									int openMode, int mode, vnode_id* vnid,
									void** cookie);
	virtual	status_t			Open(Node* node, int openMode,
									void** cookie);
	virtual	status_t			Close(Node* node, void* cookie);
	virtual	status_t			FreeCookie(Node* node, void* cookie);
	virtual	status_t			Read(Node* node, void* cookie, off_t pos,
									void* buffer, size_t bufferSize,
									size_t* bytesRead);
	virtual	status_t			Write(Node* node, void* cookie, off_t pos,
									const void* buffer, size_t bufferSize,
									size_t* bytesWritten);

			// hard links / symlinks
	virtual	status_t			Link(Node* dir, const char* name,
									Node* node);
	virtual	status_t			Unlink(Node* dir, const char* name);
	virtual	status_t			Symlink(Node* dir, const char* name,
									const char* target);
	virtual	status_t			ReadLink(Node* node, char* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			Rename(Node* oldDir, const char* oldName,
									Node* newDir, const char* newName);

			// directories
	virtual	status_t			MkDir(Node* dir, const char* name,
									int mode);
	virtual	status_t			RmDir(Node* dir, const char* name);
	virtual	status_t			OpenDir(Node* node, void** cookie);
	virtual	status_t			CloseDir(Node* node, void* cookie);
	virtual	status_t			FreeDirCookie(Node* node, void* cookie);
	virtual	status_t			ReadDir(Node* node, void* cookie,
									struct dirent* buffer, size_t bufferSize,
									int32 count, int32* countRead);
	virtual	status_t			RewindDir(Node* node, void* cookie);
	virtual	status_t			Walk(Node* dir, const char* entryName,
									char** resolvedPath, vnode_id* vnid);

			// attributes
	virtual	status_t			OpenAttrDir(Node* node, void** cookie);
	virtual	status_t			CloseAttrDir(Node* node, void* cookie);
	virtual	status_t			FreeAttrDirCookie(Node* node,
									void* cookie);
	virtual	status_t			ReadAttrDir(Node* node, void* cookie,
									struct dirent* buffer, size_t bufferSize,
									int32 count, int32* countRead);
	virtual	status_t			RewindAttrDir(Node* node, void* cookie);
	virtual	status_t			ReadAttr(Node* node, const char* name,
									int type, off_t pos, void* buffer,
									size_t bufferSize, size_t* bytesRead);
	virtual	status_t			WriteAttr(Node* node, const char* name,
									int type, off_t pos, const void* buffer,
									size_t bufferSize, size_t* bytesWritten);
	virtual	status_t			RemoveAttr(Node* node, const char* name);
	virtual	status_t			RenameAttr(Node* node,
									const char* oldName, const char* newName);
	virtual	status_t			StatAttr(Node* node, const char* name,
									struct attr_info* attrInfo);

			// queries
			status_t			GetQueryEntry(const EntryInfo& entryInfo,
									const NodeInfo& dirInfo,
									struct dirent* buffer, size_t bufferSize,
									int32* countRead);


			// service methods called from "outside"
			void				ProcessNodeMonitoringRequest(
									NodeMonitoringRequest* request);
			void				ConnectionClosed();

private:
			struct NodeMap;
			struct EntryKey;
			struct EntryMap;
			struct LocalNodeIDMap;
			struct RemoteNodeIDMap;
			struct DirCookie;
			struct AttrDirCookie;
			struct AttrDirIteratorMap;

private:
			status_t			_ReadRemoteDir(ShareDir* directory,
									RemoteShareDirIterator* remoteIterator);

			void				_HandleEntryCreatedRequest(
									EntryCreatedRequest* request);
			void				_HandleEntryRemovedRequest(
									EntryRemovedRequest* request);
			void				_HandleEntryMovedRequest(
									EntryMovedRequest* request);
			void				_HandleStatChangedRequest(
									StatChangedRequest* request);
			void				_HandleAttributeChangedRequest(
									AttributeChangedRequest* request);

			status_t			_GetLocalNodeID(NodeID remoteID, ino_t* localID,
									bool enter);
			status_t			_GetRemoteNodeID(ino_t localID,
									NodeID* remoteID);
			void				_RemoveLocalNodeID(ino_t localID);

			ShareNode*			_GetNodeByLocalID(ino_t localID);
			ShareNode*			_GetNodeByRemoteID(NodeID remoteID);
			status_t			_LoadNode(const NodeInfo& nodeInfo,
									ShareNode** node);
			status_t			_UpdateNode(const NodeInfo& nodeInfo);

			ShareDirEntry*		_GetEntryByLocalID(ino_t localDirID,
									const char* name);
			ShareDirEntry*		_GetEntryByRemoteID(NodeID remoteDirID,
									const char* name);
			status_t			_LoadEntry(ShareDir* directory,
									const EntryInfo& entryInfo,
									ShareDirEntry** entry);
			void				_RemoveEntry(ShareDirEntry* entry);
			bool				_IsObsoleteEntryInfo(
									const EntryInfo& entryInfo);

			status_t			_AddAttrDirIterator(ShareNode* node,
									ShareAttrDirIterator* iterator);
			void				_RemoveAttrDirIterator(ShareNode* node,
									ShareAttrDirIterator* iterator);
			status_t			_LoadAttrDir(ShareNode* node,
									const AttrDirInfo& attrDirInfo);
			status_t			_UpdateAttrDir(NodeID remoteID,
									const AttrDirInfo& attrDirInfo);

			void				_NodeRemoved(NodeID remoteID);
			void				_EntryCreated(NodeID remoteDirID,
									const char* name,
									const EntryInfo* entryInfo, int64 revision);
			void				_EntryRemoved(NodeID remoteDirID,
									const char* name, int64 revision);
			void				_EntryMoved(NodeID remoteOldDirID,
									const char* oldName, NodeID remoteNewDirID,
									const char* name,
									const EntryInfo* entryInfo, int64 revision);

			status_t			_Walk(NodeID remoteDirID, const char* entryName,
									bool resolveLink, WalkReply** reply);
			status_t			_MultiWalk(
									RequestMemberArray<EntryInfo>& entryInfos,
									MultiWalkReply** reply);
			status_t			_Close(intptr_t cookie);

			uint32				_GetConnectionState();
			bool				_IsConnected();
			bool				_EnsureShareMounted();
			status_t			_MountShare();

private:
			int32				fID;
			uint32				fFlags;
			Locker				fMountLock;
			ShareDir*			fRootNode;
			NodeMap*			fNodes;			// local ID -> ShareNode
			EntryMap*			fEntries;		// local ID, name -> ShareDirEntry
			AttrDirIteratorMap*	fAttrDirIterators;
				// local ID -> DoublyLinkedList<>
			LocalNodeIDMap*		fLocalNodeIDs;	// remote ID -> local ID
			RemoteNodeIDMap*	fRemoteNodeIDs;	// local ID -> remote ID
			ServerConnectionProvider* fServerConnectionProvider;
			ExtendedServerInfo*	fServerInfo;
			ExtendedShareInfo*	fShareInfo;
			ServerConnection*	fServerConnection;
			RequestConnection*	fConnection;
			uint32				fSharePermissions;
			uint32				fConnectionState;
};

#endif	// NET_FS_SHARE_VOLUME_H
