// StatableTest.h

#ifndef __sk_statable_test_h__
#define __sk_statable_test_h__

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <list>
#include <string>

#include "BasicTest.h"

class BStatable;
class BEntry;
class BNode;

// TestEntries

template <typename C>
struct TestEntries
{
	~TestEntries()
	{
		delete_all();
	}

	void delete_all()
	{
		for (list<C*>::iterator it = entries.begin();
			 it != entries.end();
			 it++) {
			// Arghh, BStatable has no virtual destructor!
			// Workaround: try to cast to one of the subclasses
			if (BNode *node = dynamic_cast<BNode*>(*it))
				delete node;
			else if (BEntry *entry = dynamic_cast<BEntry*>(*it))
				delete entry;
			else
				delete *it;
		}
		clear();
	}

	void clear()
	{
		entries.clear();
		entryNames.clear();
		rewind();
	}

	void add(C *entry, string entryName)
	{
		entries.push_back(entry);
		entryNames.push_back(entryName);
	}

	bool getNext(C *&entry, string &entryName)
	{
		bool result = (entryIt != entries.end()
					   && entryNameIt != entryNames.end());
		if (result) {
			entry = *entryIt;
			entryName = *entryNameIt;
			entryIt++;
			entryNameIt++;
		}
		return result;
	}
	
	void rewind()
	{
		entryIt = entries.begin();
		entryNameIt = entryNames.begin();
	}

	list<C*>					entries;
	list<string>				entryNames;
	list<C*>::iterator			entryIt;
	list<string>::iterator		entryNameIt;
};

typedef TestEntries<BStatable> TestStatables;


// StatableTest

class StatableTest : public BasicTest
{
public:
	template<typename DerivedClass>
	static inline void AddBaseClassTests(const char *prefix,
										 CppUnit::TestSuite *suite);

	virtual void CreateROStatables(TestStatables& testEntries) = 0;
	virtual void CreateRWStatables(TestStatables& testEntries) = 0;
	virtual void CreateUninitializedStatables(TestStatables& testEntries) = 0;

	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	void GetStatTest();
	void IsXYZTest();
	void GetXYZTest();
	void SetXYZTest();
};

// AddBaseClassTests
template<typename DerivedClass>
inline void
StatableTest::AddBaseClassTests(const char *prefix, CppUnit::TestSuite *suite)
{
	typedef CppUnit::TestCaller<DerivedClass> TC;
	string p(prefix);

	suite->addTest( new TC(p + "BStatable::GetStat Test",
						   &StatableTest::GetStatTest) );
	suite->addTest( new TC(p + "BStatable::IsXYZ Test",
						   &StatableTest::IsXYZTest) );
	suite->addTest( new TC(p + "BStatable::GetXYZ Test",
						   &StatableTest::GetXYZTest) );
	suite->addTest( new TC(p + "BStatable::SetXYZ Test",
						   &StatableTest::SetXYZTest) );
}

#endif	// __sk_statable_test_h__
