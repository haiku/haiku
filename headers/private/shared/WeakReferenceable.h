/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WEAK_REFERENCEABLE_H
#define _WEAK_REFERENCEABLE_H


#include <Referenceable.h>


namespace BPrivate {

template<typename Type> class WeakReferenceable;

template<typename Type>
class WeakPointer : public BReferenceable {
public:
			Type*				Get();
			bool				Put();

			int32				UseCount() const;

private:
	friend class WeakReferenceable<Type>;

								WeakPointer(Type* object);
								~WeakPointer();

private:
			void				_GetUnchecked();

private:
			vint32				fUseCount;
			Type*				fObject;
};

template<typename Type>
class WeakReferenceable {
public:
								WeakReferenceable(Type* object);
								~WeakReferenceable();

			void				AcquireReference()
									{ fPointer->_GetUnchecked(); }

			bool				ReleaseReference()
									{ return fPointer->Put(); }

			int32				CountReferences() const
									{ return fPointer->UseCount(); }

			WeakPointer<Type>*	GetWeakPointer();

protected:
			WeakPointer<Type>*	fPointer;
};

template<typename Type>
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

	WeakReference(WeakPointer<Type>& other)
		:
		fPointer(NULL),
		fObject(NULL)
	{
		SetTo(&other);
	}

	WeakReference(WeakPointer<Type>* other)
		:
		fPointer(NULL),
		fObject(NULL)
	{
		SetTo(other);
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

	void SetTo(WeakPointer<Type>* pointer)
	{
		Unset();

		if (pointer != NULL) {
			fPointer = pointer;
			fPointer->AcquireReference();
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
			fPointer->ReleaseReference();
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

	operator Type*() const
	{
		return fObject;
	}

	Type* operator->() const
	{
		return fObject;
	}

	WeakReference& operator=(const WeakReference<Type>& other)
	{
		if (this == &other)
			return *this;

		SetTo(other.fPointer);
		return *this;
	}

	WeakReference& operator=(const Type& other)
	{
		SetTo(&other);
		return *this;
	}

	WeakReference& operator=(WeakPointer<Type>& other)
	{
		SetTo(&other);
		return *this;
	}

	WeakReference& operator=(WeakPointer<Type>* other)
	{
		SetTo(other);
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
	WeakPointer<Type>*	fPointer;
	Type*			fObject;
};


//	#pragma mark -


template<typename Type>
inline Type*
WeakPointer<Type>::Get()
{
	int32 count = -11;

	do {
		count = atomic_get(&fUseCount);
		if (count == 0)
			return NULL;
	} while (atomic_test_and_set(&fUseCount, count + 1, count) != count);

	return fObject;
}


template<typename Type>
inline bool
WeakPointer<Type>::Put()
{
	if (atomic_add(&fUseCount, -1) == 1) {
		delete fObject;
		return true;
	}

	return false;
}


template<typename Type>
inline int32
WeakPointer<Type>::UseCount() const
{
	return fUseCount;
}


template<typename Type>
inline
WeakPointer<Type>::WeakPointer(Type* object)
	:
	fUseCount(1),
	fObject(object)
{
}


template<typename Type>
inline
WeakPointer<Type>::~WeakPointer()
{
}


template<typename Type>
inline void
WeakPointer<Type>::_GetUnchecked()
{
	atomic_add(&fUseCount, 1);
}


//	#pragma -


template<typename Type>
inline
WeakReferenceable<Type>::WeakReferenceable(Type* object)
	:
	fPointer(new WeakPointer<Type>(object))
{
}


template<typename Type>
inline
WeakReferenceable<Type>::~WeakReferenceable()
{
	fPointer->ReleaseReference();
}


template<typename Type>
inline WeakPointer<Type>*
WeakReferenceable<Type>::GetWeakPointer()
{
	fPointer->AcquireReference();
	return fPointer;
}

}	// namespace BPrivate

using BPrivate::WeakReferenceable;
using BPrivate::WeakPointer;
using BPrivate::WeakReference;

#endif	// _WEAK_REFERENCEABLE_H
