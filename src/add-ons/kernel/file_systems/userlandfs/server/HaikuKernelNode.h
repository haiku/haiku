/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_KERNEL_NODE_H
#define USERLAND_FS_HAIKU_KERNEL_NODE_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include "FSCapabilities.h"

namespace UserlandFS {

class HaikuKernelVolume;

using UserlandFSUtil::FSVNodeCapabilities;


struct HaikuKernelNode : fs_vnode {
			HaikuKernelVolume*	volume;

public:
	static	HaikuKernelNode*	GetNode(fs_vnode* node);

			HaikuKernelVolume*	GetVolume() const	{ return volume; }
};


/*static*/ inline HaikuKernelNode*
HaikuKernelNode::GetNode(fs_vnode* node)
{
	return static_cast<HaikuKernelNode*>(node);
}


}	// namespace UserlandFS

using UserlandFS::HaikuKernelNode;

#endif	// USERLAND_FS_NODE_H
