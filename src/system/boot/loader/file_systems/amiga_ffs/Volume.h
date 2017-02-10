/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef VOLUME_H
#define VOLUME_H


#include "amiga_ffs.h"

#include <SupportDefs.h>

namespace boot {
	class Partition;
}


namespace FFS {

class Directory;

class Volume {
	public:
		Volume(boot::Partition *partition);
		~Volume();

		status_t			InitCheck();

		int					Device() const { return fDevice; }
		Directory			*Root() { return fRoot; }
		int32				Type() const { return fType; }
		int32				BlockSize() const { return fRootNode.BlockSize(); }

	protected:
		int					fDevice;
		int32				fType;
		RootBlock			fRootNode;
		Directory			*fRoot;
};

}	// namespace FFS

#endif	/* VOLUME_H */
