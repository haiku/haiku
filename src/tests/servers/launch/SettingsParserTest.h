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

			void				TestConditionsEmpty();
			void				TestConditionsMultiLine();
			void				TestConditionsFlat();
			void				TestConditionsFlatWithNot();
			void				TestConditionsFlatWithArgs();
			void				TestConditionsFlatWithNotAndArgs();
			void				TestConditionsMultiLineFlatNot();
			void				TestConditionsMultiLineFlatNotWithArgs();
			void				TestConditionsMultiLineNot();

			void				TestEventsEmpty();
			void				TestEventsMultiLine();
			void				TestEventsFlat();
			void				TestEventsFlatWithArgs();

			void				TestEnvironmentMultiLine();
			void				TestEnvironmentFlat();

			void				TestRunFlat();
			void				TestRunMultiLine();
			void				TestRunIfThenElseFlat();
			void				TestRunIfThenElseMultiLine();

	static	void				AddTests(BTestSuite& suite);

private:
			status_t			_ParseCondition(const char* text,
									BMessage& message);
			status_t			_ParseEvent(const char* text,
									BMessage& message);
			status_t			_ParseName(const char* name, const char* text,
									BMessage& message);
			int32				_ArrayCount(BMessage& message,
									const char* name);
};


#endif	// SETTINGS_PARSER_TEST_H
