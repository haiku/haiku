/*
 * Copyright 2004-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REFERENCEABLE_H
#define _REFERENCEABLE_H


#include <SupportDefs.h>


namespace BPrivate {

class Referenceable {
public:
								Referenceable(
									bool deleteWhenUnreferenced = true);
										// TODO: The parameter is deprecated.
										// Override LastReferenceReleased()
										// instead!
	virtual						~Referenceable();

			void				AcquireReference();
			bool				ReleaseReference();
									// returns true after last

			int32				CountReferences() const
									{ return fReferenceCount; }

			// deprecate aliases
	inline	void				AddReference();
	inline	bool				RemoveReference();

protected:
	virtual	void				FirstReferenceAcquired();
	virtual	void				LastReferenceReleased();

protected:
			vint32				fReferenceCount;
			bool				fDeleteWhenUnreferenced;
};


void
Referenceable::AddReference()
{
	AcquireReference();
}


bool
Referenceable::RemoveReference()
{
	return ReleaseReference();
}


// Reference
template<typename Type = BPrivate::Referenceable>
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

}	// namespace BPrivate

using BPrivate::Referenceable;
using BPrivate::Reference;

typedef BPrivate::Referenceable BReferenceable;


#endif	// _REFERENCEABLE_H
