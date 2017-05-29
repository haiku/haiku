/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "NaturalCompareTest.h"

#include <NaturalCompare.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


using namespace BPrivate;


struct Sample {
	const char*	a;
	const char*	b;
	int			expectedResult;
};


NaturalCompareTest::NaturalCompareTest()
{
}


NaturalCompareTest::~NaturalCompareTest()
{
}


void
NaturalCompareTest::TestSome()
{
	static const Sample samples[] = {
		// NULL in either side of the comparison
		{NULL, NULL, 0},
		{NULL, "A", -1},
		{"A", NULL,  1},
		// Case insensitive
		{"a", "A", 0},
		{"é", "É", 0},
		// Handling of accented characters
		{"ä", "a", 1},
		// Natural number ordering
		{"3", "99", -1},
		{"9", "19", -1},
		{"13", "99", -1},
		{"9", "111", -1},
		{"00000009", "111", -1},
		// Natural number ordering, ignoring leading space
		{"Hallo2", "hallo12", -1},
		{"Hallo 2", "hallo12", -1},
		{"Hallo  2", "hallo12", -1},
		{"Hallo  2 ", "hallo12", -1},
		// A mix of everything
		{"12 äber 42", "12aber42", -1},
		{"12 äber 42", "12aber43", -1},
		{"12 äber 44", "12aber43", -1},
		{"12 äber 44", "12 aber45", -1},
	};
	_RunTests(samples, sizeof(samples) / sizeof(Sample));
}


/*static*/ void
NaturalCompareTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NaturalCompareTest");

	suite.addTest(new CppUnit::TestCaller<NaturalCompareTest>(
		"NaturalCompareTest::TestSome", &NaturalCompareTest::TestSome));

	parent.addTest("NaturalCompareTest", &suite);
}


void
NaturalCompareTest::_RunTests(const Sample* samples, int count)
{
	for (int i = 0; i < count; i++) {
		const Sample& sample = samples[i];

		_RunTest(sample.a, sample.b, sample.expectedResult);
		_RunTest(sample.b, sample.a, -sample.expectedResult);
	}
}


void
NaturalCompareTest::_RunTest(const char* a, const char* b, int expectedResult)
{
	int result = _Normalize(NaturalCompare(a, b));

	char message[256];
	snprintf(message, sizeof(message), "\"%s\" vs. \"%s\" == %d, expected %d",
		a, b, result, expectedResult);

	CppUnit::Asserter::failIf(result != expectedResult, message);
}


int
NaturalCompareTest::_Normalize(int result)
{
	if (result > 0)
		return 1;
	if (result < 0)
		return -1;
	return 0;
}
