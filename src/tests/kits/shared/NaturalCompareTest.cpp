/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <NaturalCompare.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


struct Sample {
	const char*	a;
	const char*	b;
	int			expectedResult;
};


static int
Normalize(int result)
{
	if (result > 0)
		return 1;
	if (result < 0)
		return -1;
	return 0;
}


static void
RunNaturalCompareTest(const char* a, const char* b, int expectedResult)
{
	int result = Normalize(BPrivate::NaturalCompare(a, b));

	char message[256];
	snprintf(message, sizeof(message), "\"%s\" vs. \"%s\" == %d, expected %d",
		a, b, result, expectedResult);

	CppUnit::Asserter::failIf(result != expectedResult, message);
}


class NaturalCompareTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(NaturalCompareTest);
	CPPUNIT_TEST(TestSome);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestSome()
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

		for (size_t i = 0; i < sizeof(samples) / sizeof(Sample); i++) {
			const Sample& sample = samples[i];

			RunNaturalCompareTest(sample.a, sample.b, sample.expectedResult);
			RunNaturalCompareTest(sample.b, sample.a, -sample.expectedResult);
		}
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(NaturalCompareTest, getTestSuiteName());
