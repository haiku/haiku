/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEMANGLE_TEST
#define DEMANGLE_TEST


#include <TestCase.h>
#include <TestSuite.h>


class DemangleTest : public BTestCase {
public:
					DemangleTest();
	virtual			~DemangleTest();

			void	RunGCC2Tests();
			void	RunGCC3PTests();

	static	void	AddTests(BTestSuite& suite);
};


#endif // DEMANGLE_TEST
