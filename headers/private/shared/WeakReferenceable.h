/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WEAK_REFERENCEABLE_H
#define _WEAK_REFERENCEABLE_H


#include <Referenceable.h>

#include <new>


namespace BPrivate {


class BWeakReferenceable;


class WeakPointer : public BReferenceable {
public:
								WeakPointer(BWeakReferenceable* object);
								~WeakPointer();

			BWeakReferenceable*	Get();
			bool				Put();

			int32				UseCount() const;

			void				GetUnchecked();

private:
			vint32				fUseCount;
			BWeakReferenceable*	fObject;
};


class BWeakReferenceable {
public:
								BWeakReferenceable();
	virtual						~BWeakReferenceable();

			status_t			InitCheck();

			void				AcquireReference()
									{ fPointer->GetUnchecked(); }

			bool				ReleaseReference()
									{ return fPointer->Put(); }

			int32				CountReferences() const
									{ return fPointer->UseCount(); }

			WeakPointer*		GetWeakPointer();
private:
			WeakPointer*		fPointer;
};


template<typename Type>
class BWeakReference {
public:
	BWeakReference()
		:
		fPointer(NULL)
	{
	}

	BWeakReference(Type* object)
		:
		fPointer(NULL)
	{
		SetTo(object);
	}

	BWeakReference(const BWeakReference<Type>& other)
		:
		fPointer(NULL)
	{
		SetTo(other);
	}

	BWeakReference(const BReference<Type>& other)
		:
		fPointer(NULL)
	{
		SetTo(other);
	}

	~BWeakReference()
	{
		Unset();
	}

	void SetTo(Type* object)
	{
		Unset();

		if (object != NULL)
			fPointer = object->GetWeakPointer();
	}

	void SetTo(const BWeakReference<Type>& other)
	{
		Unset();

		if (other.fPointer) {
			fPointer = other.fPointer;
			fPointer->AcquireReference();
		}
	}

	void SetTo(const BReference<Type>& other)
	{
		SetTo(other.Get());
	}

	void Unset()
	{
		if (fPointer != NULL) {
			fPointer->ReleaseReference();
			fPointer = NULL;
		}
	}

	bool IsAlive()
	{
		if (fPointer == NULL)
			return false;
		Type* object = static_cast<Type*>(fPointer->Get());
		if (object == NULL)
			return false;
		fPointer->Put();
		return true;
	}

	BReference<Type> GetReference()
	{
		Type* object = static_cast<Type*>(fPointer->Get());
		return BReference<Type>(object, true);
	}

	BWeakReference& operator=(const BWeakReference<Type>& other)
	{
		if (this == &other)
			return *this;

		SetTo(other.fPointer);
		return *this;
	}

	BWeakReference& operator=(const Type& other)
	{
		SetTo(&other);
		return *this;
	}

	BWeakReference& operator=(Type* other)
	{
		SetTo(other);
		return *this;
	}

	BWeakReference& operator=(const BReference<Type>& other)
	{
		SetTo(other.Get());
		return *this;
	}

	bool operator==(const BWeakReference<Type>& other) const
	{
		return fPointer == other.fPointer;
	}

	bool operator!=(const BWeakReference<Type>& other) const
	{
		return fPointer != other.fPointer;
	}

private:
	WeakPointer*	fPointer;
};


//	#pragma mark -


inline
WeakPointer::WeakPointer(BWeakReferenceable* object)
	:
	fUseCount(1),
	fObject(object)
{
}


inline
WeakPointer::~WeakPointer()
{
}


inline BWeakReferenceable*
WeakPointer::Get()
{
	int32 count = -11;

	do {
		count = atomic_get(&fUseCount);
		if (count == 0)
			return NULL;
	} while (atomic_test_and_set(&fUseCount, count + 1, count) != count);

	return fObject;
}


inline bool
WeakPointer::Put()
{
	if (atomic_add(&fUseCount, -1) == 1) {
		delete fObject;
		return true;
	}

	return false;
}


inline int32
WeakPointer::UseCount() const
{
	return fUseCount;
}


inline void
WeakPointer::GetUnchecked()
{
	atomic_add(&fUseCount, 1);
}


//	#pragma -


inline
BWeakReferenceable::BWeakReferenceable()
	:
	fPointer(new(std::nothrow) WeakPointer(this))
{
}


inline
BWeakReferenceable::~BWeakReferenceable()
{
	fPointer->ReleaseReference();
}


inline status_t
BWeakReferenceable::InitCheck()
{
	if (fPointer == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


inline WeakPointer*
BWeakReferenceable::GetWeakPointer()
{
	fPointer->AcquireReference();
	return fPointer;
}

}	// namespace BPrivate

using BPrivate::BWeakReferenceable;
using BPrivate::BWeakReference;

#endif	// _WEAK_REFERENCEABLE_H
