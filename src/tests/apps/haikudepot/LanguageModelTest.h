/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef LANGUAGE_MODEL_TEST_H
#define LANGUAGE_MODEL_TEST_H


#include <Message.h>

#include <TestCase.h>
#include <TestSuite.h>


class LanguageModelTest : public CppUnit::TestCase {
public:
								LanguageModelTest();
	virtual						~LanguageModelTest();

			void				TestSetPreferredLanguageCodeOnly();
			void				TestSetPreferredLanguageSystemDefaultMoreSpecific();
			void				TestSetPreferredLanguageSystemDefaultLessSpecific();

	static	void				AddTests(BTestSuite& suite);

};


#endif	// LANGUAGE_MODEL_TEST_H
