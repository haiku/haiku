/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef JWT_TOKEN_HELPER_TEST_H
#define JWT_TOKEN_HELPER_TEST_H


#include <Message.h>

#include <TestCase.h>
#include <TestSuite.h>


class JwtTokenHelperTest : public CppUnit::TestCase {
public:
								JwtTokenHelperTest();
	virtual						~JwtTokenHelperTest();

			void				TestParseTokenClaims();
			void				TestCorrupt();

	static	void				AddTests(BTestSuite& suite);

private:
			void				_AssertStringValue(const BMessage& message, const char* key,
									const char* expectedValue) const;
			void				_AssertDoubleValue(const BMessage& message, const char* key,
									int64 expectedValue) const;
};


#endif	// JWT_TOKEN_HELPER_TEST_H
