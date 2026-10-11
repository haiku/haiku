/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _STORAGE_NODE_TEST_H_
#define _STORAGE_NODE_TEST_H_


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

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "StatableTest.h"


class BNode;

typedef TestEntries<BNode> TestNodes;

static
inline
bool
operator==(const attr_info &info1, const attr_info &info2)
{
	return (info1.type == info2.type && info1.size == info2.size);
}

//

static
void
ConvertTestStatablesToNodes(TestNodes& testNodes, TestStatables& testStatables)
{
	BNode *node;
	std::string entryName;
	for (testNodes.rewind(); testNodes.getNext(node, entryName); )
		testStatables.add(node, entryName);
	testNodes.clear();	// avoid deletion
}

static
void
WriteAttributes(BNode &node, const char **attrNames, const char **attrValues,
				int32 attrCount)
{
	for (int32 i = 0; i < attrCount; i++) {
		const char *attrName = attrNames[i];
		const char *attrValue = attrValues[i];
		int32 valueSize = strlen(attrValue) + 1;
		CPPUNIT_ASSERT(node.WriteAttr(attrName, B_STRING_TYPE, 0, attrValue,
									   valueSize) == valueSize);
	}
}

class NodeTest : public StatableTest {
	CPPUNIT_TEST_SUB_SUITE(NodeTest, StatableTest);
	CPPUNIT_TEST(Uninitialized_Lock_ReturnsNoInit);
	CPPUNIT_TEST(CyclicLink_Lock_ReturnsExpected);
	CPPUNIT_TEST(RelativeFileLink_Lock_ReturnsExpected);
	CPPUNIT_TEST(RelativeDirLink_Lock_ReturnsExpected);
	CPPUNIT_TEST(FileLink_Lock_ReturnsExpected);
	CPPUNIT_TEST(DirLink_Lock_ReturnsExpected);
	CPPUNIT_TEST(SubDir_Lock_ReturnsExpected);
	CPPUNIT_TEST(Dir_Lock_ReturnsExpected);
	CPPUNIT_TEST(File_Lock_ReturnsExpected);
	CPPUNIT_TEST(Basic_Assignment_ReturnsExpected);
	CPPUNIT_TEST(Basic_Equality_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Dup_ReturnsError);
	CPPUNIT_TEST(CyclicLink_Dup_ReturnsExpected);
	CPPUNIT_TEST(RelativeFileLink_Dup_ReturnsExpected);
	CPPUNIT_TEST(RelativeDirLink_Dup_ReturnsExpected);
	CPPUNIT_TEST(FileLink_Dup_ReturnsExpected);
	CPPUNIT_TEST(DirLink_Dup_ReturnsExpected);
	CPPUNIT_TEST(SubDir_Dup_ReturnsExpected);
	CPPUNIT_TEST(Dir_Dup_ReturnsExpected);
	CPPUNIT_TEST(File_Dup_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Sync_ReturnsError);
	CPPUNIT_TEST(CyclicLink_Sync_ReturnsOk);
	CPPUNIT_TEST(RelativeFileLink_Sync_ReturnsOk);
	CPPUNIT_TEST(RelativeDirLink_Sync_ReturnsOk);
	CPPUNIT_TEST(FileLink_Sync_ReturnsOk);
	CPPUNIT_TEST(DirLink_Sync_ReturnsOk);
	CPPUNIT_TEST(SubDir_Sync_ReturnsOk);
	CPPUNIT_TEST(Dir_Sync_ReturnsOk);
	CPPUNIT_TEST(File_Sync_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_AttrBString_ReturnsError);
	CPPUNIT_TEST(CyclicLink_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(RelativeFileLink_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(RelativeDirLink_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(FileLink_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(DirLink_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(SubDir_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(Dir_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(File_AttrBString_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_GetAttrInfo_ReturnsError);
	CPPUNIT_TEST(Basic_GetAttrInfo_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_RenameAttr_ReturnsError);
	CPPUNIT_TEST(Basic_RenameAttr_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_Attr_ReturnsError);
	CPPUNIT_TEST(ReadWrite_Attr_ReturnsExpected);
	CPPUNIT_TEST(Uninitialized_AttrDir_ReturnsError);
	CPPUNIT_TEST(Basic_AttrDir_ReturnsExpected);
	CPPUNIT_TEST(Initialized_Unset_ReturnsNoInit);
	CPPUNIT_TEST(Null_SetToRef_ReturnsBadValue);
	CPPUNIT_TEST(Dir_SetToRef_ReturnsOk);
	CPPUNIT_TEST(File_SetToRef_ReturnsOk);
	CPPUNIT_TEST(NonExisting_SetToRef_ReturnsNotFound);
	CPPUNIT_TEST(DirLink_SetToRef_ReturnsOk);
	CPPUNIT_TEST(TooLong_SetToEntry_ReturnsError);
	CPPUNIT_TEST(Dir_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(File_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_SetToEntry_ReturnsError);
	CPPUNIT_TEST(Null_SetToEntry_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_SetToEntry_ReturnsNotFound);
	CPPUNIT_TEST(DirLink_SetToEntry_ReturnsOk);
	CPPUNIT_TEST(TooLong_SetToPath_ReturnsNameTooLong);
	CPPUNIT_TEST(Dir_SetToPath_ReturnsOk);
	CPPUNIT_TEST(File_SetToPath_ReturnsOk);
	CPPUNIT_TEST(Empty_SetToPath_ReturnsNotFound);
	CPPUNIT_TEST(Null_SetToPath_ReturnsError);
	CPPUNIT_TEST(NonExisting_SetToPath_ReturnsNotFound);
	CPPUNIT_TEST(FileLink_SetToPath_ReturnsOk);
	CPPUNIT_TEST(DirLink_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(BadLink_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_DirPathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(NullArgs_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NullLeaf_DirPathConstructor_ReturnsBadValue);
	CPPUNIT_TEST(EmptyLeaf_DirPathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(File_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(Dir_DirPathConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_DirPathConstructor_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirsAsDir_DirPathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(DirLink_DirPathSetTo_ReturnsOk);
	CPPUNIT_TEST(BadLink_DirPathSetTo_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_DirPathSetTo_ReturnsNotFound);
	CPPUNIT_TEST(NullArgs_DirPathSetTo_ReturnsBadValue);
	CPPUNIT_TEST(NullLeaf_DirPathSetTo_ReturnsBadValue);
	CPPUNIT_TEST(EmptyLeaf_DirPathSetTo_ReturnsNotFound);
	CPPUNIT_TEST(File_DirPathSetTo_ReturnsOk);
	CPPUNIT_TEST(Dir_DirPathSetTo_ReturnsOk);
	CPPUNIT_TEST(TooLong_DirPathSetTo_ReturnsNameTooLong);
	CPPUNIT_TEST(FileDirsAsDir_DirPathSetTo_ReturnsNotFound);
	CPPUNIT_TEST(Null_RefConstructor_ReturnsBadValue);
	CPPUNIT_TEST(Dir_RefConstructor_ReturnsOk);
	CPPUNIT_TEST(File_RefConstructor_ReturnsOk);
	CPPUNIT_TEST(NonExisting_RefConstructor_ReturnsNotFound);
	CPPUNIT_TEST(DirLink_RefConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_EntryConstructor_ReturnsError);
	CPPUNIT_TEST(Dir_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(File_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(Uninitialized_EntryConstructor_ReturnsError);
	CPPUNIT_TEST(Null_EntryConstructor_ReturnsBadValue);
	CPPUNIT_TEST(NonExisting_EntryConstructor_ReturnsNotFound);
	CPPUNIT_TEST(DirLink_EntryConstructor_ReturnsOk);
	CPPUNIT_TEST(TooLong_PathConstructor_ReturnsNameTooLong);
	CPPUNIT_TEST(Dir_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(File_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(Empty_PathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(Null_PathConstructor_ReturnsError);
	CPPUNIT_TEST(NonExisting_PathConstructor_ReturnsNotFound);
	CPPUNIT_TEST(FileLink_PathConstructor_ReturnsOk);
	CPPUNIT_TEST(DefaultConstructor_ReturnsNoInit);
	CPPUNIT_TEST_SUITE_END();

protected:
	void CreateUninitializedNodes(TestNodes& testEntries)
	{
		testEntries.add(new BNode, "");
	}

	void CreateRWNodes(TestNodes& testEntries)
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

	void CreateRONodes(TestNodes& testEntries)
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

	void CreateUninitializedStatables(TestStatables& testEntries)
	{
		TestNodes testNodes;
		CreateUninitializedNodes(testNodes);
		ConvertTestStatablesToNodes(testNodes, testEntries);
	}

	void CreateRWStatables(TestStatables& testEntries)
	{
		TestNodes testNodes;
		CreateRWNodes(testNodes);
		ConvertTestStatablesToNodes(testNodes, testEntries);
	}

	void CreateROStatables(TestStatables& testEntries)
	{
		TestNodes testNodes;
		CreateRONodes(testNodes);
		ConvertTestStatablesToNodes(testNodes, testEntries);
	}

	void _LockTest(BNode &node, const char *entryName)
	{
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Lock());
		BNode node2(entryName);
		CPPUNIT_ASSERT_EQUAL(B_BUSY, node2.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Unlock());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.Unlock());
		CPPUNIT_ASSERT_EQUAL(B_OK, node2.SetTo(entryName));
	// R5: Since two file descriptors exist at this point, locking is supposed
	// to fail according to the BeBook, but it succeeds!
		CPPUNIT_ASSERT_EQUAL(B_OK, node2.Lock());
		CPPUNIT_ASSERT_EQUAL(B_BUSY, node.Lock());
	//
		CPPUNIT_ASSERT_EQUAL(B_OK, node2.Unlock());
	}

	void _EqualityTest(BNode &n1, BNode &n2, BNode &y1a, BNode &y1b, BNode &y2) {
		CPPUNIT_ASSERT(n1 == n2);
		CPPUNIT_ASSERT(!(n1 != n2));
		CPPUNIT_ASSERT(n1 != y2);
		CPPUNIT_ASSERT(!(n1 == y2));

		CPPUNIT_ASSERT(y1a != n2);
		CPPUNIT_ASSERT(!(y1a == n2));
		CPPUNIT_ASSERT(y1a == y1b);
		CPPUNIT_ASSERT(!(y1a != y1b));
		CPPUNIT_ASSERT(y1a != y2);
		CPPUNIT_ASSERT(!(y1a == y2));

		CPPUNIT_ASSERT(n1 == n1);
		CPPUNIT_ASSERT(!(n1 != n1));
		CPPUNIT_ASSERT(y2 == y2);
		CPPUNIT_ASSERT(!(y2 != y2));
	}

	void _DupTest(BNode &node)
	{
		int fd = node.Dup();
		CPPUNIT_ASSERT(fd != -1);
		::close(fd);
	}

	void _AttrBStringTest(BNode &node)
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
			CPPUNIT_ASSERT_EQUAL(B_OK, node.WriteAttrString(attrName, &attrValue));
		}
		// read and check them
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			const char *attrValue = attrValues[i];
			BString readValue;
			CPPUNIT_ASSERT_EQUAL(B_OK, node.ReadAttrString(attrName, &readValue));
			CPPUNIT_ASSERT(readValue == attrValue);
		}
		// write a new value for each attribute
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			BString attrValue(newAttrValues[i]);
			CPPUNIT_ASSERT_EQUAL(B_OK, node.WriteAttrString(attrName, &attrValue));
		}
		// read and check them
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			const char *attrValue = newAttrValues[i];
			BString readValue;
			CPPUNIT_ASSERT_EQUAL(B_OK, node.ReadAttrString(attrName, &readValue));
			CPPUNIT_ASSERT(readValue == attrValue);
		}
		// bad args
		BString readValue;
		BString writeValue("test");
	// R5: crashes, if supplying a NULL BString
	#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.WriteAttrString(attrNames[0], NULL));		
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.ReadAttrString(attrNames[0], NULL));
	#endif
		CPPUNIT_ASSERT(equals(node.WriteAttrString(NULL, &writeValue),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.ReadAttrString(NULL, &readValue),
							   B_BAD_ADDRESS, B_BAD_VALUE));
	#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.WriteAttrString(NULL, NULL));
	#endif
		CPPUNIT_ASSERT(equals(node.ReadAttrString(NULL, NULL),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		// remove the attributes and try to read them
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			CPPUNIT_ASSERT_EQUAL(B_OK, node.RemoveAttr(attrName));
			CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.ReadAttrString(attrName, &readValue));
		}
		// too long attribute name
	// R5: Read/WriteAttrString() do not return B_NAME_TOO_LONG
		char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
		memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
		tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.WriteAttrString(tooLongAttrName, &writeValue));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.ReadAttrString(tooLongAttrName, &readValue));
	}

	void _AttrInfoTest(BNode &node)
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
			CPPUNIT_ASSERT(node.WriteAttr(attrName, attrType, 0, attrValue,
										   valueSize) == valueSize);
		}
		// get the attribute infos
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			attr_info info;
			CPPUNIT_ASSERT_EQUAL(B_OK, node.GetAttrInfo(attrName, &info));
			CPPUNIT_ASSERT(info == attrInfos[i]);
		}
		// try get an info for a non-existing attribute
		attr_info info;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetAttrInfo("non-existing attribute", &info));
		// bad values
		CPPUNIT_ASSERT(equals(node.GetAttrInfo(NULL, &info), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.GetAttrInfo(attrNames[0], NULL), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.GetAttrInfo(NULL, NULL), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		// too long attribute name
	// R5: GetAttrInfo() does not return B_NAME_TOO_LONG
		char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
		memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
		tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetAttrInfo(tooLongAttrName, &info));
	}


	void _AttrRenameTest(BNode &node)
	{
		const char attr1[] = "StorageKit::SomeAttribute";
		const char attr2[] = "StorageKit::AnotherAttribute";

		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo("./"));

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

		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.RenameAttr(attr1, attr2));

		// Write an attribute, read it to verify it, rename it, read the
		// new attribute, read the old (which fails), and then remove the new.
		CPPUNIT_ASSERT(node.WriteAttr(attr1, B_STRING_TYPE, 0, str, strLen) == strLen);
		CPPUNIT_ASSERT(node.ReadAttr(attr1, B_STRING_TYPE, 0, data, dataLen) == strLen);
		CPPUNIT_ASSERT(strcmp(data, str) == 0);		
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RenameAttr(attr1, attr2)); // <<< This fails with R5::BNode
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.ReadAttr(attr1, B_STRING_TYPE, 0, data, dataLen));
		CPPUNIT_ASSERT(node.ReadAttr(attr2, B_STRING_TYPE, 0, data, dataLen) == strLen);
		CPPUNIT_ASSERT(strcmp(data, str) == 0);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RemoveAttr(attr2));

		// bad args
		CPPUNIT_ASSERT(equals(node.RenameAttr(attr1, NULL), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.RenameAttr(NULL, attr2), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.RenameAttr(NULL, NULL), B_BAD_ADDRESS,
							   B_BAD_VALUE));
		// too long attribute name
	// R5: RenameAttr() returns B_BAD_VALUE instead of B_NAME_TOO_LONG
		char tooLongAttrName[B_ATTR_NAME_LENGTH + 2];
		memset(tooLongAttrName, 'a', B_ATTR_NAME_LENGTH);
		tooLongAttrName[B_ATTR_NAME_LENGTH + 1] = '\0';
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.RenameAttr(attr1, tooLongAttrName));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.RenameAttr(tooLongAttrName, attr1));
	#endif
	}

	void _AttrTest(BNode &node)
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
			CPPUNIT_ASSERT(node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
										  sizeof(buffer)) == valueSize);
			CPPUNIT_ASSERT(strcmp(buffer, attrValue) == 0);
		}
		// write a new value for each attribute
		WriteAttributes(node, attrNames, newAttrValues, attrCount);
		// read and check them
		for (int32 i = 0; i < attrCount; i++) {
			const char *attrName = attrNames[i];
			const char *attrValue = newAttrValues[i];
			int32 valueSize = strlen(attrValue) + 1;
			CPPUNIT_ASSERT(node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
										  sizeof(buffer)) == valueSize);
			CPPUNIT_ASSERT(strcmp(buffer, attrValue) == 0);
		}
		// bad args
		CPPUNIT_ASSERT(equals(node.ReadAttr(NULL, B_STRING_TYPE, 0, buffer,
											 sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.ReadAttr(attrNames[0], B_STRING_TYPE, 0, NULL,
											 sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.ReadAttr(NULL, B_STRING_TYPE, 0, NULL,
											 sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.WriteAttr(NULL, B_STRING_TYPE, 0, buffer,
											  sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.WriteAttr(attrNames[0], B_STRING_TYPE, 0, NULL,
											  sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.WriteAttr(NULL, B_STRING_TYPE, 0, NULL,
											  sizeof(buffer)),
							   B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.RemoveAttr(NULL), B_BAD_ADDRESS, B_BAD_VALUE));
		// too long attribute name
		// R5: Read/RemoveAttr() do not return B_NAME_TOO_LONG, but B_ENTRY_NOT_FOUND
		// R5: WriteAttr() does not return B_NAME_TOO_LONG, but B_BAD_VALUE
		// R5: Haiku has a max attribute size of 256, while R5's was 255, exclusive
		//	 of the null terminator. See changeset 4069e1f30.
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
			CPPUNIT_ASSERT_EQUAL(B_OK, node.RemoveAttr(attrName));
			CPPUNIT_ASSERT(node.ReadAttr(attrName, B_STRING_TYPE, 0, buffer,
										  sizeof(buffer)) == B_ENTRY_NOT_FOUND);
		}
		// try to remove a non-existing attribute
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.RemoveAttr("non existing attribute"));
	}

	void _AttrDirTest(BNode &node)
	{
		// node should not have any attributes at the beginning
		char nameBuffer[B_ATTR_NAME_LENGTH];
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetNextAttrName(nameBuffer));
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
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RewindAttrs());
		while (node.GetNextAttrName(nameBuffer) == B_OK)
			CPPUNIT_ASSERT(testSet.test(nameBuffer) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetNextAttrName(nameBuffer));
		// rewind, get one attribute, rewind again and iterate through the whole
		// list again
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RewindAttrs());
		testSet.rewind();
		CPPUNIT_ASSERT_EQUAL(B_OK, node.GetNextAttrName(nameBuffer));
		CPPUNIT_ASSERT(testSet.test(nameBuffer) == true);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RewindAttrs());
		testSet.rewind();
		while (node.GetNextAttrName(nameBuffer) == B_OK)
			CPPUNIT_ASSERT(testSet.test(nameBuffer) == true);
		CPPUNIT_ASSERT(testSet.testDone() == true);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.GetNextAttrName(nameBuffer));
		// bad args
		CPPUNIT_ASSERT_EQUAL(B_OK, node.RewindAttrs());
		testSet.rewind();
	// R5: crashs, if passing a NULL buffer
	#if !TEST_R5
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.GetNextAttrName(NULL));
	#endif
	}

public:
	void setUp() {
		StatableTest::setUp();
		execCommand(
			std::string ("touch ") + existingFilename
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

	void tearDown() {
		StatableTest::tearDown();
		// cleanup
		std::string cmdLine("rm -rf ");
		for (int32 i = 0; i < allFilenameCount; i++)
			cmdLine += std::string (" ") + allFilenames[i];
		if (allFilenameCount > 0)
			execCommand(cmdLine);
	}

	// entry names used by this class or derived classes
	static	const char *		existingFilename;
	static	const char *		existingSuperFilename;
	static	const char *		existingRelFilename;
	static	const char *		existingDirname;
	static	const char *		existingSuperDirname;
	static	const char *		existingRelDirname;
	static	const char *		existingSubDirname;
	static	const char *		existingRelSubDirname;
	static	const char *		nonExistingFilename;
	static	const char *		nonExistingDirname;
	static	const char *		nonExistingSuperDirname;
	static	const char *		nonExistingRelDirname;
	static	const char *		testFilename1;
	static	const char *		testDirname1;
	static	const char *		tooLongEntryname;
	static	const char *		tooLongSuperEntryname;
	static	const char *		tooLongRelEntryname;
	static	const char *		fileDirname;
	static	const char *		fileSuperDirname;
	static	const char *		fileRelDirname;
	static	const char *		dirLinkname;
	static	const char *		dirSuperLinkname;
	static	const char *		dirRelLinkname;
	static	const char *		fileLinkname;
	static	const char *		fileSuperLinkname;
	static	const char *		fileRelLinkname;
	static	const char *		relDirLinkname;
	static	const char *		relFileLinkname;
	static	const char *		badLinkname;
	static	const char *		cyclicLinkname1;
	static	const char *		cyclicLinkname2;

	static	const char *		allFilenames[];
	static	const int32 		allFilenameCount;

	void Uninitialized_Lock_ReturnsNoInit() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, node.Lock());
	}

	void CyclicLink_Lock_ReturnsExpected() {
		BNode node(cyclicLinkname1);
		_LockTest(node, cyclicLinkname1);
	}

	void RelativeFileLink_Lock_ReturnsExpected() {
		BNode node(relFileLinkname);
		_LockTest(node, relFileLinkname);
	}

	void RelativeDirLink_Lock_ReturnsExpected() {
		BNode node(relDirLinkname);
		_LockTest(node, relDirLinkname);
	}

	void FileLink_Lock_ReturnsExpected() {
		BNode node(fileLinkname);
		_LockTest(node, fileLinkname);
	}

	void DirLink_Lock_ReturnsExpected() {
		BNode node(dirLinkname);
		_LockTest(node, dirLinkname);
	}

	void SubDir_Lock_ReturnsExpected() {
		BNode node(existingSubDirname);
		_LockTest(node, existingSubDirname);
	}

	void Dir_Lock_ReturnsExpected() {
		BNode node(existingDirname);
		_LockTest(node, existingDirname);
	}

	void File_Lock_ReturnsExpected() {
		BNode node(existingFilename);
		_LockTest(node, existingFilename);
	}

	void Basic_Assignment_ReturnsExpected() {
		BNode n1, n2, y1a("/boot"), y1b("/boot"), y2("/");

		n1 = n1;		// self n
		y1a = y1b;		// psuedo self y
		y1a = y1a;		// self y

		n2 = y2;		// n = y
		y1b = n1;		// y = n
		y2 = y1a;		// y1 = y2

		_EqualityTest(n1, y1b, y1a, y2, n2);
	}

	void Basic_Equality_ReturnsExpected() {
		BNode n1, n2, y1a("/boot"), y1b("/boot"), y2("/");
		_EqualityTest(n1, n2, y1a, y1b, y2);
	}

	void Uninitialized_Dup_ReturnsError() {
		BNode node;
		CPPUNIT_ASSERT(node.Dup() == -1);
	}

	void CyclicLink_Dup_ReturnsExpected() {
		BNode node(cyclicLinkname1);
		_DupTest(node);
	}

	void RelativeFileLink_Dup_ReturnsExpected() {
		BNode node(relFileLinkname);
		_DupTest(node);
	}

	void RelativeDirLink_Dup_ReturnsExpected() {
		BNode node(relDirLinkname);
		_DupTest(node);
	}

	void FileLink_Dup_ReturnsExpected() {
		BNode node(fileLinkname);
		_DupTest(node);
	}

	void DirLink_Dup_ReturnsExpected() {
		BNode node(dirLinkname);
		_DupTest(node);
	}

	void SubDir_Dup_ReturnsExpected() {
		BNode node(existingSubDirname);
		_DupTest(node);
	}

	void Dir_Dup_ReturnsExpected() {
		BNode node(existingDirname);
		_DupTest(node);
	}

	void File_Dup_ReturnsExpected() {
		BNode node(existingFilename);
		_DupTest(node);
	}

	void Uninitialized_Sync_ReturnsError() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node.Sync());
	}

	void CyclicLink_Sync_ReturnsOk() {
		BNode node(cyclicLinkname1);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void RelativeFileLink_Sync_ReturnsOk() {
		BNode node(relFileLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void RelativeDirLink_Sync_ReturnsOk() {
		BNode node(relDirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void FileLink_Sync_ReturnsOk() {
		BNode node(fileLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void DirLink_Sync_ReturnsOk() {
		BNode node(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void SubDir_Sync_ReturnsOk() {
		BNode node(existingSubDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void Dir_Sync_ReturnsOk() {
		BNode node(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void File_Sync_ReturnsOk() {
		BNode node(existingFilename);
		const char attr[] = "StorageKit::SomeAttribute";
		const char str[] = "This string rules your world.";
		const int len = strlen(str) + 1;
		// This doesn't actually verify synching is occuring; just
		// checks for a B_OK return value.
		CPPUNIT_ASSERT(node.WriteAttr(attr, B_STRING_TYPE, 0, str, len)
						== len);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.Sync());
	}

	void Uninitialized_AttrBString_ReturnsError() {
		TestNodes testEntries;
		CreateUninitializedNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			BString value("test");
			CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node->WriteAttrString("attr1", &value));
			CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node->ReadAttrString("attr1", &value));
		}
		testEntries.delete_all();
	}

	void CyclicLink_AttrBString_ReturnsExpected() {
		BNode node(cyclicLinkname1);
		_AttrBStringTest(node);
	}

	void RelativeFileLink_AttrBString_ReturnsExpected() {
		BNode node(relFileLinkname);
		_AttrBStringTest(node);
	}

	void RelativeDirLink_AttrBString_ReturnsExpected() {
		BNode node(relDirLinkname);
		_AttrBStringTest(node);
	}

	void FileLink_AttrBString_ReturnsExpected() {
		BNode node(fileLinkname);
		_AttrBStringTest(node);
	}

	void DirLink_AttrBString_ReturnsExpected() {
		BNode node(dirLinkname);
		_AttrBStringTest(node);
	}

	void SubDir_AttrBString_ReturnsExpected() {
		BNode node(existingSubDirname);
		_AttrBStringTest(node);
	}

	void Dir_AttrBString_ReturnsExpected() {
		BNode node(existingDirname);
		_AttrBStringTest(node);
	}

	void File_AttrBString_ReturnsExpected() {
		BNode node(existingFilename);
		_AttrBStringTest(node);
	}

	void Uninitialized_GetAttrInfo_ReturnsError() {
		TestNodes testEntries;
		CreateUninitializedNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			attr_info info;
			CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node->GetAttrInfo("attr1", &info));
		}
		testEntries.delete_all();
	}

	void Basic_GetAttrInfo_ReturnsExpected() {
		TestNodes testEntries;
		CreateRWNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			_AttrInfoTest(*node);
		}
		testEntries.delete_all();
	}


	void Uninitialized_RenameAttr_ReturnsError() {
		TestNodes testEntries;
		CreateUninitializedNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node->RenameAttr("attr1", "attr2"));
		}
		testEntries.delete_all();
	}

	void Basic_RenameAttr_ReturnsExpected() {
		TestNodes testEntries;
		CreateRWNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			_AttrRenameTest(*node);
		}
		testEntries.delete_all();
	}


	void Uninitialized_Attr_ReturnsError() {
		TestNodes testEntries;
		CreateUninitializedNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			char buffer[1024];
			CPPUNIT_ASSERT(node->ReadAttr("attr1", B_STRING_TYPE, 0, buffer,
										   sizeof(buffer)) == B_FILE_ERROR);
			CPPUNIT_ASSERT(node->WriteAttr("attr1", B_STRING_TYPE, 0, buffer,
											sizeof(buffer)) == B_FILE_ERROR);
			CPPUNIT_ASSERT_EQUAL(B_FILE_ERROR, node->RemoveAttr("attr1"));
		}
		testEntries.delete_all();
	}

	void ReadWrite_Attr_ReturnsExpected() {
		TestNodes testEntries;
		CreateRWNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			_AttrTest(*node);
		}
		testEntries.delete_all();
	}


	void Uninitialized_AttrDir_ReturnsError() {
		TestNodes testEntries;
		CreateUninitializedNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			char nameBuffer[B_ATTR_NAME_LENGTH];
			CPPUNIT_ASSERT(node->RewindAttrs() != B_OK);
			CPPUNIT_ASSERT(node->GetNextAttrName(nameBuffer) != B_OK);
		}
		testEntries.delete_all();
	}

	void Basic_AttrDir_ReturnsExpected() {
		TestNodes testEntries;
		CreateRWNodes(testEntries);
		BNode *node;
		std::string nodeName;
		for (testEntries.rewind(); testEntries.getNext(node, nodeName); ) {
			_AttrDirTest(*node);
		}
		testEntries.delete_all();
	}

	void Initialized_Unset_ReturnsNoInit() {
		BNode node(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
		node.Unset();
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, node.InitCheck());
	}

	void Null_SetToRef_ReturnsBadValue() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo((entry_ref *)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void Dir_SetToRef_ReturnsOk() {
		entry_ref ref;
		get_ref_for_path(existingDirname, &ref);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_SetToRef_ReturnsOk() {
		entry_ref ref;
		get_ref_for_path(existingFilename, &ref);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void NonExisting_SetToRef_ReturnsNotFound() {
		entry_ref ref;
		get_ref_for_path(nonExistingDirname, &ref);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void DirLink_SetToRef_ReturnsOk() {
		BEntry entry(dirLinkname);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&ref));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_SetToEntry_ReturnsError() {
		BEntry entry(tooLongEntryname);
		BNode node;
		CPPUNIT_ASSERT(equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG));
		CPPUNIT_ASSERT(equals(node.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Dir_SetToEntry_ReturnsOk() {
		BEntry entry(existingDirname);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_SetToEntry_ReturnsOk() {
		BEntry entry(existingFilename);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Uninitialized_SetToEntry_ReturnsError() {
		BEntry entry;
		BNode node;
		CPPUNIT_ASSERT(equals(node.SetTo(&entry), B_BAD_ADDRESS, B_BAD_VALUE));
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Null_SetToEntry_ReturnsBadValue() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo((BEntry *)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void NonExisting_SetToEntry_ReturnsNotFound() {
		BEntry entry(nonExistingDirname);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void DirLink_SetToEntry_ReturnsOk() {
		BEntry entry(dirLinkname);
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&entry));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_SetToPath_ReturnsNameTooLong() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.SetTo(tooLongEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.InitCheck());
	}

	void Dir_SetToPath_ReturnsOk() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(existingDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_SetToPath_ReturnsOk() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(existingFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Empty_SetToPath_ReturnsNotFound() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void Null_SetToPath_ReturnsError() {
		BNode node;
		CPPUNIT_ASSERT(equals(node.SetTo((const char *)NULL), B_BAD_VALUE, B_NO_INIT));
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void NonExisting_SetToPath_ReturnsNotFound() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(nonExistingDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void FileLink_SetToPath_ReturnsOk() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(fileLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	// BNode(const BDirectory*, const char*)
	void DirLink_DirPathConstructor_ReturnsOk() {
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, dirRelLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void BadLink_DirPathConstructor_ReturnsBadValue() {
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void NonExisting_DirPathConstructor_ReturnsNotFound() {
		BDirectory pathDir(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, nonExistingRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void NullArgs_DirPathConstructor_ReturnsBadValue() {
		BNode node1((BDirectory *)NULL, (const char *)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node1.InitCheck());
		BNode node2((BDirectory *)NULL, dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node2.InitCheck());
	}

	void NullLeaf_DirPathConstructor_ReturnsBadValue() {
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, (const char *)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void EmptyLeaf_DirPathConstructor_ReturnsNotFound() {
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, "");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void File_DirPathConstructor_ReturnsOk() {
		BDirectory pathDir(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, existingRelFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Dir_DirPathConstructor_ReturnsOk() {
		BDirectory pathDir(existingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, existingRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_DirPathConstructor_ReturnsNameTooLong() {
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, tooLongRelEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.InitCheck());
	}

	void FileDirsAsDir_DirPathConstructor_ReturnsNotFound() {
		BDirectory pathDir(fileDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		BNode node(&pathDir, fileRelDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	// BNode::SetTo(const BDirectory*, const char*)
	void DirLink_DirPathSetTo_ReturnsOk() {
		BNode node;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&pathDir, dirRelLinkname));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void BadLink_DirPathSetTo_ReturnsBadValue() {
		BNode node;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo(&pathDir, dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void NonExisting_DirPathSetTo_ReturnsNotFound() {
		BNode node;
		BDirectory pathDir(nonExistingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(nonExistingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(&pathDir, nonExistingRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void NullArgs_DirPathSetTo_ReturnsBadValue() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo((BDirectory *)NULL, (const char *)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo((BDirectory *)NULL, dirLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void NullLeaf_DirPathSetTo_ReturnsBadValue() {
		BNode node;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.SetTo(&pathDir, (const char *)NULL));
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void EmptyLeaf_DirPathSetTo_ReturnsNotFound() {
		BNode node;
		BDirectory pathDir(dirSuperLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(dirSuperLinkname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(&pathDir, ""));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void File_DirPathSetTo_ReturnsOk() {
		BNode node;
		BDirectory pathDir(existingSuperFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(existingSuperFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&pathDir, existingRelFilename));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Dir_DirPathSetTo_ReturnsOk() {
		BNode node;
		BDirectory pathDir(existingSuperDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(existingSuperDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.SetTo(&pathDir, existingRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_DirPathSetTo_ReturnsNameTooLong() {
		BNode node;
		BDirectory pathDir(tooLongSuperEntryname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(tooLongSuperEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.SetTo(&pathDir, tooLongRelEntryname));
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.InitCheck());
	}

	void FileDirsAsDir_DirPathSetTo_ReturnsNotFound() {
		BNode node;
		BDirectory pathDir(fileDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, pathDir.SetTo(fileDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.SetTo(&pathDir, fileRelDirname));
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void Null_RefConstructor_ReturnsBadValue() {
		BNode node((entry_ref *)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void Dir_RefConstructor_ReturnsOk() {
		entry_ref ref;
		get_ref_for_path(existingDirname, &ref);
		BNode node(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_RefConstructor_ReturnsOk() {
		entry_ref ref;
		get_ref_for_path(existingFilename, &ref);
		BNode node(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void NonExisting_RefConstructor_ReturnsNotFound() {
		entry_ref ref;
		get_ref_for_path(nonExistingDirname, &ref);
		BNode node(&ref);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void DirLink_RefConstructor_ReturnsOk() {
		BEntry entry(dirLinkname);
		entry_ref ref;
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.GetRef(&ref));
		BNode node(&ref);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_EntryConstructor_ReturnsError() {
		BEntry entry(tooLongEntryname);
		CPPUNIT_ASSERT(equals(entry.InitCheck(), E2BIG, B_NAME_TOO_LONG));
		BNode node(&entry);
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Dir_EntryConstructor_ReturnsOk() {
		BEntry entry(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BNode node(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_EntryConstructor_ReturnsOk() {
		BEntry entry(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BNode node(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Uninitialized_EntryConstructor_ReturnsError() {
		BEntry entry;
		BNode node(&entry);
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_ADDRESS, B_BAD_VALUE));
	}

	void Null_EntryConstructor_ReturnsBadValue() {
		BNode node((BEntry *)NULL);
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, node.InitCheck());
	}

	void NonExisting_EntryConstructor_ReturnsNotFound() {
		BEntry entry(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BNode node(&entry);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void DirLink_EntryConstructor_ReturnsOk() {
		BEntry entry(dirLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, entry.InitCheck());
		BNode node(&entry);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void TooLong_PathConstructor_ReturnsNameTooLong() {
		BNode node(tooLongEntryname);
		CPPUNIT_ASSERT_EQUAL(B_NAME_TOO_LONG, node.InitCheck());
	}

	void Dir_PathConstructor_ReturnsOk() {
		BNode node(existingDirname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void File_PathConstructor_ReturnsOk() {
		BNode node(existingFilename);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void Empty_PathConstructor_ReturnsNotFound() {
		BNode node("");
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void Null_PathConstructor_ReturnsError() {
		BNode node((const char *)NULL);
		CPPUNIT_ASSERT(equals(node.InitCheck(), B_BAD_VALUE, B_NO_INIT));
	}

	void NonExisting_PathConstructor_ReturnsNotFound() {
		BNode node(nonExistingDirname);
		CPPUNIT_ASSERT_EQUAL(B_ENTRY_NOT_FOUND, node.InitCheck());
	}

	void FileLink_PathConstructor_ReturnsOk() {
		BNode node(fileLinkname);
		CPPUNIT_ASSERT_EQUAL(B_OK, node.InitCheck());
	}

	void DefaultConstructor_ReturnsNoInit() {
		BNode node;
		CPPUNIT_ASSERT_EQUAL(B_NO_INIT, node.InitCheck());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(NodeTest, getTestSuiteName());

#endif // _STORAGE_NODE_TEST_H_
