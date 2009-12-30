/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_ERROR_H
#define TEST_ERROR_H


#include <SupportDefs.h>


class Test;


class TestError {
public:
								TestError(Test* test, char* message);
								~TestError();

			Test*				GetTest() const	{ return fTest; }
			const char*			Message() const	{ return fMessage; }

			TestError*&			ListLink()	{ return fNext; }

private:
			TestError*			fNext;
			Test*				fTest;
			char*				fMessage;
};


#endif	// TEST_ERROR_H
