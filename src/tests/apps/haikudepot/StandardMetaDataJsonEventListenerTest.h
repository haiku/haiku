/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef STANDARD_META_DATA_JSON_EVENT_LISTENER_TEST_H
#define STANDARD_META_DATA_JSON_EVENT_LISTENER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class StandardMetaDataJsonEventListenerTest : public CppUnit::TestCase {
public:
								StandardMetaDataJsonEventListenerTest();
	virtual						~StandardMetaDataJsonEventListenerTest();

			void				TestTopLevel();
			void				TestIcon();
			void				TestThirdLevel();
			void				TestBroken();

	static	void				AddTests(BTestSuite& suite);

private:
			void				TestGeneric(const char* input,
									const char* jsonPath,
									status_t expectedStatus,
									uint64_t expectedCreateTimestamp,
									uint64_t expectedDataModifiedTimestamp);

};


#endif	// STANDARD_META_DATA_JSON_EVENT_LISTENER_TEST_H
