/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WEAK_REFERENCEABLE_H
#define _WEAK_REFERENCEABLE_H


#include <Referenceable.h>


namespace BPrivate {

class WeakReferenceable;

class WeakPointer : public Referenceable {
public:
			WeakReferenceable*	Get();
			bool				Put();

			int32				UseCount() const;

private:
	friend class WeakReferenceable;

								WeakPointer(WeakReferenceable* object);
								~WeakPointer();

private:
			bool				_GetUnchecked();

private:
			vint32				fUseCount;
			WeakReferenceable*	fObject;
};

class WeakReferenceable {
public:
								WeakReferenceable();
	virtual						~WeakReferenceable();

			void				AddReference()
									{ fPointer->_GetUnchecked(); }

			bool				RemoveReference()
									{ return fPointer->Put(); }

			int32				CountReferences() const
									{ return fPointer->UseCount(); }

			WeakPointer*		GetWeakPointer();

protected:
			WeakPointer*		fPointer;
};

template<typename Type = BPrivate::Referenceable>
class WeakReference {
public:
	WeakReference()
		:
		fPointer(NULL),
		fObject(NULL)
	{
	}

	WeakReference(Type* object)
		:
		fPointer(NULL),
		fObject(NULL)
	{
		SetTo(object);
	}

	WeakReference(const WeakPointer& other)
		:
		fPointer(NULL),
		fObject(NULL)
	{
		SetTo(&other);
	}

	WeakReference(const WeakReference<Type>& other)
		:
		fPointer(NULL),
		fObject(NULL)
	{
		SetTo(other.fPointer);
	}

	~WeakReference()
	{
		Unset();
	}

	void SetTo(Type* object)
	{
		Unset();

		if (object != NULL) {
			fPointer = object->GetWeakPointer();
			fObject = fPointer->Get();
		}
	}

	void SetTo(WeakPointer* pointer)
	{
		Unset();

		if (pointer != NULL) {
			fPointer = pointer->AddReference();
			fObject = pointer->Get();
		}
	}

	void Unset()
	{
		if (fPointer != NULL) {
			if (fObject != NULL) {
				fPointer->Put();
				fObject = NULL;
			}
			fPointer->RemoveReference();
			fPointer = NULL;
		}
	}

	Type* Get() const
	{
		return fObject;
	}

	Type* Detach()
	{
		Type* object = fObject;
		Unset();
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

	WeakReference& operator=(const WeakReference<Type>& other)
	{
		SetTo(other.fPointer);
		return *this;
	}

	WeakReference& operator=(const Type& other)
	{
		SetTo(&other);
		return *this;
	}

	bool operator==(const WeakReference<Type>& other) const
	{
		return fPointer == other.fPointer;
	}

	bool operator!=(const WeakReference<Type>& other) const
	{
		return fPointer != other.fPointer;
	}

private:
	WeakPointer*	fPointer;
	Type*			fObject;
};


//	#pragma mark -


inline WeakReferenceable*
WeakPointer::Get()
{
	int32 count;

	do {
		count = fUseCount;
		if (count == 0)
			return NULL;
	} while (atomic_test_and_set(&fUseCount, count, count + 1) != count);

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


inline
WeakPointer::WeakPointer(WeakReferenceable* object)
	:
	fUseCount(1),
	fObject(object)
{
}


inline
WeakPointer::~WeakPointer()
{
}


inline bool
WeakPointer::_GetUnchecked()
{
	return atomic_add(&fUseCount, 1) == 1;
}


//	#pragma -


inline
WeakReferenceable::WeakReferenceable()
	:
	fPointer(new WeakPointer(this))
{
}


inline WeakPointer*
WeakReferenceable::GetWeakPointer()
{
	fPointer->AddReference();
	return fPointer;
}

}	// namespace BPrivate

using BPrivate::WeakReferenceable;
using BPrivate::WeakPointer;
using BPrivate::WeakReference;

#endif	// _WEAK_REFERENCEABLE_H
