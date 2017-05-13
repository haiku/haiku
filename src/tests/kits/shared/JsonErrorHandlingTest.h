/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef JSON_ERROR_HANDLING_TEST_H
#define JSON_ERROR_HANDLING_TEST_H


#include <TestCase.h>
#include <TestSuite.h>

#include <DataIO.h>

class Sample;


class JsonErrorHandlingTest : public CppUnit::TestCase {
public:
								JsonErrorHandlingTest();
	virtual						~JsonErrorHandlingTest();

			void				TestCompletelyUnknown();
			void				TestObjectWithPrematureSeparator();

			void				TestStringUnterminated();

			void				TestBadStringEscape();

			void				TestBadNumber();

			void				TestIOIssue();

	static	void				AddTests(BTestSuite& suite);
private:
			void				TestParseWithUnexpectedCharacter(
									const char* input, int32 line,
									status_t expectedStatus, char expectedChar);

			void				TestParseWithUnterminatedElement(
									const char* input, int32 line,
									status_t expectedStatus);

			void				TestParseWithBadStringEscape(
									const char* input,
									int32 line, status_t expectedStatus,
									char expectedBadEscapeChar);

			void				TestParseWithErrorMessage(
									const char* input, int32 line,
									status_t expectedStatus,
									const char* expectedMessage);

			void				TestParseWithErrorMessage(
									BDataIO* data, int32 line,
									status_t expectedStatus,
									const char* expectedMessage);

};


#endif	// JSON_ERROR_HANDLING_TEST_H
