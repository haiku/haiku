/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef DUMP_EXPORT_REPOSITORY_TEST_H
#define DUMP_EXPORT_REPOSITORY_TEST_H

#include <Message.h>

#include <TestCase.h>
#include <TestSuite.h>

#include "DumpExportRepositoryModel.h"


/*!	Tests the model in insolation from the listener. */
class DumpExportRepositoryTest : public CppUnit::TestCase {
public:
								DumpExportRepositoryTest();
	virtual						~DumpExportRepositoryTest();

			void				TestEqualityTrue();
			void				TestEqualityFalse();
			void				TestEqualityTransientFalse();

			void				TestBuilderFromCold();
			void				TestBuilderFromExisting();
			void				TestBuilderFromExistingNoChange();
			void				TestBuilderClear();

			void				TestFromMessage();
			void				TestToMessage();

	static	void				AddTests(BTestSuite& suite);

private:
			DumpExportRepositoryRef
								_CreateAny();
			BMessage			_CreateAnyMessage();
};


#endif // DUMP_EXPORT_REPOSITORY_TEST_H
