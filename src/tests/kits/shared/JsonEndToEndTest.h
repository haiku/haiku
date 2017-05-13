/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef JSON_END_TO_END_TEST_H
#define JSON_END_TO_END_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class Sample;


class JsonEndToEndTest : public CppUnit::TestCase {
public:
								JsonEndToEndTest();
	virtual						~JsonEndToEndTest();

			void				TestNullA();
			void				TestTrueA();
			void				TestFalseA();
			void				TestNumberA();
			void				TestStringA();
			void				TestStringA2();
			void				TestStringB();
			void				TestArrayA();
			void				TestArrayB();
			void				TestObjectA();

			void				TestStringUnterminated();
			void				TestArrayUnterminated();
			void				TestObjectUnterminated();

	static	void				AddTests(BTestSuite& suite);
private:
			void				TestUnterminated(const char* input);

			void				TestParseAndWrite(const char* input,
									const char* expectedOutput);
};


#endif	// JSON_END_TO_END_TEST_H
