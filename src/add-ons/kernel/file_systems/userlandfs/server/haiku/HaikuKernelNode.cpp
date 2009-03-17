/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "HaikuKernelNode.h"

#include "HaikuKernelFileSystem.h"


HaikuKernelNode::~HaikuKernelNode()
{
	if (capabilities != NULL) {
		HaikuKernelFileSystem* fileSystem
			= static_cast<HaikuKernelFileSystem*>(FileSystem::GetInstance());
		fileSystem->PutNodeCapabilities(capabilities);
	}
}
