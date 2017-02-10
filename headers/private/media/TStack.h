#ifndef _TSTACK_H
#define _TSTACK_H
/* Stack - a template stack class, does not call any constructors/destructors
**
** Copyright 2001 pinc Software. All Rights Reserved.
** This file may be used under the terms of the MIT License.
**
** 2002-03-10 Modified by Marcus Overhagen
*/

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
			if (fArray)
				free(fArray);
		}
		
		bool Push(const T & value)
		{
			if (fUsed >= fMax) {
				fMax += 16;
				T *newArray = (T *)realloc(fArray, fMax * sizeof(T));
				if (newArray == NULL)
					return false;

				fArray = newArray;
			}
			fArray[fUsed++] = value;
			return true;
		}
		
		bool Pop(T *value)
		{
			if (fUsed == 0)
				return false;

			*value = fArray[--fUsed];
			return true;
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

#endif	/* TSTACK_H */
