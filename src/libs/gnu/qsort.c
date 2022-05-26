/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Isaac Turner, turner.isaac@gmail.com
 *		Jacob Secunda, secundaja@gmail.com
 */


#include <stdlib.h>
#include <string.h>


#define SORT_R_SWAP(a, b, tmp) ((tmp) = (a), (a) = (b), (b) = (tmp))


/* swap itemA and itemB */
/* itemA and itemB must not be equal! */
static inline void
sort_r_swap(char* __restrict itemA, char* __restrict itemB,
	size_t sizeOfElement)
{
	char tmp;
	char* end = itemA + sizeOfElement;
	for (; itemA < end; itemA++, itemB++)
		SORT_R_SWAP(* itemA, * itemB, tmp);
}


/* swap itemA and itemB if itemA > itemB */
/* itemA and itemB must not be equal! */
static inline int
sort_r_cmpswap(char* __restrict itemA, char* __restrict itemB,
	size_t sizeOfElement, _compare_function_qsort_r cmpFunc, void* cookie)
{
	if (cmpFunc(itemA, itemB, cookie) > 0) {
		sort_r_swap(itemA, itemB, sizeOfElement);
		return 1;
	}

	return 0;
}


/*
	Swap consecutive blocks of bytes of size na and nb starting at memory addr
	ptr, with the smallest swap so that the blocks are in the opposite order.
	Blocks may be internally re-ordered e.g.

	  12345ab  ->   ab34512
	  123abc   ->   abc123
	  12abcde  ->   deabc12
*/
static inline void
sort_r_swap_blocks(char* ptr, size_t numBytesA, size_t numBytesB)
{
	if (numBytesA > 0 && numBytesB > 0) {
		if (numBytesA > numBytesB)
			sort_r_swap(ptr, ptr + numBytesA, numBytesB);
		else
			sort_r_swap(ptr, ptr + numBytesB, numBytesA);
	}
}


/* Note: quicksort is not stable, equivalent values may be swapped */
static inline void
sort_r_simple(char* base, size_t numElements, size_t sizeOfElement,
	_compare_function_qsort_r cmpFunc, void* cookie)
{
	char* end = base + (numElements * sizeOfElement);

	if (numElements < 10) {
		// Insertion sort for arbitrarily small inputs

		char* pivIndexA;
		char* pivIndexB;
		for (pivIndexA = base + sizeOfElement; pivIndexA < end;
			pivIndexA += sizeOfElement) {
			pivIndexB = pivIndexA;
			while (pivIndexB > base
				&& sort_r_cmpswap(pivIndexB - sizeOfElement , pivIndexB,
						sizeOfElement, cmpFunc, cookie)) {
				pivIndexB -= sizeOfElement;
			}
		}
	} else {
		// Quicksort when numElements >= 10

		int cmp;
		char* nextPivCmpItem;	// pl
		char* nextPivEqualsPos;	// ple
		char* lastPivCmpItem;	// pr
		char* lastPivEqualsPos;	// pre
		char* pivot;
		char* last = base + sizeOfElement * (numElements - 1);
		char* tmp;

		// Use median of second, middle and second-last items as pivot.
		// First and last may have been swapped with pivot and therefore be
		// extreme.
		char* pivList[3];
		pivList[0] = base + sizeOfElement;
		pivList[1] = base + sizeOfElement * (numElements / 2);
		pivList[2] = last - sizeOfElement;

		if (cmpFunc(pivList[0], pivList[1], cookie) > 0)
			SORT_R_SWAP(pivList[0], pivList[1], tmp);
		if (cmpFunc(pivList[1], pivList[2], cookie) > 0) {
			SORT_R_SWAP(pivList[1], pivList[2], tmp);
			if (cmpFunc(pivList[0], pivList[1], cookie) > 0)
				SORT_R_SWAP(pivList[0], pivList[1], tmp);
		}

		// Swap mid value (pivList[1]), and last element to put pivot as last
		// element.
		if (pivList[1] != last)
			sort_r_swap(pivList[1], last, sizeOfElement);

		//									   v- end (beyond the array)
		//  EEEEEELLLLLLLLuuuuuuuuGGGGGGGEEEEEEEE.
		//  ^- base  ^- ple  ^- pl   ^- pr  ^- pre ^- last (where the pivot is)

		// Pivot comparison key:
		//   E = equal, L = less than, u = unknown, G = greater than, E = equal
		pivot = last;
		nextPivEqualsPos = nextPivCmpItem = base;
		lastPivEqualsPos = lastPivCmpItem = last;

		// Strategy:
		//	Loop into the list from the left and right at the same time to find:
		//		- an item on the left that is greater than the pivot
		//		- an item on the right that is less than the pivot
		//	Once found, they are swapped and the loop continues.
		//	Meanwhile items that are equal to the pivot are moved to the edges
		//	of the array.
		while (nextPivCmpItem < lastPivCmpItem) {
			// Move left hand items which are equal to the pivot to the far
			// left. Break when we find an item that is greater than the pivot.
			for (; nextPivCmpItem < lastPivCmpItem;
				nextPivCmpItem += sizeOfElement) {
				cmp = cmpFunc(nextPivCmpItem, pivot, cookie);
				if (cmp > 0)
					break;
				else if (cmp == 0) {
					if (nextPivEqualsPos < nextPivCmpItem) {
						sort_r_swap(nextPivEqualsPos, nextPivCmpItem,
							sizeOfElement);
					}
					nextPivEqualsPos += sizeOfElement;
				}
			}

			// break if last batch of left hand items were equal to pivot
			if (nextPivCmpItem >= lastPivCmpItem)
				break;

			// Move right hand items which are equal to the pivot to the far
			// right. Break when we find an item that is less than the pivot.
			for (; nextPivCmpItem < lastPivCmpItem;) {
				lastPivCmpItem -= sizeOfElement;
					// Move right pointer onto an unprocessed item
				cmp = cmpFunc(lastPivCmpItem, pivot, cookie);
				if (cmp == 0) {
					lastPivEqualsPos -= sizeOfElement;
					if (lastPivCmpItem < lastPivEqualsPos) {
						sort_r_swap(lastPivCmpItem, lastPivEqualsPos,
							sizeOfElement);
					}
				} else if (cmp < 0) {
					if (nextPivCmpItem < lastPivCmpItem) {
						sort_r_swap(nextPivCmpItem, lastPivCmpItem,
							sizeOfElement);
					}
					nextPivCmpItem += sizeOfElement;
					break;
				}
			}
		}

		nextPivCmpItem = lastPivCmpItem;
			// lastPivCmpItem may have gone below nextPivCmpItem

		// Now we need to go from: EEELLLGGGGEEEE
		//					 to: LLLEEEEEEEGGGG

		// Pivot comparison key:
		// E = equal, L = less than, u = unknown, G = greater than, E = equal
		sort_r_swap_blocks(base, nextPivEqualsPos - base,
			nextPivCmpItem - nextPivEqualsPos);
		sort_r_swap_blocks(lastPivCmpItem, lastPivEqualsPos - lastPivCmpItem,
			end - lastPivEqualsPos);

		sort_r_simple(base, (nextPivCmpItem - nextPivEqualsPos) / sizeOfElement,
			sizeOfElement, cmpFunc, cookie);
		sort_r_simple(end - (lastPivEqualsPos - lastPivCmpItem),
			(lastPivEqualsPos - lastPivCmpItem) / sizeOfElement, sizeOfElement,
			cmpFunc, cookie);
	}
}


inline void
qsort_r(void* base, size_t numElements, size_t sizeOfElement,
	_compare_function_qsort_r cmpFunc, void* cookie)
{
	sort_r_simple((char*)base, numElements, sizeOfElement,
		cmpFunc, cookie);
}
