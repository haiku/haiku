#ifndef STACK_H
#define STACK_H
/* Stack - a template stack class
**
** Copyright 2001 pinc Software. All Rights Reserved.
*/


#include <SupportDefs.h>


template<class T> class Stack
{
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
		
		status_t Push(T value)
		{
			if (fUsed >= fMax)
			{
				fMax += 16;
				fArray = (T *)realloc(fArray,fMax * sizeof(T));
				if (fArray == NULL)
					return B_NO_MEMORY;
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
		
	private:
		T		*fArray;
		int32	fUsed;
		int32	fMax;
};

#endif	/* STACK_H */
