/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <AutoDeleter.h>
#include <File.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "TarArchiveHeader.h"
#include "TarArchiveService.h"


class TarArchiveServiceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TarArchiveServiceTest);
	CPPUNIT_TEST(ReadHeader_Success);
	CPPUNIT_TEST_SUITE_END();

public:
	void ReadHeader_Success()
	{
		BString tarPath = "resources/apps/haikudepot/sample.tar";

		BFile tarFile(tarPath, O_RDONLY);
		tarFile.Seek(2048, SEEK_SET);
			// known offset in the same data

		TarArchiveHeader header;

		status_t result = TarArchiveService::GetEntry(tarFile, header);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString("hicn/somepkg/16.png"), header.FileName());
		CPPUNIT_ASSERT_EQUAL((uint64)657, header.Length());
		CPPUNIT_ASSERT_EQUAL((int)TAR_FILE_TYPE_NORMAL, (int)header.FileType());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TarArchiveServiceTest, getTestSuiteName());
