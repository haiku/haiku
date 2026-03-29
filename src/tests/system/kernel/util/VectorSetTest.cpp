/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <set>
#include <string>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <VectorSet.h>


using VectorSetOrder::Ascending;
using VectorSetOrder::Descending;


template<typename Value, typename TestSet, typename MyIterator, typename ReferenceIterator>
class TestIterator {
	typedef TestIterator<Value, TestSet, MyIterator, ReferenceIterator> Iterator;

public:
	inline TestIterator(TestSet* s, MyIterator myIt, ReferenceIterator refIt)
		:
		fSet(s),
		fMyIterator(myIt),
		fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator& other)
		:
		fSet(other.fSet),
		fMyIterator(other.fMyIterator),
		fReferenceIterator(other.fReferenceIterator)
	{
		CPPUNIT_ASSERT(fMyIterator == other.fMyIterator);
	}

	inline Iterator& operator++()
	{
		MyIterator& myResult = ++fMyIterator;
		ReferenceIterator& refResult = ++fReferenceIterator;
		if (refResult == fSet->fReferenceSet.end())
			CPPUNIT_ASSERT(myResult == fSet->fMySet.End());
		else
			CPPUNIT_ASSERT(*myResult == *refResult);
		return *this;
	}

	inline Iterator operator++(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator++;
		ReferenceIterator refResult = fReferenceIterator++;
		CPPUNIT_ASSERT(oldMyResult == myResult);
		if (refResult == fSet->fReferenceSet.end())
			CPPUNIT_ASSERT(myResult == fSet->fMySet.End());
		else
			CPPUNIT_ASSERT(*myResult == *refResult);
		return Iterator(fSet, myResult, refResult);
	}

	inline Iterator& operator--()
	{
		MyIterator& myResult = --fMyIterator;
		ReferenceIterator& refResult = --fReferenceIterator;
		CPPUNIT_ASSERT(*myResult == *refResult);
		return *this;
	}

	inline Iterator operator--(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator--;
		ReferenceIterator refResult = fReferenceIterator--;
		CPPUNIT_ASSERT(oldMyResult == myResult);
		CPPUNIT_ASSERT(*myResult == *refResult);
		return Iterator(fSet, myResult, refResult);
	}

	inline Iterator& operator=(const Iterator& other)
	{
		fSet = other.fSet;
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

	inline Value& operator*() const
	{
		Value& result = *fMyIterator;
		CPPUNIT_ASSERT(result == *fReferenceIterator);
		return result;
	}

	inline Value* operator->() const
	{
		Value* result = fMyIterator.operator->();
		CPPUNIT_ASSERT(*result == *fReferenceIterator);
		return result;
	}

	inline operator bool() const
	{
		bool result = fMyIterator;
		CPPUNIT_ASSERT((fMyIterator == fSet->fMySet.Null()) != result);
		return result;
	}

public:
	TestSet* fSet;
	MyIterator fMyIterator;
	ReferenceIterator fReferenceIterator;
};


template<typename Value, typename MySet, typename ReferenceSet, typename Compare>
class TestSet {
public:
	typedef TestSet<Value, MySet, ReferenceSet, Compare>				Class;

	typedef typename MySet::Iterator									MyIterator;
	typedef typename ReferenceSet::iterator								ReferenceIterator;
	typedef typename MySet::ConstIterator								MyConstIterator;
	typedef typename ReferenceSet::const_iterator						ReferenceConstIterator;
	typedef TestIterator<Value, Class, MyIterator, ReferenceIterator>	Iterator;
	typedef TestIterator<const Value, const Class, MyConstIterator, ReferenceConstIterator>
		ConstIterator;

	TestSet()
		:
		fMySet(),
		fReferenceSet(),
		fChecking(true)
	{
	}

	void Insert(const Value& value, bool replace = true)
	{
		CPPUNIT_ASSERT(fMySet.Insert(value, replace) == B_OK);
		ReferenceIterator it = fReferenceSet.find(value);
		if (it != fReferenceSet.end())
			fReferenceSet.erase(it);
		fReferenceSet.insert(value);
		Check();
	}

	void Remove(const Value& value)
	{
		int32 oldCount = Count();
		fReferenceSet.erase(value);
		int32 newCount = fReferenceSet.size();
		CPPUNIT_ASSERT(fMySet.Remove(value) == oldCount - newCount);
		Check();
	}

	Iterator Erase(const Iterator& iterator)
	{
		bool outOfRange = (iterator.fReferenceIterator == fReferenceSet.end());
		MyIterator myIt = fMySet.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CPPUNIT_ASSERT(myIt == fMySet.Null());
			return Iterator(this, myIt, fReferenceSet.end());
		}
		Value nextValue;
		ReferenceIterator refIt = iterator.fReferenceIterator;
		++refIt;
		bool noNextValue = (refIt == fReferenceSet.end());
		if (!noNextValue)
			nextValue = *refIt;
		fReferenceSet.erase(iterator.fReferenceIterator);
		if (noNextValue)
			refIt = fReferenceSet.end();
		else
			refIt = fReferenceSet.find(nextValue);
		Check();
		if (refIt == fReferenceSet.end())
			CPPUNIT_ASSERT(myIt == fMySet.End());
		else
			CPPUNIT_ASSERT(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceSet.size();
		CPPUNIT_ASSERT(fMySet.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceSet.empty();
		CPPUNIT_ASSERT(fMySet.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMySet.MakeEmpty();
		fReferenceSet.clear();
		Check();
	}

	inline Iterator Begin() { return Iterator(this, fMySet.Begin(), fReferenceSet.begin()); }
	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMySet.Begin(), fReferenceSet.begin());
	}

	inline Iterator End() { return Iterator(this, fMySet.End(), fReferenceSet.end()); }
	inline ConstIterator End() const
	{
		return ConstIterator(this, fMySet.End(), fReferenceSet.end());
	}

	inline Iterator Null() { return Iterator(this, fMySet.Null(), fReferenceSet.end()); }
	inline ConstIterator Null() const
	{
		return ConstIterator(this, fMySet.Null(), fReferenceSet.end());
	}

	// for testing only
	inline Iterator IteratorForIndex(int32 index)
	{
		if (index < 0 || index > Count())
			return End();
		MyIterator myIt = fMySet.Begin();
		ReferenceIterator refIt = fReferenceSet.begin();
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
		MyConstIterator myIt = fMySet.Begin();
		ReferenceConstIterator refIt = fReferenceSet.begin();
		for (int32 i = 0; i < index; i++) {
			++myIt;
			++refIt;
		}
		return ConstIterator(this, myIt, refIt);
	}

	Iterator Find(const Value& value)
	{
		MyIterator myIt = fMySet.Find(value);
		ReferenceIterator refIt = fReferenceSet.find(value);
		if (refIt == fReferenceSet.end())
			CPPUNIT_ASSERT(myIt = fMySet.End());
		else
			CPPUNIT_ASSERT(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	ConstIterator Find(const Value& value) const
	{
		MyConstIterator myIt = fMySet.Find(value);
		ReferenceConstIterator refIt = fReferenceSet.find(value);
		if (refIt == fReferenceSet.end())
			CPPUNIT_ASSERT(myIt = fMySet.End());
		else
			CPPUNIT_ASSERT(*myIt == *refIt);
		return ConstIterator(this, myIt, refIt);
	}

	Iterator FindClose(const Value& value, bool less)
	{
		MyIterator myIt = fMySet.FindClose(value, less);
		if (myIt == fMySet.End()) {
			if (fMySet.Count() > 0) {
				if (less)
					CPPUNIT_ASSERT(fCompare(*fMySet.Begin(), value) > 0);
				else
					CPPUNIT_ASSERT(fCompare(*--MyIterator(myIt), value) < 0);
			}
			return End();
		}
		if (less) {
			CPPUNIT_ASSERT(fCompare(*myIt, value) <= 0);
			MyIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMySet.End())
				CPPUNIT_ASSERT(fCompare(*nextMyIt, value) > 0);
		} else {
			CPPUNIT_ASSERT(fCompare(*myIt, value) >= 0);
			if (myIt != fMySet.Begin()) {
				MyIterator prevMyIt(myIt);
				--prevMyIt;
				CPPUNIT_ASSERT(fCompare(*prevMyIt, value) < 0);
			}
		}
		return Iterator(this, myIt, fReferenceSet.find(*myIt));
	}

	ConstIterator FindClose(const Value& value, bool less) const
	{
		MyConstIterator myIt = fMySet.FindClose(value, less);
		if (myIt == fMySet.End()) {
			if (fMySet.Count() > 0) {
				if (less)
					CPPUNIT_ASSERT(fCompare(*fMySet.Begin(), value) > 0);
				else
					CPPUNIT_ASSERT(fCompare(*--MyConstIterator(myIt), value) < 0);
			}
			return End();
		}
		if (less) {
			CPPUNIT_ASSERT(fCompare(*myIt, value) <= 0);
			MyConstIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMySet.End())
				CPPUNIT_ASSERT(fCompare(*nextMyIt, value) > 0);
		} else {
			CPPUNIT_ASSERT(fCompare(*myIt, value) >= 0);
			if (myIt != fMySet.Begin()) {
				MyConstIterator prevMyIt(myIt);
				--prevMyIt;
				CPPUNIT_ASSERT(fCompare(*prevMyIt, value) < 0);
			}
		}
		return ConstIterator(this, myIt, fReferenceSet.find(*myIt));
	}

	void SetChecking(bool enable) { fChecking = enable; }

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceSet.size();
			CPPUNIT_ASSERT(fMySet.Count() == count);
			CPPUNIT_ASSERT(fMySet.IsEmpty() == fReferenceSet.empty());
			MyConstIterator myIt = fMySet.Begin();
			ReferenceConstIterator refIt = fReferenceSet.begin();
			for (int32 i = 0; i < count; i++, ++myIt, ++refIt)
				CPPUNIT_ASSERT(*myIt == *refIt);
			CPPUNIT_ASSERT(myIt == fMySet.End());
		}
	}

public:
	MySet fMySet;
	ReferenceSet fReferenceSet;
	bool fChecking;
	Compare fCompare;
};


class IntStrategy {
public:
	typedef int Value;

	IntStrategy(int32 differentValues = 100000)
		:
		fDifferentValues(differentValues)
	{
		srand(0);
	}

	Value Generate() { return rand() % fDifferentValues; }

private:
	int32 fDifferentValues;
};


class StringStrategy {
public:
	typedef std::string Value;

	StringStrategy(int32 differentValues = 100000)
		:
		fDifferentValues(differentValues)
	{
		srand(0);
	}

	Value Generate()
	{
		char buffer[12];
		sprintf(buffer, "%" B_PRId32, rand() % fDifferentValues);
		return string(buffer);
	}

private:
	int32 fDifferentValues;
};


template<typename Value, typename Compare>
class CompareWrapper {
public:
	inline bool operator()(const Value& a, const Value& b) const { return fCompare(a, b) < 0; }

private:
	Compare fCompare;
};


template<typename _ValueStrategy, template<typename> class _CompareStrategy>
class TestStrategy {
public:
	typedef _ValueStrategy									ValueStrategy;
	typedef typename ValueStrategy::Value					Value;
	typedef _CompareStrategy<Value>							Compare;
	typedef CompareWrapper<Value, Compare>					BoolCompare;
	typedef VectorSet<Value, Compare>						MySet;
	typedef std::set<Value, BoolCompare>					ReferenceSet;
	typedef TestSet<Value, MySet, ReferenceSet, Compare>	TestClass;
};


typedef TestStrategy<IntStrategy, Ascending> IntAscTestStrategy;
typedef TestStrategy<IntStrategy, Descending> IntDescTestStrategy;
typedef TestStrategy<StringStrategy, Ascending> StringAscTestStrategy;
typedef TestStrategy<StringStrategy, Descending> StringDescTestStrategy;


template<typename TestStrategy>
class VectorSetTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VectorSetTest);
	CPPUNIT_TEST(Constructor100ElementsTest);
	CPPUNIT_TEST(Constructor0ElementsTest);
	CPPUNIT_TEST(Insert30ElementsTest);
	CPPUNIT_TEST(Insert200ElementsTest);
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

	typedef typename TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename TestStrategy::Value			Value;
	typedef typename TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;

	void InsertTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++)
			v.Insert(strategy.Generate());
	}

	void Fill(TestClass& v, ValueStrategy strategy, int32 maxNumber)
	{
		v.SetChecking(false);
		for (int32 i = 0; v.Count() < maxNumber; i++)
			v.Insert(strategy.Generate());
		v.SetChecking(true);
		v.Check();
	}

	void RemoveTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		Fill(v, strategy, maxNumber);
		while (v.Count() > 0) {
			int32 index = rand() % (v.Count());
			Value value = *v.IteratorForIndex(index);
			v.Remove(value);
			v.Remove(value);
		}
	}

	void EraseTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		Fill(v, strategy, maxNumber);
		for (int32 i = maxNumber - 1; i >= 0; i--) {
			int32 index = rand() % (i + 1);
			v.Erase(v.IteratorForIndex(index));
		}
	}

	void MakeEmptyTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		v.MakeEmpty();
		Fill(v, strategy, maxNumber);
		v.MakeEmpty();
		v.MakeEmpty();
	}

	void FindTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		Fill(v, strategy, maxNumber);
		const TestClass& cv = v;
		// find the values in the set
		for (int32 i = 0; i < maxNumber; i++) {
			const Value& value = *v.IteratorForIndex(i);
			Iterator it = v.Find(value);
			ConstIterator cit = cv.Find(value);
			CPPUNIT_ASSERT(&*it == &*cit);
		}
		// try to find some random values
		for (int32 i = 0; i < maxNumber; i++) {
			Value value = strategy.Generate();
			Iterator it = v.Find(value);
			ConstIterator cit = cv.Find(value);
			if (it != v.End())
				CPPUNIT_ASSERT(&*it == &*cit);
		}
	}

	void FindCloseTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		Fill(v, strategy, maxNumber);
		const TestClass& cv = v;
		// find the values in the set
		for (int32 i = 0; i < maxNumber; i++) {
			const Value& value = *v.IteratorForIndex(i);
			// less
			Iterator it = v.FindClose(value, true);
			ConstIterator cit = cv.FindClose(value, true);
			CPPUNIT_ASSERT(*it == value);
			CPPUNIT_ASSERT(&*it == &*cit);
			// greater
			it = v.FindClose(value, false);
			cit = cv.FindClose(value, false);
			CPPUNIT_ASSERT(*it == value);
			CPPUNIT_ASSERT(&*it == &*cit);
		}
		// try to find some random values
		for (int32 i = 0; i < maxNumber; i++) {
			Value value = strategy.Generate();
			// less
			Iterator it = v.FindClose(value, true);
			ConstIterator cit = cv.FindClose(value, true);
			if (it != v.End())
				CPPUNIT_ASSERT(&*it == &*cit);
			// greater
			it = v.FindClose(value, false);
			cit = cv.FindClose(value, false);
			if (it != v.End())
				CPPUNIT_ASSERT(&*it == &*cit);
		}
	}

	void IteratorTest(int32 maxNumber)
	{
		ValueStrategy strategy;
		TestClass v;
		Fill(v, strategy, maxNumber);
		const TestClass& cv = v;
		Iterator it = v.Begin();
		ConstIterator cit = cv.Begin();
		for (; it != v.End(); ++it, ++cit) {
			CPPUNIT_ASSERT(&*it == &*cit);
			CPPUNIT_ASSERT(&*it == it.operator->());
			CPPUNIT_ASSERT(&*cit == cit.operator->());
			CPPUNIT_ASSERT(it);
			CPPUNIT_ASSERT(cit);
		}
		CPPUNIT_ASSERT(cit == cv.End());
		while (it != v.Begin()) {
			--it;
			--cit;
			CPPUNIT_ASSERT(&*it == &*cit);
			CPPUNIT_ASSERT(&*it == it.operator->());
			CPPUNIT_ASSERT(&*cit == cit.operator->());
			CPPUNIT_ASSERT(it);
			CPPUNIT_ASSERT(cit);
		}
		CPPUNIT_ASSERT(cit == cv.Begin());
		CPPUNIT_ASSERT(!v.Null());
		CPPUNIT_ASSERT(!cv.Null());
	}

public:
	void Constructor100ElementsTest()
	{
		VectorSet<Value> vector(100);
		CPPUNIT_ASSERT(vector.Count() == 0);
		CPPUNIT_ASSERT(vector.IsEmpty());
	}

	void Constructor0ElementsTest()
	{
		VectorSet<Value> vector(0);
		CPPUNIT_ASSERT(vector.Count() == 0);
		CPPUNIT_ASSERT(vector.IsEmpty());
	}

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


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorSetTest<IntAscTestStrategy>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorSetTest<IntDescTestStrategy>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorSetTest<StringAscTestStrategy>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorSetTest<StringDescTestStrategy>, getTestSuiteName());
