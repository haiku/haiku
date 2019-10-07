/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef VALIDATION_FAILURE_TEST_H
#define VALIDATION_FAILURE_TEST_H

#include "Message.h"

#include <TestCase.h>
#include <TestSuite.h>


class ValidationFailureTest : public CppUnit::TestCase {
public:
								ValidationFailureTest();
	virtual						~ValidationFailureTest();

			void				TestArchive();
			void				TestDearchive();

	static	void				AddTests(BTestSuite& suite);

private:
	static	status_t			FindMessageWithProperty(
									const char* property,
									BMessage& validationFailuresMessage,
									BMessage& validationFailureMessage);
	static	void				FindValidationMessages(
									BMessage& validationFailureMessage,
									BStringList& validationMessages);
};


#endif	// VALIDATION_FAILURE_TEST_H
