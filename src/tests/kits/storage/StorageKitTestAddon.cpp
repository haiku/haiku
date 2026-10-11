/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "NodeTest.h"

const char*
getTestSuiteName()
{
	return "StorageKit";
}

// entry names used in tests
const char *NodeTest::existingFilename			= "/tmp/existing-file";
const char *NodeTest::existingSuperFilename		= "/tmp";
const char *NodeTest::existingRelFilename		= "existing-file";
const char *NodeTest::existingDirname			= "/tmp/existing-dir";
const char *NodeTest::existingSuperDirname		= "/tmp";
const char *NodeTest::existingRelDirname		= "existing-dir";
const char *NodeTest::existingSubDirname
	= "/tmp/existing-dir/existing-subdir";
const char *NodeTest::existingRelSubDirname		= "existing-subdir";
const char *NodeTest::nonExistingFilename		= "/tmp/non-existing-file";
const char *NodeTest::nonExistingDirname		= "/tmp/non-existing-dir";
const char *NodeTest::nonExistingSuperDirname	= "/tmp";
const char *NodeTest::nonExistingRelDirname		= "non-existing-dir";
const char *NodeTest::testFilename1				= "/tmp/test-file1";
const char *NodeTest::testDirname1				= "/tmp/test-dir1";
const char *NodeTest::tooLongEntryname			=
	"/tmp/This is an awfully long name for an entry. It is that kind of entry "
	"that just can't exist due to its long name. In fact its path name is not "
	"too long -- a path name can contain 1024 characters -- but the name of "
	"the entry itself is restricted to 256 characters, which this entry's "
	"name does exceed.";
const char *NodeTest::tooLongSuperEntryname		= "/tmp";
const char *NodeTest::tooLongRelEntryname		=
	"This is an awfully long name for an entry. It is that kind of entry "
	"that just can't exist due to its long name. In fact its path name is not "
	"too long -- a path name can contain 1024 characters -- but the name of "
	"the entry itself is restricted to 256 characters, which this entry's "
	"name does exceed.";
const char *NodeTest::fileDirname				= "/tmp/test-file1/some-dir";
const char *NodeTest::fileSuperDirname			= "/tmp";
const char *NodeTest::fileRelDirname			= "test-file1/some-dir";
const char *NodeTest::dirLinkname				= "/tmp/link-to-dir1";
const char *NodeTest::dirSuperLinkname			= "/tmp";
const char *NodeTest::dirRelLinkname			= "link-to-dir1";
const char *NodeTest::fileLinkname				= "/tmp/link-to-file1";
const char *NodeTest::fileSuperLinkname			= "/tmp";
const char *NodeTest::fileRelLinkname			= "link-to-file1";
const char *NodeTest::relDirLinkname			= "/tmp/rel-link-to-dir1";
const char *NodeTest::relFileLinkname			= "/tmp/rel-link-to-file1";
const char *NodeTest::badLinkname				= "/tmp/link-to-void";
const char *NodeTest::cyclicLinkname1			= "/tmp/cyclic-link1";
const char *NodeTest::cyclicLinkname2			= "/tmp/cyclic-link2";

const char *NodeTest::allFilenames[] = {
	existingFilename,
	existingDirname,
	nonExistingFilename,
	nonExistingDirname,
	testFilename1,
	testDirname1,
	dirLinkname,
	fileLinkname,
	relDirLinkname,
	relFileLinkname,
	badLinkname,
	cyclicLinkname1,
	cyclicLinkname2,
};
const int32 NodeTest::allFilenameCount
	= sizeof(allFilenames) / sizeof(const char*);