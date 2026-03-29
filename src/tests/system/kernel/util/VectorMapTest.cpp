/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <map>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <VectorMap.h>

#include <KernelUtilsOrder.h>


template<typename _Value> class SimpleValueStrategy {
public:
	typedef _Value Value;

	SimpleValueStrategy(int32 differentValues = 100000)
		:
		fDifferentValues(differentValues)
	{
		srand(0);
	}

	Value Generate();

private:
	int32 fDifferentValues;
};


template<>
int
SimpleValueStrategy<int>::Generate()
{
	return rand() % fDifferentValues;
}


template<>
string
SimpleValueStrategy<string>::Generate()
{
	char buffer[10];
	sprintf(buffer, "%ld", rand() % fDifferentValues);
	return string(buffer);
}


template<typename _KeyStrategy, typename _ValueStrategy>
class PairEntryStrategy {
public:
	typedef _KeyStrategy					KeyStrategy;
	typedef _ValueStrategy					ValueStrategy;
	typedef typename KeyStrategy::Value		Key;
	typedef typename ValueStrategy::Value	Value;

	inline Key GenerateKey() { return fKeyStrategy.Generate(); }

	inline Value GenerateValue() { return fValueStrategy.Generate(); }

	inline void Generate(Key& key, Value& value)
	{
		key = GenerateKey();
		value = GenerateValue();
	}

private:
	KeyStrategy fKeyStrategy;
	ValueStrategy fValueStrategy;
};


template<typename _KeyStrategy, typename _ValueStrategy, typename GetKey>
class ImplicitKeyStrategy {
public:
	typedef _KeyStrategy					KeyStrategy;
	typedef _ValueStrategy					ValueStrategy;
	typedef typename KeyStrategy::Value		Key;
	typedef typename ValueStrategy::Value	Value;

	inline Key GenerateKey() { return fKeyStrategy.Generate(); }
	inline Value GenerateValue() { return fValueStrategy.Generate(); }
	inline void Generate(Key& key, Value& value)
	{
		value = GenerateValue();
		key = fGetKey(value);
	}

private:
	KeyStrategy fKeyStrategy;
	ValueStrategy fValueStrategy;
	GetKey fGetKey;
};


// Non-template wrapper for the Ascending compare strategy.
// Work-around for our compiler not eating nested template template
// parameters.
struct Ascending {
	template<typename Value>
	class Strategy : public KernelUtilsOrder::Ascending<Value> {};
};


// Non-template wrapper for the Descending compare strategy.
// Work-around for our compiler not eating nested template template
// parameters.
struct Descending {
	template<typename Value>
	class Strategy : public KernelUtilsOrder::Descending<Value> {};
};


template<typename Value, typename Compare>
class CompareWrapper {
public:
	inline bool operator()(const Value& a, const Value& b) const
		{ return fCompare(a, b) < 0; }

private:
	Compare fCompare;
};


template<typename Entry, typename TestMap, typename MyIterator, typename ReferenceIterator>
class TestIterator {
private:
	typedef TestIterator<Entry, TestMap, MyIterator, ReferenceIterator> Iterator;

public:
	inline TestIterator(TestMap* s, MyIterator myIt, ReferenceIterator refIt)
		:
		fMap(s),
		fMyIterator(myIt),
		fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator& other)
		:
		fMap(other.fMap),
		fMyIterator(other.fMyIterator),
		fReferenceIterator(other.fReferenceIterator)
	{
		CPPUNIT_ASSERT(fMyIterator == other.fMyIterator);
	}

	inline Iterator& operator++()
	{
		MyIterator& myResult = ++fMyIterator;
		ReferenceIterator& refResult = ++fReferenceIterator;
		if (refResult == fMap->fReferenceMap.end()) {
			CPPUNIT_ASSERT(myResult == fMap->fMyMap.End());
		} else {
			CPPUNIT_ASSERT(myResult->Key() == refResult->first);
			CPPUNIT_ASSERT(myResult->Value() == refResult->second);
		}
		return *this;
	}

	inline Iterator operator++(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator++;
		ReferenceIterator refResult = fReferenceIterator++;
		CPPUNIT_ASSERT(oldMyResult == myResult);
		if (refResult == fMap->fReferenceMap.end()) {
			CPPUNIT_ASSERT(myResult == fMap->fMyMap.End());
		} else {
			CPPUNIT_ASSERT(myResult->Key() == refResult->first);
			CPPUNIT_ASSERT(myResult->Value() == refResult->second);
		}
		return Iterator(fMap, myResult, refResult);
	}

	inline Iterator& operator--()
	{
		MyIterator& myResult = --fMyIterator;
		ReferenceIterator& refResult = --fReferenceIterator;
		CPPUNIT_ASSERT(myResult->Key() == refResult->first);
		CPPUNIT_ASSERT(myResult->Value() == refResult->second);
		return *this;
	}

	inline Iterator operator--(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator--;
		ReferenceIterator refResult = fReferenceIterator--;
		CPPUNIT_ASSERT(oldMyResult == myResult);
		CPPUNIT_ASSERT(myResult->Key() == refResult->first);
		CPPUNIT_ASSERT(myResult->Value() == refResult->second);
		return Iterator(fMap, myResult, refResult);
	}

	inline Iterator& operator=(const Iterator& other)
	{
		fMap = other.fMap;
		fMyIterator = other.fMyIterator;
		fReferenceIterator = other.fReferenceIterator;
		CPPUNIT_ASSERT(fMyIterator == other.fMyIterator);
		return *this;
	}

	inline bool operator==(const Iterator& other) const
	{
		bool result = (fMyIterator == other.fMyIterator);
		CPPUNIT_ASSERT((fReferenceIterator == other.fReferenceIterator) == result);
		return result;
	}

	inline bool operator!=(const Iterator& other) const
	{
		bool result = (fMyIterator != other.fMyIterator);
		CPPUNIT_ASSERT((fReferenceIterator != other.fReferenceIterator) == result);
		return result;
	}

	inline Entry operator*() const
	{
		Entry entry = *fMyIterator;
		CPPUNIT_ASSERT(entry.Key() == fReferenceIterator->first);
		CPPUNIT_ASSERT(entry.Value() == fReferenceIterator->second);
		return entry;
	}

	inline Entry operator->() const
	{
		Entry entry = fMyIterator.operator->();
		CPPUNIT_ASSERT(entry.Key() == fReferenceIterator->first);
		CPPUNIT_ASSERT(entry.Value() == fReferenceIterator->second);
		return entry;
	}

	inline operator bool() const
	{
		bool result = fMyIterator;
		CPPUNIT_ASSERT((fMyIterator == fMap->fMyMap.Null()) != result);
		return result;
	}

public:
	TestMap* fMap;
	MyIterator fMyIterator;
	ReferenceIterator fReferenceIterator;
};


template<typename Key, typename Value, typename MyMap, typename ReferenceMap, typename Compare>
class TestMap {
public:
	typedef TestMap<Key, Value, MyMap, ReferenceMap, Compare>			Class;

	typedef typename MyMap::Iterator									MyIterator;
	typedef typename ReferenceMap::iterator								ReferenceIterator;
	typedef typename MyMap::ConstIterator 								MyConstIterator;
	typedef typename ReferenceMap::const_iterator 						ReferenceConstIterator;
	typedef typename MyMap::Entry										Entry;
	typedef typename MyMap::ConstEntry									ConstEntry;
	typedef TestIterator<Entry, Class, MyIterator, ReferenceIterator>	Iterator;
	typedef TestIterator<ConstEntry, const Class, MyConstIterator, ReferenceConstIterator>
		ConstIterator;

	TestMap()
		:
		fMyMap(),
		fReferenceMap(),
		fChecking(true)
	{
	}

	void Insert(const Key& key, const Value& value)
	{
		CPPUNIT_ASSERT(fMyMap.Insert(key, value) == B_OK);
		fReferenceMap[key] = value;
		Check();
	}

	void Put(const Key& key, const Value& value)
	{
		CPPUNIT_ASSERT(fMyMap.Put(key, value) == B_OK);
		fReferenceMap[key] = value;
		Check();
	}

	Value& Get(const Key& key)
	{
		Value& value = fMyMap.Get(key);
		CPPUNIT_ASSERT(value == fReferenceMap[key]);
		return value;
	}

	const Value& Get(const Key& key) const
	{
		const Value& value = fMyMap.Get(key);
		CPPUNIT_ASSERT(value == fReferenceMap.find(key)->second);
		return value;
	}

	void Remove(const Key& key)
	{
		int32 oldCount = Count();
		ReferenceIterator it = fReferenceMap.find(key);
		if (it != fReferenceMap.end())
			fReferenceMap.erase(it);
		int32 newCount = fReferenceMap.size();
		CPPUNIT_ASSERT(fMyMap.Remove(key) == oldCount - newCount);
		Check();
	}

	Iterator Erase(const Iterator& iterator)
	{
		bool outOfRange = (iterator.fReferenceIterator == fReferenceMap.end());
		MyIterator myIt = fMyMap.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CPPUNIT_ASSERT(myIt == fMyMap.Null());
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
		if (refIt == fReferenceMap.end()) {
			CPPUNIT_ASSERT(myIt == fMyMap.End());
		} else {
			CPPUNIT_ASSERT(myIt->Key() == refIt->first);
			CPPUNIT_ASSERT(myIt->Value() == refIt->second);
		}
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceMap.size();
		CPPUNIT_ASSERT(fMyMap.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceMap.empty();
		CPPUNIT_ASSERT(fMyMap.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMyMap.MakeEmpty();
		fReferenceMap.clear();
		Check();
	}

	inline Iterator Begin() { return Iterator(this, fMyMap.Begin(), fReferenceMap.begin()); }

	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMyMap.Begin(), fReferenceMap.begin());
	}

	inline Iterator End() { return Iterator(this, fMyMap.End(), fReferenceMap.end()); }

	inline ConstIterator End() const
	{
		return ConstIterator(this, fMyMap.End(), fReferenceMap.end());
	}

	inline Iterator Null() { return Iterator(this, fMyMap.Null(), fReferenceMap.end()); }

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

	Iterator Find(const Key& key)
	{
		MyIterator myIt = fMyMap.Find(key);
		ReferenceIterator refIt = fReferenceMap.find(key);
		if (refIt == fReferenceMap.end()) {
			CPPUNIT_ASSERT(myIt = fMyMap.End());
		} else {
			CPPUNIT_ASSERT(myIt->Key() == refIt->first);
			CPPUNIT_ASSERT(myIt->Value() == refIt->second);
		}
		return Iterator(this, myIt, refIt);
	}

	ConstIterator Find(const Key& key) const
	{
		MyConstIterator myIt = fMyMap.Find(key);
		ReferenceConstIterator refIt = fReferenceMap.find(key);
		if (refIt == fReferenceMap.end()) {
			CPPUNIT_ASSERT(myIt = fMyMap.End());
		} else {
			CPPUNIT_ASSERT(myIt->Key() == refIt->first);
			CPPUNIT_ASSERT(myIt->Value() == refIt->second);
		}
		return ConstIterator(this, myIt, refIt);
	}

	Iterator FindClose(const Key& key, bool less)
	{
		MyIterator myIt = fMyMap.FindClose(key, less);
		if (myIt == fMyMap.End()) {
			if (fMyMap.Count() > 0) {
				if (less)
					CPPUNIT_ASSERT(fCompare(fMyMap.Begin()->Key(), key) > 0);
				else
					CPPUNIT_ASSERT(fCompare((--MyIterator(myIt))->Key(), key) < 0);
			}
			return End();
		}
		if (less) {
			CPPUNIT_ASSERT(fCompare(myIt->Key(), key) <= 0);
			MyIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMyMap.End())
				CPPUNIT_ASSERT(fCompare(nextMyIt->Key(), key) > 0);
		} else {
			CPPUNIT_ASSERT(fCompare(myIt->Key(), key) >= 0);
			if (myIt != fMyMap.Begin()) {
				MyIterator prevMyIt(myIt);
				--prevMyIt;
				CPPUNIT_ASSERT(fCompare(prevMyIt->Key(), key) < 0);
			}
		}
		return Iterator(this, myIt, fReferenceMap.find(myIt->Key()));
	}

	ConstIterator FindClose(const Key& key, bool less) const
	{
		MyConstIterator myIt = fMyMap.FindClose(key, less);
		if (myIt == fMyMap.End()) {
			if (fMyMap.Count() > 0) {
				if (less)
					CPPUNIT_ASSERT(fCompare(fMyMap.Begin()->Key(), key) > 0);
				else
					CPPUNIT_ASSERT(fCompare((--MyConstIterator(myIt))->Key(), key) < 0);
			}
			return End();
		}
		if (less) {
			CPPUNIT_ASSERT(fCompare(myIt->Key(), key) <= 0);
			MyConstIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMyMap.End())
				CPPUNIT_ASSERT(fCompare(nextMyIt->Key(), key) > 0);
		} else {
			CPPUNIT_ASSERT(fCompare(myIt->Key(), key) >= 0);
			if (myIt != fMyMap.Begin()) {
				MyConstIterator prevMyIt(myIt);
				--prevMyIt;
				CPPUNIT_ASSERT(fCompare(prevMyIt->Key(), key) < 0);
			}
		}
		return ConstIterator(this, myIt, fReferenceMap.find(myIt->Key()));
	}

	void SetChecking(bool enable) { fChecking = enable; }

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceMap.size();
			CPPUNIT_ASSERT(fMyMap.Count() == count);
			CPPUNIT_ASSERT(fMyMap.IsEmpty() == fReferenceMap.empty());
			MyConstIterator myIt = fMyMap.Begin();
			ReferenceConstIterator refIt = fReferenceMap.begin();
			for (int32 i = 0; i < count; i++, ++myIt, ++refIt) {
				CPPUNIT_ASSERT(myIt->Key() == refIt->first);
				CPPUNIT_ASSERT(myIt->Value() == refIt->second);
				CPPUNIT_ASSERT((*myIt).Key() == refIt->first);
				CPPUNIT_ASSERT((*myIt).Value() == refIt->second);
			}
			CPPUNIT_ASSERT(myIt == fMyMap.End());
		}
	}

public:
	MyMap fMyMap;
	ReferenceMap fReferenceMap;
	bool fChecking;
	Compare fCompare;
};


template<template<typename> class _MyMap, typename _EntryStrategy,
	//		 template<typename> class _CompareStrategy>
	class CompareStrategyWrapper>
class TestStrategy {
public:
	typedef _EntryStrategy EntryStrategy;
	typedef typename EntryStrategy::KeyStrategy						KeyStrategy;
	typedef typename EntryStrategy::ValueStrategy					ValueStrategy;
	typedef typename KeyStrategy::Value								Key;
	typedef typename ValueStrategy::Value							Value;
	//	typedef _CompareStrategy<Key>								Compare;
	typedef typename CompareStrategyWrapper::template Strategy<Key>	Compare;
	typedef CompareWrapper<Key, Compare>							BoolCompare;
	typedef _MyMap<Compare>											MyMap;
	typedef std::map<Key, Value, BoolCompare>						ReferenceMap;
	typedef TestMap<Key, Value, MyMap, ReferenceMap, Compare>		TestClass;
};


template<class TestStrategy>
class VectorMapTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VectorMapTest);
	CPPUNIT_TEST(ConstructorTest);
	CPPUNIT_TEST(Insert30ElementsTest);
	CPPUNIT_TEST(Insert200ElementsTest);
	CPPUNIT_TEST(Put30ElementsTest);
	CPPUNIT_TEST(Put200ElementsTest);
	CPPUNIT_TEST(Get30ElementsTest);
	CPPUNIT_TEST(Get200ElementsTest);
	CPPUNIT_TEST(Remove30ElementsTest);
	CPPUNIT_TEST(Remove200ElementsTest);
	CPPUNIT_TEST(Erase30ElementsTest);
	CPPUNIT_TEST(Erase200ElementsTest);
	CPPUNIT_TEST(MakeEmpty30ElementsTest);
	CPPUNIT_TEST(MakeEmpty200ElementsTest);
	CPPUNIT_TEST(Find30ElementsTest);
	CPPUNIT_TEST(Find200ElementsTest);
	CPPUNIT_TEST(FindClose30ElementsTest);
	CPPUNIT_TEST(FindClose200ElementsTest);
	CPPUNIT_TEST(Iterator30ElementsTest);
	CPPUNIT_TEST(Iterator200ElementsTest);
	CPPUNIT_TEST_SUITE_END();

	typedef typename TestStrategy::EntryStrategy	EntryStrategy;
	typedef typename TestStrategy::Key				Key;
	typedef typename TestStrategy::Value			Value;
	typedef typename TestStrategy::TestClass		TestClass;
	typedef typename TestStrategy::MyMap			MyMap;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;

	void InsertTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++) {
			Key key;
			Value value;
			entryStrategy.Generate(key, value);
			v.Insert(key, value);
		}
	}

	void PutTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++) {
			Key key;
			Value value;
			entryStrategy.Generate(key, value);
			v.Put(key, value);
		}
	}

	void GetTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		const TestClass& cv = v;
		// find the keys in the map
		for (int32 i = 0; i < maxNumber; i++) {
			Iterator it = v.IteratorForIndex(i);
			Key key = it->Key();
			const Value& value = it->Value();
			CPPUNIT_ASSERT(&v.Get(key) == &value);
			CPPUNIT_ASSERT(&cv.Get(key) == &value);
		}
	}

	void Fill(TestClass& v, EntryStrategy strategy, int32 maxNumber)
	{
		typedef typename EntryStrategy::Key Key;
		typedef typename EntryStrategy::Value Value;
		v.SetChecking(false);
		for (int32 i = 0; v.Count() < maxNumber; i++) {
			Key key;
			Value value;
			strategy.Generate(key, value);
			v.Put(key, value);
		}
		v.SetChecking(true);
		v.Check();
	}

	void RemoveTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		while (v.Count() > 0) {
			int32 index = rand() % (v.Count());
			Key key = v.IteratorForIndex(index)->Key();
			v.Remove(key);
			v.Remove(key);
		}
	}

	void EraseTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		for (int32 i = maxNumber - 1; i >= 0; i--) {
			int32 index = rand() % (i + 1);
			v.Erase(v.IteratorForIndex(index));
		}
	}

	void MakeEmptyTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		v.MakeEmpty();
		Fill(v, entryStrategy, maxNumber);
		v.MakeEmpty();
		v.MakeEmpty();
	}

	void FindTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		const TestClass& cv = v;
		// find the values in the set
		for (int32 i = 0; i < maxNumber; i++) {
			Key key = v.IteratorForIndex(i)->Key();
			Iterator it = v.Find(key);
			ConstIterator cit = cv.Find(key);
			CPPUNIT_ASSERT(it->Key() == key);
			CPPUNIT_ASSERT(it->Key() == cit->Key());
			CPPUNIT_ASSERT((*it).Key() == (*it).Key());
			CPPUNIT_ASSERT(&it->Value() == &cit->Value());
			CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
		}
		// try to find some random values
		for (int32 i = 0; i < maxNumber; i++) {
			Key key = v.IteratorForIndex(i)->Key();
			Iterator it = v.Find(key);
			ConstIterator cit = cv.Find(key);
			if (it != v.End()) {
				CPPUNIT_ASSERT(it->Key() == key);
				CPPUNIT_ASSERT(it->Key() == cit->Key());
				CPPUNIT_ASSERT((*it).Key() == (*it).Key());
				CPPUNIT_ASSERT(&it->Value() == &cit->Value());
				CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
			}
		}
	}

	void FindCloseTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		const TestClass& cv = v;
		// find the values in the set
		for (int32 i = 0; i < maxNumber; i++) {
			Key key = v.IteratorForIndex(i)->Key();
			// less
			Iterator it = v.FindClose(key, true);
			ConstIterator cit = cv.FindClose(key, true);
			CPPUNIT_ASSERT(it->Key() == key);
			CPPUNIT_ASSERT(it->Key() == cit->Key());
			CPPUNIT_ASSERT((*it).Key() == (*it).Key());
			CPPUNIT_ASSERT(&it->Value() == &cit->Value());
			CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
			// greater
			it = v.FindClose(key, false);
			cit = cv.FindClose(key, false);
			CPPUNIT_ASSERT(it->Key() == key);
			CPPUNIT_ASSERT(it->Key() == cit->Key());
			CPPUNIT_ASSERT((*it).Key() == (*it).Key());
			CPPUNIT_ASSERT(&it->Value() == &cit->Value());
			CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
		}
		// try to find some random values
		for (int32 i = 0; i < maxNumber; i++) {
			Key key = entryStrategy.GenerateKey();
			// less
			Iterator it = v.FindClose(key, true);
			ConstIterator cit = cv.FindClose(key, true);
			if (it != v.End()) {
				CPPUNIT_ASSERT(it->Key() == cit->Key());
				CPPUNIT_ASSERT((*it).Key() == (*it).Key());
				CPPUNIT_ASSERT(&it->Value() == &cit->Value());
				CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
			}
			// greater
			it = v.FindClose(key, false);
			cit = cv.FindClose(key, false);
			if (it != v.End()) {
				CPPUNIT_ASSERT(it->Key() == cit->Key());
				CPPUNIT_ASSERT((*it).Key() == (*it).Key());
				CPPUNIT_ASSERT(&it->Value() == &cit->Value());
				CPPUNIT_ASSERT(&(*it).Value() == &(*it).Value());
			}
		}
	}

	void IteratorTest(int32 maxNumber)
	{
		EntryStrategy entryStrategy;
		TestClass v;
		Fill(v, entryStrategy, maxNumber);
		const TestClass& cv = v;
		Iterator it = v.Begin();
		ConstIterator cit = cv.Begin();
		for (; it != v.End(); ++it, ++cit) {
			CPPUNIT_ASSERT(it->Key() == cit->Key());
			CPPUNIT_ASSERT(&it->Value() == &cit->Value());
			CPPUNIT_ASSERT(it->Key() == (*it).Key());
			CPPUNIT_ASSERT(cit->Key() == (*cit).Key());
			CPPUNIT_ASSERT(&it->Value() == &(*it).Value());
			CPPUNIT_ASSERT(&cit->Value() == &(*cit).Value());
			CPPUNIT_ASSERT(it->Key() == it.operator->().Key());
			CPPUNIT_ASSERT(&it->Value() == &it.operator->().Value());
			CPPUNIT_ASSERT(cit->Key() == cit.operator->().Key());
			CPPUNIT_ASSERT(&cit->Value() == &cit.operator->().Value());
			CPPUNIT_ASSERT(it);
			CPPUNIT_ASSERT(cit);
		}
		CPPUNIT_ASSERT(cit == cv.End());
		while (it != v.Begin()) {
			--it;
			--cit;
			CPPUNIT_ASSERT(it->Key() == cit->Key());
			CPPUNIT_ASSERT(&it->Value() == &cit->Value());
			CPPUNIT_ASSERT(it->Key() == (*it).Key());
			CPPUNIT_ASSERT(cit->Key() == (*cit).Key());
			CPPUNIT_ASSERT(&it->Value() == &(*it).Value());
			CPPUNIT_ASSERT(&cit->Value() == &(*cit).Value());
			CPPUNIT_ASSERT(it->Key() == it.operator->().Key());
			CPPUNIT_ASSERT(&it->Value() == &it.operator->().Value());
			CPPUNIT_ASSERT(cit->Key() == cit.operator->().Key());
			CPPUNIT_ASSERT(&cit->Value() == &cit.operator->().Value());
			CPPUNIT_ASSERT(it);
			CPPUNIT_ASSERT(cit);
		}
		CPPUNIT_ASSERT(cit == cv.Begin());
		CPPUNIT_ASSERT(!v.Null());
		CPPUNIT_ASSERT(!cv.Null());
	}

public:
	void ConstructorTest()
	{
		MyMap v1;
		CPPUNIT_ASSERT(v1.Count() == 0);
		CPPUNIT_ASSERT(v1.IsEmpty());
	}

	void Put30ElementsTest() { PutTest(30); }
	void Put200ElementsTest() { PutTest(200); }
	void Get30ElementsTest() { GetTest(30); }
	void Get200ElementsTest() { GetTest(200); }
	void Insert30ElementsTest() { InsertTest(30); }
	void Insert200ElementsTest() { InsertTest(200); }
	void Remove30ElementsTest() { RemoveTest(30); }
	void Remove200ElementsTest() { RemoveTest(200); }
	void Erase30ElementsTest() { EraseTest(30); }
	void Erase200ElementsTest() { EraseTest(200); }
	void MakeEmpty30ElementsTest() { MakeEmptyTest(30); }
	void MakeEmpty200ElementsTest() { MakeEmptyTest(200); }
	void Find30ElementsTest() { FindTest(30); }
	void Find200ElementsTest() { FindTest(200); }
	void FindClose30ElementsTest() { FindCloseTest(30); }
	void FindClose200ElementsTest() { FindCloseTest(200); }
	void Iterator30ElementsTest() { IteratorTest(30); }
	void Iterator200ElementsTest() { IteratorTest(200); }
};


// That's how it should work, if we had a working compiler.
/*
template<typename Key, typename Value>
struct PairTestBase {
	typedef SimpleValueStrategy<Key>						KeyStrategy;
	typedef SimpleValueStrategy<Value>						ValueStrategy;
	typedef PairEntryStrategy<KeyStrategy, ValueStrategy>	EntryStrategy;

	template<typename CompareStrategy>
	struct MyMap
		: public VectorMap<Key, Value, VectorMapEntryStrategy::Pair<Key, Value,
						   CompareStrategy> > {
	};

	template<class CompareStrategy>
	struct Strategy : public TestStrategy<MyMap, EntryStrategy,
										  CompareStrategy> {
	};
};
typedef typename PairTestBase<int, int>::Strategy	IntIntTestStrategy;
// ...
*/

// ugly work-around:
template<typename Key, typename Value>
struct PairTestBase {
	typedef SimpleValueStrategy<Key>						KeyStrategy;
	typedef SimpleValueStrategy<Value>						ValueStrategy;
	typedef PairEntryStrategy<KeyStrategy, ValueStrategy>	EntryStrategy;

	template<typename CompareStrategy>
	struct MyMap
		: public VectorMap<Key, Value, VectorMapEntryStrategy::Pair<Key, Value, CompareStrategy> > {
	};
};


#define DECLARE_TEST_STRATEGY(Key, Value, Map, Strategy) \
	template<typename CS> struct Map : PairTestBase<Key, Value>::MyMap<CS> {}; \
	template<typename CS> \
	struct Strategy : public TestStrategy<Map, PairTestBase<Key, Value>::EntryStrategy, CS> {};


DECLARE_TEST_STRATEGY(int, int, IntIntMap, IntIntTestStrategy)
DECLARE_TEST_STRATEGY(int, string, IntStringMap, IntStringTestStrategy)
DECLARE_TEST_STRATEGY(string, int, StringIntMap, StringIntTestStrategy)
DECLARE_TEST_STRATEGY(string, string, StringStringMap, StringStringTestStrategy)


// TestStrategy for the ImplicitKey entry strategy

// string_hash (from the Dragon Book: a slightly modified hashpjw())
static inline int
string_hash(const char* name)
{
	uint32 h = 0;
	for (; *name; name++) {
		if (uint32 g = h & 0xf0000000)
			h ^= g >> 24;
		h = (h << 4) + *name;
	}
	return (int)h;
}


struct ImplicitKeyTestGetKey {
	int operator()(string value) const { return string_hash(value.c_str()); }
};


template<typename CompareStrategy>
struct ImplicitKeyTestMap : public VectorMap<int, string,
								VectorMapEntryStrategy::ImplicitKey<int, string,
									ImplicitKeyTestGetKey, CompareStrategy> > {};


typedef ImplicitKeyStrategy<SimpleValueStrategy<int>, SimpleValueStrategy<string>,
	ImplicitKeyTestGetKey>
	ImplicitKeyTestEntryStrategy;


template<class CompareStrategy>
struct ImplicitKeyTestStrategy
	: public TestStrategy<ImplicitKeyTestMap, ImplicitKeyTestEntryStrategy, CompareStrategy> {};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<IntIntTestStrategy<Ascending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<IntIntTestStrategy<Descending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<IntStringTestStrategy<Ascending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<IntStringTestStrategy<Descending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<StringIntTestStrategy<Ascending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<StringIntTestStrategy<Descending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<StringStringTestStrategy<Ascending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<StringStringTestStrategy<Descending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<ImplicitKeyTestStrategy<Ascending> >,
	getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorMapTest<ImplicitKeyTestStrategy<Descending> >,
	getTestSuiteName());
