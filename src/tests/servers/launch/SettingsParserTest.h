/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_PARSER_TEST_H
#define SETTINGS_PARSER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>

#include <Message.h>


class SettingsParserTest : public CppUnit::TestCase {
public:
								SettingsParserTest();
	virtual						~SettingsParserTest();

			void				TestConditionsMultiLine();
			void				TestConditionsFlat();
			void				TestConditionsFlatWithNot();
			void				TestConditionsFlatWithArgs();
			void				TestConditionsFlatWithNotAndArgs();
			void				TestConditionsMultiLineFlatNot();
			void				TestConditionsMultiLineFlatNotWithArgs();
			void				TestConditionsMultiLineNot();

	static	void				AddTests(BTestSuite& suite);

private:
			status_t			_ParseCondition(const char* text,
									BMessage& message);
};


#endif	// SETTINGS_PARSER_TEST_H
