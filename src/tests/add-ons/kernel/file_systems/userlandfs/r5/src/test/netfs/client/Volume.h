// Volume.h

#ifndef NET_FS_VOLUME_H
#define NET_FS_VOLUME_H

#include <fsproto.h>

#include "FSObject.h"
#include "Locker.h"
#include "String.h"

class Node;
class QueryIterator;
class VolumeEvent;
class VolumeManager;

class Volume : public FSObject {
public:
								Volume(VolumeManager* volumeManager);
								~Volume();

			VolumeManager*		GetVolumeManager() const;

			void				SetParentVolume(Volume* parent);
			Volume*				GetParentVolume() const;
// TODO: Add the parent node ID and maybe get rid of parent volume.

			void				PutVolume();

			status_t			Init(const char* name);
			void				Uninit();

			const char*			GetName() const;

			vnode_id			GetRootID() const;
	virtual	Node*				GetRootNode() const = 0;

			void				SetUnmounting(bool unmounting);
			bool				IsUnmounting() const;

	virtual	void				PrepareToUnmount();

	virtual	void				RemoveChildVolume(Volume* volume) = 0;

	virtual	void				HandleEvent(VolumeEvent* event);

			// client methods
			status_t			GetVNode(vnode_id vnid, Node** node);
			status_t			PutVNode(vnode_id vnid);
			status_t			NewVNode(vnode_id vnid, Node* node);
			status_t			RemoveVNode(vnode_id vnid);
			status_t			UnremoveVNode(vnode_id vnid);
			int					IsVNodeRemoved(vnode_id vnid);

			int					SendNotification(port_id port, int32 token,
									uint32 what, int32 op, nspace_id nsida,
									nspace_id nsidb, vnode_id vnida,
									vnode_id vnidb, vnode_id vnidc,
									const char *name);
			int					NotifyListener(int32 opcode, nspace_id nsid,
									vnode_id vnida,	vnode_id vnidb,
									vnode_id vnidc, const char *name);

			// FS
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSStat(fs_info* info);
	virtual	status_t			WriteFSStat(struct fs_info* info, int32 mask);

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
	virtual	status_t			IOCtl(Node* node, void* cookie, int cmd,
									void* buffer, size_t bufferSize);
	virtual	status_t			SetFlags(Node* node, void* cookie,
									int flags);

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
	virtual	status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, int32 token,
									QueryIterator** iterator);
	virtual	void				FreeQueryIterator(QueryIterator* iterator);
	virtual	status_t			ReadQuery(QueryIterator* iterator,
									struct dirent* buffer, size_t bufferSize,
									int32 count, int32* countRead);

protected:
			Locker				fLock;
			VolumeManager*		fVolumeManager;
			Volume*				fParentVolume;
			String				fName;
			bool				fUnmounting;
};

#endif	// NET_FS_VOLUME_H
