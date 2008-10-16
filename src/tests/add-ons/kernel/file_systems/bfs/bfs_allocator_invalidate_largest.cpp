/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


int32_t fLargestStart = -1;
int32_t fLargestLength = -1;
bool fLargestValid = false;
int32_t fNumBits = 100;


void
allocate(uint16_t start, int32_t length)
{
	if (start + length > fNumBits)
		return;

	if (fLargestValid) {
		bool cut = false;
		if (fLargestStart == start) {
			// cut from start
			fLargestStart += length;
			fLargestLength -= length;
			cut = true;
		} else if (start > fLargestStart
			&& start < fLargestStart + fLargestLength) {
			// cut from end
			fLargestLength = start - fLargestStart;
			cut = true;
		}
		if (cut && (fLargestLength < fLargestStart
				|| fLargestLength
						< (int32_t)fNumBits - (fLargestStart + fLargestLength))) {
			// might not be the largest block anymore
			fLargestValid = false;
		}
	}
}


void
free(uint16_t start, int32_t length)
{
	if (start + length > fNumBits)
		return;

	// The range to be freed cannot be part of the valid largest range
	if (!(!fLargestValid || start + length <= fLargestStart
		|| start > fLargestStart))
		printf("ASSERT failed!\n");

	if (fLargestValid
		&& (start + length == fLargestStart
			|| fLargestStart + fLargestLength == start
			|| (start < fLargestStart && fLargestStart > fLargestLength)
			|| (start > fLargestStart
				&& (int32_t)fNumBits - (fLargestStart + fLargestLength)
						> fLargestLength))) {
		fLargestValid = false;
	}
}


void
test(int32_t num, int32_t nextLargestStart, int32_t nextLargestLength,
	bool nextLargestValid)
{
	const char* error = NULL;
	if (nextLargestValid != fLargestValid)
		error = "valid differs";
	else if (!nextLargestValid)
		return;

	if (nextLargestStart != fLargestStart)
		error = "start differ";
	else if (nextLargestLength != fLargestLength)
		error = "length differs";

	if (error == NULL)
		return;

	printf("%d: %s: is %d.%d%s, should be %d.%d%s\n", num, error, fLargestStart,
		fLargestLength, fLargestValid ? "" : " (INVALID)", nextLargestStart,
		nextLargestLength, nextLargestValid ? "" : " (INVALID)");
	exit(1);
}


void
test_allocate(int32_t num, int32_t largestStart, int32_t largestLength,
	int32_t start, int32_t length, int32_t nextLargestStart,
	int32_t nextLargestLength, bool nextLargestValid)
{
	fLargestStart = largestStart;
	fLargestLength = largestLength;
	fLargestValid = true;

	printf("Test %d: %d.%d - allocate %d.%d\n", num, fLargestStart,
		fLargestLength, start, length);

	allocate(start, length);
	test(num, nextLargestStart, nextLargestLength, nextLargestValid);
}


void
test_free(int32_t num, int32_t largestStart, int32_t largestLength, int32_t start,
	int32_t length, int32_t nextLargestStart, int32_t nextLargestLength,
	bool nextLargestValid)
{
	fLargestStart = largestStart;
	fLargestLength = largestLength;
	fLargestValid = true;

	printf("Test %d: %d.%d - free %d.%d\n", num, fLargestStart, fLargestLength,
		start, length);

	free(start, length);
	test(num, nextLargestStart, nextLargestLength, nextLargestValid);
}


int
main(int argc, char** argv)
{
	puts("------------- Allocate Tests -------------\n");
	test_allocate(1, 40, 50, 20, 20, 40, 50, true);	// touch start
	test_allocate(2, 40, 50, 20, 19, 40, 50, true);	// don't touch start
	test_allocate(3, 40, 10, 40, 1, 41, 9, false);	// cut start
	test_allocate(4, 40, 50, 40, 1, 41, 49, true);	// cut start
	test_allocate(5, 40, 50, 90, 1, 40, 50, true);	// touch end
	test_allocate(6, 40, 50, 89, 1, 40, 49, true);	// cut end
	test_allocate(7, 40, 20, 59, 1, 40, 19, false);	// cut end
	test_allocate(8, 0, 51, 0, 1, 1, 50, true);		// cut start
	test_allocate(9, 0, 51, 0, 2, 2, 49, true);		// cut start
	test_allocate(10, 0, 51, 0, 3, 3, 48, false);	// cut start

	puts("------------- Free Tests -------------\n");
	test_free(1, 40, 50, 20, 20, 40, 50, false);	// touch start
	test_free(2, 40, 50, 20, 19, 40, 50, true);		// before start
	test_free(3, 50, 40, 20, 19, 40, 50, false);	// before start
	test_free(4, 40, 40, 80, 20, 40, 80, false);	// touch end
	test_free(5, 40, 40, 81, 20, 40, 40, true);		// after end
	test_free(6, 40, 20, 60, 20, 40, 80, false);	// touch end
	test_free(7, 40, 20, 61, 20, 40, 80, false);	// after end
	test_free(8, 40, 20, 41, 20, 40, 80, false);	// after end
	return 0;
}
