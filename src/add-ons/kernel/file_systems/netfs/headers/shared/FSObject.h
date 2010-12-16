// FSObject.h

#ifndef NET_FS_FS_OBJECT_H
#define NET_FS_FS_OBJECT_H

#include <Referenceable.h>

class FSObject : public BReferenceable {
public:
								FSObject();
	virtual						~FSObject();

			void				MarkRemoved();
			bool				IsRemoved() const;

protected:
	virtual	void				LastReferenceReleased();

private:
			bool				fRemoved;
};

#endif	// NET_FS_FS_OBJECT_H
