/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCALE_UTILS_TEST_H
#define LOCALE_UTILS_TEST_H


#include <Message.h>

#include <TestCase.h>
#include <TestSuite.h>


class LocaleUtilsTest : public CppUnit::TestCase
{
public:
								LocaleUtilsTest();
	virtual						~LocaleUtilsTest();

			void				TestLanguageIsBeforeFalseAfter();
			void				TestLanguageIsBeforeFalseEqual();
			void				TestLanguageIsBeforeTrueBefore();
			void				TestLanguageSorting();

			void				TestDeriveDefaultLanguageCodeOnly();
			void				TestDeriveDefaultLanguageSystemDefaultMoreSpecific();
			void				TestDeriveDefaultLanguageSystemDefaultLessSpecific();

	static	void				AddTests(BTestSuite& suite);

};


#endif // LOCALE_UTILS_TEST_H
