/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "StandardMetaData.h"
#include "StandardMetaDataJsonEventListener.h"
#include "StandardMetaDataJsonEventListenerTest.h"

#include <stdio.h>

#include <AutoDeleter.h>
#include <Json.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


#define INPUT_TOP_LEVEL "{\"createTimestamp\":123456," \
	"\"dataModifiedTimestamp\":789012}"

// This example is from the HDS server and comes back as part of the icons
// bundle; real-world example.

#define INPUT_ICON "{\"createTimestamp\":1501456139480," \
	"\"createTimestampIso\":\"2017-07-30 23:08:59\"," \
	"\"dataModifiedTimestamp\":1500536391000," \
	"\"dataModifiedTimestampIso\":\"2017-07-20 07:39:51\"," \
	"\"agent\":\"hds\"," \
	"\"agentVersion\":\"1.0.86\"" \
	"}"

#define INPUT_THIRD_LEVEL "{" \
	"\"createTimestamp\":99999," \
	"\"gonk\":{\"zink\":[ 123, { \"a\":\"b\" } ]}\n," \
	"\"rink\":{" \
		"\"tonk\":{" \
			"\"createTimestamp\":665544," \
			"\"dataModifiedTimestamp\":554433" \
		"}" \
	"}," \
	"\"cake\": { \"type\": \"bananas\" }," \
	"\"createTimestamp\":99999," \
"}"

#define INPUT_BROKEN "{\"broken\",,"


StandardMetaDataJsonEventListenerTest::StandardMetaDataJsonEventListenerTest()
{
}


StandardMetaDataJsonEventListenerTest::~StandardMetaDataJsonEventListenerTest()
{
}


void
StandardMetaDataJsonEventListenerTest::TestGeneric(
	const char* input, const char* jsonPath, status_t expectedStatus,
	uint64_t expectedCreateTimestamp, uint64_t expectedDataModifiedTimestamp)
{
	StandardMetaData metaData;
	StandardMetaDataJsonEventListener listener(jsonPath, metaData);

	BDataIO* inputData = new BMemoryIO(input, strlen(input));
	ObjectDeleter<BDataIO> inputDataDeleter(inputData);
	BMallocIO* outputData = new BMallocIO();
	ObjectDeleter<BMallocIO> outputDataDeleter(outputData);

// ----------------------
	BPrivate::BJson::Parse(inputData, &listener);
// ----------------------

	CPPUNIT_ASSERT_EQUAL(expectedStatus, listener.ErrorStatus());
	CPPUNIT_ASSERT_EQUAL(expectedCreateTimestamp,
		metaData.GetCreateTimestamp());
	CPPUNIT_ASSERT_EQUAL(expectedDataModifiedTimestamp,
		metaData.GetDataModifiedTimestamp());
}


void
StandardMetaDataJsonEventListenerTest::TestTopLevel()
{
	TestGeneric(INPUT_TOP_LEVEL, "$", B_OK, 123456L, 789012L);
}


void
StandardMetaDataJsonEventListenerTest::TestIcon()
{
	TestGeneric(INPUT_ICON, "$", B_OK, 1501456139480ULL, 1500536391000ULL);
}


void
StandardMetaDataJsonEventListenerTest::TestThirdLevel()
{
	TestGeneric(INPUT_THIRD_LEVEL, "$.rink.tonk", B_OK, 665544L, 554433L);
}


void
StandardMetaDataJsonEventListenerTest::TestBroken()
{
	TestGeneric(INPUT_BROKEN, "$", B_BAD_DATA, 0L, 0L);
}


/*static*/ void
StandardMetaDataJsonEventListenerTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"StandardMetaDataJsonEventListenerTest");

	suite.addTest(
		new CppUnit::TestCaller<StandardMetaDataJsonEventListenerTest>(
			"StandardMetaDataJsonEventListenerTest::TestTopLevel",
			&StandardMetaDataJsonEventListenerTest::TestTopLevel));

	suite.addTest(
		new CppUnit::TestCaller<StandardMetaDataJsonEventListenerTest>(
			"StandardMetaDataJsonEventListenerTest::TestIcon",
			&StandardMetaDataJsonEventListenerTest::TestIcon));

	suite.addTest(
		new CppUnit::TestCaller<StandardMetaDataJsonEventListenerTest>(
			"StandardMetaDataJsonEventListenerTest::TestThirdLevel",
			&StandardMetaDataJsonEventListenerTest::TestThirdLevel));

	suite.addTest(
		new CppUnit::TestCaller<StandardMetaDataJsonEventListenerTest>(
			"StandardMetaDataJsonEventListenerTest::TestBroken",
			&StandardMetaDataJsonEventListenerTest::TestBroken));

	parent.addTest("StandardMetaDataJsonEventListenerTest", &suite);
}