/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <CalendarView.h>
#include <Window.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class CalendarViewTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CalendarViewTest);
	CPPUNIT_TEST(ValidDate_SetDate_SetsCorrectDate);
	CPPUNIT_TEST(LeapYear_SetDate_SetsCorrectDate);
	CPPUNIT_TEST(EndOfMonthDate_SetMonth_AdjustsDay);
	CPPUNIT_TEST(LeapYearToLeapYear_SetYear_PreservesLeapDay);
	CPPUNIT_TEST(LeapYearToNonLeapYear_SetYear_AdjustsLeapDay);
	CPPUNIT_TEST_SUITE_END();

	BApplication*				fApp;
	BWindow*					fWindow;
	BPrivate::BCalendarView*	fView;

public:
	void setUp()
	{
		fApp = new BApplication(
			"application/x-vnd.CalendarViewTest.test");
		fWindow = new BWindow(BRect(50, 50, 550, 550),
			"CalendarViewTest", B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE, 0);
		fView = new BPrivate::BCalendarView("test");
		fWindow->AddChild(fView);
	}

	void tearDown()
	{
		delete fApp;
		be_app = NULL;
	}

	void ValidDate_SetDate_SetsCorrectDate()
	{
		fView->SetDate(2014, 8, 31);
		CPPUNIT_ASSERT_EQUAL(2014, (int)fView->Year());
		CPPUNIT_ASSERT_EQUAL(8, (int)fView->Month());
		CPPUNIT_ASSERT_EQUAL(31, (int)fView->Day());
	}

	void LeapYear_SetDate_SetsCorrectDate()
	{
		fView->SetDate(2004, 2, 29);
		CPPUNIT_ASSERT_EQUAL(2004, (int)fView->Year());
		CPPUNIT_ASSERT_EQUAL(2, (int)fView->Month());
		CPPUNIT_ASSERT_EQUAL(29, (int)fView->Day());
	}

	void EndOfMonthDate_SetMonth_AdjustsDay()
	{
		fView->SetDate(2014, 8, 31);
		// Moving to month with less days should adjust day
		fView->SetMonth(2);
		CPPUNIT_ASSERT_EQUAL(2014, (int)fView->Year());
		CPPUNIT_ASSERT_EQUAL(2, (int)fView->Month());
		CPPUNIT_ASSERT_EQUAL(28, (int)fView->Day());
	}

	void LeapYearToLeapYear_SetYear_PreservesLeapDay()
	{
		fView->SetDate(2004, 2, 29);
		// Moving from leap year to leap year on 29 feb. must not change day
		fView->SetYear(2008);
		CPPUNIT_ASSERT_EQUAL(2008, (int)fView->Year());
		CPPUNIT_ASSERT_EQUAL(2, (int)fView->Month());
		CPPUNIT_ASSERT_EQUAL(29, (int)fView->Day());
	}

	void LeapYearToNonLeapYear_SetYear_AdjustsLeapDay()
	{
		fView->SetDate(2004, 2, 29);
		// Moving from leap year to non-leap year on 29 feb. must go back to 28
		fView->SetYear(2014);
		CPPUNIT_ASSERT_EQUAL(2014, (int)fView->Year());
		CPPUNIT_ASSERT_EQUAL(2, (int)fView->Month());
		CPPUNIT_ASSERT_EQUAL(28, (int)fView->Day());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CalendarViewTest, getTestSuiteName());
