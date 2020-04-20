// Volume.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef VOLUME_H
#define VOLUME_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include "hashes.h"
#include "List.h"

struct reiserfs_super_block;

class BlockCache;
class Tree;
class Settings;
class SuperBlock;
class VNode;

class Volume {
public:
	Volume();
	~Volume();

	status_t Identify(int fd, partition_data *partition);

	status_t Mount(fs_volume *fsVolume, const char *path);
	status_t Unmount();

	dev_t GetID() const				{ return fFSVolume->id; }
	fs_volume *GetFSVolume() const	{ return fFSVolume; }

	off_t GetBlockSize() const;
	off_t CountBlocks() const;
	off_t CountFreeBlocks() const;
	const char *GetName() const;
	void UpdateName(partition_id partitionID);
	const char *GetDeviceName() const;

	BlockCache *GetBlockCache() const	{ return fBlockCache; }
	Tree *GetTree() const				{ return fTree; }
	Settings *GetSettings() const		{ return fSettings; }

	status_t GetKeyOffsetForName(const char * name, int len, uint64 *result);

	VNode *GetRootVNode() const			{ return fRootVNode; }

	status_t GetVNode(ino_t id, VNode **node);
	status_t PutVNode(ino_t id);

	status_t FindVNode(ino_t id, VNode *node);
	status_t FindVNode(uint32 dirID, uint32 objectID, VNode *node);

	status_t FindDirEntry(VNode *dir, const char *entryName,
						  VNode *foundNode, bool failIfHidden = false);

	status_t ReadLink(VNode *node, char *buffer, size_t bufferSize,
					  size_t *linkLength);

	status_t FindEntry(const VNode *rootDir, const char *path,
					   VNode *foundNode);

	bool IsNegativeEntry(ino_t id);
	bool IsNegativeEntry(uint32 dirID, uint32 objectID);

	bool GetHideEsoteric() const;

private:
	status_t _ReadSuperBlock();
	void _InitHashFunction();
	uint32 _DetectHashFunction();
	bool _VerifyHashFunction(hash_function_t function);
	void _InitNegativeEntries();

private:
	fs_volume				*fFSVolume;
	int						fDevice;
	BlockCache				*fBlockCache;
	Tree					*fTree;
	SuperBlock				*fSuperBlock;
	hash_function_t			fHashFunction;
	VNode					*fRootVNode;
	char					*fDeviceName;
	Settings				*fSettings;
	List<ino_t>				fNegativeEntries;
	char					fVolumeName[B_OS_NAME_LENGTH];
};

#endif	// VOLUME_H
