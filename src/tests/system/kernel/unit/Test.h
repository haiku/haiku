/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_H
#define TEST_H


#include "TestContext.h"


class TestSuite;
class TestVisitor;


class Test {
public:
								Test(const char* name);
	virtual						~Test();

			const char*			Name() const	{ return fName; }

			TestSuite*			Suite() const	{ return fSuite; }
			void				SetSuite(TestSuite* suite);

	virtual	bool				IsLeafTest() const;
	virtual	status_t			Setup(TestContext& context);
	virtual	bool				Run(TestContext& context) = 0;
	virtual	bool				Run(TestContext& context, const char* name);
	virtual	void				Cleanup(TestContext& context, bool setupOK);

	virtual	Test*				Visit(TestVisitor& visitor);

private:
			const char*			fName;
			TestSuite*			fSuite;
};


class StandardTestDelegate {
public:
								StandardTestDelegate();
	virtual						~StandardTestDelegate();

	virtual	status_t			Setup(TestContext& context);
	virtual	void				Cleanup(TestContext& context, bool setupOK);
};


template<typename TestClass>
class StandardTest : public Test {
public:
								StandardTest(const char* name,
									TestClass* object,
									bool (TestClass::*method)(TestContext&));
	virtual						~StandardTest();

	virtual	status_t			Setup(TestContext& context);
	virtual	bool				Run(TestContext& context);
	virtual	void				Cleanup(TestContext& context, bool setupOK);

private:
			TestClass*			fObject;
			bool				(TestClass::*fMethod)(TestContext&);
};


template<typename TestClass>
StandardTest<TestClass>::StandardTest(const char* name, TestClass* object,
	bool (TestClass::*method)(TestContext&))
	:
	Test(name),
	fObject(object),
	fMethod(method)
{
}


template<typename TestClass>
StandardTest<TestClass>::~StandardTest()
{
	delete fObject;
}


template<typename TestClass>
status_t
StandardTest<TestClass>::Setup(TestContext& context)
{
	return fObject->Setup(context);
}


template<typename TestClass>
bool
StandardTest<TestClass>::Run(TestContext& context)
{
	return (fObject->*fMethod)(context);
}


template<typename TestClass>
void
StandardTest<TestClass>::Cleanup(TestContext& context, bool setupOK)
{
	fObject->Cleanup(context, setupOK);
}


#define TEST_ASSERT(condition)											\
	do {																\
		if (!(condition)) {												\
			TestContext::Current()->AssertFailed(__FILE__, __LINE__,	\
				__PRETTY_FUNCTION__, #condition);						\
			return false;												\
		}																\
	} while (false)

#define TEST_ASSERT_PRINT(condition, format...)							\
	do {																\
		if (!(condition)) {												\
			TestContext::Current()->AssertFailed(__FILE__, __LINE__,	\
				__PRETTY_FUNCTION__, #condition, format);				\
			return false;												\
		}																\
	} while (false)

#define TEST_PROPAGATE(result)											\
	do {																\
		if (!(result))													\
			return false;												\
	} while (false)


#endif	// TEST_H
