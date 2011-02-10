/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <lock.h>
#include <string.h>

#include "exfat.h"
#include "SplayTree.h"


struct node_key {
	cluster_t cluster;
	uint32 offset;
};

struct node {
	struct node_key key;
	ino_t ino;
	ino_t parent;
	SplayTreeLink<struct node> nodeTreeLink;
	SplayTreeLink<struct node> inoTreeLink;
};


struct NodeTreeDefinition {
	typedef struct node_key KeyType;
	typedef	struct node NodeType;

	static KeyType GetKey(const NodeType* node)
	{
		return node->key;
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->nodeTreeLink;
	}

	static int Compare(KeyType key, const NodeType* node)
	{
		if (key.cluster == node->key.cluster) {
			if (key.offset == node->key.offset)
				return 0;
			return key.offset < node->key.offset ? -1 : 1;
		}
		return key.cluster < node->key.cluster ? -1 : 1;
	}
};

struct InoTreeDefinition {
	typedef ino_t KeyType;
	typedef	struct node NodeType;

	static KeyType GetKey(const NodeType* node)
	{
		return node->ino;
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->inoTreeLink;
	}

	static int Compare(KeyType key, const NodeType* node)
	{
		if (key != node->ino)
			return key < node->ino ? -1 : 1;
		return 0;
	}
};


typedef SplayTree<NodeTreeDefinition> NodeTree;
typedef SplayTree<InoTreeDefinition> InoTree;
class Inode;
struct InodesInoTreeDefinition;
typedef IteratableSplayTree<InodesInoTreeDefinition> InodesInoTree;
struct InodesClusterTreeDefinition;
typedef IteratableSplayTree<InodesClusterTreeDefinition> InodesClusterTree;


enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};


class Volume {
public:
								Volume(fs_volume* volume);
								~Volume();

			status_t			Mount(const char* device, uint32 flags);
			status_t			Unmount();

			bool				IsValidSuperBlock();
			bool				IsReadOnly() const
								{ return (fFlags & VOLUME_READ_ONLY) != 0; }

			Inode*				RootNode() const { return fRootNode; }
			int					Device() const { return fDevice; }

			dev_t				ID() const
								{ return fFSVolume ? fFSVolume->id : -1; }
			fs_volume*			FSVolume() const { return fFSVolume; }
			const char*			Name() const;
			void				SetName(const char* name)
								{ strlcpy(fName, name, sizeof(fName)); }
			
			uint32				BlockSize() const { return fBlockSize; }
			uint32				EntriesPerBlock() const
								{ return fEntriesPerBlock; }
			uint32				EntriesPerCluster()
								{ return fEntriesPerBlock
									<< SuperBlock().BlocksPerClusterShift(); }
			size_t				ClusterSize() { return fBlockSize 
									<< SuperBlock().BlocksPerClusterShift(); }
			exfat_super_block&	SuperBlock() { return fSuperBlock; }

			status_t			LoadSuperBlock();

			// cache access
			void*				BlockCache() { return fBlockCache; }

	static	status_t			Identify(int fd, exfat_super_block* superBlock);

			status_t			ClusterToBlock(cluster_t cluster,
									fsblock_t &block);
			Inode *				FindInode(ino_t id);
			Inode *				FindInode(cluster_t cluster);
			cluster_t			NextCluster(cluster_t cluster);
			ino_t				GetIno(cluster_t cluster, uint32 offset, ino_t parent);
			struct node_key*	GetNode(ino_t ino, ino_t &parent);
private:
			ino_t				_NextID() { return fNextId++; }

			mutex				fLock;
			fs_volume*			fFSVolume;
			int					fDevice;
			exfat_super_block	fSuperBlock;
			char				fName[32];

			uint16				fFlags;
			uint32				fBlockSize;
			uint32				fEntriesPerBlock;
			Inode*				fRootNode;
			ino_t				fNextId;

			void*				fBlockCache;
			InodesInoTree*		fInodesInoTree;
			InodesClusterTree*	fInodesClusterTree;
			NodeTree			fNodeTree;
			InoTree				fInoTree;
};

#endif	// VOLUME_H

