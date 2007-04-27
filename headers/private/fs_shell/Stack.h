/* Stack - a template stack class (plus some handy methods)
 *
 * Copyright 2001-2005, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _FSSH_STACK_H
#define _FSSH_STACK_H


#include "fssh_defs.h"
#include "fssh_errors.h"


namespace FSShell {


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

		fssh_status_t Push(T value)
		{
			if (fUsed >= fMax) {
				fMax += 16;
				T *newArray = (T *)realloc(fArray, fMax * sizeof(T));
				if (newArray == NULL)
					return FSSH_B_NO_MEMORY;

				fArray = newArray;
			}
			fArray[fUsed++] = value;
			return FSSH_B_OK;
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

		int32_t CountItems() const
		{
			return fUsed;
		}

	private:
		T		*fArray;
		int32_t	fUsed;
		int32_t	fMax;
};

}	// namespace FSShell

using FSShell::Stack;


#endif	/* _FSSH_STACK_H */
