
#include <stdio.h>
#include <stdlib.h>

#include <map>

#include <TestUtils.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <VectorMap.h>

#include "common.h"
#include "VectorMapTest.h"

using KernelUtilsOrder::Ascending;
using KernelUtilsOrder::Descending;

VectorMapTest::VectorMapTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
VectorMapTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("VectorMap");

	ADD_TEST4(VectorMap, suite, VectorMapTest, ConstructorTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, InsertTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, PutTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, GetTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, RemoveTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, EraseTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, MakeEmptyTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, FindTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, FindCloseTest);
	ADD_TEST4(VectorMap, suite, VectorMapTest, IteratorTest);

	return suite;
}

//! ConstructorTest
void
VectorMapTest::ConstructorTest()
{
	NextSubTest();
	VectorMap<int, int> v1(100);
	CHK(v1.Count() == 0);
	CHK(v1.IsEmpty());

	NextSubTest();
	VectorMap<string, string> v2(100);
	CHK(v2.Count() == 0);
	CHK(v2.IsEmpty());

	NextSubTest();
	VectorMap<int, int> v3(0);
	CHK(v3.Count() == 0);
	CHK(v3.IsEmpty());

	NextSubTest();
	VectorMap<string, string> v4(0);
	CHK(v4.Count() == 0);
	CHK(v4.IsEmpty());
}

// TestIterator
template<typename Entry, typename TestMap, typename MyIterator,
		 typename ReferenceIterator>
class TestIterator {
private:
	typedef TestIterator<Entry, TestMap, MyIterator, ReferenceIterator>
			Iterator;

public:
	inline TestIterator(TestMap *s, MyIterator myIt, ReferenceIterator refIt)
		: fMap(s),
		  fMyIterator(myIt),
		  fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator &other)
		: fMap(other.fMap),
		  fMyIterator(other.fMyIterator),
		  fReferenceIterator(other.fReferenceIterator)
	{
		CHK(fMyIterator == other.fMyIterator);
	}

	inline Iterator &operator++()
	{
		MyIterator &myResult = ++fMyIterator;
		ReferenceIterator &refResult = ++fReferenceIterator;
		if (refResult == fMap->fReferenceMap.end())
			CHK(myResult == fMap->fMyMap.End());
		else {
			CHK(myResult->Key() == refResult->first);
			CHK(myResult->Value() == refResult->second);
		}
		return *this;
	}

	inline Iterator operator++(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator++;
		ReferenceIterator refResult = fReferenceIterator++;
		CHK(oldMyResult == myResult);
		if (refResult == fMap->fReferenceMap.end())
			CHK(myResult == fMap->fMyMap.End());
		else {
			CHK(myResult->Key() == refResult->first);
			CHK(myResult->Value() == refResult->second);
		}
		return Iterator(fMap, myResult, refResult);
	}

	inline Iterator &operator--()
	{
		MyIterator &myResult = --fMyIterator;
		ReferenceIterator &refResult = --fReferenceIterator;
		CHK(myResult->Key() == refResult->first);
		CHK(myResult->Value() == refResult->second);
		return *this;
	}

	inline Iterator operator--(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator--;
		ReferenceIterator refResult = fReferenceIterator--;
		CHK(oldMyResult == myResult);
		CHK(myResult->Key() == refResult->first);
		CHK(myResult->Value() == refResult->second);
		return Iterator(fMap, myResult, refResult);
	}

	inline Iterator &operator=(const Iterator &other)
	{
		fMap = other.fMap;
		fMyIterator = other.fMyIterator;
		fReferenceIterator = other.fReferenceIterator;
		CHK(fMyIterator == other.fMyIterator);
		return *this;
	}

	inline bool operator==(const Iterator &other) const
	{
		bool result = (fMyIterator == other.fMyIterator);
		CHK((fReferenceIterator == other.fReferenceIterator) == result);
		return result;
	}

	inline bool operator!=(const Iterator &other) const
	{
		bool result = (fMyIterator != other.fMyIterator);
		CHK((fReferenceIterator != other.fReferenceIterator) == result);
		return result;
	}

	inline Entry operator*() const
	{
		Entry entry = *fMyIterator;
		CHK(entry.Key() == fReferenceIterator->first);
		CHK(entry.Value() == fReferenceIterator->second);
		return entry;
	}

	inline Entry operator->() const
	{
		Entry entry = fMyIterator.operator->();
		CHK(entry.Key() == fReferenceIterator->first);
		CHK(entry.Value() == fReferenceIterator->second);
		return entry;
	}

	inline operator bool() const
	{
		bool result = fMyIterator;
		CHK((fMyIterator == fMap->fMyMap.Null()) != result);
		return result;
	}

public:
	TestMap				*fMap;
	MyIterator			fMyIterator;
	ReferenceIterator	fReferenceIterator;
};

// TestMap
template<typename Key, typename Value, typename MyMap, typename ReferenceMap,
		 typename Compare>
class TestMap {
public:
	typedef TestMap<Key, Value, MyMap, ReferenceMap, Compare>	Class;

	typedef typename MyMap::Iterator				MyIterator;
	typedef typename ReferenceMap::iterator			ReferenceIterator;
	typedef typename MyMap::ConstIterator			MyConstIterator;
	typedef typename ReferenceMap::const_iterator	ReferenceConstIterator;
	typedef typename MyMap::Entry					Entry;
	typedef typename MyMap::ConstEntry				ConstEntry;
	typedef TestIterator<Entry, Class, MyIterator,
						 ReferenceIterator>			Iterator;
	typedef TestIterator<ConstEntry, const Class, MyConstIterator,
						 ReferenceConstIterator>	ConstIterator;

	TestMap()
		: fMyMap(),
		  fReferenceMap(),
		  fChecking(true)
	{
	}

	void Insert(const Key &key, const Value &value)
	{
		CHK(fMyMap.Insert(key, value) == B_OK);
		fReferenceMap[key] = value;
		Check();
	}

	void Put(const Key &key, const Value &value)
	{
		CHK(fMyMap.Put(key, value) == B_OK);
		fReferenceMap[key] = value;
		Check();
	}

	Value &Get(const Key &key)
	{
		Value &value = fMyMap.Get(key);
		CHK(value == fReferenceMap[key]);
		return value;
	}

	const Value &Get(const Key &key) const
	{
		const Value &value = fMyMap.Get(key);
		CHK(value == fReferenceMap.find(key)->second);
		return value;
	}

	void Remove(const Key &key)
	{
		int32 oldCount = Count();
		ReferenceIterator it = fReferenceMap.find(key);
		if (it != fReferenceMap.end())
			fReferenceMap.erase(it);
		int32 newCount = fReferenceMap.size();
		CHK(fMyMap.Remove(key) == oldCount - newCount);
		Check();
	}

	Iterator Erase(const Iterator &iterator)
	{
		bool outOfRange
			= (iterator.fReferenceIterator == fReferenceMap.end());
		MyIterator myIt = fMyMap.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CHK(myIt == fMyMap.Null());
			return Iterator(this, myIt, fReferenceMap.end());
		}
		Key nextKey;
		ReferenceIterator refIt = iterator.fReferenceIterator;
		++refIt;
		bool noNextEntry = (refIt == fReferenceMap.end());
		if (!noNextEntry)
			nextKey = refIt->first;
		fReferenceMap.erase(iterator.fReferenceIterator);
		if (noNextEntry)
			refIt = fReferenceMap.end();
		else
			refIt = fReferenceMap.find(nextKey);
		Check();
		if (refIt == fReferenceMap.end())
			CHK(myIt == fMyMap.End());
		else {
			CHK(myIt->Key() == refIt->first);
			CHK(myIt->Value() == refIt->second);
		}
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceMap.size();
		CHK(fMyMap.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceMap.empty();
		CHK(fMyMap.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMyMap.MakeEmpty();
		fReferenceMap.clear();
		Check();
	}

	inline Iterator Begin()
	{
		return Iterator(this, fMyMap.Begin(), fReferenceMap.begin());
	}

	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMyMap.Begin(),
							 fReferenceMap.begin());
	}

	inline Iterator End()
	{
		return Iterator(this, fMyMap.End(), fReferenceMap.end());
	}

	inline ConstIterator End() const
	{
		return ConstIterator(this, fMyMap.End(), fReferenceMap.end());
	}

	inline Iterator Null()
	{
		return Iterator(this, fMyMap.Null(), fReferenceMap.end());
	}

	inline ConstIterator Null() const
	{
		return ConstIterator(this, fMyMap.Null(), fReferenceMap.end());
	}

	// for testing only
	inline Iterator IteratorForIndex(int32 index)
	{
		if (index < 0 || index > Count())
			return End();
		MyIterator myIt = fMyMap.Begin();
		ReferenceIterator refIt = fReferenceMap.begin();
		for (int32 i = 0; i < index; i++) {
			++myIt;
			++refIt;
		}
		return Iterator(this, myIt, refIt);
	}

	// for testing only
	inline ConstIterator IteratorForIndex(int32 index) const
	{
		if (index < 0 || index > Count())
			return End();
		MyConstIterator myIt = fMyMap.Begin();
		ReferenceConstIterator refIt = fReferenceMap.begin();
		for (int32 i = 0; i < index; i++) {
			++myIt;
			++refIt;
		}
		return ConstIterator(this, myIt, refIt);
	}

	Iterator Find(const Key &key)
	{
		MyIterator myIt = fMyMap.Find(key);
		ReferenceIterator refIt = fReferenceMap.find(key);
		if (refIt == fReferenceMap.end())
			CHK(myIt = fMyMap.End());
		else {
			CHK(myIt->Key() == refIt->first);
			CHK(myIt->Value() == refIt->second);
		}
		return Iterator(this, myIt, refIt);
	}

	ConstIterator Find(const Key &key) const
	{
		MyConstIterator myIt = fMyMap.Find(key);
		ReferenceConstIterator refIt = fReferenceMap.find(key);
		if (refIt == fReferenceMap.end())
			CHK(myIt = fMyMap.End());
		else {
			CHK(myIt->Key() == refIt->first);
			CHK(myIt->Value() == refIt->second);
		}
		return ConstIterator(this, myIt, refIt);
	}

	Iterator FindClose(const Key &key, bool less)
	{
		MyIterator myIt = fMyMap.FindClose(key, less);
		if (myIt == fMyMap.End()) {
			if (fMyMap.Count() > 0) {
				if (less)
					CHK(fCompare(fMyMap.Begin()->Key(), key) > 0);
				else
					CHK(fCompare((--MyIterator(myIt))->Key(), key) < 0);
			}
			return End();
		}
		if (less) {
			CHK(fCompare(myIt->Key(), key) <= 0);
			MyIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMyMap.End())
				CHK(fCompare(nextMyIt->Key(), key) > 0);
		} else {
			CHK(fCompare(myIt->Key(), key) >= 0);
			if (myIt != fMyMap.Begin()) {
				MyIterator prevMyIt(myIt);
				--prevMyIt;
				CHK(fCompare(prevMyIt->Key(), key) < 0);
			}
		}
		return Iterator(this, myIt, fReferenceMap.find(myIt->Key()));
	}

	ConstIterator FindClose(const Key &key, bool less) const
	{
		MyConstIterator myIt = fMyMap.FindClose(key, less);
		if (myIt == fMyMap.End()) {
			if (fMyMap.Count() > 0) {
				if (less)
					CHK(fCompare(fMyMap.Begin()->Key(), key) > 0);
				else
					CHK(fCompare((--MyConstIterator(myIt))->Key(), key) < 0);
			}
			return End();
		}
		if (less) {
			CHK(fCompare(myIt->Key(), key) <= 0);
			MyConstIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMyMap.End())
				CHK(fCompare(nextMyIt->Key(), key) > 0);
		} else {
			CHK(fCompare(myIt->Key(), key) >= 0);
			if (myIt != fMyMap.Begin()) {
				MyConstIterator prevMyIt(myIt);
				--prevMyIt;
				CHK(fCompare(prevMyIt->Key(), key) < 0);
			}
		}
		return ConstIterator(this, myIt, fReferenceMap.find(myIt->Key()));
	}

	void SetChecking(bool enable)
	{
		fChecking = enable;
	}

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceMap.size();
			CHK(fMyMap.Count() == count);
			CHK(fMyMap.IsEmpty() == fReferenceMap.empty());
			MyConstIterator myIt = fMyMap.Begin();
			ReferenceConstIterator refIt = fReferenceMap.begin();
			for (int32 i = 0; i < count; i++, ++myIt, ++refIt) {
				CHK(myIt->Key() == refIt->first);
				CHK(myIt->Value() == refIt->second);
				CHK((*myIt).Key() == refIt->first);
				CHK((*myIt).Value() == refIt->second);
			}
			CHK(myIt == fMyMap.End());
		}
	}

//private:
public:
	MyMap			fMyMap;
	ReferenceMap	fReferenceMap;
	bool			fChecking;
	Compare			fCompare;
};


// IntStrategy
class IntStrategy {
public:
	typedef int	Value;

	IntStrategy(int32 differentValues = 100000)
		: fDifferentValues(differentValues)
	{
		srand(0);
	}

	Value Generate()
	{
		return rand() % fDifferentValues;
	}

private:
	int32	fDifferentValues;
};

// StringStrategy
class StringStrategy {
public:
	typedef string	Value;

	StringStrategy(int32 differentValues = 100000)
		: fDifferentValues(differentValues)
	{
		srand(0);
	}

	Value Generate()
	{
		char buffer[10];
		sprintf(buffer, "%ld", rand() % fDifferentValues);
		return string(buffer);
	}

private:
	int32	fDifferentValues;
};

// CompareWrapper
template<typename Value, typename Compare>
class CompareWrapper {
public:
	inline bool operator()(const Value &a, const Value &b) const
	{
		return (fCompare(a, b) < 0);
	}

private:
	Compare	fCompare;
};

// TestStrategy
template<typename _KeyStrategy, typename _ValueStrategy,
		 template<typename> class _CompareStrategy>
class TestStrategy {
public:
	typedef _KeyStrategy										KeyStrategy;
	typedef _ValueStrategy										ValueStrategy;
	typedef typename KeyStrategy::Value							Key;
	typedef typename ValueStrategy::Value						Value;
	typedef _CompareStrategy<Key>								Compare;
	typedef CompareWrapper<Key, Compare>						BoolCompare;
	typedef VectorMap<Key, Value,
		VectorMapEntryStrategy::Pair<Key, Value, Compare> >		MyMap;
	typedef map<Key, Value, BoolCompare>						ReferenceMap;
	typedef TestMap<Key, Value, MyMap, ReferenceMap, Compare>	TestClass;
};

typedef TestStrategy<IntStrategy, IntStrategy, Ascending>
		AIntIntTestStrategy;
typedef TestStrategy<IntStrategy, StringStrategy, Ascending>
		AIntStringTestStrategy;
typedef TestStrategy<IntStrategy, IntStrategy, Descending>
		DIntIntTestStrategy;
typedef TestStrategy<IntStrategy, StringStrategy, Descending>
		DIntStringTestStrategy;

typedef TestStrategy<StringStrategy, IntStrategy, Ascending>
		AStringIntTestStrategy;
typedef TestStrategy<StringStrategy, StringStrategy, Ascending>
		AStringStringTestStrategy;
typedef TestStrategy<StringStrategy, IntStrategy, Descending>
		DStringIntTestStrategy;
typedef TestStrategy<StringStrategy, StringStrategy, Descending>
		DStringStringTestStrategy;

// GenericInsertTest
template<typename _TestStrategy>
static
void
GenericInsertTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	for (int32 i = 0; i < maxNumber; i++)
		v.Insert(keyStrategy.Generate(), valueStrategy.Generate());
}

// GenericInsertTest
template<typename _TestStrategy>
static
void
GenericInsertTest()
{
	GenericInsertTest<_TestStrategy>(30);
	GenericInsertTest<_TestStrategy>(200);
}

// InsertTest
void
VectorMapTest::InsertTest()
{
	NextSubTest();
	GenericInsertTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericInsertTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericInsertTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericInsertTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericInsertTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericInsertTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericInsertTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericInsertTest<DStringStringTestStrategy>();
}

// GenericPutTest
template<typename _TestStrategy>
static
void
GenericPutTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	for (int32 i = 0; i < maxNumber; i++)
		v.Put(keyStrategy.Generate(), valueStrategy.Generate());
}

// GenericPutTest
template<typename _TestStrategy>
static
void
GenericPutTest()
{
	GenericPutTest<_TestStrategy>(30);
	GenericPutTest<_TestStrategy>(200);
}

// PutTest
void
VectorMapTest::PutTest()
{
	NextSubTest();
	GenericPutTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericPutTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericPutTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericPutTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericPutTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericPutTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericPutTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericPutTest<DStringStringTestStrategy>();
}

// GenericGetTest
template<typename _TestStrategy>
static
void
GenericGetTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	const TestClass &cv = v;
	// find the keys in the map
	for (int32 i = 0; i < maxNumber; i++) {
		Iterator it = v.IteratorForIndex(i);
		const Key &key = it->Key();
		const Value &value = it->Value();
		CHK(&v.Get(key) == &value);
		CHK(&cv.Get(key) == &value);
	}
}

// GenericGetTest
template<typename _TestStrategy>
static
void
GenericGetTest()
{
	GenericGetTest<_TestStrategy>(30);
	GenericGetTest<_TestStrategy>(200);
}

// GetTest
void
VectorMapTest::GetTest()
{
	NextSubTest();
	GenericGetTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericGetTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericGetTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericGetTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericGetTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericGetTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericGetTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericGetTest<DStringStringTestStrategy>();
}

// GenericFill
template<typename TestClass, typename KeyStrategy, typename ValueStrategy>
static
void
GenericFill(TestClass &v, KeyStrategy keyStrategy, ValueStrategy valueStrategy,
			int32 maxNumber)
{
	v.SetChecking(false);
	for (int32 i = 0; v.Count() < maxNumber; i++)
		v.Put(keyStrategy.Generate(), valueStrategy.Generate());
	v.SetChecking(true);
	v.Check();
}

// GenericRemoveTest
template<typename _TestStrategy>
static
void
GenericRemoveTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	while (v.Count() > 0) {
		int32 index = rand() % (v.Count());
		Key key = v.IteratorForIndex(index)->Key();
		v.Remove(key);
		v.Remove(key);
	}
}

// GenericRemoveTest
template<typename _TestStrategy>
static
void
GenericRemoveTest()
{
	GenericRemoveTest<_TestStrategy>(30);
	GenericRemoveTest<_TestStrategy>(200);
}

// RemoveTest
void
VectorMapTest::RemoveTest()
{
	NextSubTest();
	GenericRemoveTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericRemoveTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericRemoveTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericRemoveTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericRemoveTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericRemoveTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericRemoveTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericRemoveTest<DStringStringTestStrategy>();
}

// GenericEraseTest
template<typename _TestStrategy>
static
void
GenericEraseTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	for (int32 i = maxNumber - 1; i >= 0; i--) {
		int32 index = rand() % (i + 1);
		v.Erase(v.IteratorForIndex(index));
	}
}

// GenericEraseTest
template<typename _TestStrategy>
static
void
GenericEraseTest()
{
	GenericEraseTest<_TestStrategy>(30);
	GenericEraseTest<_TestStrategy>(200);
}

// EraseTest
void
VectorMapTest::EraseTest()
{
	NextSubTest();
	GenericEraseTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericEraseTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericEraseTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericEraseTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericEraseTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericEraseTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericEraseTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericEraseTest<DStringStringTestStrategy>();
}

// GenericMakeEmptyTest
template<typename _TestStrategy>
static
void
GenericMakeEmptyTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	v.MakeEmpty();
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	v.MakeEmpty();
	v.MakeEmpty();
}

// GenericMakeEmptyTest
template<typename _TestStrategy>
static
void
GenericMakeEmptyTest()
{
	GenericMakeEmptyTest<_TestStrategy>(30);
	GenericMakeEmptyTest<_TestStrategy>(200);
}

// MakeEmptyTest
void
VectorMapTest::MakeEmptyTest()
{
	NextSubTest();
	GenericMakeEmptyTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericMakeEmptyTest<DStringStringTestStrategy>();
}

// GenericFindTest
template<typename _TestStrategy>
static
void
GenericFindTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	const TestClass &cv = v;
	// find the values in the set
	for (int32 i = 0; i < maxNumber; i++) {
		const Key &key = v.IteratorForIndex(i)->Key();
		Iterator it = v.Find(key);
		ConstIterator cit = cv.Find(key);
		CHK(&it->Key() == &key);
		CHK(&it->Key() == &cit->Key());
		CHK(&(*it).Key() == &(*it).Key());
		CHK(&it->Value() == &cit->Value());
		CHK(&(*it).Value() == &(*it).Value());
	}
	// try to find some random values
	for (int32 i = 0; i < maxNumber; i++) {
		const Key &key = v.IteratorForIndex(i)->Key();
		Iterator it = v.Find(key);
		ConstIterator cit = cv.Find(key);
		if (it != v.End()) {
			CHK(&it->Key() == &key);
			CHK(&it->Key() == &cit->Key());
			CHK(&(*it).Key() == &(*it).Key());
			CHK(&it->Value() == &cit->Value());
			CHK(&(*it).Value() == &(*it).Value());
		}
	}
}

// GenericFindTest
template<typename _TestStrategy>
static
void
GenericFindTest()
{
	GenericFindTest<_TestStrategy>(30);
	GenericFindTest<_TestStrategy>(200);
}

// FindTest
void
VectorMapTest::FindTest()
{
	NextSubTest();
	GenericFindTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericFindTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericFindTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericFindTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericFindTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericFindTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericFindTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericFindTest<DStringStringTestStrategy>();
}

// GenericFindCloseTest
template<typename _TestStrategy>
static
void
GenericFindCloseTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	const TestClass &cv = v;
	// find the values in the set
	for (int32 i = 0; i < maxNumber; i++) {
		const Key &key = v.IteratorForIndex(i)->Key();
		// less
		Iterator it = v.FindClose(key, true);
		ConstIterator cit = cv.FindClose(key, true);
		CHK(&it->Key() == &key);
		CHK(&it->Key() == &cit->Key());
		CHK(&(*it).Key() == &(*it).Key());
		CHK(&it->Value() == &cit->Value());
		CHK(&(*it).Value() == &(*it).Value());
		// greater
		it = v.FindClose(key, false);
		cit = cv.FindClose(key, false);
		CHK(&it->Key() == &key);
		CHK(&it->Key() == &cit->Key());
		CHK(&(*it).Key() == &(*it).Key());
		CHK(&it->Value() == &cit->Value());
		CHK(&(*it).Value() == &(*it).Value());
	}
	// try to find some random values
	for (int32 i = 0; i < maxNumber; i++) {
		Key key = keyStrategy.Generate();
		// less
		Iterator it = v.FindClose(key, true);
		ConstIterator cit = cv.FindClose(key, true);
		if (it != v.End()) {
			CHK(&it->Key() == &cit->Key());
			CHK(&(*it).Key() == &(*it).Key());
			CHK(&it->Value() == &cit->Value());
			CHK(&(*it).Value() == &(*it).Value());
		}
		// greater
		it = v.FindClose(key, false);
		cit = cv.FindClose(key, false);
		if (it != v.End()) {
			CHK(&it->Key() == &cit->Key());
			CHK(&(*it).Key() == &(*it).Key());
			CHK(&it->Value() == &cit->Value());
			CHK(&(*it).Value() == &(*it).Value());
		}
	}
}

// GenericFindCloseTest
template<typename _TestStrategy>
static
void
GenericFindCloseTest()
{
	GenericFindCloseTest<_TestStrategy>(30);
	GenericFindCloseTest<_TestStrategy>(200);
}

// FindCloseTest
void
VectorMapTest::FindCloseTest()
{
	NextSubTest();
	GenericFindCloseTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericFindCloseTest<DStringStringTestStrategy>();
}

// GenericIteratorTest
template<typename _TestStrategy>
static
void
GenericIteratorTest(int32 maxNumber)
{
	typedef typename _TestStrategy::KeyStrategy		KeyStrategy;
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Key				Key;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	KeyStrategy keyStrategy;
	ValueStrategy valueStrategy;
	TestClass v;
	GenericFill(v, keyStrategy, valueStrategy, maxNumber);
	const TestClass &cv = v;
	Iterator it = v.Begin();
	ConstIterator cit = cv.Begin();
	for (; it != v.End(); ++it, ++cit) {
		CHK(&it->Key() == &cit->Key());
		CHK(&it->Value() == &cit->Value());
		CHK(&it->Key() == &(*it).Key());
		CHK(&cit->Key() == &(*cit).Key());
		CHK(&it->Value() == &(*it).Value());
		CHK(&cit->Value() == &(*cit).Value());
		CHK(&it->Key() == &it.operator->().Key());
		CHK(&it->Value() == &it.operator->().Value());
		CHK(&cit->Key() == &cit.operator->().Key());
		CHK(&cit->Value() == &cit.operator->().Value());
		CHK(it);
		CHK(cit);
	}
	CHK(cit == cv.End());
	while (it != v.Begin()) {
		--it;
		--cit;
		CHK(&it->Key() == &cit->Key());
		CHK(&it->Value() == &cit->Value());
		CHK(&it->Key() == &(*it).Key());
		CHK(&cit->Key() == &(*cit).Key());
		CHK(&it->Value() == &(*it).Value());
		CHK(&cit->Value() == &(*cit).Value());
		CHK(&it->Key() == &it.operator->().Key());
		CHK(&it->Value() == &it.operator->().Value());
		CHK(&cit->Key() == &cit.operator->().Key());
		CHK(&cit->Value() == &cit.operator->().Value());
		CHK(it);
		CHK(cit);
	}
	CHK(cit == cv.Begin());
	CHK(!v.Null());
	CHK(!cv.Null());
}

// GenericIteratorTest
template<typename _TestStrategy>
static
void
GenericIteratorTest()
{
	GenericIteratorTest<_TestStrategy>(30);
	GenericIteratorTest<_TestStrategy>(200);
}

// IteratorTest
void
VectorMapTest::IteratorTest()
{
	NextSubTest();
	GenericIteratorTest<AIntIntTestStrategy>();
	NextSubTest();
	GenericIteratorTest<DIntIntTestStrategy>();
	NextSubTest();
	GenericIteratorTest<AIntStringTestStrategy>();
	NextSubTest();
	GenericIteratorTest<DIntStringTestStrategy>();
	NextSubTest();
	GenericIteratorTest<AStringIntTestStrategy>();
	NextSubTest();
	GenericIteratorTest<DStringIntTestStrategy>();
	NextSubTest();
	GenericIteratorTest<AStringStringTestStrategy>();
	NextSubTest();
	GenericIteratorTest<DStringStringTestStrategy>();
}

