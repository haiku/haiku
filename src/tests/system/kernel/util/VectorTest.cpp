/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <Vector.h>


template<typename Value, typename TestVector, typename MyIterator, typename ReferenceIterator>
class TestIterator {
private:
	typedef TestIterator<Value, TestVector, MyIterator, ReferenceIterator> Iterator;

public:
	inline TestIterator(TestVector* vector, MyIterator myIt, ReferenceIterator refIt)
		:
		fVector(vector),
		fMyIterator(myIt),
		fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator& other)
		:
		fVector(other.fVector),
		fMyIterator(other.fMyIterator),
		fReferenceIterator(other.fReferenceIterator)
	{
		CPPUNIT_ASSERT(fMyIterator == other.fMyIterator);
	}

	inline Iterator& operator++()
	{
		MyIterator& myResult = ++fMyIterator;
		ReferenceIterator& refResult = ++fReferenceIterator;
		if (refResult == fVector->fReferenceVector.end())
			CPPUNIT_ASSERT(myResult == fVector->fMyVector.End());
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
		if (refResult == fVector->fReferenceVector.end())
			CPPUNIT_ASSERT(myResult == fVector->fMyVector.End());
		else
			CPPUNIT_ASSERT(*myResult == *refResult);
		return Iterator(fVector, myResult, refResult);
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
		return Iterator(fVector, myResult, refResult);
	}

	inline Iterator& operator=(const Iterator& other)
	{
		fVector = other.fVector;
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
		CPPUNIT_ASSERT((fMyIterator == fVector->fMyVector.Null()) != result);
		return result;
	}

public:
	TestVector* fVector;
	MyIterator fMyIterator;
	ReferenceIterator fReferenceIterator;
};


template<typename Value>
class TestVector {
public:
	typedef typename Vector<Value>::Iterator			MyIterator;
	typedef typename Vector<Value>::ConstIterator		MyConstIterator;
	typedef typename std::vector<Value>::iterator		ReferenceIterator;
	typedef typename std::vector<Value>::const_iterator	ReferenceConstIterator;
	typedef TestIterator<Value, TestVector<Value>, MyIterator, ReferenceIterator>
		Iterator;
	typedef TestIterator<const Value, const TestVector<Value>, MyConstIterator, ReferenceConstIterator>
		ConstIterator;

	TestVector()
		:
		fMyVector(),
		fReferenceVector(),
		fChecking(true)
	{
	}

	void PushFront(const Value& value)
	{
		CPPUNIT_ASSERT(fMyVector.PushFront(value) == B_OK);
		fReferenceVector.insert(fReferenceVector.begin(), value);
		Check();
	}

	void PushBack(const Value& value)
	{
		CPPUNIT_ASSERT(fMyVector.PushBack(value) == B_OK);
		fReferenceVector.push_back(value);
		Check();
	}

	void PopFront()
	{
		fMyVector.PopFront();
		fReferenceVector.erase(fReferenceVector.begin());
		Check();
	}

	void PopBack()
	{
		fMyVector.PopBack();
		fReferenceVector.pop_back();
		Check();
	}

	void Insert(const Value& value, int32 index)
	{
		if (index >= 0 && index <= Count()) {
			CPPUNIT_ASSERT(fMyVector.Insert(value, index) == B_OK);
			typename vector<Value>::iterator it = fReferenceVector.begin();
			for (int32 i = 0; i < index; i++)
				++it;
			fReferenceVector.insert(it, value);
		} else {
			CPPUNIT_ASSERT(fMyVector.Insert(value, index) == B_BAD_VALUE);
		}
		Check();
	}

	void Insert(const Value& value, const Iterator& iterator)
	{

		if (iterator.fMyIterator == fMyVector.Null()) {
			CPPUNIT_ASSERT(fMyVector.Insert(value, iterator.fMyIterator) == B_BAD_VALUE);
		} else {
			CPPUNIT_ASSERT(fMyVector.Insert(value, iterator.fMyIterator) == B_OK);
			fReferenceVector.insert(iterator.fReferenceIterator, value);
		}
		Check();
	}

	void Remove(const Value& value)
	{
		int32 oldCount = Count();
		for (ReferenceIterator it = fReferenceVector.begin(); it != fReferenceVector.end();)
			if (*it == value)
				it = fReferenceVector.erase(it);
			else
				++it;
		int32 newCount = fReferenceVector.size();
		CPPUNIT_ASSERT(fMyVector.Remove(value) == oldCount - newCount);
		Check();
	}

	Iterator Erase(int32 index)
	{
		bool outOfRange = (index < 0 || index >= Count());
		MyIterator myIt = fMyVector.Erase(index);
		if (outOfRange) {
			CPPUNIT_ASSERT(myIt == fMyVector.Null());
			return Iterator(this, myIt, fReferenceVector.end());
		}
		ReferenceIterator it = fReferenceVector.begin();
		for (int32 i = 0; i < index; i++)
			++it;
		ReferenceIterator refIt = fReferenceVector.erase(it);
		Check();
		if (refIt == fReferenceVector.end())
			CPPUNIT_ASSERT(myIt == fMyVector.End());
		else
			CPPUNIT_ASSERT(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	Iterator Erase(const Iterator& iterator)
	{
		bool outOfRange = (iterator.fReferenceIterator == fReferenceVector.end());
		MyIterator myIt = fMyVector.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CPPUNIT_ASSERT(myIt == fMyVector.Null());
			return Iterator(this, myIt, fReferenceVector.end());
		}
		ReferenceIterator refIt = fReferenceVector.erase(iterator.fReferenceIterator);
		Check();
		if (refIt == fReferenceVector.end())
			CPPUNIT_ASSERT(myIt == fMyVector.End());
		else
			CPPUNIT_ASSERT(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceVector.size();
		CPPUNIT_ASSERT(fMyVector.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceVector.empty();
		CPPUNIT_ASSERT(fMyVector.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMyVector.MakeEmpty();
		fReferenceVector.clear();
		Check();
	}

	inline Iterator Begin() { return Iterator(this, fMyVector.Begin(), fReferenceVector.begin()); }
	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMyVector.Begin(), fReferenceVector.begin());
	}

	inline Iterator End() { return Iterator(this, fMyVector.End(), fReferenceVector.end()); }
	inline ConstIterator End() const
	{
		return ConstIterator(this, fMyVector.End(), fReferenceVector.end());
	}

	inline Iterator Null() { return Iterator(this, fMyVector.Null(), fReferenceVector.end()); }
	inline ConstIterator Null() const
	{
		return ConstIterator(this, fMyVector.Null(), fReferenceVector.end());
	}

	inline Iterator IteratorForIndex(int32 index)
	{
		if (index < 0 || index > Count()) {
			CPPUNIT_ASSERT(fMyVector.IteratorForIndex(index) == fMyVector.End());
			return End();
		} else {
			MyIterator myIt = fMyVector.IteratorForIndex(index);
			ReferenceIterator it = fReferenceVector.begin();
			for (int32 i = 0; i < index; i++)
				++it;
			return Iterator(this, myIt, it);
		}
	}

	inline ConstIterator IteratorForIndex(int32 index) const
	{
		if (index < 0 || index > Count()) {
			CPPUNIT_ASSERT(fMyVector.IteratorForIndex(index) == fMyVector.End());
			return End();
		} else {
			MyIterator myIt = fMyVector.IteratorForIndex(index);
			ReferenceIterator it = fReferenceVector.begin();
			for (int32 i = 0; i < index; i++)
				++it;
			return ConstIterator(this, myIt, it);
		}
	}

	inline const Value& ElementAt(int32 index) const
	{
		const Value& result = fReferenceVector[index];
		CPPUNIT_ASSERT(result == fMyVector.ElementAt(index));
		return result;
	}

	inline Value& ElementAt(int32 index)
	{
		Value& result = fReferenceVector[index];
		CPPUNIT_ASSERT(result == fMyVector.ElementAt(index));
		return result;
	}

	int32 IndexOf(const Value& value, int32 start = 0) const
	{
		int32 result = -1;
		if (start >= 0 && start < Count()) {
			ReferenceConstIterator it = fReferenceVector.begin();
			for (int32 i = 0; i < start; i++)
				++it;
			for (int32 i = start; it != fReferenceVector.end(); ++it, i++) {
				if (*it == value) {
					result = i;
					break;
				}
			}
		}
		CPPUNIT_ASSERT(result == fMyVector.IndexOf(value, start));
		return result;
	}

	Iterator Find(const Value& value)
	{
		Iterator start(Begin());
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyIterator myIt = fMyVector.Find(value);
					CPPUNIT_ASSERT(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CPPUNIT_ASSERT(fMyVector.Find(value) == fMyVector.End());
			CPPUNIT_ASSERT(start.fMyIterator == fMyVector.End());
			return start;
		}
		CPPUNIT_ASSERT(fMyVector.Find(value) == fMyVector.End());
		return start;
	}

	Iterator Find(const Value& value, const Iterator& _start)
	{
		Iterator start(_start);
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyIterator myIt = fMyVector.Find(value, _start.fMyIterator);
					CPPUNIT_ASSERT(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CPPUNIT_ASSERT(fMyVector.Find(value, _start.fMyIterator) == fMyVector.End());
			CPPUNIT_ASSERT(start.fMyIterator == fMyVector.End());
			return start;
		}
		CPPUNIT_ASSERT(fMyVector.Find(value, start.fMyIterator) == fMyVector.End());
		return start;
	}

	ConstIterator Find(const Value& value) const
	{
		ConstIterator start(Begin());
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyConstIterator myIt = fMyVector.Find(value);
					CPPUNIT_ASSERT(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CPPUNIT_ASSERT(fMyVector.Find(value) == fMyVector.End());
			CPPUNIT_ASSERT(start.fMyIterator == fMyVector.End());
			return start;
		}
		CPPUNIT_ASSERT(fMyVector.Find(value) == fMyVector.End());
		return start;
	}

	ConstIterator Find(const Value& value, const ConstIterator& _start) const
	{
		ConstIterator start(_start);
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyConstIterator myIt = fMyVector.Find(value, _start.fMyIterator);
					CPPUNIT_ASSERT(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CPPUNIT_ASSERT(fMyVector.Find(value, _start.fMyIterator) == fMyVector.End());
			CPPUNIT_ASSERT(start.fMyIterator == fMyVector.End());
			return start;
		}
		CPPUNIT_ASSERT(fMyVector.Find(value, start.fMyIterator) == fMyVector.End());
		return start;
	}

	inline Value& operator[](int32 index)
	{
		Value& result = fMyVector[index];
		CPPUNIT_ASSERT(result == fReferenceVector[index]);
		return result;
	}

	inline const Value& operator[](int32 index) const
	{
		const Value& result = fMyVector[index];
		CPPUNIT_ASSERT(result == fReferenceVector[index]);
		return result;
	}

	void SetChecking(bool enable) { fChecking = enable; }

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceVector.size();
			CPPUNIT_ASSERT(fMyVector.Count() == count);
			CPPUNIT_ASSERT(fMyVector.IsEmpty() == fReferenceVector.empty());
			for (int32 i = 0; i < count; i++)
				CPPUNIT_ASSERT(fMyVector[i] == fReferenceVector[i]);
		}
	}

public:
	Vector<Value> fMyVector;
	vector<Value> fReferenceVector;
	bool fChecking;
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
		char buffer[10];
		sprintf(buffer, "%ld", rand() % fDifferentValues);
		return std::string(buffer);
	}

private:
	int32 fDifferentValues;
};


template<typename _ValueStrategy>
class TestStrategy {
public:
	typedef _ValueStrategy					ValueStrategy;
	typedef typename ValueStrategy::Value	Value;
	typedef TestVector<Value>				TestClass;
};


typedef TestStrategy<IntStrategy>		IntTestStrategy;
typedef TestStrategy<StringStrategy>	StringTestStrategy;


template<typename TestStrategy>
class VectorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VectorTest);
	CPPUNIT_TEST(Constructor100ElementsTest);
	CPPUNIT_TEST(Constructor0ElementsTest);
	CPPUNIT_TEST(PushPopFront30ElementsTest);
	CPPUNIT_TEST(PushPopFront200ElementsTest);
	CPPUNIT_TEST(PushPopBack30ElementsTest);
	CPPUNIT_TEST(PushPopBack200ElementsTest);
	CPPUNIT_TEST(Insert30ElementsTest);
	CPPUNIT_TEST(Insert200ElementsTest);
	CPPUNIT_TEST(Remove30ElementsTest);
	CPPUNIT_TEST(Remove200ElementsTest);
	CPPUNIT_TEST(Erase30ElementsTest);
	CPPUNIT_TEST(Erase200ElementsTest);
	CPPUNIT_TEST(MakeEmpty30ElementsTest);
	CPPUNIT_TEST(MakeEmpty200ElementsTest);
	CPPUNIT_TEST(IndexAccess300ElementsTest);
	CPPUNIT_TEST(IndexAccess2000ElementsTest);
	CPPUNIT_TEST(Find30ElementsTest);
	CPPUNIT_TEST(Find200ElementsTest);
	CPPUNIT_TEST(Iterator0ElementsTest);
	CPPUNIT_TEST(Iterator300ElementsTest);
	CPPUNIT_TEST(Iterator2000ElementsTest);
	CPPUNIT_TEST_SUITE_END();

	typedef typename TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename TestStrategy::Value			Value;
	typedef typename TestStrategy::TestClass		TestClass;

	void PushPopFrontTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++)
			v.PushFront(strategy.Generate());
		for (int32 i = 0; i < maxNumber / 2; i++)
			v.PopFront();
		for (int32 i = 0; i < maxNumber; i++)
			v.PushFront(strategy.Generate());
		int32 count = v.Count();
		for (int32 i = 0; i < count; i++)
			v.PopFront();
	}

	void PushPopBackTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++)
			v.PushBack(strategy.Generate());
		for (int32 i = 0; i < maxNumber / 2; i++)
			v.PopBack();
		for (int32 i = 0; i < maxNumber; i++)
			v.PushBack(strategy.Generate());
		int32 count = v.Count();
		for (int32 i = 0; i < count; i++)
			v.PopBack();
	}

	void InsertTest1(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++) {
			int32 index = rand() % (i + 1);
			v.Insert(strategy.Generate(), index);
		}
	}

	void InsertTest2(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		for (int32 i = 0; i < maxNumber; i++) {
			int32 index = rand() % (i + 1);
			v.Insert(strategy.Generate(), v.IteratorForIndex(index));
		}
	}

	void Fill(TestClass& v, ValueStrategy strategy, int32 maxNumber)
	{
		v.SetChecking(false);
		for (int32 i = 0; i < maxNumber; i++) {
			int32 index = rand() % (i + 1);
			v.Insert(strategy.Generate(), index);
		}
		v.SetChecking(true);
		v.Check();
	}

	void RemoveTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		while (v.Count() > 0) {
			int32 index = rand() % (v.Count());
			Value value = v[index];
			v.Remove(value);
			v.Remove(value);
		}
	}

	void EraseTest1(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		for (int32 i = maxNumber - 1; i >= 0; i--) {
			int32 index = rand() % (i + 1);
			v.Erase(index);
		}
	}

	void EraseTest2(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		for (int32 i = maxNumber - 1; i >= 0; i--) {
			int32 index = rand() % (i + 1);
			v.Erase(v.IteratorForIndex(index));
		}
	}

	void MakeEmptyTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		v.MakeEmpty();
		Fill(v, strategy, maxNumber);
		v.MakeEmpty();
		v.MakeEmpty();
	}

	void IndexAccessTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		const TestClass& cv = v;
		for (int32 i = 0; i < maxNumber; i++) {
			CPPUNIT_ASSERT(v[i] == cv[i]);
			CPPUNIT_ASSERT(v.ElementAt(i) == cv.ElementAt(i));
		}
	}

	void FindTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		// find the values in the vector
		const TestClass& cv = v;
		for (int32 i = 0; i < maxNumber; i++) {
			typename TestClass::ConstIterator cit = cv.Begin();
			int32 index = 0;
			for (typename TestClass::Iterator it = v.Begin(); it != v.End();) {
				CPPUNIT_ASSERT(&v[index] == &*it);
				CPPUNIT_ASSERT(&cv[index] == &*cit);
				CPPUNIT_ASSERT(*it == *cit);
				it = v.Find(v[i], ++it);
				cit = cv.Find(cv[i], ++cit);
				index = v.IndexOf(v[i], index + 1);
			}
		}
		// try to find some random values
		for (int32 i = 0; i < maxNumber; i++) {
			Value value = strategy.Generate();
			typename TestClass::Iterator it = v.Find(value);
			typename TestClass::ConstIterator cit = cv.Find(value);
			if (it != v.End())
				CPPUNIT_ASSERT(&*it == &*cit);
		}
	}

	void IteratorTest(ValueStrategy strategy, int32 maxNumber)
	{
		TestClass v;
		Fill(v, strategy, maxNumber);
		const TestClass& cv = v;
		typename TestClass::Iterator it = v.Begin();
		typename TestClass::ConstIterator cit = cv.Begin();
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
		Vector<Value> vector(100);
		CPPUNIT_ASSERT(vector.Count() == 0);
		CPPUNIT_ASSERT(vector.IsEmpty());
		CPPUNIT_ASSERT(vector.GetCapacity() == 100);
	}

	void Constructor0ElementsTest()
	{
		Vector<Value> vector(0);
		CPPUNIT_ASSERT(vector.Count() == 0);
		CPPUNIT_ASSERT(vector.IsEmpty());
		CPPUNIT_ASSERT(vector.GetCapacity() > 0);
	}

	void PushPopFront30ElementsTest() { PushPopFrontTest(ValueStrategy(), 30); }
	void PushPopFront200ElementsTest() { PushPopFrontTest(ValueStrategy(), 200); }
	void PushPopBack30ElementsTest() { PushPopBackTest(ValueStrategy(), 30); }
	void PushPopBack200ElementsTest() { PushPopBackTest(ValueStrategy(), 200); }

	void Insert30ElementsTest()
	{
		InsertTest1(ValueStrategy(), 30);
		InsertTest2(ValueStrategy(), 30);
	}

	void Insert200ElementsTest()
	{
		InsertTest1(ValueStrategy(), 200);
		InsertTest2(ValueStrategy(), 200);
	}

	void Remove30ElementsTest()
	{
		RemoveTest(ValueStrategy(), 30);
		RemoveTest(ValueStrategy(10), 30);
	}

	void Remove200ElementsTest()
	{
		RemoveTest(ValueStrategy(), 200);
		RemoveTest(ValueStrategy(20), 200);
	}

	void Erase30ElementsTest()
	{
		EraseTest1(ValueStrategy(), 30);
		EraseTest2(ValueStrategy(), 30);
	}

	void Erase200ElementsTest()
	{
		EraseTest1(ValueStrategy(), 200);
		EraseTest2(ValueStrategy(), 200);
	}

	void MakeEmpty30ElementsTest() { MakeEmptyTest(ValueStrategy(), 30); }
	void MakeEmpty200ElementsTest() { MakeEmptyTest(ValueStrategy(), 200); }
	void IndexAccess300ElementsTest() { IndexAccessTest(ValueStrategy(), 300); }
	void IndexAccess2000ElementsTest() { IndexAccessTest(ValueStrategy(), 2000); }

	void Find30ElementsTest()
	{
		FindTest(ValueStrategy(), 30);
		FindTest(ValueStrategy(10), 30);
	}

	void Find200ElementsTest()
	{
		FindTest(ValueStrategy(), 200);
		FindTest(ValueStrategy(50), 200);
	}

	void Iterator0ElementsTest() { IteratorTest(ValueStrategy(), 0); }
	void Iterator300ElementsTest() { IteratorTest(ValueStrategy(), 300); }
	void Iterator2000ElementsTest() { IteratorTest(ValueStrategy(), 2000); }
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorTest<IntTestStrategy>, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(VectorTest<StringTestStrategy>, getTestSuiteName());
