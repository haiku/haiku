/*
 * Copyright 2004-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REFERENCEABLE_H
#define _REFERENCEABLE_H


#include <SupportDefs.h>


// #pragma mark - BReferenceable


class BReferenceable {
public:
								BReferenceable();
	virtual						~BReferenceable();

								// acquire and release return
								// the previous ref count
			int32				AcquireReference();
			int32				ReleaseReference();

			int32				CountReferences() const
									{ return fReferenceCount; }

protected:
	virtual	void				FirstReferenceAcquired();
	virtual	void				LastReferenceReleased();

protected:
			vint32				fReferenceCount;
};


// #pragma mark - BReference


template<typename Type = BReferenceable>
class BReference {
public:
	BReference()
		:
		fObject(NULL)
	{
	}

	BReference(Type* object, bool alreadyHasReference = false)
		:
		fObject(NULL)
	{
		SetTo(object, alreadyHasReference);
	}

	BReference(const BReference<Type>& other)
		:
		fObject(NULL)
	{
		SetTo(other.fObject);
	}

	
	template<typename OtherType>
	BReference(const BReference<OtherType>& other)
		:
		fObject(NULL)
	{
		SetTo(other.Get());
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

	BReference& operator=(Type* other)
	{
		SetTo(other);
		return *this;
	}

	template<typename OtherType>
	BReference& operator=(const BReference<OtherType>& other)
	{
		SetTo(other.Get());
		return *this;
	}

	bool operator==(const BReference<Type>& other) const
	{
		return fObject == other.fObject;
	}

	bool operator==(const Type* other) const
	{
		return fObject == other;
	}

	bool operator!=(const BReference<Type>& other) const
	{
		return fObject != other.fObject;
	}

	bool operator!=(const Type* other) const
	{
		return fObject != other;
	}

private:
	Type*	fObject;
};


#endif	// _REFERENCEABLE_H
