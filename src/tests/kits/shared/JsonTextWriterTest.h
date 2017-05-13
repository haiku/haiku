/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef JSON_WRITER_TEST_H
#define JSON_WRITER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>

#include <DataIO.h>

class Sample;


class JsonTextWriterTest : public CppUnit::TestCase {
public:
								JsonTextWriterTest();
	virtual						~JsonTextWriterTest();

			void				TestArrayA();
			void				TestObjectA();
			void				TestObjectAForPerformance();
			void				TestDouble();
			void				TestInteger();
			void				TestFalse();

	static	void				AddTests(BTestSuite& suite);
private:

};


#endif	// JSON_WRITER_TEST_H
