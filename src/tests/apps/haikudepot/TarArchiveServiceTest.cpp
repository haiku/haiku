/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "TarArchiveServiceTest.h"

#include <stdlib.h>

#include <AutoDeleter.h>
#include <File.h>
#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "TarArchiveHeader.h"
#include "TarArchiveService.h"

#include "TarArchiveServiceTestSample.h"


TarArchiveServiceTest::TarArchiveServiceTest()
{
}


TarArchiveServiceTest::~TarArchiveServiceTest()
{
}


void
TarArchiveServiceTest::TestReadHeader()
{
	BString tarPath;
	if (CreateSampleFile(tarPath) != B_OK) {
		printf("! unable to setup the sample tar --> exit");
		exit(1);
	}

	BFile *tarFile = new BFile(tarPath, O_RDONLY);
	tarFile->Seek(2048, SEEK_SET);
		// known offset in the same data

	TarArchiveHeader header;

// ----------------------
	status_t result = TarArchiveService::GetEntry(*tarFile, header);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(BString("hicn/somepkg/16.png"), header.FileName());
	CPPUNIT_ASSERT_EQUAL(657, header.Length());
	CPPUNIT_ASSERT_EQUAL(TAR_FILE_TYPE_NORMAL, header.FileType());

	delete tarFile;

	DeleteSampleFile(tarPath);
}


/*!	This method will store the tar file sample into a temporary file and will
	return the pathname to that file.
*/

status_t
TarArchiveServiceTest::CreateSampleFile(BString& path)
{
	path.SetTo(tmpnam(NULL));
	BFile* file = new BFile(path, O_WRONLY | O_CREAT);
	ObjectDeleter<BFile> tarIoDeleter(file);
	status_t result = file->WriteExactly(kSampleTarPayload, kSampleTarLength);

	if (result != B_OK) {
		printf("! unable to write the sample data to [%s]\n", path.String());
	}

	return result;
}


status_t
TarArchiveServiceTest::DeleteSampleFile(const BString& path)
{
	if (remove(path.String()) != 0)
		return B_ERROR;
	return B_OK;
}


/*static*/ void
TarArchiveServiceTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("TarArchiveServiceTest");

	suite.addTest(
		new CppUnit::TestCaller<TarArchiveServiceTest>(
			"TarArchiveServiceTest::TestReadHeader",
			&TarArchiveServiceTest::TestReadHeader));

	parent.addTest("TarArchiveServiceTest", &suite);
}