/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef INODE_H
#define INODE_H


#include <fs_cache.h>
#include <lock.h>
#include <string.h>

#include "DirectoryIterator.h"
#include "exfat.h"
#include "SplayTree.h"
#include "Volume.h"


//#define TRACE_EXFAT
#ifdef TRACE_EXFAT
#	define TRACEI(x...) dprintf("\33[34mexfat:\33[0m " x)
#else
#	define TRACEI(x...) ;
#endif


struct InodesTreeDefinition;


class Inode : EntryVisitor {
public:
						Inode(Volume* volume, cluster_t cluster,
							uint32 offset);
						Inode(Volume* volume, ino_t ino);
						~Inode();

			status_t	InitCheck();

			ino_t		ID() const { return fID; }
			ino_t		Parent() const { return fParent; }
			cluster_t	Cluster() const { return fCluster; }
			uint32_t	Offset() const { return fOffset; }
			cluster_t	StartCluster() const
							{ return fFileInfoEntry.file_info.StartCluster(); }
			bool		IsContiguous() const
							{ return fFileInfoEntry.file_info.IsContiguous(); }
			cluster_t	NextCluster(cluster_t cluster) const;

			rw_lock*	Lock() { return &fLock; }

			status_t	UpdateNodeFromDisk();

			bool		IsDirectory() const
							{ return S_ISDIR(Mode()); }
			bool		IsFile() const
							{ return S_ISREG(Mode()); }
			bool		IsSymLink() const
							{ return S_ISLNK(Mode()); }
			status_t	CheckPermissions(int accessMode) const;

			mode_t		Mode() const;
			off_t		Size() const { return fFileInfoEntry.file_info.Size(); }
			uid_t		UserID() const { return 0;/*fNode.UserID();*/ }
			gid_t		GroupID() const { return 0;/*fNode.GroupID();*/ }
			void		GetChangeTime(struct timespec &timespec) const
							{ GetModificationTime(timespec); }
			void		GetModificationTime(struct timespec &timespec) const
							{ _GetTimespec(fFileEntry.file.ModificationDate(),
								fFileEntry.file.ModificationTime(), timespec); }
			void		GetCreationTime(struct timespec &timespec) const
							{ _GetTimespec(fFileEntry.file.CreationDate(),
								fFileEntry.file.CreationTime(), timespec); }
			void		GetAccessTime(struct timespec &timespec) const
							{ _GetTimespec(fFileEntry.file.AccessDate(),
								fFileEntry.file.AccessTime(), timespec); }

			Volume*		GetVolume() const { return fVolume; }

			status_t	FindBlock(off_t logical, off_t& physical,
							off_t *_length = NULL);
			status_t	ReadAt(off_t pos, uint8 *buffer, size_t *length);
			status_t	FillGapWithZeros(off_t start, off_t end);

			void*		FileCache() const { return fCache; }
			void*		Map() const { return fMap; }

			bool		VisitFile(struct exfat_entry*);
			bool		VisitFileInfo(struct exfat_entry*);
	
private:
			friend struct InodesInoTreeDefinition;
			friend struct InodesClusterTreeDefinition;
 
						Inode(Volume* volume);
						Inode(const Inode&);
						Inode &operator=(const Inode&);
							// no implementation

			void		_GetTimespec(uint16 date, uint16 time,
							struct timespec &timespec) const;
			void		_Init();

			rw_lock		fLock;
			::Volume*	fVolume;
			ino_t		fID;
			ino_t		fParent;
			cluster_t 	fCluster;
			uint32		fOffset;
			uint32		fFlags;
			void*		fCache;
			void*		fMap;
			status_t	fInitStatus;

			SplayTreeLink<Inode> fInoTreeLink;
			Inode*				fInoTreeNext;
			SplayTreeLink<Inode> fClusterTreeLink;
			Inode*				fClusterTreeNext;

			struct exfat_entry	fFileEntry;
			struct exfat_entry	fFileInfoEntry;
};


// The Vnode class provides a convenience layer upon get_vnode(), so that
// you don't have to call put_vnode() anymore, which may make code more
// readable in some cases

class Vnode {
public:
	Vnode(Volume* volume, ino_t id)
		:
		fInode(NULL)
	{
		SetTo(volume, id);
	}

	Vnode()
		:
		fStatus(B_NO_INIT),
		fInode(NULL)
	{
	}

	~Vnode()
	{
		Unset();
	}

	status_t InitCheck()
	{
		return fStatus;
	}

	void Unset()
	{
		if (fInode != NULL) {
			put_vnode(fInode->GetVolume()->FSVolume(), fInode->ID());
			fInode = NULL;
			fStatus = B_NO_INIT;
		}
	}

	status_t SetTo(Volume* volume, ino_t id)
	{
		Unset();

		return fStatus = get_vnode(volume->FSVolume(), id, (void**)&fInode);
	}

	status_t Get(Inode** _inode)
	{
		*_inode = fInode;
		return fStatus;
	}

	void Keep()
	{
		TRACEI("Vnode::Keep()\n");
		fInode = NULL;
	}

private:
	status_t	fStatus;
	Inode*		fInode;
};


struct InodesInoTreeDefinition {
	typedef ino_t KeyType;
	typedef	Inode NodeType;

	static KeyType GetKey(const NodeType* node)
	{
		return node->ID();
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->fInoTreeLink;
	}

	static int Compare(KeyType key, const NodeType* node)
	{
		return key == node->ID() ? 0
			: (key < node->ID() ? -1 : 1);
	}

	static NodeType** GetListLink(NodeType* node)
	{
		return &node->fInoTreeNext;
	}
};

typedef IteratableSplayTree<InodesInoTreeDefinition> InodesInoTree;

struct InodesClusterTreeDefinition {
	typedef cluster_t KeyType;
	typedef	Inode NodeType;

	static KeyType GetKey(const NodeType* node)
	{
		return node->Cluster();
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->fClusterTreeLink;
	}

	static int Compare(KeyType key, const NodeType* node)
	{
		return key == node->Cluster() ? 0
			: (key < node->Cluster() ? -1 : 1);
	}

	static NodeType** GetListLink(NodeType* node)
	{
		return &node->fClusterTreeNext;
	}
};

typedef IteratableSplayTree<InodesClusterTreeDefinition> InodesClusterTree;

#endif	// INODE_H
