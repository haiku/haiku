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
			int32				fReferenceCount;
};


// #pragma mark - BReference


template<typename Type = BReferenceable, typename ConstType = Type>
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
		SetTo(other.Get());
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

	ConstType* Get() const
	{
		return fObject;
	}

	ConstType* Detach()
	{
		Type* object = fObject;
		fObject = NULL;
		return object;
	}

	ConstType& operator*() const
	{
		return *fObject;
	}

	ConstType* operator->() const
	{
		return fObject;
	}

	operator ConstType*() const
	{
		return fObject;
	}

	BReference& operator=(const BReference<Type, ConstType>& other)
	{
		SetTo(other.fObject);
		return *this;
	}

	BReference& operator=(Type* other)
	{
		SetTo(other);
		return *this;
	}

	template<typename OtherType, typename OtherConstType>
	BReference& operator=(const BReference<OtherType, OtherConstType>& other)
	{
		SetTo(other.Get());
		return *this;
	}

	bool operator==(const BReference<Type, ConstType>& other) const
	{
		return fObject == other.fObject;
	}

	bool operator==(const Type* other) const
	{
		return fObject == other;
	}

	bool operator!=(const BReference<Type, ConstType>& other) const
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


// #pragma mark - BReference


template<typename Type = BReferenceable>
class BConstReference: public BReference<Type, const Type> {
public:
	BConstReference()
		:
		BReference<Type, const Type>()
	{
	}

	BConstReference(Type* object, bool alreadyHasReference = false)
		:
		BReference<Type, const Type>(object, alreadyHasReference)
	{
	}

	BConstReference(const BReference<Type>& other)
		:
		BReference<Type, const Type>(other)
	{
	}

	// Allow assignment of a const reference from a mutable one (but not the
	// reverse).
	BConstReference& operator=(const BReference<Type, Type>& other)
	{
		SetTo(other.Get());
		return *this;
	}

};


#endif	// _REFERENCEABLE_H
