/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NATURAL_COMPARE_TEST_H
#define NATURAL_COMPARE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class Sample;


class NaturalCompareTest : public CppUnit::TestCase {
public:
								NaturalCompareTest();
	virtual						~NaturalCompareTest();

			void				TestSome();

	static	void				AddTests(BTestSuite& suite);

private:
			void				_RunTests(const Sample* samples, int count);
			void				_RunTest(const char* a, const char* b,
									int expectedResult);
			int					_Normalize(int result);
};


#endif	// NATURAL_COMPARE_TEST_H
