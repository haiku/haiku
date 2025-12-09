/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_STACK_H
#define KERNEL_UTIL_STACK_H


#include <stdlib.h>

#include <SupportDefs.h>

#include <AutoDeleter.h>


template<class T> class Stack {
	public:
		Stack()
			:
			fArray(NULL),
			fUsed(0),
			fMax(0)
		{
		}

		~Stack()
		{
			free(fArray);
		}

		bool IsEmpty() const
		{
			return fUsed == 0;
		}

		void MakeEmpty()
		{
			// could also free the memory
			fUsed = 0;
		}

		status_t Push(T value)
		{
			if (fUsed >= fMax) {
				fMax += 16;
				T *newArray = (T *)realloc(fArray, fMax * sizeof(T));
				if (newArray == NULL)
					return B_NO_MEMORY;

				fArray = newArray;
			}
			fArray[fUsed++] = value;
			return B_OK;
		}

		bool Pop(T *value)
		{
			if (fUsed == 0)
				return false;

			*value = fArray[--fUsed];
			return true;
		}

		T *Array()
		{
			return fArray;
		}

		int32 CountItems() const
		{
			return fUsed;
		}

	private:
		T		*fArray;
		int32	fUsed;
		int32	fMax;
};


template<typename T> class StackDelete {
public:
	inline void operator()(Stack<T>* stack)
	{
		if (stack == NULL)
			return;

		T item;
		while (stack->Pop(&item)) {
			delete item;
		}
	
		delete stack;
	}
};

template<typename T> class StackDeleter
	: public BPrivate::AutoDeleter<Stack<T>, StackDelete<T> > {
public:
	StackDeleter()
	{
	}

	StackDeleter(Stack<T>* stack)
		: BPrivate::AutoDeleter<Stack<T>, StackDelete<T> >(stack)
	{
	}
};

#endif	/* KERNEL_UTIL_STACK_H */
