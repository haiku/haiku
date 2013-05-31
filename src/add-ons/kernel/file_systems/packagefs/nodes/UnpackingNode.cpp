/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingNode.h"

#include "DebugSupport.h"
#include "Node.h"
#include "PackageNode.h"


UnpackingNode::~UnpackingNode()
{
}


status_t
UnpackingNode::CloneTransferPackageNodes(ino_t id, UnpackingNode*& _newNode)
{
	return B_BAD_VALUE;
}


status_t
UnpackingNode::NodeInitVFS(dev_t deviceID, ino_t nodeID,
	PackageNode* packageNode)
{
	status_t error = B_OK;
	if (packageNode != NULL)
		error = packageNode->VFSInit(deviceID, nodeID);

	return error;
}


void
UnpackingNode::NodeUninitVFS(PackageNode* packageNode, uint32& nodeFlags)
{
	if (packageNode != NULL) {
		if ((nodeFlags & NODE_FLAG_VFS_INIT_ERROR) == 0)
			packageNode->VFSUninit();
		else
			nodeFlags &= ~(uint32)NODE_FLAG_VFS_INIT_ERROR;
	}
}


void
UnpackingNode::NodeReinitVFS(dev_t deviceID, ino_t nodeID,
	PackageNode* packageNode, PackageNode* previousPackageNode,
	uint32& nodeFlags)
{
	if ((nodeFlags & NODE_FLAG_KNOWN_TO_VFS) == 0)
		return;

	if (packageNode != previousPackageNode) {
		bool hadInitError = (nodeFlags & NODE_FLAG_VFS_INIT_ERROR) != 0;
		nodeFlags &= ~(uint32)NODE_FLAG_VFS_INIT_ERROR;

		if (packageNode != NULL) {
			status_t error = packageNode->VFSInit(deviceID, nodeID);
			if (error != B_OK) {
				ERROR("UnpackingNode::NodeReinitVFS(): VFSInit() failed for "
					"(%" B_PRIdDEV ", %" B_PRIdINO ")\n", deviceID, nodeID);
				nodeFlags |= NODE_FLAG_VFS_INIT_ERROR;
			}
		}

		if (previousPackageNode != NULL && !hadInitError)
			previousPackageNode->VFSUninit();
	}
}
