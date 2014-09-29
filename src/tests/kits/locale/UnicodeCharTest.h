/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNICODE_CHAR_TEST_H
#define UNICODE_CHAR_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class UnicodeCharTest: public BTestCase {
public:
					UnicodeCharTest();
	virtual			~UnicodeCharTest();

			void	TestAscii();
			void	TestISO8859();
			void	TestUTF8();

	static	void	AddTests(BTestSuite& suite);

private:
	struct Result {
		const char* value;
		bool isAlpha;
		bool isAlNum;
		bool isLower;
		bool isUpper;
		bool isDefined;
		int type;
		int32 toUpper;
		int32 toLower;
	};

			void	_ToString(uint32 c, char* text);
			void	_TestChar(uint32 c, Result result);
};

#endif
