//------------------------------------------------------------------------------
//	main.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/app/Messenger.h>
#include <be/app/MessageFilter.h>

#if !defined(SYSTEM_TEST)
#include <be/app/Looper.h>
#else
#include "../../../../source/lib/application/headers/Looper.h"
#endif

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "IsMessageWaitingTest.h"
#include "RemoveHandlerTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BMessageQueue test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BMessageQueue, once for the
 *             OpenBeOS implementation.
 */

Test* addonTestFunc(void)
{
	TestSuite* tests = new TestSuite("BLooper");

	tests->addTest(TIsMessageWaitingTest::Suite());
	tests->addTest(TRemoveHandlerTest::Suite());

	return tests;
}
#if 0
class TestLooper : public BLooper
{
	public:
		TestLooper() : BLooper(), fYouCanQuit(false) {;}

		virtual bool QuitRequested()
		{
			printf("\n%s: %s%s\n", __PRETTY_FUNCTION__, fYouCanQuit ? "" : "not ",
				   "quitting");
			return fYouCanQuit;
		}
		void SetYouCanQuit(bool yesNo) { fYouCanQuit = yesNo; }

	private:
		bool	fYouCanQuit;
};

filter_result QuitFilter(BMessage* message, BHandler** target,
						BMessageFilter* filter)
{
	printf("\nAnd here we are\n");
	return B_DISPATCH_MESSAGE;
}
#endif
int main()
{
	Test* tests = addonTestFunc();

	TextTestResult Result;
	tests->run(&Result);
	cout << Result << endl;

	delete tests;

	return !Result.wasSuccessful();
#if 0
	BLooper* Looper = new TestLooper;
	Looper->AddFilter(new BMessageFilter(_QUIT_, QuitFilter));
	Looper->Run();
	Looper->Lock();
	((TestLooper*)Looper)->SetYouCanQuit(true);
	Looper->Quit();

	snooze(1000000);

//#else
	BMessage QuitMsg(B_QUIT_REQUESTED);

	printf("\nLooper thread: 0x%lx\n", Looper->Thread());

	status_t err;
	BMessenger Msgr(NULL, Looper, &err);
	if (!err)
	{
		err = Msgr.SendMessage(&QuitMsg, &QuitMsg);
		if (!err)
		{
			QuitMsg.PrintToStream();
			BMessage QuitMsg2(B_QUIT_REQUESTED);
			((TestLooper*)Looper)->SetYouCanQuit(true);
			err = Msgr.SendMessage(&QuitMsg2, &QuitMsg2);
			if (!err)
			{
				QuitMsg2.PrintToStream();
			}
		}
	}

	return 0;
#endif
}

/*
 * $Log $
 *
 * $Id  $
 *
 */

