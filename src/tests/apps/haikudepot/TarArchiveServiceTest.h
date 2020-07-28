/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef TAR_ARCHIVE_SERVICE_TEST_H
#define TAR_ARCHIVE_SERVICE_TEST_H

#include "String.h"

#include <TestCase.h>
#include <TestSuite.h>


class TarArchiveServiceTest : public CppUnit::TestCase {
public:
								TarArchiveServiceTest();
	virtual						~TarArchiveServiceTest();

			void				TestReadHeader();

	static	void				AddTests(BTestSuite& suite);
private:
			status_t			CreateSampleFile(BString& path);
			status_t			DeleteSampleFile(const BString& path);
};


#endif	// TAR_ARCHIVE_SERVICE_TEST_H
