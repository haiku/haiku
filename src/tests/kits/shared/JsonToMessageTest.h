/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef JSON_TO_MESSAGE_TEST_H
#define JSON_TO_MESSAGE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>

#include <DataIO.h>

class Sample;


class JsonToMessageTest : public CppUnit::TestCase {
public:
								JsonToMessageTest();
	virtual						~JsonToMessageTest();

			void				TestArrayA();
			void				TestArrayB();
			void				TestObjectAForPerformance();
			void				TestObjectA();
			void				TestObjectB();
			void				TestObjectC();
			void				TestUnterminatedObject();
			void				TestUnterminatedArray();
			void				TestHaikuDepotFetchBatch();

	static	void				AddTests(BTestSuite& suite);
private:

};


#endif	// JSON_TO_MESSAGE_TEST_H
