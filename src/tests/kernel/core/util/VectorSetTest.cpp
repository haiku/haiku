
#include <stdio.h>
#include <stdlib.h>

#include <set>

#include <TestUtils.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <VectorSet.h>

#include "common.h"
#include "VectorSetTest.h"

using VectorSetOrder::Ascending;
using VectorSetOrder::Descending;

VectorSetTest::VectorSetTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
VectorSetTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("VectorSet");

	ADD_TEST4(VectorSet, suite, VectorSetTest, ConstructorTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, InsertTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, RemoveTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, EraseTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, MakeEmptyTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, FindTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, FindCloseTest);
	ADD_TEST4(VectorSet, suite, VectorSetTest, IteratorTest);

	return suite;
}

//! ConstructorTest
void
VectorSetTest::ConstructorTest()
{
	NextSubTest();
	VectorSet<int> v1(100);
	CHK(v1.Count() == 0);
	CHK(v1.IsEmpty());

	NextSubTest();
	VectorSet<string> v2(100);
	CHK(v2.Count() == 0);
	CHK(v2.IsEmpty());

	NextSubTest();
	VectorSet<int> v3(0);
	CHK(v3.Count() == 0);
	CHK(v3.IsEmpty());

	NextSubTest();
	VectorSet<string> v4(0);
	CHK(v4.Count() == 0);
	CHK(v4.IsEmpty());
}

// TestIterator
template<typename Value, typename TestSet, typename MyIterator,
		 typename ReferenceIterator>
class TestIterator {
private:
	typedef TestIterator<Value, TestSet, MyIterator, ReferenceIterator>
			Iterator;

public:
	inline TestIterator(TestSet *s, MyIterator myIt, ReferenceIterator refIt)
		: fSet(s),
		  fMyIterator(myIt),
		  fReferenceIterator(refIt)
	{
	}

	inline TestIterator(const Iterator &other)
		: fSet(other.fSet),
		  fMyIterator(other.fMyIterator),
		  fReferenceIterator(other.fReferenceIterator)
	{
		CHK(fMyIterator == other.fMyIterator);
	}

	inline Iterator &operator++()
	{
		MyIterator &myResult = ++fMyIterator;
		ReferenceIterator &refResult = ++fReferenceIterator;
		if (refResult == fSet->fReferenceSet.end())
			CHK(myResult == fSet->fMySet.End());
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
		if (refResult == fSet->fReferenceSet.end())
			CHK(myResult == fSet->fMySet.End());
		else
			CHK(*myResult == *refResult);
		return Iterator(fSet, myResult, refResult);
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
		return Iterator(fSet, myResult, refResult);
	}

	inline Iterator &operator=(const Iterator &other)
	{
		fSet = other.fSet;
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
		CHK((fMyIterator == fSet->fMySet.Null()) != result);
		return result;
	}

public:
	TestSet				*fSet;
	MyIterator			fMyIterator;
	ReferenceIterator	fReferenceIterator;
};

// TestSet
template<typename Value, typename MySet, typename ReferenceSet,
		 typename Compare>
class TestSet {
public:
	typedef TestSet<Value, MySet, ReferenceSet, Compare>	Class;

	typedef typename MySet::Iterator				MyIterator;
	typedef typename ReferenceSet::iterator			ReferenceIterator;
	typedef typename MySet::ConstIterator			MyConstIterator;
	typedef typename ReferenceSet::const_iterator	ReferenceConstIterator;
	typedef TestIterator<Value, Class, MyIterator,
						 ReferenceIterator>			Iterator;
	typedef TestIterator<const Value, const Class, MyConstIterator,
						 ReferenceConstIterator>	ConstIterator;

	TestSet()
		: fMySet(),
		  fReferenceSet(),
		  fChecking(true)
	{
	}

	void Insert(const Value &value, bool replace = true)
	{
		CHK(fMySet.Insert(value, replace) == B_OK);
		ReferenceIterator it = fReferenceSet.find(value);
		if (it != fReferenceSet.end())
			fReferenceSet.erase(it);
		fReferenceSet.insert(value);
		Check();
	}

	void Remove(const Value &value)
	{
		int32 oldCount = Count();
		fReferenceSet.erase(value);
		int32 newCount = fReferenceSet.size();
		CHK(fMySet.Remove(value) == oldCount - newCount);
		Check();
	}

	Iterator Erase(const Iterator &iterator)
	{
		bool outOfRange
			= (iterator.fReferenceIterator == fReferenceSet.end());
		MyIterator myIt = fMySet.Erase(iterator.fMyIterator);
		if (outOfRange) {
			CHK(myIt == fMySet.Null());
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
			CHK(myIt == fMySet.End());
		else
			CHK(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	inline int32 Count() const
	{
		int32 count = fReferenceSet.size();
		CHK(fMySet.Count() == count);
		return count;
	}

	inline bool IsEmpty() const
	{
		bool result = fReferenceSet.empty();
		CHK(fMySet.IsEmpty() == result);
		return result;
	}

	void MakeEmpty()
	{
		fMySet.MakeEmpty();
		fReferenceSet.clear();
		Check();
	}

	inline Iterator Begin()
	{
		return Iterator(this, fMySet.Begin(), fReferenceSet.begin());
	}

	inline ConstIterator Begin() const
	{
		return ConstIterator(this, fMySet.Begin(),
							 fReferenceSet.begin());
	}

	inline Iterator End()
	{
		return Iterator(this, fMySet.End(), fReferenceSet.end());
	}

	inline ConstIterator End() const
	{
		return ConstIterator(this, fMySet.End(), fReferenceSet.end());
	}

	inline Iterator Null()
	{
		return Iterator(this, fMySet.Null(), fReferenceSet.end());
	}

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

	Iterator Find(const Value &value)
	{
		MyIterator myIt = fMySet.Find(value);
		ReferenceIterator refIt = fReferenceSet.find(value);
		if (refIt == fReferenceSet.end())
			CHK(myIt = fMySet.End());
		else
			CHK(*myIt == *refIt);
		return Iterator(this, myIt, refIt);
	}

	ConstIterator Find(const Value &value) const
	{
		MyConstIterator myIt = fMySet.Find(value);
		ReferenceConstIterator refIt = fReferenceSet.find(value);
		if (refIt == fReferenceSet.end())
			CHK(myIt = fMySet.End());
		else
			CHK(*myIt == *refIt);
		return ConstIterator(this, myIt, refIt);
	}

	Iterator FindClose(const Value &value, bool less)
	{
		MyIterator myIt = fMySet.FindClose(value, less);
		if (myIt == fMySet.End()) {
			if (fMySet.Count() > 0) {
				if (less)
					CHK(fCompare(*fMySet.Begin(), value) > 0);
				else
					CHK(fCompare(*--MyIterator(myIt), value) < 0);
			}
			return End();
		}
		if (less) {
			CHK(fCompare(*myIt, value) <= 0);
			MyIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMySet.End())
				CHK(fCompare(*nextMyIt, value) > 0);
		} else {
			CHK(fCompare(*myIt, value) >= 0);
			if (myIt != fMySet.Begin()) {
				MyIterator prevMyIt(myIt);
				--prevMyIt;
				CHK(fCompare(*prevMyIt, value) < 0);
			}
		}
		return Iterator(this, myIt, fReferenceSet.find(*myIt));
	}

	ConstIterator FindClose(const Value &value, bool less) const
	{
		MyConstIterator myIt = fMySet.FindClose(value, less);
		if (myIt == fMySet.End()) {
			if (fMySet.Count() > 0) {
				if (less)
					CHK(fCompare(*fMySet.Begin(), value) > 0);
				else
					CHK(fCompare(*--MyConstIterator(myIt), value) < 0);
			}
			return End();
		}
		if (less) {
			CHK(fCompare(*myIt, value) <= 0);
			MyConstIterator nextMyIt(myIt);
			++nextMyIt;
			if (nextMyIt != fMySet.End())
				CHK(fCompare(*nextMyIt, value) > 0);
		} else {
			CHK(fCompare(*myIt, value) >= 0);
			if (myIt != fMySet.Begin()) {
				MyConstIterator prevMyIt(myIt);
				--prevMyIt;
				CHK(fCompare(*prevMyIt, value) < 0);
			}
		}
		return ConstIterator(this, myIt, fReferenceSet.find(*myIt));
	}

	void SetChecking(bool enable)
	{
		fChecking = enable;
	}

	void Check() const
	{
		if (fChecking) {
			int32 count = fReferenceSet.size();
			CHK(fMySet.Count() == count);
			CHK(fMySet.IsEmpty() == fReferenceSet.empty());
			MyConstIterator myIt = fMySet.Begin();
			ReferenceConstIterator refIt = fReferenceSet.begin();
			for (int32 i = 0; i < count; i++, ++myIt, ++refIt)
				CHK(*myIt == *refIt);
			CHK(myIt == fMySet.End());
		}
	}

//private:
public:
	MySet			fMySet;
	ReferenceSet	fReferenceSet;
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
template<typename _ValueStrategy, template<typename> class _CompareStrategy>
class TestStrategy {
public:
	typedef _ValueStrategy									ValueStrategy;
	typedef typename ValueStrategy::Value					Value;
	typedef _CompareStrategy<Value>							Compare;
	typedef CompareWrapper<Value, Compare>					BoolCompare;
	typedef VectorSet<Value, Compare>						MySet;
	typedef set<Value, BoolCompare>							ReferenceSet;
	typedef TestSet<Value, MySet, ReferenceSet, Compare>	TestClass;
};

typedef TestStrategy<IntStrategy, Ascending>		AIntTestStrategy;
typedef TestStrategy<StringStrategy, Ascending>		AStringTestStrategy;
typedef TestStrategy<IntStrategy, Descending>		DIntTestStrategy;
typedef TestStrategy<StringStrategy, Descending>	DStringTestStrategy;

// GenericInsertTest
template<typename _TestStrategy>
static
void
GenericInsertTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	ValueStrategy strategy;
	TestClass v;
	for (int32 i = 0; i < maxNumber; i++)
		v.Insert(strategy.Generate());
}

// InsertTest
void
VectorSetTest::InsertTest()
{
	NextSubTest();
	GenericInsertTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericInsertTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericInsertTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericInsertTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericInsertTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericInsertTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericInsertTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericInsertTest<DStringTestStrategy>(200);
}

// GenericFill
template<typename TestClass, typename ValueStrategy>
static
void
GenericFill(TestClass &v, ValueStrategy strategy, int32 maxNumber)
{
	v.SetChecking(false);
	for (int32 i = 0; v.Count() < maxNumber; i++)
		v.Insert(strategy.Generate());
	v.SetChecking(true);
	v.Check();
}

// GenericRemoveTest
template<typename _TestStrategy>
static
void
GenericRemoveTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	ValueStrategy strategy;
	TestClass v;
	GenericFill(v, strategy, maxNumber);
	while (v.Count() > 0) {
		int32 index = rand() % (v.Count());
		Value value = *v.IteratorForIndex(index);
		v.Remove(value);
		v.Remove(value);
	}
}

// RemoveTest
void
VectorSetTest::RemoveTest()
{
	NextSubTest();
	GenericRemoveTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericRemoveTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericRemoveTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericRemoveTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericRemoveTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericRemoveTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericRemoveTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericRemoveTest<DStringTestStrategy>(200);
}

// GenericEraseTest
template<typename _TestStrategy>
static
void
GenericEraseTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	ValueStrategy strategy;
	TestClass v;
	GenericFill(v, strategy, maxNumber);
	for (int32 i = maxNumber - 1; i >= 0; i--) {
		int32 index = rand() % (i + 1);
		v.Erase(v.IteratorForIndex(index));
	}
}

// EraseTest
void
VectorSetTest::EraseTest()
{
	NextSubTest();
	GenericEraseTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericEraseTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericEraseTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericEraseTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericEraseTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericEraseTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericEraseTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericEraseTest<DStringTestStrategy>(200);
}

// GenericMakeEmptyTest
template<typename _TestStrategy>
static
void
GenericMakeEmptyTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	ValueStrategy strategy;
	TestClass v;
	v.MakeEmpty();
	GenericFill(v, strategy, maxNumber);
	v.MakeEmpty();
	v.MakeEmpty();
}

// MakeEmptyTest
void
VectorSetTest::MakeEmptyTest()
{
	NextSubTest();
	GenericMakeEmptyTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericMakeEmptyTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericMakeEmptyTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericMakeEmptyTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericMakeEmptyTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericMakeEmptyTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericMakeEmptyTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericMakeEmptyTest<DStringTestStrategy>(200);
}

// GenericFindTest
template<typename _TestStrategy>
static
void
GenericFindTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	ValueStrategy strategy;
	TestClass v;
	GenericFill(v, strategy, maxNumber);
	const TestClass &cv = v;
	// find the values in the set
	for (int32 i = 0; i < maxNumber; i++) {
		const Value &value = *v.IteratorForIndex(i);
		Iterator it = v.Find(value);
		ConstIterator cit = cv.Find(value);
		CHK(&*it == &*cit);
	}
	// try to find some random values
	for (int32 i = 0; i < maxNumber; i++) {
		Value value = strategy.Generate();
		Iterator it = v.Find(value);
		ConstIterator cit = cv.Find(value);
		if (it != v.End())
			CHK(&*it == &*cit);
	}
}

// FindTest
void
VectorSetTest::FindTest()
{
	NextSubTest();
	GenericFindTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericFindTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericFindTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericFindTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericFindTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericFindTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericFindTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericFindTest<DStringTestStrategy>(200);
}

// GenericFindCloseTest
template<typename _TestStrategy>
static
void
GenericFindCloseTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	ValueStrategy strategy;
	TestClass v;
	GenericFill(v, strategy, maxNumber);
	const TestClass &cv = v;
	// find the values in the set
	for (int32 i = 0; i < maxNumber; i++) {
		const Value &value = *v.IteratorForIndex(i);
		// less
		Iterator it = v.FindClose(value, true);
		ConstIterator cit = cv.FindClose(value, true);
		CHK(*it == value);
		CHK(&*it == &*cit);
		// greater
		it = v.FindClose(value, false);
		cit = cv.FindClose(value, false);
		CHK(*it == value);
		CHK(&*it == &*cit);
	}
	// try to find some random values
	for (int32 i = 0; i < maxNumber; i++) {
		Value value = strategy.Generate();
		// less
		Iterator it = v.FindClose(value, true);
		ConstIterator cit = cv.FindClose(value, true);
		if (it != v.End())
			CHK(&*it == &*cit);
		// greater
		it = v.FindClose(value, false);
		cit = cv.FindClose(value, false);
		if (it != v.End())
			CHK(&*it == &*cit);
	}
}

// FindCloseTest
void
VectorSetTest::FindCloseTest()
{
	NextSubTest();
	GenericFindCloseTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericFindCloseTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericFindCloseTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericFindCloseTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericFindCloseTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericFindCloseTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericFindCloseTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericFindCloseTest<DStringTestStrategy>(200);
}

// GenericIteratorTest
template<typename _TestStrategy>
static
void
GenericIteratorTest(int32 maxNumber)
{
	typedef typename _TestStrategy::ValueStrategy	ValueStrategy;
	typedef typename _TestStrategy::Value			Value;
	typedef typename _TestStrategy::TestClass		TestClass;
	typedef typename TestClass::Iterator			Iterator;
	typedef typename TestClass::ConstIterator		ConstIterator;
	ValueStrategy strategy;
	TestClass v;
	GenericFill(v, strategy, maxNumber);
	const TestClass &cv = v;
	Iterator it = v.Begin();
	ConstIterator cit = cv.Begin();
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
VectorSetTest::IteratorTest()
{
	NextSubTest();
	GenericIteratorTest<AIntTestStrategy>(30);
	NextSubTest();
	GenericIteratorTest<DIntTestStrategy>(30);
	NextSubTest();
	GenericIteratorTest<AIntTestStrategy>(200);
	NextSubTest();
	GenericIteratorTest<DIntTestStrategy>(200);
	NextSubTest();
	GenericIteratorTest<AStringTestStrategy>(30);
	NextSubTest();
	GenericIteratorTest<DStringTestStrategy>(30);
	NextSubTest();
	GenericIteratorTest<AStringTestStrategy>(200);
	NextSubTest();
	GenericIteratorTest<DStringTestStrategy>(200);
}

