/*
 * Copyright 2017-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <AutoDeleter.h>
#include <Json.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "DumpExportRepositoryJsonListener.h"


// This repository example is valid, but does have some additional
// rubbish data at the start.  Some values have been shortened to make
// testing easier.

#define SINGLE_REPOSITORY_WITH_RUBBISH "{\n" \
	"  \"rubbish\": {\n" \
	"    \"key\": \"value\",\n" \
	"    \"anylist\": [\n" \
	"      1,\n" \
	"      2,\n" \
	"      3,\n" \
	"      null\n" \
	"    ]\n" \
	"  },\n" \
	"  \"code\": \"haikuports\",\n" \
	"  \"name\": \"HaikuPorts\",\n" \
	"  \"description\": \"HaikuPorts is a centralized collection...\",\n" \
	"  \"informationUrl\": \"https://example.com\",\n" \
	"  \"repositorySources\": [\n" \
	"    {\n" \
	"      \"code\": \"haikuports_x86_64\",\n" \
	"      \"identifier\": \"haiku:hpkr:haikuports_x86_64\",\n" \
	"      \"extraIdentifiers\":[\"zing\"]\n" \
	"    },\n" \
	"    {\n" \
	"      \"code\": \"haikuports_x86_gcc2\",\n" \
	"      \"identifier\": \"haiku:hpkr:haikuports_x86_gcc2\"\n" \
	"    }\n" \
	"  ]\n" \
	"}\n"

// this is more or less the data that would come back from the server when
// a bulk list of repositories is requested.  Some values have been shortened
// to make testing easier.

#define BULK_CONTAINER_REPOSITORY_WITH_RUBBISH "{\n" \
   "  \"info\": {\n" \
   "    \"createTimestamp\": 1506547178986,\n" \
   "    \"createTimestampIso\": \"2017-09-27 21:19:38\",\n" \
   "    \"dataModifiedTimestamp\": 1500605691000,\n" \
   "    \"dataModifiedTimestampIso\": \"2017-07-21 02:54:51\",\n" \
   "    \"agent\": \"hds\",\n" \
   "    \"agentVersion\": \"1.0.87\"\n" \
   "  },\n" \
   "  \"items\": [\n" \
   "    {\n" \
   "      \"code\": \"fatelk\",\n" \
   "      \"name\": \"FatElk\",\n" \
   "      \"informationUrl\": \"http://fatelk.com/\",\n" \
   "      \"repositorySources\": [\n" \
   "        {\n" \
   "          \"code\": \"fatelk_x86_gcc2\",\n" \
   "          \"identifier\": \"can-be-anything\",\n" \
   "          \"extraIdentifiers\":[\"zing\"]\n" \
   "        }\n" \
   "      ]\n" \
   "    },\n" \
   "    {\n" \
   "      \"code\": \"besly\",\n" \
   "      \"name\": \"BeSly\",\n" \
   "      \"informationUrl\": \"http://www.software.besly.de/\",\n" \
   "      \"repositorySources\": [\n" \
   "        {\n" \
   "          \"code\": \"besly_x86_gcc2\",\n" \
   "          \"identifier\": \"haiku:hpkr:wojfqdi23e\"\n" \
   "        }\n" \
   "      ]\n" \
   "    },\n" \
   "    {\n" \
   "      \"code\": \"clasqm\",\n" \
   "      \"name\": \"Clasqm\",\n" \
   "      \"informationUrl\": \"http://AA18\",\n" \
   "      \"repositorySources\": [\n" \
   "        {\n" \
   "          \"code\": \"clasqm_x86_64\",\n" \
   "          \"identifier\": \"haiku:hpkr:23r829rro\"\n" \
   "        },\n" \
   "        {\n" \
   "          \"code\": \"clasqm_x86_gcc2\",\n" \
   "          \"identifier\": \"haiku:hpkr:joihir32r\"\n" \
   "        }\n" \
   "      ]\n" \
   "    },\n" \
   "    {\n" \
   "      \"code\": \"haikuports\",\n" \
   "      \"name\": \"HaikuPorts\",\n" \
   "      \"description\": \"HaikuPorts is a centralized collection ...\",\n" \
   "      \"informationUrl\": \"https://github.com/haikuports/haikuports\",\n" \
   "      \"repositorySources\": [\n" \
   "        {\n" \
   "          \"code\": \"haikuports_x86_64\",\n" \
   "          \"identifier\": \"haiku:hpkr:jqod2333r3r\"\n" \
   "        },\n" \
   "        {\n" \
   "          \"code\": \"haikuports_x86_gcc2\",\n" \
   "          \"identifier\": \"haiku:hpkr:wyeuhfwiewe\"\n" \
   "        }\n" \
   "      ]\n" \
   "    }\n" \
   "  ]\n" \
   "}"


class TestBulkContainerItemListener : public DumpExportRepositoryListener {
public:
							TestBulkContainerItemListener() { fWasCompleteInvoked = false; }
		virtual				~TestBulkContainerItemListener() {}

			bool			Handle(DumpExportRepositoryRef item)
			{
				if (!fConcatenatedCodes.IsEmpty())
					fConcatenatedCodes.Append(" ");
				fConcatenatedCodes.Append(item->Code().String());
				for (int32 i = 0; i < item->CountRepositorySources(); i++) {
					if (!fConcatenatedSourcesUrl.IsEmpty())
						fConcatenatedSourcesUrl.Append(" ");
					fConcatenatedSourcesUrl.Append(
						item->RepositorySourcesItemAt(i)->Identifier().String());
				}
				return true;
			}
			void			Complete() { fWasCompleteInvoked = true; }

			BString			ConcatenatedCodes() { return fConcatenatedCodes; }
			BString			ConcatenatedSourcesUrls() { return fConcatenatedSourcesUrl; }
			bool			WasCompleteInvoked() { return fWasCompleteInvoked; }

private:
			bool			fWasCompleteInvoked;
			BString			fConcatenatedCodes;
			BString			fConcatenatedSourcesUrl;
};


class DumpExportRepositoryJsonListenerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DumpExportRepositoryJsonListenerTest);
	CPPUNIT_TEST(SingleRepositoryJson_Parse_ReturnsSuccess);
	CPPUNIT_TEST(BulkContainerRepositoryJson_Parse_ReturnsSuccess);
	CPPUNIT_TEST_SUITE_END();

public:
	void SingleRepositoryJson_Parse_ReturnsSuccess()
	{
		BDataIO* inputData = new BMemoryIO(SINGLE_REPOSITORY_WITH_RUBBISH,
			strlen(SINGLE_REPOSITORY_WITH_RUBBISH));
		ObjectDeleter<BDataIO> inputDataDeleter(inputData);

		SingleDumpExportRepositoryJsonListener* listener =
			new SingleDumpExportRepositoryJsonListener();
		ObjectDeleter<SingleDumpExportRepositoryJsonListener>
			listenerDeleter(listener);

		BPrivate::BJson::Parse(inputData, listener);

		DumpExportRepositoryRef repository = listener->Result();

		CPPUNIT_ASSERT_EQUAL(B_OK, listener->ErrorStatus());

		CPPUNIT_ASSERT_EQUAL(
			BString("haikuports"), (repository->Code()));
		CPPUNIT_ASSERT_EQUAL(
			BString("HaikuPorts"), (repository->Name()));
		CPPUNIT_ASSERT_EQUAL(
			BString("HaikuPorts is a centralized collection..."),
			(repository->Description()));
		CPPUNIT_ASSERT_EQUAL(
			BString("https://example.com"),
			(repository->InformationUrl()));
		CPPUNIT_ASSERT_EQUAL(2, (int)repository->CountRepositorySources());

		DumpExportRepositorySource *source0 =
			repository->RepositorySourcesItemAt(0);
		DumpExportRepositorySource *source1 =
			repository->RepositorySourcesItemAt(1);

		CPPUNIT_ASSERT_EQUAL(BString("haikuports_x86_64"), source0->Code());
		CPPUNIT_ASSERT_EQUAL(BString("haiku:hpkr:haikuports_x86_64"), source0->Identifier());
		CPPUNIT_ASSERT_EQUAL(BString("zing"), source0->ExtraIdentifiersItemAt(0));

		CPPUNIT_ASSERT_EQUAL(BString("haikuports_x86_gcc2"), source1->Code());
		CPPUNIT_ASSERT_EQUAL(BString("haiku:hpkr:haikuports_x86_gcc2"), source1->Identifier());
	}

	void BulkContainerRepositoryJson_Parse_ReturnsSuccess()
	{
		BDataIO* inputData = new BMemoryIO(BULK_CONTAINER_REPOSITORY_WITH_RUBBISH,
			strlen(BULK_CONTAINER_REPOSITORY_WITH_RUBBISH));
		ObjectDeleter<BDataIO> inputDataDeleter(inputData);

		TestBulkContainerItemListener itemListener;

		BulkContainerDumpExportRepositoryJsonListener* listener =
			new BulkContainerDumpExportRepositoryJsonListener(&itemListener);
		ObjectDeleter<BulkContainerDumpExportRepositoryJsonListener>
			listenerDeleter(listener);

		BPrivate::BJson::Parse(inputData, listener);

		CPPUNIT_ASSERT_EQUAL_MESSAGE("!B_OK",
			B_OK, listener->ErrorStatus());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!WasCompleteInvoked",
			true, itemListener.WasCompleteInvoked());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!ConcatenatedCodes",
			BString("fatelk besly clasqm haikuports"),
			itemListener.ConcatenatedCodes());
		CPPUNIT_ASSERT_EQUAL_MESSAGE("!ConcatenatedSourcesUrls",
			BString("can-be-anything"
				" haiku:hpkr:wojfqdi23e"
				" haiku:hpkr:23r829rro"
				" haiku:hpkr:joihir32r"
				" haiku:hpkr:jqod2333r3r"
				" haiku:hpkr:wyeuhfwiewe"),
			itemListener.ConcatenatedSourcesUrls());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DumpExportRepositoryJsonListenerTest, getTestSuiteName());
