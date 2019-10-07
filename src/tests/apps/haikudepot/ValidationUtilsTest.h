/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef VALIDATION_UTILS_TEST_H
#define VALIDATION_UTILS_TEST_H

#include "Message.h"

#include <TestCase.h>
#include <TestSuite.h>


class ValidationUtilsTest : public CppUnit::TestCase {
public:
								ValidationUtilsTest();
	virtual						~ValidationUtilsTest();

			void				TestEmailValid();
			void				TestEmailInvalidNoAt();
			void				TestEmailInvalidNoMailbox();
			void				TestEmailInvalidNoDomain();
			void				TestEmailInvalidTwoAts();

			void				TestNicknameValid();
			void				TestNicknameInvalid();
			void				TestNicknameInvalidBadChars();

			void				TestPasswordClearValid();
			void				TestPasswordClearInvalid();

	static	void				AddTests(BTestSuite& suite);

};


#endif	// VALIDATION_UTILS_TEST_H
