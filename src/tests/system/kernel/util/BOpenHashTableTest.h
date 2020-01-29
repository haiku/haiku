#ifndef BOPENHASHTABLE_TEST_H
#define BOPENHASHTABLE_TEST_H

#include <TestCase.h>

class BOpenHashTableTest : public BTestCase {
public:
	BOpenHashTableTest(std::string name = "");

	void InsertTest();
	void InsertUncheckedTest();
	void InsertUncheckedUninitializedTest();
	void IterateAndCountTest();
	void ResizeTest();
	void LookupTest();
	void RemoveTest();
	void RemoveUncheckedTest();
	void RemoveWhenNotPresentTest();
	void DuplicateInsertTest();
	void DisableAutoExpandTest();
	void InitWithZeroSizeTest();
	void ClearTest();
	void ClearAndReturnTest();

	static CppUnit::Test* Suite();
};

#endif // BOPENHASHTABLE_TEST_H
