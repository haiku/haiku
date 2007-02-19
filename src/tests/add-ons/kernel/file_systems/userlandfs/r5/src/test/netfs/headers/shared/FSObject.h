// FSObject.h

#ifndef NET_FS_FS_OBJECT_H
#define NET_FS_FS_OBJECT_H

#include "Referencable.h"

class FSObject : public Referencable {
public:
								FSObject();
	virtual						~FSObject();

			void				MarkRemoved();
			bool				IsRemoved() const;

private:
			bool				fRemoved;
};

#endif	// NET_FS_FS_OBJECT_H
