// NodeTest.h

#ifndef __sk_node_test_h__
#define __sk_node_test_h__

#include "StatableTest.h"

class BNode;

typedef TestEntries<BNode> TestNodes;

class NodeTest : public StatableTest {
public:
	static CppUnit::Test* Suite();

	template<typename DerivedClass>
	static inline void AddBaseClassTests(const char *prefix,
										 CppUnit::TestSuite *suite);

	virtual void CreateROStatables(TestStatables& testEntries);
	virtual void CreateRWStatables(TestStatables& testEntries);
	virtual void CreateUninitializedStatables(TestStatables& testEntries);

	virtual void CreateRONodes(TestNodes& testEntries);
	virtual void CreateRWNodes(TestNodes& testEntries);
	virtual void CreateUninitializedNodes(TestNodes& testEntries);

	// This function is called before *each* test added in Suite()
	void setUp();
	
	// This function is called after *each* test added in Suite()
	void tearDown();

	// Tests
	void InitTest1();
	void InitTest2();
	void AttrDirTest();
	void AttrTest();
	void AttrRenameTest();
	void AttrInfoTest();
	void AttrBStringTest();
	void DupTest();
	void SyncTest();
	void EqualityTest();
	void AssignmentTest();
	void LockTest(); // Locking isn't implemented for the Posix versions

	// Helper functions
	void AttrDirTest(BNode &node);
	void AttrTest(BNode &node);
	void AttrRenameTest(BNode &node);
	void AttrInfoTest(BNode &node);
	void AttrBStringTest(BNode &node);
	void DupTest(BNode &node);
	void LockTest(BNode &node, const char *entryName);
	void EqualityTest(BNode &n1, BNode &n2, BNode &y1a, BNode &y1b, BNode &y2);

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
};

// AddBaseClassTests
template<typename DerivedClass>
inline void
NodeTest::AddBaseClassTests(const char *prefix, CppUnit::TestSuite *suite)
{
	StatableTest::AddBaseClassTests<DerivedClass>(prefix, suite);

	typedef CppUnit::TestCaller<DerivedClass> TC;
	string p(prefix);

	suite->addTest( new TC(p + "BNode::AttrDir Test", &NodeTest::AttrDirTest) );
	suite->addTest( new TC(p + "BNode::Attr Test", &NodeTest::AttrTest) );
	suite->addTest( new TC(p + "BNode::AttrRename Test"
#if TEST_R5
								" (NOTE: test not actually performed with R5 libraries)"
#endif								
								, &NodeTest::AttrRenameTest) );
	suite->addTest( new TC(p + "BNode::AttrInfo Test", &NodeTest::AttrInfoTest) );
	// TODO: AttrBString deadlocks entire OS (UnitTester at 100% CPU,
	// windows don't respond to actions, won't open, OS won't even shut down)
	//suite->addTest( new TC(p + "BNode::AttrBString Test", &NodeTest::AttrBStringTest) );
	suite->addTest( new TC(p + "BNode::Sync Test", &NodeTest::SyncTest) );
	suite->addTest( new TC(p + "BNode::Dup Test", &NodeTest::DupTest) );
	suite->addTest( new TC(p + "BNode::Lock Test"
#if TEST_OBOS /* !!!POSIX ONLY!!! */
								" (NOTE: test not actually performed with Haiku Posix libraries)"
#endif
								, &NodeTest::LockTest) );
}


#endif	// __sk_node_test_h__




