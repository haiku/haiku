/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * Andrew Aldridge, i80and@foxquill.com
 */


#ifndef CRYPT_TEST_H
#define CRYPT_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class CryptTest : public CppUnit::TestCase {
public:
				CryptTest();
	virtual			~CryptTest();

	virtual	void		setUp();
	virtual	void		tearDown();

		void		TestLegacy();
		void		TestLegacyBSD();
		void		TestCustomSalt();
		void		TestSaltGeneration();
		void		TestBadSalt();
		void		TestCryptR();

	static	void		AddTests(BTestSuite& suite);
};


#endif  // CRYPT_TEST_H
