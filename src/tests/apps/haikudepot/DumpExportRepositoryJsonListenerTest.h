/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef DUMP_EXPORT_REPOSITORY_JSON_LISTENER_TEST_H
#define DUMP_EXPORT_REPOSITORY_JSON_LISTENER_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class DumpExportRepositoryJsonListenerTest : public CppUnit::TestCase {
public:
								DumpExportRepositoryJsonListenerTest();
	virtual						~DumpExportRepositoryJsonListenerTest();

			void				TestSingle();
			void				TestBulkContainer();

	static	void				AddTests(BTestSuite& suite);

};


#endif	// DUMP_EXPORT_REPOSITORY_JSON_LISTENER_TEST_H
