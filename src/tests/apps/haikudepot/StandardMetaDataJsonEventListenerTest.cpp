/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <AutoDeleter.h>
#include <Json.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "StandardMetaData.h"
#include "StandardMetaDataJsonEventListener.h"


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


class StandardMetaDataJsonEventListenerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StandardMetaDataJsonEventListenerTest);
	CPPUNIT_TEST(TopLevel);
	CPPUNIT_TEST(Icon);
	CPPUNIT_TEST(ThirdLevel);
	CPPUNIT_TEST(Broken);
	CPPUNIT_TEST_SUITE_END();

	void _TestGeneric(const char* input, const char* jsonPath, status_t expectedStatus,
		uint64_t expectedCreateTimestamp, uint64_t expectedDataModifiedTimestamp)
	{
		StandardMetaData metaData;
		StandardMetaDataJsonEventListener listener(jsonPath, metaData);

		BDataIO* inputData = new BMemoryIO(input, strlen(input));
		ObjectDeleter<BDataIO> inputDataDeleter(inputData);

		// ----------------------
		BPrivate::BJson::Parse(inputData, &listener);
		// ----------------------

		CPPUNIT_ASSERT_EQUAL(expectedStatus, listener.ErrorStatus());
		CPPUNIT_ASSERT_EQUAL(expectedCreateTimestamp,
			metaData.GetCreateTimestamp());
		CPPUNIT_ASSERT_EQUAL(expectedDataModifiedTimestamp,
			metaData.GetDataModifiedTimestamp());
	}

public:
	void Broken()
	{
		_TestGeneric(INPUT_BROKEN, "$", B_BAD_DATA, 0L, 0L);
	}

	void ThirdLevel()
	{
		_TestGeneric(INPUT_THIRD_LEVEL, "$.rink.tonk", B_OK, 665544L, 554433L);
	}

	void Icon()
	{
		_TestGeneric(INPUT_ICON, "$", B_OK, 1501456139480ULL, 1500536391000ULL);
	}

	void TopLevel()
	{
		_TestGeneric(INPUT_TOP_LEVEL, "$", B_OK, 123456L, 789012L);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StandardMetaDataJsonEventListenerTest, getTestSuiteName());
