/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#define __USE_GNU
	// for wmempcpy() and wcschrnul()

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>


static int sign (int a)
{
	if (a < 0)
		return -1;
	if (a > 0)
		return 1;
	return 0;
}


// #pragma mark - wcslen -------------------------------------------------------


void
test_wcslen()
{
	printf("wcslen()/wcsnlen()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		size_t result = wcslen(string);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcslen(\"%ls\") = %lu (expected %lu),"
					" errno = %x (expected %x)\n", string, result, expected,
				errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"test";
		size_t result = wcslen(string);
		size_t expected = 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcslen(\"%ls\") = %lu (expected %lu),"
					" errno = %x (expected %x)\n", string, result, expected,
				errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"t\xE4st";
		size_t result = wcslen(string);
		size_t expected = 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcslen(\"%ls\") = %lu (expected %lu),"
					" errno = %x (expected %x)\n", string, result, expected,
				errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"te\x00st";
		size_t result = wcslen(string);
		size_t expected = 2;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcslen(\"%ls\") = %lu (expected %lu),"
					" errno = %x (expected %x)\n", string, result, expected,
				errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"test";
		size_t result = wcsnlen(string, 0);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsnlen(\"%ls\", 0) = %lu "
					"(expected %lu), errno = %x (expected %x)\n",
				string, result, expected, errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"test";
		size_t result = wcsnlen(string, 4);
		size_t expected = 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsnlen(\"%ls\", 4) = %lu "
					"(expected %lu), errno = %x (expected %x)\n",
				string, result, expected, errno, 0);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"test";
		size_t result = wcsnlen(string, 6);
		size_t expected = 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsnlen(\"%ls\", 6) = %lu "
					"(expected %lu), errno = %x (expected %x)\n",
				string, result, expected, errno, 0);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcscmp -------------------------------------------------------


void
test_wcscmp()
{
	printf("wcscmp()/wcsncmp()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* a = L"";
		const wchar_t* b = L"";
		int result = sign(wcscmp(a, b));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"b";
		int result = sign(wcscmp(a, b));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"b";
		const wchar_t* b = L"a";
		int result = sign(wcscmp(a, b));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"A";
		int result = sign(wcscmp(a, b));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst";
		const wchar_t* b = L"täst";
		int result = sign(wcscmp(a, b));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst";
		const wchar_t* b = L"täst ";
		int result = sign(wcscmp(a, b));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täSt";
		const wchar_t* b = L"täs";
		int result = sign(wcscmp(a, b));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst1";
		const wchar_t* b = L"täst0";
		int result = sign(wcsncmp(a, b, 0));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncmp(\"%ls\", \"%ls\", 0) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst1";
		const wchar_t* b = L"täst0";
		int result = sign(wcsncmp(a, b, 4));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncmp(\"%ls\", \"%ls\", 4) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst1";
		const wchar_t* b = L"täst0";
		int result = sign(wcsncmp(a, b, 5));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncmp(\"%ls\", \"%ls\", 5) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täs";
		const wchar_t* b = L"täst123";
		int result = sign(wcsncmp(a, b, (size_t)-1));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncmp(\"%ls\", \"%ls\", -1) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcscasecmp ---------------------------------------------------


void
test_wcscasecmp()
{
	printf("wcscasecmp()/wcsncasecmp()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* a = L"";
		const wchar_t* b = L"";
		int result = sign(wcscasecmp(a, b));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"b";
		int result = sign(wcscasecmp(a, b));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"B";
		const wchar_t* b = L"a";
		int result = sign(wcscasecmp(a, b));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"A";
		int result = sign(wcscasecmp(a, b));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"TÄST";
		const wchar_t* b = L"täst";
		int result = sign(wcscasecmp(a, b));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"tÄst";
		const wchar_t* b = L"täst ";
		int result = sign(wcscasecmp(a, b));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"TäSt";
		const wchar_t* b = L"täs";
		int result = sign(wcscasecmp(a, b));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasecmp(\"%ls\", \"%ls\") = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"tÄst1";
		const wchar_t* b = L"täst0";
		int result = sign(wcsncasecmp(a, b, 0));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscasencmp(\"%ls\", \"%ls\", 0) = %d"
					" (expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst1";
		const wchar_t* b = L"täSt0";
		int result = sign(wcsncasecmp(a, b, 4));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncasecmp(\"%ls\", \"%ls\", 4) = %d"
					" (expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täsT1";
		const wchar_t* b = L"täst0";
		int result = sign(wcsncasecmp(a, b, 5));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncasecmp(\"%ls\", \"%ls\", 5) = %d"
					" (expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täs";
		const wchar_t* b = L"täSt123";
		int result = sign(wcsncasecmp(a, b, (size_t)-1));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsncasecmp(\"%ls\", \"%ls\", -1) = "
					"%d (expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcschr -------------------------------------------------------


void
test_wcschr()
{
	printf("wcschr()/wcschrnul()/wcsrchr()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L's';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'S';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcschr(string, ch);
		const wchar_t* expected = string + 14;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string + wcslen(string);
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L' ';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L's';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'S';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string + wcslen(string);
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcschrnul(string, ch);
		const wchar_t* expected = string + 14;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcschrnul(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L' ';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t ch = L' ';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L' ';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = string + 9;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L's';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'S';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t ch = L'\0';
		const wchar_t* result = wcsrchr(string, ch);
		const wchar_t* expected = string + 14;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsrchr(\"%ls\", '%lc') = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcsdup -------------------------------------------------------


void
test_wcsdup()
{
	printf("wcsdup()\n");

	int problemCount = 0;
	errno = 0;

#ifdef __HAIKU__
	{
		const wchar_t* string = NULL;
		wchar_t* result = wcsdup(string);
		if (result != NULL || errno != 0) {
			printf("\tPROBLEM: result for wcsdup(%p) = \"%ls\", errno = %x"
					" (expected 0)\n", string, result, errno);
			problemCount++;
		}
	}
#endif

	{
		const wchar_t* string = L"";
		wchar_t* result = wcsdup(string);
		if (result == NULL || wcscmp(result, string) != 0 || errno != 0) {
			printf("\tPROBLEM: result for wcsdup(\"%ls\") = \"%ls\", errno = %x"
					" (expected 0)\n", string, result, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"tÄstdata with some charäcters";
		wchar_t* result = wcsdup(string);
		if (result == NULL || wcscmp(result, string) != 0 || errno != 0) {
			printf("\tPROBLEM: result for wcsdup(\"%ls\") = \"%ls\", errno = %x"
					" (expected 0)\n", string, result, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcscpy -------------------------------------------------------


void
test_wcscpy()
{
	printf("wcscpy()/wcsncpy()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcscpy(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 0) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 0\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'\0') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'\0');
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"test";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcscpy(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcscpy(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"te\x00st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcscpy(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 2) {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 2\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L'\0') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L'\0');
			problemCount++;
		}
		if (destination[3] != L'X') {
			printf("\tPROBLEM: wcscpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcsncpy(destination, source, 0);
		if (result != destination) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 0) -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 0) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcsncpy(destination, source, 2);
		if (result != destination) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 2) -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 2) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 2) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 2) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L'X') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 2) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcsncpy(destination, source, 4);
		if (result != destination) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'X') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 4) -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcsncpy(destination, source, 8);
		if (result != destination) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'\0') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'\0');
			problemCount++;
		}
		if (destination[6] != L'\0') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[6]=%x, expected %x\n", source, destination[6],
				L'\0');
			problemCount++;
		}
		if (destination[7] != L'\0') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[7]=%x, expected %x\n", source, destination[7],
				L'\0');
			problemCount++;
		}
		if (destination[8] != L'X') {
			printf("\tPROBLEM: wcsncpy(destination, \"%ls\", 8) -> "
					"destination[8]=%x, expected %x\n", source, destination[8],
				L'X');
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcpcpy -------------------------------------------------------


void
test_wcpcpy()
{
	printf("wcpcpy()/wcpncpy()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpcpy(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 0) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 0\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'\0') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'\0');
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"test";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpcpy(destination, source);
		if (result != destination + 4) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination + 4);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpcpy(destination, source);
		if (result != destination + 4) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination + 4);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"te\x00st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpcpy(destination, source);
		if (result != destination + 2) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> result=%p, "
					"expected %p\n", source, result, destination + 2);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 2) {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"wcslen(destination)=%lu, expected 2\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L'\0') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L'\0');
			problemCount++;
		}
		if (destination[3] != L'X') {
			printf("\tPROBLEM: wcpcpy(destination, \"%ls\") -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpncpy(destination, source, 0);
		if (result != destination) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 0) -> result=%p, "
					"expected %p\n", source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 0) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpncpy(destination, source, 2);
		if (result != destination + 2) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 2) -> result=%p, "
					"expected %p\n", source, result, destination + 2);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 2) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 2) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 2) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L'X') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 2) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpncpy(destination, source, 4);
		if (result != destination + 4) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> result=%p, "
					"expected %p\n", source, result, destination + 4);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'X') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 4) -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"t\xE4st";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		wchar_t* result = wcpncpy(destination, source, 8);
		if (result != destination + 4) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> result=%p, "
					"expected %p\n", source, result, destination + 4);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'\xE4') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'\xE4');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'\0') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'\0');
			problemCount++;
		}
		if (destination[6] != L'\0') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[6]=%x, expected %x\n", source, destination[6],
				L'\0');
			problemCount++;
		}
		if (destination[7] != L'\0') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[7]=%x, expected %x\n", source, destination[7],
				L'\0');
			problemCount++;
		}
		if (destination[8] != L'X') {
			printf("\tPROBLEM: wcpncpy(destination, \"%ls\", 8) -> "
					"destination[8]=%x, expected %x\n", source, destination[8],
				L'X');
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcscat -------------------------------------------------------


void
test_wcscat()
{
	printf("wcscat()/wcsncat()\n");

	int problemCount = 0;
	errno = 0;
	wchar_t destination[] = L"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
	destination[0] = L'\0';
	wchar_t backup[33];

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"";
		wchar_t* result = wcscat(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 0) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> "
					"wcslen(destination)=%lu, expected 0\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'\0') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[0]=%x, "
					"expected %x\n", backup, source, destination[0], L'\0');
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[1]=%x, "
					"expected %x\n", backup, source, destination[1], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"test";
		wchar_t* result = wcscat(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> "
					"wcslen(destination)=%lu, expected 4\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[0]=%x, "
					"expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[1]=%x, "
					"expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[2]=%x, "
					"expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[3]=%x, "
					"expected %x\n", backup, source, destination[3], L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[4]=%x, "
					"expected %x\n", backup, source, destination[4], L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[5]=%x, "
					"expected %x\n", backup, source, destination[5], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"t\xE4st";
		wchar_t* result = wcscat(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 8) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> "
					"wcslen(destination)=%lu, expected 8\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[0]=%x, "
					"expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[1]=%x, "
					"expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[2]=%x, "
					"expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[3]=%x, "
					"expected %x\n", backup, source, destination[3], L't');
			problemCount++;
		}
		if (destination[4] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[4]=%x, "
					"expected %x\n", backup, source, destination[4], L't');
			problemCount++;
		}
		if (destination[5] != L'\xE4') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[5]=%x, "
					"expected %x\n", backup, source, destination[5], L'\xE4');
			problemCount++;
		}
		if (destination[6] != L's') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[6]=%x, "
					"expected %x\n", backup, source, destination[6], L's');
			problemCount++;
		}
		if (destination[7] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[7]=%x, "
					"expected %x\n", backup, source, destination[7], L't');
			problemCount++;
		}
		if (destination[8] != L'\0') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[8]=%x, "
					"expected %x\n", backup, source, destination[8], L'\0');
			problemCount++;
		}
		if (destination[9] != L'X') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[9]=%x, "
					"expected %x\n", backup, source, destination[9], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"te\x00st";
		wchar_t* result = wcscat(destination, source);
		if (result != destination) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 10) {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> "
					"wcslen(destination)=%lu, expected 10\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[0]=%x, "
					"expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[1]=%x, "
					"expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[2]=%x, "
					"expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[3]=%x, "
					"expected %x\n", backup, source, destination[3], L't');
			problemCount++;
		}
		if (destination[4] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[4]=%x, "
					"expected %x\n", backup, source, destination[4], L't');
			problemCount++;
		}
		if (destination[5] != L'\xE4') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[5]=%x, "
					"expected %x\n", backup, source, destination[5], L'\xE4');
			problemCount++;
		}
		if (destination[6] != L's') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[6]=%x, "
					"expected %x\n", backup, source, destination[6], L's');
			problemCount++;
		}
		if (destination[7] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[7]=%x, "
					"expected %x\n", backup, source, destination[7], L't');
			problemCount++;
		}
		if (destination[8] != L't') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[8]=%x, "
					"expected %x\n", backup, source, destination[8], L't');
			problemCount++;
		}
		if (destination[9] != L'e') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[9]=%x, "
					"expected %x\n", backup, source, destination[9], L'e');
			problemCount++;
		}
		if (destination[10] != L'\0') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[10]=%x, "
					"expected %x\n", backup, source, destination[10], L'\0');
			problemCount++;
		}
		if (destination[11] != L'X') {
			printf("\tPROBLEM: wcscat(\"%ls\", \"%ls\") -> destination[11]=%x, "
					"expected %x\n", backup, source, destination[11], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(destination, L"some");
		wcscpy(backup, destination);
		const wchar_t* source = L" other text";
		wchar_t* result = wcsncat(destination, source, 0);
		if (result != destination) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 0) -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 0) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 0) -> "
					"wcslen(destination)=%lu, expected 4\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (wcscmp(destination, L"some") != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 0) -> \"%ls\"\n",
				backup, source, destination);
			problemCount++;
		}
	}

	{
		wcscpy(destination, L"some");
		wcscpy(backup, destination);
		const wchar_t* source = L" other text";
		wchar_t* result = wcsncat(destination, source, 6);
		if (result != destination) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 6) -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 6) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 10) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 6) -> "
					"wcslen(destination)=%lu, expected 10\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (wcscmp(destination, L"some other") != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 6) -> \"%ls\"\n",
				backup, source, destination);
			problemCount++;
		}
	}

	{
		wcscpy(destination, L"some");
		wcscpy(backup, destination);
		const wchar_t* source = L" other text";
		wchar_t* result = wcsncat(destination, source, 20);
		if (result != destination) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 20) -> result=%p, "
					"expected %p\n", backup, source, result, destination);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 20) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 15) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 20) -> "
					"wcslen(destination)=%lu, expected 15\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (wcscmp(destination, L"some other text") != 0) {
			printf("\tPROBLEM: wcsncat(\"%ls\", \"%ls\", 20) -> \"%ls\"\n",
				backup, source, destination);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcslcat ------------------------------------------------------


#ifdef __HAIKU__

void
test_wcslcat()
{
	printf("wcslcat()\n");

	int problemCount = 0;
	errno = 0;
	wchar_t destination[] = L"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
	wchar_t backup[33];

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"";
		size_t result = wcslcat(destination, source, 0);
		size_t expectedResult = 0;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 0) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 0) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 32) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 0) -> "
					"wcslen(destination)=%lu, expected 32\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 0) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L'X');
			problemCount++;
		}
	}

	{
		destination[0] = L'\0';
		wcscpy(backup, destination);
		const wchar_t* source = L"";
		size_t result = wcslcat(destination, source, 32);
		size_t expectedResult = 0;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> "
					"wcslen(destination)=%lu, expected 0\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'\0') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L'\0');
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[1]="
					"%x, expected %x\n", backup, source, destination[1], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"test";
		size_t result = wcslcat(destination, source, 3);
		size_t expectedResult = 4;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 2) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> "
					"wcslen(destination)=%lu, expected 2\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> destination[1]="
					"%x, expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L'\0') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> destination[2]="
					"%x, expected %x\n", backup, source, destination[2], L'\0');
			problemCount++;
		}
		if (destination[3] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 3) -> destination[3]="
					"%x, expected %x\n", backup, source, destination[3], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"st";
		size_t result = wcslcat(destination, source, 4);
		size_t expectedResult = 4;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 3) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> "
					"wcslen(destination)=%lu, expected 3\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> destination[1]="
					"%x, expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> destination[2]="
					"%x, expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L'\0') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> destination[3]="
					"%x, expected %x\n", backup, source, destination[3], L'\0');
			problemCount++;
		}
		if (destination[4] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 4) -> destination[4]="
					"%x, expected %x\n", backup, source, destination[4], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"t";
		size_t result = wcslcat(destination, source, 5);
		size_t expectedResult = 4;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> "
					"wcslen(destination)=%lu, expected 4\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[1]="
					"%x, expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[2]="
					"%x, expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[3]="
					"%x, expected %x\n", backup, source, destination[3], L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[4]="
					"%x, expected %x\n", backup, source, destination[4], L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 5) -> destination[5]="
					"%x, expected %x\n", backup, source, destination[5], L'X');
			problemCount++;
		}
	}

	{
		wcscpy(backup, destination);
		const wchar_t* source = L"t\xE4st";
		size_t result = wcslcat(destination, source, 32);
		size_t expectedResult = 8;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> result=%ld, "
					"expected %ld\n", backup, source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> errno=%d, "
					"expected 0\n", backup, source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 8) {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> "
					"wcslen(destination)=%lu, expected 8\n", backup, source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[0]="
					"%x, expected %x\n", backup, source, destination[0], L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[1]="
					"%x, expected %x\n", backup, source, destination[1], L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[2]="
					"%x, expected %x\n", backup, source, destination[2], L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[3]="
					"%x, expected %x\n", backup, source, destination[3], L't');
			problemCount++;
		}
		if (destination[4] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[4]="
					"%x, expected %x\n", backup, source, destination[4], L't');
			problemCount++;
		}
		if (destination[5] != L'\xE4') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[5]="
					"%x, expected %x\n", backup, source, destination[5],
				L'\xE4');
			problemCount++;
		}
		if (destination[6] != L's') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[6]="
					"%x, expected %x\n", backup, source, destination[6], L's');
			problemCount++;
		}
		if (destination[7] != L't') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[7]="
					"%x, expected %x\n", backup, source, destination[7], L't');
			problemCount++;
		}
		if (destination[8] != L'\0') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[8]="
					"%x, expected %x\n", backup, source, destination[8], L'\0');
			problemCount++;
		}
		if (destination[9] != L'X') {
			printf("\tPROBLEM: wcslcat(\"%ls\", \"%ls\", 32) -> destination[9]="
					"%x, expected %x\n", backup, source, destination[9], L'X');
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}

#endif


// #pragma mark - wcslcpy ------------------------------------------------------


#ifdef __HAIKU__

void
test_wcslcpy()
{
	printf("wcslcpy()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";

		size_t result = wcslcpy(destination, source, 0);
		size_t expectedResult = 0;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 0) -> result=%ld, "
					"expected %ld\n", source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 0) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 16) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 0) -> "
					"wcslen(destination)=%lu, expected 16\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		size_t result = wcslcpy(destination, source, 16);
		size_t expectedResult = 0;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> result=%ld,"
					" expected %ld\n", source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 0) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"wcslen(destination)=%lu, expected 0\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L'\0') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'\0');
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"test";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		size_t result = wcslcpy(destination, source, 3);
		size_t expectedResult = 4;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> result=%ld, "
					"expected %ld\n", source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 2) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 2) -> "
					"wcslen(destination)=%lu, expected 3\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L'\0') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L'\0');
			problemCount++;
		}
		if (destination[3] != L'X') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 3) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"test";
		wchar_t destination[] = L"XXXXXXXXXXXXXXXX";
		size_t result = wcslcpy(destination, source, 16);
		size_t expectedResult = 4;
		if (result != expectedResult) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> result=%ld, "
					"expected %ld\n", source, result, expectedResult);
			problemCount++;
		}
		if (errno != 0) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> errno=%d, "
					"expected 0\n", source, errno);
			problemCount++;
		}
		if (wcslen(destination) != 4) {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"wcslen(destination)=%lu, expected 4\n", source,
				wcslen(destination));
			problemCount++;
		}
		if (destination[0] != L't') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L't');
			problemCount++;
		}
		if (destination[1] != L'e') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'e');
			problemCount++;
		}
		if (destination[2] != L's') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[2]=%x, expected %x\n", source, destination[2],
				L's');
			problemCount++;
		}
		if (destination[3] != L't') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[3]=%x, expected %x\n", source, destination[3],
				L't');
			problemCount++;
		}
		if (destination[4] != L'\0') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[4]=%x, expected %x\n", source, destination[4],
				L'\0');
			problemCount++;
		}
		if (destination[5] != L'X') {
			printf("\tPROBLEM: wcslcpy(destination, \"%ls\", 16) -> "
					"destination[5]=%x, expected %x\n", source, destination[5],
				L'X');
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}

#endif


// #pragma mark - collation ----------------------------------------------------


struct coll_data {
	const wchar_t* a;
	const wchar_t* b;
	int result;
	int err;
};


void
test_coll(bool useWcsxfrm, const char* locale, const coll_data coll[])
{
	setlocale(LC_COLLATE, locale);
	printf("%s in %s locale\n", useWcsxfrm ? "wcsxfrm" : "wcscoll", locale);

	int problemCount = 0;
	for (unsigned int i = 0; coll[i].a != NULL; ++i) {
		errno = 0;
		int result;
		char funcCall[256];
		if (useWcsxfrm) {
			wchar_t sortKeyA[100], sortKeyB[100];
			wcsxfrm(sortKeyA, coll[i].a, 100);
			wcsxfrm(sortKeyB, coll[i].b, 100);
			result = sign(wcscmp(sortKeyA, sortKeyB));
			sprintf(funcCall, "wcscmp(wcsxfrm(\"%ls\"), wcsxfrm(\"%ls\"))",
				coll[i].a, coll[i].b);
		} else {
			result = sign(wcscoll(coll[i].a, coll[i].b));
			sprintf(funcCall, "wcscoll(\"%ls\", \"%ls\")", coll[i].a,
				coll[i].b);
		}

		if (result != coll[i].result || errno != coll[i].err) {
			printf(
				"\tPROBLEM: %s = %d (expected %d), errno = %x (expected %x)\n",
				funcCall, result, coll[i].result, errno, coll[i].err);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_collation()
{
	const coll_data coll_posix[] = {
		{ L"", L"", 0, 0 },
		{ L"test", L"test", 0, 0 },
		{ L"tester", L"test", 1, 0 },
		{ L"tEst", L"teSt", -1, 0 },
		{ L"test", L"tester", -1, 0 },
		{ L"tast", L"t\xE4st", -1, EINVAL },
		{ L"t\xE6st", L"test", 1, EINVAL },
		{ NULL, NULL, 0, 0 }
	};
	test_coll(0, "POSIX", coll_posix);
	test_coll(1, "POSIX", coll_posix);

	const coll_data coll_en[] = {
		{ L"", L"", 0, 0 },
		{ L"test", L"test", 0, 0 },
		{ L"tester", L"test", 1, 0 },
		{ L"tEst", L"test", 1, 0 },
		{ L"test", L"tester", -1, 0 },
		{ L"t\xE4st", L"t\xE4st", 0, 0 },
		{ L"tast", L"t\xE4st", -1, 0 },
		{ L"tbst", L"t\xE4st", 1, 0 },
		{ L"tbst", L"t\xE6st", 1, 0 },
		{ L"t\xE4st", L"t\xC4st", -1, 0 },
		{ L"tBst", L"t\xC4st", 1, 0 },
		{ L"tBst", L"t\xE4st", 1, 0 },
		{ L"taest", L"t\xE6st", -1, 0 },
		{ L"tafst", L"t\xE6st", 1, 0 },
		{ L"taa", L"t\xE4"L"a", -1, 0 },
		{ L"tab", L"t\xE4"L"b", -1, 0 },
		{ L"tad", L"t\xE4"L"d", -1, 0 },
		{ L"tae", L"t\xE4"L"e", -1, 0 },
		{ L"taf", L"t\xE4"L"f", -1, 0 },
		{ L"cote", L"cot\xE9", -1, 0 },
		{ L"cot\xE9", L"c\xF4te", -1, 0 },
		{ L"c\xF4te", L"c\xF4t\xE9", -1, 0 },
		{ NULL, NULL, 0, 0 }
	};
	test_coll(0, "en_US.UTF-8", coll_en);
	test_coll(1, "en_US.UTF-8", coll_en);

	const coll_data coll_de[] = {
		{ L"", L"", 0, 0 },
		{ L"test", L"test", 0, 0 },
		{ L"tester", L"test", 1, 0 },
		{ L"tEst", L"test", 1, 0 },
		{ L"test", L"tester", -1, 0 },
		{ L"t\xE4st", L"t\xE4st", 0, 0 },
		{ L"tast", L"t\xE4st", -1, 0 },
		{ L"tbst", L"t\xE4st", 1, 0 },
		{ L"tbst", L"t\xE6st", 1, 0 },
		{ L"t\xE4st", L"t\xC4st", -1, 0 },
		{ L"tBst", L"t\xC4st", 1, 0 },
		{ L"tBst", L"t\xE4st", 1, 0 },
		{ L"taest", L"t\xE6st", -1, 0 },
		{ L"tafst", L"t\xE6st", 1, 0 },
		{ L"taa", L"t\xE4", 1, 0 },
		{ L"tab", L"t\xE4", 1, 0 },
		{ L"tad", L"t\xE4", 1, 0 },
		{ L"tae", L"t\xE4", 1, 0 },
		{ L"taf", L"t\xE4", 1, 0 },
		{ L"cote", L"cot\xE9", -1, 0 },
		{ L"cot\xE9", L"c\xF4te", -1, 0 },
		{ L"c\xF4te", L"c\xF4t\xE9", -1, 0 },
		{ NULL, NULL, 0, 0 }
	};
	test_coll(0, "de_DE.UTF-8", coll_de);
	test_coll(1, "de_DE.UTF-8", coll_de);

	const coll_data coll_de_phonebook[] = {
		{ L"", L"", 0, 0 },
		{ L"test", L"test", 0, 0 },
		{ L"tester", L"test", 1, 0 },
		{ L"tEst", L"test", 1, 0 },
		{ L"test", L"tester", -1, 0 },
		{ L"t\xE4st", L"t\xE4st", 0, 0 },
		{ L"tast", L"t\xE4st", 1, 0 },
		{ L"tbst", L"t\xE4st", 1, 0 },
		{ L"tbst", L"t\xE6st", 1, 0 },
		{ L"t\xE4st", L"t\xC4st", -1, 0 },
		{ L"tBst", L"t\xC4st", 1, 0 },
		{ L"tBst", L"t\xE4st", 1, 0 },
		{ L"taest", L"t\xE6st", -1, 0 },
		{ L"tafst", L"t\xE6st", 1, 0 },
		{ L"taa", L"t\xE4", -1, 0 },
		{ L"tab", L"t\xE4", -1, 0 },
		{ L"tad", L"t\xE4", -1, 0 },
		{ L"tae", L"t\xE4", -1, 0 },
		{ L"taf", L"t\xE4", 1, 0 },
		{ L"cote", L"cot\xE9", -1, 0 },
		{ L"cot\xE9", L"c\xF4te", -1, 0 },
		{ L"c\xF4te", L"c\xF4t\xE9", -1, 0 },
		{ NULL, NULL, 0, 0 }
	};
	test_coll(0, "de_DE.UTF-8@collation=phonebook", coll_de_phonebook);
	test_coll(1, "de_DE.UTF-8@collation=phonebook", coll_de_phonebook);

	const coll_data coll_fr[] = {
		{ L"", L"", 0, 0 },
		{ L"test", L"test", 0, 0 },
		{ L"tester", L"test", 1, 0 },
		{ L"tEst", L"test", 1, 0 },
		{ L"test", L"tester", -1, 0 },
		{ L"t\xE4st", L"t\xE4st", 0, 0 },
		{ L"tast", L"t\xE4st", -1, 0 },
		{ L"tbst", L"t\xE4st", 1, 0 },
		{ L"tbst", L"t\xE6st", 1, 0 },
		{ L"t\xE4st", L"t\xC4st", -1, 0 },
		{ L"tBst", L"t\xC4st", 1, 0 },
		{ L"tBst", L"t\xE4st", 1, 0 },
		{ L"taest", L"t\xE6st", -1, 0 },
		{ L"tafst", L"t\xE6st", 1, 0 },
		{ L"taa", L"t\xE4", 1, 0 },
		{ L"tab", L"t\xE4", 1, 0 },
		{ L"tad", L"t\xE4", 1, 0 },
		{ L"tae", L"t\xE4", 1, 0 },
		{ L"taf", L"t\xE4", 1, 0 },
		{ L"cote", L"cot\xE9", -1, 0 },
		{ L"cot\xE9", L"c\xF4te", 1, 0 },
		{ L"c\xF4te", L"c\xF4t\xE9", -1, 0 },
		{ NULL, NULL, 0, 0 }
	};
	// CLDR-1.9 has adjusted the defaults of fr_FR to no longer do reverse
	// ordering of secondary differences (accents), but fr_CA still does that
	// by default
	test_coll(0, "fr_CA.UTF-8", coll_fr);
	test_coll(1, "fr_CA.UTF-8", coll_fr);
}


// #pragma mark - wcsftime -----------------------------------------------------


struct wcsftime_data {
	const wchar_t* format;
	const wchar_t* result;
};


void
test_wcsftime(const char* locale, const wcsftime_data data[])
{
	setlocale(LC_TIME, locale);
	setlocale(LC_CTYPE, locale);
	printf("wcsftime for '%s'\n", locale);

	time_t nowSecs = 1279391169;	// pure magic
	tm* now = localtime(&nowSecs);
	int problemCount = 0;
	for (int i = 0; data[i].format != NULL; ++i) {
		wchar_t buf[100];
		wcsftime(buf, 100, data[i].format, now);
		if (wcscmp(buf, data[i].result) != 0) {
			printf(
				"\tPROBLEM: wcsftime(\"%ls\") = \"%ls\" (expected \"%ls\")\n",
				data[i].format, buf, data[i].result);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_wcsftime()
{
	setenv("TZ", "GMT", 1);

	const wcsftime_data wcsftime_posix[] = {
		{ L"%c", L"Sat Jul 17 18:26:09 2010" },
		{ L"%x", L"07/17/10" },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"Sat" },
		{ L"%A", L"Saturday" },
		{ L"%b", L"Jul" },
		{ L"%B", L"July" },
		{ NULL, NULL }
	};
	test_wcsftime("POSIX", wcsftime_posix);

	const wcsftime_data wcsftime_de[] = {
		{ L"%c", L"Samstag, 17. Juli 2010 18:26:09 GMT" },
		{ L"%x", L"17.07.2010" },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"Sa." },
		{ L"%A", L"Samstag" },
		{ L"%b", L"Jul" },
		{ L"%B", L"Juli" },
		{ NULL, NULL }
	};
	test_wcsftime("de_DE.UTF-8", wcsftime_de);

	const wcsftime_data wcsftime_hr[] = {
		{ L"%c", L"subota, 17. srpnja 2010. 18:26:09 GMT" },
		{ L"%x", L"17. 07. 2010." },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"sub" },
		{ L"%A", L"subota" },
		{ L"%b", L"srp" },
		{ L"%B", L"srpnja" },
		{ NULL, NULL }
	};
	test_wcsftime("hr_HR.ISO8859-2", wcsftime_hr);

	const wcsftime_data wcsftime_gu[] = {
		{ L"%c", L"શનિવાર, 17 જુલાઈ, 2010 06:26:09 PM GMT" },
		{ L"%x", L"17 જુલાઈ, 2010" },
		{ L"%X", L"06:26:09 PM" },
		{ L"%a", L"શનિ" },
		{ L"%A", L"શનિવાર" },
		{ L"%b", L"જુલાઈ" },
		{ L"%B", L"જુલાઈ" },
		{ NULL, NULL }
	};
	test_wcsftime("gu_IN", wcsftime_gu);

	const wcsftime_data wcsftime_it[] = {
		{ L"%c", L"sabato 17 luglio 2010 18:26:09 GMT" },
		{ L"%x", L"17/lug/2010" },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"sab" },
		{ L"%A", L"sabato" },
		{ L"%b", L"lug" },
		{ L"%B", L"luglio" },
		{ NULL, NULL }
	};
	test_wcsftime("it_IT", wcsftime_it);

	const wcsftime_data wcsftime_nl[] = {
		{ L"%c", L"zaterdag 17 juli 2010 18:26:09 GMT" },
		{ L"%x", L"17 jul. 2010" },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"za" },
		{ L"%A", L"zaterdag" },
		{ L"%b", L"jul." },
		{ L"%B", L"juli" },
		{ NULL, NULL }
	};
	test_wcsftime("nl_NL", wcsftime_nl);

	const wcsftime_data wcsftime_nb[] = {
		{ L"%c", L"kl. 18:26:09 GMT l\xF8rdag 17. juli 2010" },
		{ L"%x", L"17. juli 2010" },
		{ L"%X", L"18:26:09" },
		{ L"%a", L"lør." },
		{ L"%A", L"lørdag" },
		{ L"%b", L"juli" },
		{ L"%B", L"juli" },
		{ NULL, NULL }
	};
	test_wcsftime("nb_NO", wcsftime_nb);
}


// #pragma mark - wcspbrk ------------------------------------------------------


void
test_wcspbrk()
{
	printf("wcspbrk()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t* accept = L" ";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t* accept = L" ";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L" ";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L"UY\xE4 ";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L" emorstx";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L"EMORSTX\xA0";
		const wchar_t* result = wcspbrk(string, accept);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcspbrk(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcscspn -------------------------------------------------------


void
test_wcscspn()
{
	printf("wcscspn()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t* reject = L" ";
		size_t result = wcscspn(string, reject);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t* reject = L" ";
		size_t result = wcscspn(string, reject);
		size_t expected = 8;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* reject = L" mos";
		size_t result = wcscspn(string, reject);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* reject = L"t";
		size_t result = wcscspn(string, reject);
		size_t expected = 10;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* reject = L"abcdfghijklnpquvwyz\t";
		size_t result = wcscspn(string, reject);
		size_t expected = wcslen(string);
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* reject = L"";
		size_t result = wcscspn(string, reject);
		size_t expected = wcslen(string);
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcscspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, reject,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcsspn -------------------------------------------------------


void
test_wcsspn()
{
	printf("wcsspn()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t* accept = L" ";
		size_t result = wcsspn(string, accept);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t* accept = L" ";
		size_t result = wcsspn(string, accept);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L" emo";
		size_t result = wcsspn(string, accept);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L" emorstx";
		size_t result = wcsspn(string, accept);
		size_t expected = wcslen(string);
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* accept = L"";
		size_t result = wcsspn(string, accept);
		size_t expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsspn(\"%ls\", \"%ls\") = %ld "
					"(expected %ld), errno = %x (expected 0)\n", string, accept,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcsstr ------------------------------------------------------


void
test_wcsstr()
{
	printf("wcsstr()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t* sought = L" ";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t* sought = L"som ";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t* sought = L"soMe";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* sought = L"some ";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* sought = L" more";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* sought = L"some more text";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some more text";
		const wchar_t* sought = L"some more text ";
		const wchar_t* result = wcsstr(string, sought);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wcsstr(\"%ls\", \"%ls\") = %p "
					"(expected %p), errno = %x (expected 0)\n", string, sought,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wcstok ------------------------------------------------------


void
test_wcstok()
{
	printf("wcstok()\n");

	int problemCount = 0;

	{
		wchar_t string[] = L"";
		const wchar_t* delim = L" \t\n";
		wchar_t* state;
		wchar_t* result = wcstok(string, delim, &state);
		wchar_t* expected = NULL;
		wchar_t* expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = NULL;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
	}

	{
		wchar_t string[] = L"\t\t\t\n   \t";
		const wchar_t* delim = L" \t\n";
		wchar_t* state;
		wchar_t* result = wcstok(string, delim, &state);
		wchar_t* expected = NULL;
		wchar_t* expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = NULL;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
	}

	{
		wchar_t string[] = L"just some text here!";
		const wchar_t* delim = L" ";
		wchar_t* state;
		wchar_t* result = wcstok(string, delim, &state);
		wchar_t* expected = string;
		wchar_t* expectedState = string + 5;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 5;
		expectedState = string + 10;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 10;
		expectedState = string + 15;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 15;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = NULL;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
	}

	{
		wchar_t string[] = L" just \t\nsome\t\t\ttext\n\n\nhere!";
		const wchar_t* delim = L"\n\t ";
		wchar_t* state;
		wchar_t* result = wcstok(string, delim, &state);
		wchar_t* expected = string + 1;
		wchar_t* expectedState = string + 6;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
		if (wcscmp(result, L"just") != 0) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %ls "
					"(expected %ls)\n", string, delim, &state, result, L"just");
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 8;
		expectedState = string + 13;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
		if (wcscmp(result, L"some") != 0) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %ls "
					"(expected %ls)\n", string, delim, &state, result, L"some");
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 15;
		expectedState = string + 20;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
		if (wcscmp(result, L"text") != 0) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %ls "
					"(expected %ls)\n", string, delim, &state, result, L"text");
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = string + 22;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
		if (wcscmp(result, L"here!") != 0) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %ls "
					"(expected %ls)\n", string, delim, &state, result, L"here!");
			problemCount++;
		}

		result = wcstok(NULL, delim, &state);
		expected = NULL;
		expectedState = NULL;
		if (result != expected || state != expectedState) {
			printf("\tPROBLEM: result for wcstok(\"%ls\", \"%ls\", %p) = %p "
					"(expected %p), state = %p (expected %p)\n", string, delim,
				&state, result, expected, state, expectedState);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmemchr ------------------------------------------------------


void
test_wmemchr()
{
	printf("wmemchr()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* string = L"";
		const wchar_t ch = L' ';
		const wchar_t* result = wmemchr(string, ch, 0);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 0) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L'\0';
		const wchar_t* result = wmemchr(string, ch, 0);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 0) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"";
		const wchar_t ch = L'\0';
		const wchar_t* result = wmemchr(string, ch, 1);
		const wchar_t* expected = string;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"sometext";
		const wchar_t ch = L' ';
		const wchar_t* result = wmemchr(string, ch, 8);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some text";
		const wchar_t ch = L' ';
		const wchar_t* result = wmemchr(string, ch, 9);
		const wchar_t* expected = string + 4;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some text";
		const wchar_t ch = L'M';
		const wchar_t* result = wmemchr(string, ch, 9);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some\0text";
		const wchar_t ch = L't';
		const wchar_t* result = wmemchr(string, ch, 4);
		const wchar_t* expected = NULL;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* string = L"some\0text";
		const wchar_t ch = L't';
		const wchar_t* result = wmemchr(string, ch, 9);
		const wchar_t* expected = string + 5;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemchr(\"%ls\", '%lc', 1) = %p "
					"(expected %p), errno = %x (expected 0)\n", string, ch,
				result, expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmemcmp ------------------------------------------------------


void
test_wmemcmp()
{
	printf("wmemcmp()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* a = L"";
		const wchar_t* b = L"";
		int result = sign(wmemcmp(a, b, 0));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 0) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"";
		const wchar_t* b = L"";
		int result = sign(wmemcmp(a, b, 1));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 0) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"b";
		int result = sign(wmemcmp(a, b, 0));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 0) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"b";
		int result = sign(wmemcmp(a, b, 1));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 1) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"b";
		const wchar_t* b = L"a";
		int result = sign(wmemcmp(a, b, 2));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 2) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"a";
		const wchar_t* b = L"A";
		int result = sign(wmemcmp(a, b, 2));
		int expected = 1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 2) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst";
		const wchar_t* b = L"täst";
		int result = sign(wmemcmp(a, b, 5));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 5) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täst";
		const wchar_t* b = L"täst ";
		int result = sign(wmemcmp(a, b, 5));
		int expected = -1;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 5) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* a = L"täSt";
		const wchar_t* b = L"täs";
		int result = sign(wmemcmp(a, b, 2));
		int expected = 0;
		if (result != expected || errno != 0) {
			printf("\tPROBLEM: result for wmemcmp(\"%ls\", \"%ls\", 2) = %d "
					"(expected %d), errno = %x (expected 0)\n", a, b, result,
				expected, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmemcpy ------------------------------------------------------


void
test_wmemcpy()
{
	printf("wmemcpy()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmemcpy(destination, source, 0);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmemcpy(destination, \"%ls\", 0) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wmemcpy(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmemcpy(destination, source, 1);
		if (result != destination || wmemcmp(destination, source, 1) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmemcpy(destination, \"%ls\", 1) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wmemcpy(destination, \"%ls\", 1) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"tÄstdata \0with some charäcters";
		wchar_t destination[64];
		wchar_t* result = wmemcpy(destination, source, 31);
		if (result != destination || wmemcmp(destination, source, 31) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmemcpy(destination, \"%ls\", 31) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmempcpy ------------------------------------------------------


void
test_wmempcpy()
{
	printf("wmempcpy()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmempcpy(destination, source, 0);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmempcpy(destination, \"%ls\", 0) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wmempcpy(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmempcpy(destination, source, 1);
		if (result != destination + 1 || wmemcmp(destination, source, 1) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmempcpy(destination, \"%ls\", 1) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wmempcpy(destination, \"%ls\", 1) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"tÄstdata \0with some charäcters";
		wchar_t destination[64];
		wchar_t* result = wmempcpy(destination, source, 31);
		if (result != destination + 31 || wmemcmp(destination, source, 31) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmempcpy(destination, \"%ls\", 31) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmemmove ------------------------------------------------------


void
test_wmemmove()
{
	printf("wmemmove()\n");

	int problemCount = 0;
	errno = 0;

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmemmove(destination, source, 0);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmemmove(destination, \"%ls\", 0) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wmemmove(destination, \"%ls\", 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"";
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmemmove(destination, source, 1);
		if (result != destination || wmemcmp(destination, source, 1) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmemmove(destination, \"%ls\", 1) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[1] != L'X') {
			printf("\tPROBLEM: wmemmove(destination, \"%ls\", 1) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'X');
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"tÄstdata \0with some charäcters";
		wchar_t destination[64];
		wmemcpy(destination, source, 31);
		wchar_t* result = wmemmove(destination, destination + 4, 27);
		if (result != destination || wmemcmp(destination, source + 4, 27) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmemmove(destination, \"%ls\", 27) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n",
				source + 4, result, destination, errno);
			problemCount++;
		}
	}

	{
		const wchar_t* source = L"tÄstdata \0with some charäcters";
		wchar_t destination[64];
		wmemcpy(destination, source, 31);
		wchar_t* result = wmemmove(destination + 2, destination, 8);
		if (result != destination + 2
			|| wmemcmp(destination, L"tÄtÄstdatawith some charäcters", 31) != 0
			|| errno != 0) {
			printf("\tPROBLEM: result for wmemmove(destination + 9, \"%ls\", 8)"
					" = \"%ls\" (expected %p), errno = %x (expected 0)\n",
				source, result, destination, errno);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - wmemset ------------------------------------------------------


void
test_wmemset()
{
	printf("wmemset()\n");

	int problemCount = 0;
	errno = 0;

	{
		wchar_t source = L'\0';
		wchar_t destination[] = L"XXXX";
		wchar_t* result = wmemset(destination, source, 0);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmemset(destination, '%lc', 0) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[0] != L'X') {
			printf("\tPROBLEM: wmemset(destination, '%lc', 0) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'X');
			problemCount++;
		}
	}

	{
		wchar_t source = L'M';
		wchar_t destination[] = L"some text";
		wchar_t* result = wmemset(destination, source, 1);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmemset(destination, '%lc', 1) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		if (destination[0] != L'M') {
			printf("\tPROBLEM: wmemset(destination, '%lc', 1) -> "
					"destination[0]=%x, expected %x\n", source, destination[0],
				L'M');
			problemCount++;
		}
		if (destination[1] != L'o') {
			printf("\tPROBLEM: wmemset(destination, '%lc', 1) -> "
					"destination[1]=%x, expected %x\n", source, destination[1],
				L'o');
			problemCount++;
		}
	}

	{
		wchar_t source = L'M';
		wchar_t destination[] = L"some text";
		wchar_t* result = wmemset(destination, source, 9);
		if (result != destination || errno != 0) {
			printf("\tPROBLEM: result for wmemset(destination, '%lc', 9) = "
					"\"%ls\" (expected %p), errno = %x (expected 0)\n", source,
				result, destination, errno);
			problemCount++;
		}
		for (int i = 0; i < 9; ++i) {
			if (destination[i] != L'M') {
				printf("\tPROBLEM: wmemset(destination, '%lc', 9) -> "
						"destination[%d]=%x, expected %x\n", source, i,
					destination[i], L'M');
				problemCount++;
			}
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - sprintf ------------------------------------------------------


struct sprintf_data {
	const char* format;
	const void* value;
	const char* result;
};


void
test_sprintf(const char* locale, const sprintf_data data[])
{
	setlocale(LC_ALL, locale);
	printf("sprintf for '%s'\n", locale);

	int problemCount = 0;
	for (int i = 0; data[i].format != NULL; ++i) {
		char buf[100];
		if (strstr(data[i].format, "%ls") != NULL)
			sprintf(buf, data[i].format, (wchar_t*)data[i].value);
		else if (strstr(data[i].format, "%s") != NULL)
			sprintf(buf, data[i].format, (char*)data[i].value);
		if (strcmp(buf, data[i].result) != 0) {
			printf("\tPROBLEM: sprintf(\"%s\") = \"%s\" (expected \"%s\")\n",
				data[i].format, buf, data[i].result);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_sprintf()
{
	const sprintf_data sprintf_posix[] = {
		{ "%s", (const void*)"test", "test" },
		{ "%ls", (const void*)L"test", "test" },
		{ NULL, NULL, NULL }
	};
	test_sprintf("POSIX", sprintf_posix);

	const sprintf_data sprintf_de[] = {
		{ "%s", "test", "test" },
		{ "%ls", L"test", "test" },
		{ "%s", "t\xC3\xA4st", "t\xC3\xA4st" },
		{ "%ls", L"t\xE4st", "t\xC3\xA4st" },
		{ NULL, NULL, NULL }
	};
	test_sprintf("de_DE.UTF-8", sprintf_de);

	const sprintf_data sprintf_de_iso[] = {
		{ "%s", "test", "test" },
		{ "%ls", L"test", "test" },
		{ "%s", "t\xC3\xA4st", "t\xC3\xA4st" },
		{ "%s", "t\xE4st", "t\xE4st" },
		{ "%ls", L"t\xE4st", "t\xE4st" },
		{ NULL, NULL, NULL }
	};
	test_sprintf("de_DE.ISO8859-1", sprintf_de_iso);
}


// #pragma mark - swprintf ----------------------------------------------------


struct swprintf_data {
	const wchar_t* format;
	const void* value;
	const wchar_t* result;
};


void
test_swprintf(const char* locale, const swprintf_data data[])
{
	setlocale(LC_ALL, locale);
	printf("swprintf for '%s'\n", locale);

	int problemCount = 0;
	for (int i = 0; data[i].format != NULL; ++i) {
		wchar_t buf[100];
		if (wcsstr(data[i].format, L"%ls") != NULL)
			swprintf(buf, 100, data[i].format, (wchar_t*)data[i].value);
		else if (wcsstr(data[i].format, L"%s") != NULL)
			swprintf(buf, 100, data[i].format, (char*)data[i].value);
		if (wcscmp(buf, data[i].result) != 0) {
			printf("\tPROBLEM: swprintf(\"%ls\") = \"%ls\" (expected \"%ls\")\n",
				data[i].format, buf, data[i].result);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_swprintf()
{
	const swprintf_data swprintf_posix[] = {
		{ L"%s", (const void*)"test", L"test" },
		{ L"%ls", (const void*)L"test", L"test" },
		{ NULL, NULL, NULL }
	};
	test_swprintf("POSIX", swprintf_posix);

	const swprintf_data swprintf_de[] = {
		{ L"%s", "test", L"test" },
		{ L"%ls", L"test", L"test" },
		{ L"%s", "t\xC3\xA4st", L"t\xE4st" },
		{ L"%ls", L"t\xE4st", L"t\xE4st" },
		{ NULL, NULL, NULL }
	};
	test_swprintf("de_DE.UTF-8", swprintf_de);

	const swprintf_data swprintf_de_iso[] = {
		{ L"%s", "test", L"test" },
		{ L"%ls", L"test", L"test" },
		{ L"%s", "t\xC3\xA4st", L"t\xC3\xA4st" },
		{ L"%s", "t\xE4st", L"t\xE4st" },
		{ L"%ls", L"t\xE4st", L"t\xE4st" },
		{ NULL, NULL, NULL }
	};
	test_swprintf("de_DE.ISO8859-1", swprintf_de_iso);
}


// #pragma mark - main ---------------------------------------------------------


/*
 * Test several different aspects of the wchar-string functions.
 */
int
main(void)
{
	setlocale(LC_ALL, "de_DE");

	test_collation();

	test_sprintf();
	test_swprintf();

	test_wcsftime();

	test_wcpcpy();
	test_wcscasecmp();
	test_wcscat();
	test_wcschr();
	test_wcscmp();
	test_wcscpy();
	test_wcscspn();
	test_wcsdup();
#ifdef __HAIKU__
	test_wcslcat();
	test_wcslcpy();
#endif
	test_wcslen();
	test_wcspbrk();
	test_wcsspn();
	test_wcsstr();
	test_wcstok();

	test_wmemchr();
	test_wmemcmp();
	test_wmemcpy();
	test_wmemmove();
	test_wmempcpy();
	test_wmemset();

	return 0;
}
