/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Language.h>
#include <Collator.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class CollatorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CollatorTest);
	CPPUNIT_TEST(B_COLLATE_PRIMARY_Compare_ReturnsCorrectDifference);
	CPPUNIT_TEST(B_COLLATE_SECONDARY_Compare_ReturnsCorrectDifference);
	CPPUNIT_TEST(B_COLLATE_TERTIARY_Compare_ReturnsCorrectDifference);
	CPPUNIT_TEST_SUITE_END();

	BCollator fCollator;
	struct TestCase {
		const char* first;
		const char* second;
		int result;
	};

	void _CompareTest(collator_strengths strength, const TestCase* cases, size_t length)
	{
		for (size_t i = 0; i < length; i++) {
			BString a, b;
			fCollator.SetStrength(strength);
			fCollator.GetSortKey(cases[i].first, &a);
			fCollator.GetSortKey(cases[i].second, &b);

			int difference = fCollator.Compare(cases[i].first, cases[i].second);
			CPPUNIT_ASSERT_EQUAL_MESSAGE(std::string(cases[i].first), cases[i].result, difference);
			int keydiff = strcmp(a.String(), b.String());
			// Check that the keys compare the same as the strings. Either both
			// are 0, or both have the same sign.
			if (difference == 0)
				CPPUNIT_ASSERT_EQUAL(0, keydiff);
			else
				CPPUNIT_ASSERT(keydiff * difference > 0);
		}
	}

public:
	void setUp()
	{
		BLanguage language("en");
		BLocale locale = BLocale(&language);
		locale.GetCollator(&fCollator);
		// FIXME: BCollator("en") crashes
	}

	void B_COLLATE_PRIMARY_Compare_ReturnsCorrectDifference() {
		const TestCase cases[] = {
			{"gehen", "géhen", 0},
			{"aus", "äUß", 1},
			{"auss", "äUß", 1},
			{"WO", "wÖ", -1},
			{"SO", "so", -1},
			{"açñ", "acn", 0},
			//{NULL, NULL, 0}
		};
		_CompareTest(B_COLLATE_PRIMARY, cases, sizeof(cases) / sizeof(TestCase));
	}

	void B_COLLATE_SECONDARY_Compare_ReturnsCorrectDifference() {
		const TestCase cases[] = {
			{"gehen", "géhen", -1},
			{"aus", "äUß", 1},
			{"auss", "äUß", 1},
			{"WO", "wÖ", -1},
			{"SO", "so", -1},
			{"açñ", "acn", 1},
			//{NULL, NULL, 0}
		};
		_CompareTest(B_COLLATE_SECONDARY, cases, sizeof(cases) / sizeof(TestCase));
	}

	void B_COLLATE_TERTIARY_Compare_ReturnsCorrectDifference() {
		const TestCase cases[] = {
			{"gehen", "géhen", -1},
			{"aus", "äUß", 1},
			{"auss", "äUß", 1},
			{"WO", "wÖ", -1},
			{"SO", "so", -1},
			{"açñ", "acn", 1},
			//{NULL, NULL, 0}
		};
		_CompareTest(B_COLLATE_TERTIARY, cases, sizeof(cases) / sizeof(TestCase));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CollatorTest, getTestSuiteName());
