
#include <stdio.h>
#include <stdlib.h>

#include <map>

#include <TestUtils.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <VectorMap.h>

#include "common.h"
#include "OrderedMapTest.h"
#include "VectorMapTest.h"

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
		: public VectorMap<Key, Value, VectorMapEntryStrategy::Pair<Key, Value,
						   CompareStrategy> > {
	};
};

#define DECLARE_TEST_STRATEGY(Key, Value, Map, Strategy, ClassName) \
	template<typename CS> struct Map : PairTestBase<Key, Value>::MyMap<CS> {};\
	template<typename CS> \
	struct Strategy \
		: public TestStrategy<Map, PairTestBase<Key, Value>::EntryStrategy, \
							  CS> { \
		static const char *kClassName; \
	}; \
	template<typename CS> const char *Strategy<CS>::kClassName = ClassName;

DECLARE_TEST_STRATEGY(int, int, IntIntMap, IntIntTestStrategy,
					  "VectorMap<int, int, Pair>")
DECLARE_TEST_STRATEGY(int, string, IntStringMap, IntStringTestStrategy,
					  "VectorMap<int, string, Pair>")
DECLARE_TEST_STRATEGY(string, int, StringIntMap, StringIntTestStrategy,
					  "VectorMap<string, int, Pair>")
DECLARE_TEST_STRATEGY(string, string, StringStringMap,
					  StringStringTestStrategy,
					  "VectorMap<string, string, Pair>")


// TestStrategy for the ImplicitKey entry strategy

// string_hash (from the Dragon Book: a slightly modified hashpjw())
static inline
int
string_hash(const char *name)
{
	uint32 h = 0;
	for (; *name; name++) {
		if (uint32 g = g & 0xf0000000)
			h ^= g >> 24;
		h = (h << 4) + *name;
	}
	return (int)h;
}

struct ImplicitKeyTestGetKey {
	int operator()(string value) const
	{
		return string_hash(value.c_str());
	}
};

template<typename CompareStrategy>
struct ImplicitKeyTestMap
	: public VectorMap<int, string,
		VectorMapEntryStrategy::ImplicitKey<int, string, ImplicitKeyTestGetKey,
											CompareStrategy> > {
};

typedef ImplicitKeyStrategy<SimpleValueStrategy<int>,
							SimpleValueStrategy<string>,
							ImplicitKeyTestGetKey>
	ImplicitKeyTestEntryStrategy;

template<class CompareStrategy>
struct ImplicitKeyTestStrategy : public TestStrategy<
	ImplicitKeyTestMap, ImplicitKeyTestEntryStrategy, CompareStrategy> {
	static const char *kClassName;
};
template<class CompareStrategy>
const char *ImplicitKeyTestStrategy<CompareStrategy>::kClassName
	= "VectorMap<int, string, ImplicitKey>";


// constructor
VectorMapTest::VectorMapTest(std::string name)
	: BTestCase(name)
{
}

// Suite
CppUnit::Test*
VectorMapTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("VectorMap");

	suite->addTest(OrderedMapTest<IntIntTestStrategy>::Suite());
	suite->addTest(OrderedMapTest<IntStringTestStrategy>::Suite());
	suite->addTest(OrderedMapTest<StringIntTestStrategy>::Suite());
	suite->addTest(OrderedMapTest<StringStringTestStrategy>::Suite());
	suite->addTest(OrderedMapTest<ImplicitKeyTestStrategy>::Suite());

	return suite;
}

