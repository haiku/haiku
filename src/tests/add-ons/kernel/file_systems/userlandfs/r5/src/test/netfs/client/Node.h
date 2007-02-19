// Node.h

#ifndef NET_FS_NODE_H
#define NET_FS_NODE_H

#include <fsproto.h>

#include "ObjectTracker.h"

class Volume;

class Node ONLY_OBJECT_TRACKABLE_BASE_CLASS {
public:
								Node(Volume* volume, vnode_id id);
	virtual						~Node();

			Volume*				GetVolume() const			{ return fVolume; }
			vnode_id			GetID() const				{ return fID; }

			void				SetKnownToVFS(bool known);
			bool				IsKnownToVFS() const;

private:
			Volume*				fVolume;
			vnode_id			fID;
			bool				fKnownToVFS;
};

#endif	// NET_FS_NODE_H
