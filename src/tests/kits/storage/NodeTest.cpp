// NodeTest.cpp

#include <cppunit/TestCase.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <TestUtils.h>

#include <errno.h>
#include <fs_attr.h>	// For struct attr_info
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>	// For struct stat

#include <Directory.h>
#include <Entry.h>
#include <Node.h>
#include <StorageDefs.h>
#include <String.h>
#include <TypeConstants.h>

#include "NodeTest.h"


// == for attr_info
static
inline
bool
operator==(const attr_info &info1, const attr_info &info2)
{
	return (info1.type == info2.type && info1.size == info2.size);
}

// Suite
CppUnit::Test*
NodeTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();

	StatableTest::AddBaseClassTests<NodeTest>("BNode::", suite);

	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Init Test1", &NodeTest::InitTest1) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Init Test2", &NodeTest::InitTest2) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Attribute Directory Test", &NodeTest::AttrDirTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Attribute Read/Write/Remove Test", &NodeTest::AttrTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Attribute Rename Test"
#if TEST_R5
														" (NOTE: test not actually performed with R5 libraries)"
#endif
														, &NodeTest::AttrRenameTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Attribute Info Test", &NodeTest::AttrInfoTest) );
	// TODO: AttrBString deadlocks entire OS (UnitTester at 100% CPU,
	// windows don't respond to actions, won't open, OS won't even shut down)
	//suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Attribute BString Test", &NodeTest::AttrBStringTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Sync Test", &NodeTest::SyncTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Dup Test", &NodeTest::DupTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Equality Test", &NodeTest::EqualityTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Assignment Test", &NodeTest::AssignmentTest) );
	suite->addTest( new CppUnit::TestCaller<NodeTest>("BNode::Lock Test"
														, &NodeTest::LockTest) );
		
	return suite;
}		

// ConvertTestNodesToStatables
static
void
ConvertTestStatablesToNodes(TestNodes& testNodes, TestStatables& testStatables)
{
	BNode *node;
	string entryName;
	for (testNodes.rewind(); testNodes.getNext(node, entryName); )
		testStatables.add(node, entryName);
	testNodes.clear();	// avoid deletion
}

// CreateROStatables
void
NodeTest::CreateROStatables(TestStatables& testEntries)
{
	TestNodes testNodes;
	CreateRONodes(testNodes);
	ConvertTestStatablesToNodes(testNodes, testEntries);
}

// CreateRWStatables
void
NodeTest::CreateRWStatables(TestStatables& testEntries)
{
	TestNodes testNodes;
	CreateRWNodes(testNodes);
	ConvertTestStatablesToNodes(testNodes, testEntries);
}

// CreateUninitializedStatables
void
NodeTest::CreateUninitializedStatables(TestStatables& testEntries)
{
	TestNodes testNodes;
	CreateUninitializedNodes(testNodes);
	ConvertTestStatablesToNodes(testNodes, testEntries);
}

// CreateRONodes
void
NodeTest::CreateRONodes(TestNodes& testEntries)
{
	const char *filename;
	filename = "/tmp";
	testEntries.add(new BNode(filename), filename);
	filename = "/";
	testEntries.add(new BNode(filename), filename);
	filename = "/boot";
	testEntries.add(new BNode(filename), filename);
	filename = "/boot/home";
	testEntries.add(new BNode(filename), filename);
	filename = "/boot/home/Desktop";
	testEntries.add(new BNode(filename), filename);
	filename = existingFilename;
	testEntries.add(new BNode(filename), filename);
	filename = dirLinkname;
	testEntries.add(new BNode(filename), filename);
	filename = fileLinkname;
	testEntries.add(new BNode(filename), filename);
}

// CreateRWNodes
void
NodeTest::CreateRWNodes(TestNodes& testEntries)
{
	const char *filename;
	filename = existingFilename;
	testEntries.add(new BNode(filename), filename);
	filename = existingDirname;
	testEntries.add(new BNode(filename), filename);
	filename = existingSubDirname;
	testEntries.add(new BNode(filename), filename);
	filename = dirLinkname;
	testEntries.add(new BNode(filename), filename);
	filename = fileLinkname;
	testEntries.add(new BNode(filename), filename);
	filename = relDirLinkname;
	testEntries.add(new BNode(filename), filename);
	filename = relFileLinkname;
	testEntries.add(new BNode(filename), filename);
	filename = cyclicLinkname1;
	testEntries.add(new BNode(filename), filename);
}

// CreateUninitializedNodes
void
NodeTest::CreateUninitializedNodes(TestNodes& testEntries)
{
	testEntries.add(new BNode, "");
}

// setUp
void
NodeTest::setUp()
{
	StatableTest::setUp();
	execCommand(
		string("touch ") + existingFilename
		+ "; mkdir " + existingDirname
		+ "; mkdir " + existingSubDirname
		+ "; ln -s " + existingDirname + " " + dirLinkname
		+ "; ln -s " + existingFilename + " " + fileLinkname
		+ "; ln -s " + existingRelDirname + " " + relDirLinkname
		+ "; ln -s " + existingRelFilename + " " + relFileLinkname
		+ "; ln -s " + nonExistingDirname + " " + badLinkname
		+ "; ln -s " + cyclicLinkname1 + " " + cyclicLinkname2
		+ "; ln -s " + cyclicLinkname2 + " " + cyclicLinkname1
	);
}

// tearDown
void
NodeTest::tearDown()
{
	StatableTest::tearDown();
	// cleanup
	string cmdLine("rm -rf ");
	for (int32 i = 0; i < allFilenameCount; i++)
		cmdLine += string(" ") + allFilenames[i];
	if (allFilenameCount > 0)
		execCommand(cmdLine);
}

// InitTest1
void
NodeTest::InitTest1()
{
	const char *dirLink = dirLinkname;
	const char *dirSuperLink = dirSuperLinkname;
	const char *dirRelLink = dirRelLinkname;
	const char *fileLink = fileLinkname;
	const char *existingDir = existingDirname;
	const char *existingSuperDir = existingSuperDirname;
	const char *existingRelDir = existingRelDirname;
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	// 1. default constructor
	NextSubTest();
	{
		BNode node;
		CPPUNIT_ASSERT( node.InitCheck() == B_NO_INIT );
	}

	// 2. BNode(const char*)
	NextSubTest();
	{
		BNode node(fileLink);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BNode node(nonExisting);
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	NextSubTest();
	{
		BNode node((const char *)NULL);
		CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	}
	NextSubTest();
	{
		BNode node("");
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	NextSubTest();
	{
		BNode node(existingFile);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BNode node(existingDir);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BNode node(tooLongEntryname);
		CPPUNIT_ASSERT( node.InitCheck() == B_NAME_TOO_LONG );
	}

	// 3. BNode(const BEntry*)
	NextSubTest();
	{
		BEntry entry(dirLink);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BNode node(&entry);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BNode node(&entry);
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	NextSubTest();
	{
		BNode node((BEntry *)NULL);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BEntry entry;
		BNode node(&entry);
		CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}
	NextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BNode node(&entry);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );

	}
	NextSubTest();
	{
		BEntry entry(existingDir);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		BNode node(&entry);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );

	}
	NextSubTest();
	{
		BEntry entry(tooLongEntryname);
		// R5 returns E2BIG instead of B_NAME_TOO_LONG
		CPPUNIT_ASSERT( equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG) );
		BNode node(&entry);
		CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	}

	// 4. BNode(const entry_ref*)
	NextSubTest();
	{
		BEntry entry(dirLink);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BNode node(&ref);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BEntry entry(nonExisting);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BNode node(&ref);
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	NextSubTest();
	{
		BNode node((entry_ref *)NULL);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BEntry entry(existingFile);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BNode node(&ref);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BEntry entry(existingDir);
		CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
		entry_ref ref;
		CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
		BNode node(&ref);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}

	// 5. BNode(const BDirectory*, const char*)
	NextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, dirRelLink);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, dirLink);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BDirectory pathDir(nonExistingSuper);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, nonExistingRel);
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
	NextSubTest();
	{
		BNode node((BDirectory *)NULL, (const char *)NULL);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BNode node((BDirectory *)NULL, dirLink);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, (const char *)NULL);
		CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	}
	NextSubTest();
	{
		BDirectory pathDir(dirSuperLink);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, "");
		CPPUNIT_ASSERT(node.InitCheck() == B_ENTRY_NOT_FOUND);
	}
	NextSubTest();
	{
		BDirectory pathDir(existingSuperFile);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, existingRelFile);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BDirectory pathDir(existingSuperDir);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, existingRelDir);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	}
	NextSubTest();
	{
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, tooLongRelEntryname);
		CPPUNIT_ASSERT( node.InitCheck() == B_NAME_TOO_LONG );
	}
	NextSubTest();
	{
		BDirectory pathDir(fileSuperDirname);
		CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
		BNode node(&pathDir, fileRelDirname);
		CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	}
}

// InitTest2
void
NodeTest::InitTest2()
{
	const char *dirLink = dirLinkname;
	const char *dirSuperLink = dirSuperLinkname;
	const char *dirRelLink = dirRelLinkname;
	const char *fileLink = fileLinkname;
	const char *existingDir = existingDirname;
	const char *existingSuperDir = existingSuperDirname;
	const char *existingRelDir = existingRelDirname;
	const char *existingFile = existingFilename;
	const char *existingSuperFile = existingSuperFilename;
	const char *existingRelFile = existingRelFilename;
	const char *nonExisting = nonExistingDirname;
	const char *nonExistingSuper = nonExistingSuperDirname;
	const char *nonExistingRel = nonExistingRelDirname;
	BNode node;
	// 2. BNode(const char*)
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo(fileLink) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo(nonExisting) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	NextSubTest();
	CPPUNIT_ASSERT( equals(node.SetTo((const char *)NULL), B_BAD_VALUE,
						   B_NO_INIT) );
	CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_VALUE, B_NO_INIT) );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo("") == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo(tooLongEntryname) == B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( node.InitCheck() == B_NAME_TOO_LONG );

	// 3. BNode(const BEntry*)
	NextSubTest();
	BEntry entry(dirLink);
	CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&entry) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo((BEntry *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	entry.Unset();
	CPPUNIT_ASSERT( entry.InitCheck() == B_NO_INIT );
	CPPUNIT_ASSERT( equals(node.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&entry) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	// R5 returns E2BIG instead of B_NAME_TOO_LONG
	CPPUNIT_ASSERT( equals(entry.SetTo(tooLongEntryname), E2BIG, B_NAME_TOO_LONG) );
	CPPUNIT_ASSERT( equals(node.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE) );

	// 4. BNode(const entry_ref*)
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(dirLink) == B_OK );
	entry_ref ref;
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(nonExisting) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&ref) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo((entry_ref *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingFile) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( entry.SetTo(existingDir) == B_OK );
	CPPUNIT_ASSERT( entry.GetRef(&ref) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&ref) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );

	// 5. BNode(const BDirectory*, const char*)
	NextSubTest();
	BDirectory pathDir(dirSuperLink);
	CPPUNIT_ASSERT( pathDir.InitCheck() == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, dirRelLink) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, dirLink) == B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(nonExistingSuper) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, nonExistingRel) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo((BDirectory *)NULL, (const char *)NULL)
					== B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	CPPUNIT_ASSERT( node.SetTo((BDirectory *)NULL, dirLink) == B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, (const char *)NULL) == B_BAD_VALUE );
	CPPUNIT_ASSERT( node.InitCheck() == B_BAD_VALUE );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(dirSuperLink) == B_OK );
	CPPUNIT_ASSERT(node.SetTo(&pathDir, "") == B_ENTRY_NOT_FOUND);
	CPPUNIT_ASSERT(node.InitCheck() == B_ENTRY_NOT_FOUND);
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(existingSuperFile) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, existingRelFile) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(existingSuperDir) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, existingRelDir) == B_OK );
	CPPUNIT_ASSERT( node.InitCheck() == B_OK );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(tooLongSuperEntryname) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, tooLongRelEntryname) == B_NAME_TOO_LONG );
	CPPUNIT_ASSERT( node.InitCheck() == B_NAME_TOO_LONG );
	//
	NextSubTest();
	CPPUNIT_ASSERT( pathDir.SetTo(fileSuperDirname) == B_OK );
	CPPUNIT_ASSERT( node.SetTo(&pathDir, fileRelDirname) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.InitCheck() == B_ENTRY_NOT_FOUND );
}

// WriteAttributes
static
void
WriteAttributes(BNode &node, const char **attrNames, const char **attrValues,
				int32 attrCount)
{
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = attrValues[i];
		int32 valueSize = strlen(attrValue) + 1;
		CPPUNIT_ASSERT( node.WriteAttr(attrName, B_STRING_TYPE, 0, attrValue,
									   valueSize) == valueSize );
	}
}

// AttrDirTest
void
NodeTest::AttrDirTest(BNode &node)
{
	// node should not have any attributes at the beginning
	char nameBuffer[B_ATTR_NAME_LENGTH];
	CPPUNIT_ASSERT( node.GetNextAttrName(nameBuffer) == B_ENTRY_NOT_FOUND );
	// add some
	const char *attrNames[] = {
		"attr1", "attr2", "attr3", "attr4", "attr5"
	};
	const char *attrValues[] = {
		"value1", "value2", "value3", "value4", "value5"
	};
	int32 attrCount = sizeof(attrNames) / sizeof(const char *);
	WriteAttributes(node, attrNames, attrValues, attrCount);
	TestSet testSet;
	for (int32 i = 0; i < attrCount; i++)
		testSet.add(attrNames[i]);
	// get all attribute names
	// R5: We have to rewind, we wouldn't get any attribute otherwise.
	CPPUNIT_ASSERT( node.RewindAttrs() == B_OK );
	while (node.GetNextAttrName(nameBuffer) == B_OK)
		CPPUNIT_ASSERT( testSet.test(nameBuffer) == true );
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( node.GetNextAttrName(nameBuffer) == B_ENTRY_NOT_FOUND );
	// rewind, get one attribute, rewind again and iterate through the whole
	// list again
	CPPUNIT_ASSERT( node.RewindAttrs() == B_OK );
	testSet.rewind();
	CPPUNIT_ASSERT( node.GetNextAttrName(nameBuffer) == B_OK );
	CPPUNIT_ASSERT( testSet.test(nameBuffer) == true );
	CPPUNIT_ASSERT( node.RewindAttrs() == B_OK );
	testSet.rewind();
	while (node.GetNextAttrName(nameBuffer) == B_OK)
		CPPUNIT_ASSERT( testSet.test(nameBuffer) == true );
	CPPUNIT_ASSERT( testSet.testDone() == true );
	CPPUNIT_ASSERT( node.GetNextAttrName(nameBuffer) == B_ENTRY_NOT_FOUND );
	// bad args
	CPPUNIT_ASSERT( node.RewindAttrs() == B_OK );
	testSet.rewind();
// R5: crashs, if passing a NULL buffer
#if !TEST_R5
	CPPUNIT_ASSERT( node.GetNextAttrName(NULL) == B_BAD_VALUE );
#endif
}

// AttrDirTest
void
NodeTest::AttrDirTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		char nameBuffer[B_ATTR_NAME_LENGTH];
		CPPUNIT_ASSERT( node->RewindAttrs() != B_OK );
		CPPUNIT_ASSERT( node->GetNextAttrName(nameBuffer) != B_OK );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		AttrDirTest(*node);
	}
	testEntries.delete_all();
}
	
// AttrTest
void
NodeTest::AttrTest(BNode &node)
{
	// add some attributes
	const char *attrNames[] = {
		"attr1", "attr2", "attr3", "attr4", "attr5"
	};
	const char *attrValues[] = {
		"value1", "value2", "value3", "value4", "value5"
	};
	const char *newAttrValues[] = {
		"fd", "kkgkjsdhfgkjhsd", "lihuhuh", "", "alkfgnakdfjgn"
	};
	int32 attrCount = sizeof(attrNames) / sizeof(const char *);
	WriteAttributes(node, attrNames, attrValues, attrCount);
	char buffer[1024];
	// read and check them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = attrValues[i];
		int32 valueSize = strlen(attrValue) + 1;
		CPPUNIT_ASSERT( node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
									  sizeof(buffer)) == valueSize );
		CPPUNIT_ASSERT( strcmp(buffer, attrValue) == 0 );
	}
	// write a new value for each attribute
	WriteAttributes(node, attrNames, newAttrValues, attrCount);
	// read and check them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = newAttrValues[i];
		int32 valueSize = strlen(attrValue) + 1;
		CPPUNIT_ASSERT( node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
									  sizeof(buffer)) == valueSize );
		CPPUNIT_ASSERT( strcmp(buffer, attrValue) == 0 );
	}
	// bad args
	CPPUNIT_ASSERT( equals(node.ReadAttr(NULL, B_STRING_TYPE, 0, buffer,
										 sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.ReadAttr(attrNames[0], B_STRING_TYPE, 0, NULL,
										 sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.ReadAttr(NULL, B_STRING_TYPE, 0, NULL,
										 sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.WriteAttr(NULL, B_STRING_TYPE, 0, buffer,
										  sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.WriteAttr(attrNames[0], B_STRING_TYPE, 0, NULL,
										  sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.WriteAttr(NULL, B_STRING_TYPE, 0, NULL,
										  sizeof(buffer)),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.RemoveAttr(NULL), B_BAD_ADDRESS, B_BAD_VALUE) );
	// too long attribute name
	// R5: Read/RemoveAttr() do not return B_NAME_TOO_LONG, but B_ENTRY_NOT_FOUND
	// R5: WriteAttr() does not return B_NAME_TOO_LONG, but B_BAD_VALUE
	// R5: Haiku has a max attribute size of 256, while R5's was 255, exclusive
	//     of the null terminator. See changeset 4069e1f30.
	char tooLongAttrName[B_ATTR_NAME_LENGTH + 3];
	memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH + 1);
	tooLongAttrName[B_ATTR_NAME_LENGTH + 2] = '\0';
	CPPUNIT_ASSERT_EQUAL(
		node.WriteAttr(tooLongAttrName, B_STRING_TYPE, 0, buffer,
			sizeof(buffer)),
		B_NAME_TOO_LONG);
	CPPUNIT_ASSERT_EQUAL(
		node.ReadAttr(tooLongAttrName, B_STRING_TYPE, 0, buffer,
			sizeof(buffer)),
		B_NAME_TOO_LONG);
	CPPUNIT_ASSERT_EQUAL(node.RemoveAttr(tooLongAttrName), B_NAME_TOO_LONG);

	// remove the attributes and try to read them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		CPPUNIT_ASSERT( node.RemoveAttr(attrName) == B_OK );
		CPPUNIT_ASSERT( node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
									  sizeof(buffer)) == B_ENTRY_NOT_FOUND );
	}
	// try to remove a non-existing attribute
	CPPUNIT_ASSERT( node.RemoveAttr("non existing attribute")
					== B_ENTRY_NOT_FOUND );
}

// AttrTest
void
NodeTest::AttrTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		char buffer[1024];
		CPPUNIT_ASSERT( node->ReadAttr("attr1", B_STRING_TYPE, 0, buffer,
									   sizeof(buffer)) == B_FILE_ERROR );
		CPPUNIT_ASSERT( node->WriteAttr("attr1", B_STRING_TYPE, 0, buffer,
										sizeof(buffer)) == B_FILE_ERROR );
		CPPUNIT_ASSERT( node->RemoveAttr("attr1") == B_FILE_ERROR );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		AttrTest(*node);
	}
	testEntries.delete_all();
}

// AttrRenameTest
void
NodeTest::AttrRenameTest(BNode &node)
{
	const char attr1[] = "StorageKit::SomeAttribute";
	const char attr2[] = "StorageKit::AnotherAttribute";

	CPPUNIT_ASSERT( node.SetTo("./") == B_OK );

	// Test the case of the first attribute not existing
	node.RemoveAttr(attr1);

#if 1
	// The actual tests in the else block below are disabled because as of
	// right now, BFS doesn't support attribute rename. bfs_rename_attr()
	// always reutrns B_NOT_SUPPORTED, which means BNode::RenameAttr() will
	// also always return that result.
	//
	// So until that is implemented, we'll just test for B_NOT_SUPPORTED here.
	// Once that functionality is implemented, this test will pass and someone
	// can remove this section.
	CPPUNIT_ASSERT_EQUAL(node.RenameAttr(attr1, attr2), B_NOT_SUPPORTED);
#else
	const char str[] = "This is my testing string and it rules your world.";
	const int strLen = strlen(str) + 1;
	const int dataLen = 1024;
	char data[dataLen];

	CPPUNIT_ASSERT( node.RenameAttr(attr1, attr2) == B_BAD_VALUE );

	// Write an attribute, read it to verify it, rename it, read the
	// new attribute, read the old (which fails), and then remove the new.
	CPPUNIT_ASSERT( node.WriteAttr(attr1, B_STRING_TYPE, 0, str, strLen) == strLen );
	CPPUNIT_ASSERT( node.ReadAttr(attr1, B_STRING_TYPE, 0, data, dataLen) == strLen );
	CPPUNIT_ASSERT( strcmp(data, str) == 0 );		
	CPPUNIT_ASSERT( node.RenameAttr(attr1, attr2) == B_OK ); // <<< This fails with R5::BNode
	CPPUNIT_ASSERT( node.ReadAttr(attr1, B_STRING_TYPE, 0, data, dataLen) == B_ENTRY_NOT_FOUND );
	CPPUNIT_ASSERT( node.ReadAttr(attr2, B_STRING_TYPE, 0, data, dataLen) == strLen );
	CPPUNIT_ASSERT( strcmp(data, str) == 0 );
	CPPUNIT_ASSERT( node.RemoveAttr(attr2) == B_OK );

	// bad args
	CPPUNIT_ASSERT( equals(node.RenameAttr(attr1, NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.RenameAttr(NULL, attr2), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.RenameAttr(NULL, NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	// too long attribute name
// R5: RenameAttr() returns B_BAD_VALUE instead of B_NAME_TOO_LONG
	char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
	memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
	tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
	CPPUNIT_ASSERT( node.RenameAttr(attr1, tooLongAttrName)
					== B_BAD_VALUE );
	CPPUNIT_ASSERT( node.RenameAttr(tooLongAttrName, attr1)
					== B_BAD_VALUE );
#endif
}

	
// AttrRenameTest
void
NodeTest::AttrRenameTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		CPPUNIT_ASSERT( node->RenameAttr("attr1", "attr2") == B_FILE_ERROR );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		AttrRenameTest(*node);
	}
	testEntries.delete_all();
}

// AttrInfoTest
void
NodeTest::AttrInfoTest(BNode &node)
{
	// add some attributes
	const char *attrNames[] = {
		"attr1", "attr2", "attr3", "attr4", "attr5"
	};
	const int32 attrCount = sizeof(attrNames) / sizeof(const char*);
	const char attrValue1[] = "This is the greatest string ever.";
	int32 attrValue2 = 17;
	uint64 attrValue3 = 42;
	double attrValue4 = 435.5;
	struct flat_entry_ref { dev_t device; ino_t directory; char name[256]; }
		attrValue5 = { 9, 16, "Hello world!" };
	const void *attrValues[] = {
		attrValue1, &attrValue2, &attrValue3, &attrValue4, &attrValue5
	};
	attr_info attrInfos[] = {
		{ B_STRING_TYPE, sizeof(attrValue1) },
		{ B_INT32_TYPE, sizeof(attrValue2) },
		{ B_UINT64_TYPE, sizeof(attrValue3) },
		{ B_DOUBLE_TYPE, sizeof(attrValue4) },
		{ B_REF_TYPE, sizeof(attrValue5) }
	};
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const void *attrValue = attrValues[i];
		int32 valueSize = attrInfos[i].size;
		uint32 attrType = attrInfos[i].type;
		CPPUNIT_ASSERT( node.WriteAttr(attrName, attrType, 0, attrValue,
									   valueSize) == valueSize );
	}
	// get the attribute infos
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		attr_info info;
		CPPUNIT_ASSERT( node.GetAttrInfo(attrName, &info) == B_OK );
		CPPUNIT_ASSERT( info == attrInfos[i] );
	}
	// try get an info for a non-existing attribute
	attr_info info;
	CPPUNIT_ASSERT( node.GetAttrInfo("non-existing attribute", &info)
					== B_ENTRY_NOT_FOUND );
	// bad values
	CPPUNIT_ASSERT( equals(node.GetAttrInfo(NULL, &info), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.GetAttrInfo(attrNames[0], NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.GetAttrInfo(NULL, NULL), B_BAD_ADDRESS,
						   B_BAD_VALUE) );
	// too long attribute name
// R5: GetAttrInfo() does not return B_NAME_TOO_LONG
	char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
	memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
	tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
	CPPUNIT_ASSERT( node.GetAttrInfo(tooLongAttrName, &info)
					== B_ENTRY_NOT_FOUND );
}

// AttrInfoTest
void
NodeTest::AttrInfoTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		attr_info info;
		CPPUNIT_ASSERT( node->GetAttrInfo("attr1", &info) == B_FILE_ERROR );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		AttrInfoTest(*node);
	}
	testEntries.delete_all();
}

// AttrBStringTest
void
NodeTest::AttrBStringTest(BNode &node)
{
	// add some attributes
	const char *attrNames[] = {
		"attr1", "attr2", "attr3", "attr4", "attr5"
	};
	const char *attrValues[] = {
		"value1", "value2", "value3", "value4", "value5"
	};
	const char *newAttrValues[] = {
		"fd", "kkgkjsdhfgkjhsd", "lihuhuh", "", "alkfgnakdfjgn"
	};
	int32 attrCount = sizeof(attrNames) / sizeof(const char *);
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		BString attrValue(attrValues[i]);
		CPPUNIT_ASSERT( node.WriteAttrString(attrName, &attrValue) == B_OK );
	}
	// read and check them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = attrValues[i];
		BString readValue;
		CPPUNIT_ASSERT( node.ReadAttrString(attrName, &readValue) == B_OK );
		CPPUNIT_ASSERT( readValue == attrValue );
	}
	// write a new value for each attribute
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		BString attrValue(newAttrValues[i]);
		CPPUNIT_ASSERT( node.WriteAttrString(attrName, &attrValue) == B_OK );
	}
	// read and check them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = newAttrValues[i];
		BString readValue;
		CPPUNIT_ASSERT( node.ReadAttrString(attrName, &readValue) == B_OK );
		CPPUNIT_ASSERT( readValue == attrValue );
	}
	// bad args
	BString readValue;
	BString writeValue("test");
// R5: crashes, if supplying a NULL BString
#if !TEST_R5
	CPPUNIT_ASSERT( node.WriteAttrString(attrNames[0], NULL) == B_BAD_VALUE );		
	CPPUNIT_ASSERT( node.ReadAttrString(attrNames[0], NULL) == B_BAD_VALUE );
#endif
	CPPUNIT_ASSERT( equals(node.WriteAttrString(NULL, &writeValue),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	CPPUNIT_ASSERT( equals(node.ReadAttrString(NULL, &readValue),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
#if !TEST_R5
	CPPUNIT_ASSERT( node.WriteAttrString(NULL, NULL) == B_BAD_VALUE );
#endif
	CPPUNIT_ASSERT( equals(node.ReadAttrString(NULL, NULL),
						   B_BAD_ADDRESS, B_BAD_VALUE) );
	// remove the attributes and try to read them
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		CPPUNIT_ASSERT( node.RemoveAttr(attrName) == B_OK );
		CPPUNIT_ASSERT( node.ReadAttrString(attrName, &readValue)
						== B_ENTRY_NOT_FOUND );
	}
	// too long attribute name
// R5: Read/WriteAttrString() do not return B_NAME_TOO_LONG 
	char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
	memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
	tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
	CPPUNIT_ASSERT( node.WriteAttrString(tooLongAttrName, &writeValue)
					== B_BAD_VALUE );
	CPPUNIT_ASSERT( node.ReadAttrString(tooLongAttrName, &readValue)
					== B_ENTRY_NOT_FOUND );
}
	
// AttrBStringTest
void
NodeTest::AttrBStringTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		BString value("test");
		CPPUNIT_ASSERT( node->WriteAttrString("attr1", &value)
						== B_FILE_ERROR );
		CPPUNIT_ASSERT( node->ReadAttrString("attr1", &value)
						== B_FILE_ERROR );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		AttrBStringTest(*node);
	}
	testEntries.delete_all();
}

// This doesn't actually verify synching is occuring; just
// checks for a B_OK return value.
void
NodeTest::SyncTest() {
	const char attr[] = "StorageKit::SomeAttribute";
	const char str[] = "This string rules your world.";
	const int len = strlen(str) + 1;
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		CPPUNIT_ASSERT( node->Sync() == B_FILE_ERROR );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		CPPUNIT_ASSERT( node->WriteAttr(attr, B_STRING_TYPE, 0, str, len)
						== len );
		CPPUNIT_ASSERT( node->Sync() == B_OK );
	}
	testEntries.delete_all();
}

// DupTest	
void
NodeTest::DupTest(BNode &node)
{
	int fd = node.Dup();
	CPPUNIT_ASSERT( fd != -1 );
	::close(fd);
}

// DupTest	
void
NodeTest::DupTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		CPPUNIT_ASSERT( node->Dup() == -1 );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		DupTest(*node);
	}
	testEntries.delete_all();
}

// n1 and n2 should both be uninitialized. y1a and y1b should be initialized
// to the same node, y2 should be initialized to a different node
void
NodeTest::EqualityTest(BNode &n1, BNode &n2, BNode &y1a, BNode &y1b, BNode &y2) {
	CPPUNIT_ASSERT( n1 == n2 );
	CPPUNIT_ASSERT( !(n1 != n2) );
	CPPUNIT_ASSERT( n1 != y2 );
	CPPUNIT_ASSERT( !(n1 == y2) );

	CPPUNIT_ASSERT( y1a != n2 );
	CPPUNIT_ASSERT( !(y1a == n2) );
	CPPUNIT_ASSERT( y1a == y1b );
	CPPUNIT_ASSERT( !(y1a != y1b) );
	CPPUNIT_ASSERT( y1a != y2 );
	CPPUNIT_ASSERT( !(y1a == y2) );

	CPPUNIT_ASSERT( n1 == n1 );
	CPPUNIT_ASSERT( !(n1 != n1) );
	CPPUNIT_ASSERT( y2 == y2 );
	CPPUNIT_ASSERT( !(y2 != y2) );			
}

// EqualityTest
void
NodeTest::EqualityTest()
{
	BNode n1, n2, y1a("/boot"), y1b("/boot"), y2("/");
		
	EqualityTest(n1, n2, y1a, y1b, y2);		
}

// AssignmentTest
void
NodeTest::AssignmentTest()
{	
	BNode n1, n2, y1a("/boot"), y1b("/boot"), y2("/");

	n1 = n1;		// self n
	y1a = y1b;		// psuedo self y
	y1a = y1a;		// self y
	n2 = y2;		// n = y
	y1b = n1;		// y = n
	y2 = y1a;		// y1 = y2
		
	EqualityTest(n1, y1b, y1a, y2, n2);
}
	
// Locking isn't really implemented yet...
void
NodeTest::LockTest(BNode &node, const char *entryName)
{
	CPPUNIT_ASSERT( node.Lock() == B_OK );
	BNode node2(entryName);
	CPPUNIT_ASSERT( node2.InitCheck() == B_BUSY );
	CPPUNIT_ASSERT( node.Unlock() == B_OK );
	CPPUNIT_ASSERT( node.Unlock() == B_BAD_VALUE );
	CPPUNIT_ASSERT( node2.SetTo(entryName) == B_OK );
// R5: Since two file descriptors exist at this point, locking is supposed
// to fail according to the BeBook, but it succeeds!
	CPPUNIT_ASSERT( node2.Lock() == B_OK );
	CPPUNIT_ASSERT( node.Lock() == B_BUSY );
//
	CPPUNIT_ASSERT( node2.Unlock() == B_OK );
}

// Locking isn't really implemented yet...
void
NodeTest::LockTest()
{
	// uninitialized objects
	NextSubTest();
	TestNodes testEntries;
	CreateUninitializedNodes(testEntries);
	BNode *node;
	string nodeName;
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		CPPUNIT_ASSERT( node->Dup() == -1 );
	}
	testEntries.delete_all();
	// existing entries
	NextSubTest();
	CreateRWNodes(testEntries);
	for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
		LockTest(*node, nodeName.c_str());
	}
	testEntries.delete_all();
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

