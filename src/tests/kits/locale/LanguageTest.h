/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_TEST_H
#define LANGUAGE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class LanguageTest: public BTestCase {
public:
					LanguageTest();
	virtual			~LanguageTest();

			void	TestLanguageParseJapanese();
			void	TestLanguageParseFrenchWithCountry();
			void	TestLanguageParseSerbianScriptAndCountry();
			void	TestLanguageParseSerbianScriptAndCountryHyphens();

			void	TestLanguageNameFrenchInEnglish();
			void	TestLanguageNameFrenchInFrench();
			void	TestLanguagePropertiesFrench();

	static	void	AddTests(BTestSuite& suite);
};


#endif
