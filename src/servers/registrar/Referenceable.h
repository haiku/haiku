/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef REFERENCABLE_H
#define REFERENCABLE_H

#include <SupportDefs.h>

// Referenceable
class Referenceable {
public:
								Referenceable(
									bool deleteWhenUnreferenced = false);
	virtual						~Referenceable();

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

#endif	// REFERENCABLE_H
