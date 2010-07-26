/*
 * Copyright 2004-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REFERENCEABLE_H
#define _REFERENCEABLE_H


#include <SupportDefs.h>


class BReferenceable {
public:
								BReferenceable(
									bool deleteWhenUnreferenced = true);
										// TODO: The parameter is deprecated.
										// Override LastReferenceReleased()
										// instead!
	virtual						~BReferenceable();

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
BReferenceable::AddReference()
{
	AcquireReference();
}


bool
BReferenceable::RemoveReference()
{
	return ReleaseReference();
}


// BReference
template<typename Type = BReferenceable>
class BReference {
public:
	BReference()
		: fObject(NULL)
	{
	}

	BReference(Type* object, bool alreadyHasReference = false)
		: fObject(NULL)
	{
		SetTo(object, alreadyHasReference);
	}

	BReference(const BReference<Type>& other)
		: fObject(NULL)
	{
		SetTo(other.fObject);
	}

	~BReference()
	{
		Unset();
	}

	void SetTo(Type* object, bool alreadyHasReference = false)
	{
		if (object != NULL && !alreadyHasReference)
			object->AcquireReference();

		Unset();

		fObject = object;
	}

	void Unset()
	{
		if (fObject) {
			fObject->ReleaseReference();
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

	operator Type*() const
	{
		return fObject;
	}

	BReference& operator=(const BReference<Type>& other)
	{
		SetTo(other.fObject);
		return *this;
	}

	bool operator==(const BReference<Type>& other) const
	{
		return (fObject == other.fObject);
	}

	bool operator!=(const BReference<Type>& other) const
	{
		return (fObject != other.fObject);
	}

private:
	Type*	fObject;
};


// #pragma mark Obsolete API

// TODO: To be phased out!


namespace BPrivate {


// Reference
template<typename Type = BReferenceable>
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
		if (object != NULL && !alreadyHasReference)
			object->AddReference();

		Unset();

		fObject = object;
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


typedef BReferenceable Referenceable;

}	// namespace BPrivate

using BPrivate::Referenceable;
using BPrivate::Reference;


#endif	// _REFERENCEABLE_H
