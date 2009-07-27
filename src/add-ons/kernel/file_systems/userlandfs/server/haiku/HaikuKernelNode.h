/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_KERNEL_NODE_H
#define USERLAND_FS_HAIKU_KERNEL_NODE_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include <util/OpenHashTable.h>

#include "FSCapabilities.h"


namespace UserlandFS {

class HaikuKernelVolume;

using UserlandFSUtil::FSVNodeCapabilities;


struct HaikuKernelNode : fs_vnode {
	struct Capabilities;

			ino_t				id;
			HaikuKernelVolume*	volume;
			Capabilities*		capabilities;
			bool				published;

public:
	inline						HaikuKernelNode(HaikuKernelVolume* volume,
									ino_t vnodeID, void* privateNode,
									fs_vnode_ops* ops,
									Capabilities* capabilities);
								~HaikuKernelNode();

	static	HaikuKernelNode*	GetNode(fs_vnode* node);

			HaikuKernelVolume*	GetVolume() const	{ return volume; }
};


struct HaikuKernelNode::Capabilities {
	int32				refCount;
	fs_vnode_ops*		ops;
	FSVNodeCapabilities	capabilities;
	Capabilities*		hashLink;

	Capabilities(fs_vnode_ops* ops, FSVNodeCapabilities	capabilities)
		:
		refCount(1),
		ops(ops),
		capabilities(capabilities)
	{
	}
};


HaikuKernelNode::HaikuKernelNode(HaikuKernelVolume* volume, ino_t vnodeID,
	void* privateNode, fs_vnode_ops* ops, Capabilities* capabilities)
	:
	id(vnodeID),
	volume(volume),
	capabilities(capabilities),
	published(false)
{
	this->private_node = privateNode;
	this->ops = ops;
}


/*static*/ inline HaikuKernelNode*
HaikuKernelNode::GetNode(fs_vnode* node)
{
	return static_cast<HaikuKernelNode*>(node);
}


}	// namespace UserlandFS

using UserlandFS::HaikuKernelNode;

#endif	// USERLAND_FS_NODE_H
