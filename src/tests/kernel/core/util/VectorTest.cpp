
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include <TestUtils.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <Vector.h>

#include "common.h"
#include "VectorTest.h"

VectorTest::VectorTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
VectorTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("Vector");

	ADD_TEST4(Vector, suite, VectorTest, ConstructorTest1);
	ADD_TEST4(Vector, suite, VectorTest, ConstructorTest2);
	ADD_TEST4(Vector, suite, VectorTest, PushPopFrontTest);
	ADD_TEST4(Vector, suite, VectorTest, PushPopBackTest);
	ADD_TEST4(Vector, suite, VectorTest, InsertTest);
	ADD_TEST4(Vector, suite, VectorTest, RemoveTest);
	ADD_TEST4(Vector, suite, VectorTest, EraseTest);
	ADD_TEST4(Vector, suite, VectorTest, MakeEmptyTest);
	ADD_TEST4(Vector, suite, VectorTest, IndexAccessTest);
	ADD_TEST4(Vector, suite, VectorTest, FindTest);
	ADD_TEST4(Vector, suite, VectorTest, IteratorTest);

	return suite;
}

//! ConstructorTest1
void
VectorTest::ConstructorTest1()
{
	Vector<int> v1(100);
	CHK(v1.Count() == 0);
	CHK(v1.IsEmpty());
	CHK(v1.GetCapacity() == 100);

	Vector<string> v2(100);
	CHK(v2.Count() == 0);
	CHK(v2.IsEmpty());
	CHK(v2.GetCapacity() == 100);
}

//! ConstructorTest2
void
VectorTest::ConstructorTest2()
{
	Vector<int> v1(0);
	CHK(v1.Count() == 0);
	CHK(v1.IsEmpty());
	CHK(v1.GetCapacity() > 0);

	Vector<string> v2(0);
	CHK(v2.Count() == 0);
	CHK(v2.IsEmpty());
	CHK(v2.GetCapacity() > 0);
}

// TestIterator
template<typename Value, typename TestVector, typename MyIterator,
		 typename ReferenceIterator>
class TestIterator {
private:
	typedef TestIterator<Value, TestVector, MyIterator, ReferenceIterator>
			Iterator;

public:
	inline TestIterator(TestVector *vector, MyIterator myIt,
						ReferenceIterator refIt)
		: fVector(vector),
		  fMyIterator(myIt),
		  fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator &other)
		: fVector(other.fVector),
		  fMyIterator(other.fMyIterator),
		  fReferenceIterator(other.fReferenceIterator)
	{
		CHK(fMyIterator == other.fMyIterator);
	}

	inline Iterator &operator++()
	{
		MyIterator &myResult = ++fMyIterator;
		ReferenceIterator &refResult = ++fReferenceIterator;
		if (refResult == fVector->fReferenceVector.end())
			CHK(myResult == fVector->fMyVector.End());
		else
			CHK(*myResult == *refResult);
		return *this;
	}

	inline Iterator operator++(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator++;
		ReferenceIterator refResult = fReferenceIterator++;
		CHK(oldMyResult == myResult);
		if (refResult == fVector->fReferenceVector.end())
			CHK(myResult == fVector->fMyVector.End());
		else
			CHK(*myResult == *refResult);
		return Iterator(fVector, myResult, refResult);
	}

	inline Iterator &operator--()
	{
		MyIterator &myResult = --fMyIterator;
		ReferenceIterator &refResult = --fReferenceIterator;
		CHK(*myResult == *refResult);
		return *this;
	}

	inline Iterator operator--(int)
	{
		MyIterator oldMyResult = fMyIterator;
		MyIterator myResult = fMyIterator--;
		ReferenceIterator refResult = fReferenceIterator--;
		CHK(oldMyResult == myResult);
		CHK(*myResult == *refResult);
		return Iterator(fVector, myResult, refResult);
	}

	inline Iterator &operator=(const Iterator &other)
	{
		fVector = other.fVector;
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

	inline Value &operator*() const
	{
		Value &result = *fMyIterator;
		CHK(result == *fReferenceIterator);
		return result;
	}

	inline Value *operator->() const
	{
		Value *result = fMyIterator.operator->();
		CHK(*result == *fReferenceIterator);
		return result;
	}

	inline operator bool() const
	{
		bool result = fMyIterator;
		CHK((fMyIterator == fVector->fMyVector.Null()) != result);
		return result;
	}

public:
	TestVector			*fVector;
	MyIterator			fMyIterator;
	ReferenceIterator	fReferenceIterator;
};

// TestVector
template<typename Value>
class TestVector {
public:
	typedef Vector<Value>::Iterator					MyIterator;
	typedef vector<Value>::iterator					ReferenceIterator;
	typedef Vector<Value>::ConstIterator			MyConstIterator;
	typedef vector<Value>::const_iterator			ReferenceConstIterator;
	typedef TestIterator<Value, TestVector<Value>, MyIterator,
						 ReferenceIterator>			Iterator;
	typedef TestIterator<const Value, const TestVector<Value>, MyConstIterator,
						 ReferenceConstIterator>	ConstIterator;

	TestVector()
		: fMyVector(),
		  fReferenceVector(),
		  fChecking(true)
	{
	}

	void PushFront(const Value &value)
	{
		CHK(fMyVector.PushFront(value) == B_OK);
		fReferenceVector.insert(fReferenceVector.begin(), value);
		Check();
	}

	void PushBack(const Value &value)
	{
		CHK(fMyVector.PushBack(value) == B_OK);
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

	void Insert(const Value &value, int32 index)
	{
		if (index >= 0 && index <= Count()) {
			CHK(fMyVector.Insert(value, index) == B_OK);
			vector<Value>::iterator it = fReferenceVector.begin();
			for (int32 i = 0; i < index; i++)
				++it;
			fReferenceVector.insert(it, value);
				// Any better way?
		} else {
			CHK(fMyVector.Insert(value, index) == B_BAD_VALUE);
		}
		Check();
	}

	void Insert(const Value &value, const Iterator &iterator)
	{

		if (iterator.fMyIterator == fMyVector.Null()) {
			CHK(fMyVector.Insert(value, iterator.fMyIterator) == B_BAD_VALUE);
		} else {
			CHK(fMyVector.Insert(value, iterator.fMyIterator) == B_OK);
			fReferenceVector.insert(iterator.fReferenceIterator, value);
		}
		Check();
	}

	void Remove(const Value &value)
	{
		int32 oldCount = Count();
		for (ReferenceIterator it = fReferenceVector.begin();
			 it != fReferenceVector.end();) {
			if (*it == value)
				it = fReferenceVector.erase(it);
			else
				++it;
		}
		int32 newCount = fReferenceVector.size();
		CHK(fMyVector.Remove(value) == oldCount - newCount);
		Check();
	}

	Iterator Erase(int32 index)
	{
		bool outOfRange = (index < 0 || index >= Count());
		MyIterator myIt = fMyVector.Erase(index);
		if (outOfRange) {
			CHK(myIt == fMyVector.Null());
			return Iterator(this, myIt, fReferenceVector.end());
		}
		ReferenceIterator it = fReferenceVector.begin();
		for (int32 i = 0; i < index; i++)
			++it;
		ReferenceIterator refIt = fReferenceVector.erase(it);
			// Any better way?
		Check();
		if (refIt == fReferenceVector.end())
			CHK(myIt == fMyVector.End());
		else
			CHK(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	Iterator Erase(const Iterator &iterator)
	{
		bool outOfRange
			= (iterator.fReferenceIterator == fReferenceVector.end());
		MyIterator myIt = fMyVector.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CHK(myIt == fMyVector.Null());
			return Iterator(this, myIt, fReferenceVector.end());
		}
		ReferenceIterator refIt
			= fReferenceVector.erase(iterator.fReferenceIterator);
		Check();
		if (refIt == fReferenceVector.end())
			CHK(myIt == fMyVector.End());
		else
			CHK(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceVector.size();
		CHK(fMyVector.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceVector.empty();
		CHK(fMyVector.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMyVector.MakeEmpty();
		fReferenceVector.clear();
		Check();
	}

	inline Iterator Begin()
	{
		return Iterator(this, fMyVector.Begin(), fReferenceVector.begin());
	}

	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMyVector.Begin(),
							 fReferenceVector.begin());
	}

	inline Iterator End()
	{
		return Iterator(this, fMyVector.End(), fReferenceVector.end());
	}

	inline ConstIterator End() const
	{
		return ConstIterator(this, fMyVector.End(), fReferenceVector.end());
	}

	inline Iterator Null()
	{
		return Iterator(this, fMyVector.Null(), fReferenceVector.end());
	}

	inline ConstIterator Null() const
	{
		return ConstIterator(this, fMyVector.Null(), fReferenceVector.end());
	}

	inline Iterator IteratorForIndex(int32 index)
	{
		if (index < 0 || index > Count()) {
			CHK(fMyVector.IteratorForIndex(index) == fMyVector.End());
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
			CHK(fMyVector.IteratorForIndex(index) == fMyVector.End());
			return End();
		} else {
			MyIterator myIt = fMyVector.IteratorForIndex(index);
			ReferenceIterator it = fReferenceVector.begin();
			for (int32 i = 0; i < index; i++)
				++it;
			return ConstIterator(this, myIt, it);
		}
	}

	inline const Value &ElementAt(int32 index) const
	{
		const Value &result = fReferenceVector[index];
		CHK(result == fMyVector.ElementAt(index));
		return result;
	}

	inline Value &ElementAt(int32 index)
	{
		Value &result = fReferenceVector[index];
		CHK(result == fMyVector.ElementAt(index));
		return result;
	}

	int32 IndexOf(const Value &value, int32 start = 0) const
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
		CHK(result == fMyVector.IndexOf(value, start));
		return result;
	}

	Iterator Find(const Value &value)
	{
		Iterator start(Begin());
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				 ++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyIterator myIt = fMyVector.Find(value);
					CHK(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CHK(fMyVector.Find(value) == fMyVector.End());
			CHK(start.fMyIterator == fMyVector.End());
			return start;
		}
		CHK(fMyVector.Find(value) == fMyVector.End());
		return start;
	}

	Iterator Find(const Value &value, const Iterator &_start)
	{
		Iterator start(_start);
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				 ++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyIterator myIt = fMyVector.Find(value,
													 _start.fMyIterator);
					CHK(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CHK(fMyVector.Find(value, _start.fMyIterator) == fMyVector.End());
			CHK(start.fMyIterator == fMyVector.End());
			return start;
		}
		CHK(fMyVector.Find(value, start.fMyIterator) == fMyVector.End());
		return start;
	}

	ConstIterator Find(const Value &value) const
	{
		ConstIterator start(Begin());
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				 ++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyConstIterator myIt = fMyVector.Find(value);
					CHK(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CHK(fMyVector.Find(value) == fMyVector.End());
			CHK(start.fMyIterator == fMyVector.End());
			return start;
		}
		CHK(fMyVector.Find(value) == fMyVector.End());
		return start;
	}

	ConstIterator Find(const Value &value, const ConstIterator &_start) const
	{
		ConstIterator start(_start);
		if (start.fReferenceIterator != fReferenceVector.end()) {
			for (; start.fReferenceIterator != fReferenceVector.end();
				 ++start.fReferenceIterator, ++start.fMyIterator) {
				if (*start.fReferenceIterator == value) {
					MyConstIterator myIt = fMyVector.Find(value,
						_start.fMyIterator);
					CHK(&*myIt == &*start.fMyIterator);
					return start;
				}
			}
			CHK(fMyVector.Find(value, _start.fMyIterator) == fMyVector.End());
			CHK(start.fMyIterator == fMyVector.End());
			return start;
		}
		CHK(fMyVector.Find(value, start.fMyIterator) == fMyVector.End());
		return start;
	}

	inline Value &operator[](int32 index)
	{
		Value &result = fMyVector[index];
		CHK(result == fReferenceVector[index]);
		return result;
	}

	inline const Value &operator[](int32 index) const
	{
		const Value &result = fMyVector[index];
		CHK(result == fReferenceVector[index]);
		return result;
	}

	void SetChecking(bool enable)
	{
		fChecking = enable;
	}

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceVector.size();
			CHK(fMyVector.Count() == count);
			CHK(fMyVector.IsEmpty() == fReferenceVector.empty());
			for (int32 i = 0; i < count; i++)
				CHK(fMyVector[i] == fReferenceVector[i]);
		}
	}

//private:
public:
	Vector<Value>	fMyVector;
	vector<Value>	fReferenceVector;
	bool			fChecking;
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

// GenericPushPopFrontTest
template<typename ValueStrategy>
static
void
GenericPushPopFrontTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
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

// PushPopFrontTest
void
VectorTest::PushPopFrontTest()
{
	GenericPushPopFrontTest(IntStrategy(), 30);
	GenericPushPopFrontTest(IntStrategy(), 200);
	GenericPushPopFrontTest(StringStrategy(), 30);
	GenericPushPopFrontTest(StringStrategy(), 200);
}

// GenericPushPopBackTest
template<typename ValueStrategy>
static
void
GenericPushPopBackTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
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

// PushPopBackTest
void
VectorTest::PushPopBackTest()
{
	GenericPushPopBackTest(IntStrategy(), 30);
	GenericPushPopBackTest(IntStrategy(), 200);
	GenericPushPopBackTest(StringStrategy(), 30);
	GenericPushPopBackTest(StringStrategy(), 200);
}

// GenericInsertTest1
template<typename ValueStrategy>
static
void
GenericInsertTest1(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	for (int32 i = 0; i < maxNumber; i++) {
		int32 index = rand() % (i + 1);
		v.Insert(strategy.Generate(), index);
	}
}

// GenericInsertTest2
template<typename ValueStrategy>
static
void
GenericInsertTest2(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	for (int32 i = 0; i < maxNumber; i++) {
		int32 index = rand() % (i + 1);
		v.Insert(strategy.Generate(), v.IteratorForIndex(index));
	}
}

// InsertTest
void
VectorTest::InsertTest()
{
	NextSubTest();
	GenericInsertTest1(IntStrategy(), 30);
	NextSubTest();
	GenericInsertTest2(IntStrategy(), 30);
	NextSubTest();
	GenericInsertTest1(IntStrategy(), 200);
	NextSubTest();
	GenericInsertTest2(IntStrategy(), 200);
	NextSubTest();
	GenericInsertTest1(StringStrategy(), 30);
	NextSubTest();
	GenericInsertTest2(StringStrategy(), 30);
	NextSubTest();
	GenericInsertTest1(StringStrategy(), 200);
	NextSubTest();
	GenericInsertTest2(StringStrategy(), 200);
}

// GenericFill
template<typename Value, typename ValueStrategy>
static
void
GenericFill(TestVector<Value> &v, ValueStrategy strategy, int32 maxNumber)
{
	v.SetChecking(false);
	for (int32 i = 0; i < maxNumber; i++) {
		int32 index = rand() % (i + 1);
		v.Insert(strategy.Generate(), index);
	}
	v.SetChecking(true);
	v.Check();
}

// GenericRemoveTest
template<typename ValueStrategy>
static
void
GenericRemoveTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	while (v.Count() > 0) {
		int32 index = rand() % (v.Count());
		Value value = v[index];
		v.Remove(value);
		v.Remove(value);
	}
}

// RemoveTest
void
VectorTest::RemoveTest()
{
	NextSubTest();
	GenericRemoveTest(IntStrategy(), 30);
	NextSubTest();
	GenericRemoveTest(IntStrategy(10), 30);
	NextSubTest();
	GenericRemoveTest(IntStrategy(), 200);
	NextSubTest();
	GenericRemoveTest(IntStrategy(20), 200);
	NextSubTest();
	GenericRemoveTest(StringStrategy(), 30);
	NextSubTest();
	GenericRemoveTest(StringStrategy(10), 30);
	NextSubTest();
	GenericRemoveTest(StringStrategy(), 200);
	NextSubTest();
	GenericRemoveTest(StringStrategy(20), 200);
}

// GenericEraseTest1
template<typename ValueStrategy>
static
void
GenericEraseTest1(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	for (int32 i = maxNumber - 1; i >= 0; i--) {
		int32 index = rand() % (i + 1);
		v.Erase(index);
	}
}

// GenericEraseTest2
template<typename ValueStrategy>
static
void
GenericEraseTest2(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	for (int32 i = maxNumber - 1; i >= 0; i--) {
		int32 index = rand() % (i + 1);
		v.Erase(v.IteratorForIndex(index));
	}
}

// EraseTest
void
VectorTest::EraseTest()
{
	NextSubTest();
	GenericEraseTest1(IntStrategy(), 30);
	NextSubTest();
	GenericEraseTest2(IntStrategy(), 30);
	NextSubTest();
	GenericEraseTest1(IntStrategy(), 200);
	NextSubTest();
	GenericEraseTest2(IntStrategy(), 200);
	NextSubTest();
	GenericEraseTest1(StringStrategy(), 30);
	NextSubTest();
	GenericEraseTest2(StringStrategy(), 30);
	NextSubTest();
	GenericEraseTest1(StringStrategy(), 200);
	NextSubTest();
	GenericEraseTest2(StringStrategy(), 200);
}

// GenericMakeEmptyTest
template<typename ValueStrategy>
static
void
GenericMakeEmptyTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	v.MakeEmpty();
	GenericFill(v, strategy, maxNumber);
	v.MakeEmpty();
	v.MakeEmpty();
}

// MakeEmptyTest
void
VectorTest::MakeEmptyTest()
{
	NextSubTest();
	GenericMakeEmptyTest(IntStrategy(), 30);
	NextSubTest();
	GenericMakeEmptyTest(IntStrategy(), 200);
	NextSubTest();
	GenericMakeEmptyTest(StringStrategy(), 30);
	NextSubTest();
	GenericMakeEmptyTest(StringStrategy(), 200);
}

// GenericIndexAccessTest
template<typename ValueStrategy>
static
void
GenericIndexAccessTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	const TestVector<Value> &cv = v;
	for (int32 i = 0; i < maxNumber; i++) {
		CHK(v[i] == cv[i]);
		CHK(v.ElementAt(i) == cv.ElementAt(i));
	}
}

// IndexAccessTest
void
VectorTest::IndexAccessTest()
{
	NextSubTest();
	GenericMakeEmptyTest(IntStrategy(), 300);
	NextSubTest();
	GenericMakeEmptyTest(IntStrategy(), 2000);
	NextSubTest();
	GenericMakeEmptyTest(StringStrategy(), 300);
	NextSubTest();
	GenericMakeEmptyTest(StringStrategy(), 2000);
}

// GenericFindTest
template<typename ValueStrategy>
static
void
GenericFindTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	// find the values in the vector
	const TestVector<Value> &cv = v;
	for (int32 i = 0; i < maxNumber; i++) {
		TestVector<Value>::ConstIterator cit = cv.Begin();
		int32 index = 0;
		for (TestVector<Value>::Iterator it = v.Begin(); it != v.End(); ) {
			CHK(&v[index] == &*it);
			CHK(&cv[index] == &*cit);
			CHK(*it == *cit);
			it = v.Find(v[i], ++it);
			cit = cv.Find(cv[i], ++cit);
			index = v.IndexOf(v[i], index + 1);
		}
	}
	// try to find some random values
	for (int32 i = 0; i < maxNumber; i++) {
		Value value = strategy.Generate();
		TestVector<Value>::Iterator it = v.Find(value);
		TestVector<Value>::ConstIterator cit = cv.Find(value);
		if (it != v.End())
			CHK(&*it == &*cit);
	}
}

// FindTest
void
VectorTest::FindTest()
{
	NextSubTest();
	GenericFindTest(IntStrategy(), 30);
	NextSubTest();
	GenericFindTest(IntStrategy(10), 30);
	NextSubTest();
	GenericFindTest(IntStrategy(), 200);
	NextSubTest();
	GenericFindTest(IntStrategy(50), 200);
	NextSubTest();
	GenericFindTest(StringStrategy(), 30);
	NextSubTest();
	GenericFindTest(StringStrategy(10), 30);
	NextSubTest();
	GenericFindTest(StringStrategy(), 200);
	NextSubTest();
	GenericFindTest(StringStrategy(50), 200);
}

// GenericIteratorTest
template<typename ValueStrategy>
static
void
GenericIteratorTest(ValueStrategy strategy, int32 maxNumber)
{
	typedef typename ValueStrategy::Value Value;
	TestVector<Value> v;
	GenericFill(v, strategy, maxNumber);
	const TestVector<Value> &cv = v;
	TestVector<Value>::Iterator it = v.Begin();
	TestVector<Value>::ConstIterator cit = cv.Begin();
	for (; it != v.End(); ++it, ++cit) {
		CHK(&*it == &*cit);
		CHK(&*it == it.operator->());
		CHK(&*cit == cit.operator->());
		CHK(it);
		CHK(cit);
	}
	CHK(cit == cv.End());
	while (it != v.Begin()) {
		--it;
		--cit;
		CHK(&*it == &*cit);
		CHK(&*it == it.operator->());
		CHK(&*cit == cit.operator->());
		CHK(it);
		CHK(cit);
	}
	CHK(cit == cv.Begin());
	CHK(!v.Null());
	CHK(!cv.Null());
}

// IteratorTest
void
VectorTest::IteratorTest()
{
	NextSubTest();
	GenericIteratorTest(IntStrategy(), 0);
	NextSubTest();
	GenericIteratorTest(IntStrategy(), 300);
	NextSubTest();
	GenericIteratorTest(IntStrategy(), 2000);
	NextSubTest();
	GenericIteratorTest(StringStrategy(), 0);
	NextSubTest();
	GenericIteratorTest(StringStrategy(), 300);
	NextSubTest();
	GenericIteratorTest(StringStrategy(), 2000);
}

