/*
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CPPUNIT_TEST_ASSERT_H_
#define _CPPUNIT_TEST_ASSERT_H_

#include <cppunit/TestAssert.h>

#include <SupportDefs.h>

#include <string>

std::string DecodeResult(status_t result);

namespace CppUnit {

template struct assertion_traits<status_t>;

}

#endif