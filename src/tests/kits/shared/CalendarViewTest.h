/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef CALENDAR_VIEW_TEST_H
#define CALENDAR_VIEW_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class CalendarViewTest : public BTestCase {
public:
								CalendarViewTest();
	virtual						~CalendarViewTest();

			void				TestSetters();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// CALENDAR_VIEW_TEST_H
