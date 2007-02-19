// Referencable.h

#ifndef USERLAND_FS_REFERENCABLE_H
#define USERLAND_FS_REFERENCABLE_H

#include <SupportDefs.h>

#include "ObjectTracker.h"

namespace UserlandFSUtil {

// Referencable
class Referencable ONLY_OBJECT_TRACKABLE_BASE_CLASS {
public:
								Referencable(
									bool deleteWhenUnreferenced = false);
	virtual						~Referencable();

			void				AddReference();
			bool				RemoveReference();	// returns true after last

			int32				CountReferences() const;

protected:
			vint32				fReferenceCount;
			bool				fDeleteWhenUnreferenced;
};

// Reference
template<typename Type>
class Reference {
public:
	Reference()
		: fObject(NULL)
	{
	}

	Reference(Type* object, bool alreadyHasReference = false)
		: fObject(NULL)
	{
		SetTo(object, alreadyHasReference);
	}

	Reference(const Reference<Type>& other)
		: fObject(NULL)
	{
		SetTo(other.fObject);
	}

	~Reference()
	{
		Unset();
	}

	void SetTo(Type* object, bool alreadyHasReference = false)
	{
		Unset();
		fObject = object;
		if (fObject && !alreadyHasReference)
			fObject->AddReference();
	}

	void Unset()
	{
		if (fObject) {
			fObject->RemoveReference();
			fObject = NULL;
		}
	}

	Type* Get() const
	{
		return fObject;
	}

	Type* Detach()
	{
		Type* object = fObject;
		fObject = NULL;
		return object;
	}

	Type& operator*() const
	{
		return *fObject;
	}

	Type* operator->() const
	{
		return fObject;
	}

	Reference& operator=(const Reference<Type>& other)
	{
		SetTo(other.fObject);
		return *this;
	}

	bool operator==(const Reference<Type>& other) const
	{
		return (fObject == other.fObject);
	}

	bool operator!=(const Reference<Type>& other) const
	{
		return (fObject != other.fObject);
	}

private:
	Type*	fObject;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::Referencable;
using UserlandFSUtil::Reference;

#endif	// USERLAND_FS_REFERENCABLE_H
