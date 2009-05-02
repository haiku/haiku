#include <TestListener.h>

#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <iostream>
#include <stdio.h>
#include <OS.h>

using std::cout;
using std::endl;

_EXPORT
void
BTestListener::startTest( CppUnit::Test *test ) {
   	fOkay = true;
   	cout << test->getName() << endl;
   	startTime = real_time_clock_usecs();
}

_EXPORT
void
BTestListener::addFailure( const CppUnit::TestFailure &failure ) {
   	fOkay = false;
   	cout << "  - ";
   	cout << (failure.isError() ? "ERROR" : "FAILURE");
   	cout << " -- ";
   	cout << (failure.thrownException() != NULL
   	           ? failure.thrownException()->what()
   	             : "(unknown error)");
   	cout << endl;
}

_EXPORT
void
BTestListener::endTest( CppUnit::Test *test )  {
	bigtime_t length = real_time_clock_usecs() - startTime;
   	if (fOkay)
   		cout << "  + PASSED" << endl;
//   	else
//   		cout << "  - FAILED" << endl;
	printTime(length);
   	cout << endl;
}

_EXPORT
void
BTestListener::printTime(bigtime_t time) {
	// Print out the elapsed time all pretty and stuff:
	// time >= 1 minute: HH:MM:SS
	// 1 minute > time:  XXX ms
	const bigtime_t oneMillisecond = 1000;
	const bigtime_t oneSecond = oneMillisecond*1000;
	const bigtime_t oneMinute = oneSecond*60;
	const bigtime_t oneHour = oneMinute*60;
	const bigtime_t oneDay = oneHour*24;
	if (time >= oneDay) {
		cout << "    Your test ran for longer than an entire day. Honestly," << endl;
		cout << "    that's 24 hours. That's a long time. Please write shorter" << endl;
		cout << "    tests. Clock time: " << time << " microseconds." << endl;
	} else {
		cout << "    Clock time: ";
		if (time >= oneMinute) {
			bool begun = true;
			if (begun || time >= oneHour) {
				begun = true;
				cout.width(2);
				cout.fill('0');
				cout << time / oneHour << ":";
				time %= oneHour;
			}
			if (begun || time >= oneMinute) {
				begun = true;
				cout.width(2);
				cout.fill('0');
				cout << time / oneMinute << ":";
				time %= oneMinute;
			}
			if (begun || time >= oneSecond) {
				begun = true;
				cout.width(2);
				cout.fill('0');
				cout << time / oneSecond;
				time %= oneSecond;
			}
		} else {
			cout << time / oneMillisecond << " ms";
		}
		cout << endl;
	}
}
