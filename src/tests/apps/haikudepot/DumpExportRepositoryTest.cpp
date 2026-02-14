/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "DumpExportRepositoryTest.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "DumpExportRepositoryModel.h"


DumpExportRepositoryTest::DumpExportRepositoryTest()
{
}


DumpExportRepositoryTest::~DumpExportRepositoryTest()
{
}


void
DumpExportRepositoryTest::TestEqualityTrue()
{
	DumpExportRepositoryRef repositoryRef1 = _CreateAny();
	DumpExportRepositoryRef repositoryRef2 = _CreateAny();

	// ----------------------
	bool result = (*(repositoryRef1.Get()) == *(repositoryRef2.Get()));
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("==", true, result);
}


void
DumpExportRepositoryTest::TestEqualityFalse()
{
	DumpExportRepositoryRef repositoryRef1 = _CreateAny();
	DumpExportRepositoryRef repositoryRef2
		= DumpExportRepositoryBuilder(repositoryRef1).WithCode("craters").BuildRef();

	// ----------------------
	bool result = (*(repositoryRef1.Get()) != *(repositoryRef2.Get()));
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("!=", true, result);
}


/*! This test case checks what happens when data changes down the hierarchy of
	objects. It should still see this as part of the equality.
*/
void
DumpExportRepositoryTest::TestEqualityTransientFalse()
{
	DumpExportRepositoryRef repositoryRef1 = _CreateAny();
	DumpExportRepositorySourceRef repositorySourceRef1 = repositoryRef1->RepositorySourcesItemAt(0);

	DumpExportRepositoryRef repositoryRef2
		= DumpExportRepositoryBuilder(repositoryRef1)
			  .WithoutRepositorySources()
			  .AddToRepositorySources(DumpExportRepositorySourceBuilder(repositorySourceRef1)
					  .WithCode("pigeon")
					  .BuildRef())
			  .BuildRef();

	// ----------------------
	bool result = (*(repositoryRef1.Get()) == *(repositoryRef2.Get()));
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("R1 CountRepositorySources", 1,
		repositoryRef1->CountRepositorySources());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("R2 CountRepositorySources", 1,
		repositoryRef2->CountRepositorySources());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("==", false, result);
}


/*!	Check to make sure that the Clear method works. */
void
DumpExportRepositoryTest::TestBuilderClear()
{
	DumpExportRepositoryBuilder builder
		= DumpExportRepositoryBuilder().WithCode("oak").WithName("Oak Tree");

	DumpExportRepositoryRef repositoryBeforeRef = builder.BuildRef();

	// ----------------------
	builder.Clear();
	// ----------------------

	DumpExportRepositoryRef repositoryAfterRef = builder.WithCode("birch").BuildRef();

	CPPUNIT_ASSERT_EQUAL_MESSAGE("CodeBefore", BString("oak"), repositoryBeforeRef->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("NameBefore", BString("Oak Tree"), repositoryBeforeRef->Name());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CodeAfter", BString("birch"), repositoryAfterRef->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetNameAfter", false, repositoryAfterRef->IsSetName());
}


/*! Checks use of the builder without any prior object. */
void
DumpExportRepositoryTest::TestBuilderFromCold()
{

	// ----------------------
	DumpExportRepositoryRef repositoryRef = _CreateAny();
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code", BString("flamingo"), repositoryRef->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetCode", true, repositoryRef->IsSetCode());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Description", BString("Flamingo is a bird"),
		repositoryRef->Description());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Name", BString("Flamingo"), repositoryRef->Name());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("InformationUrl", BString("http://example.com"),
		repositoryRef->InformationUrl());

	CPPUNIT_ASSERT_EQUAL_MESSAGE("SourcesCount", 1, repositoryRef->CountRepositorySources());
	DumpExportRepositorySourceRef repositorySourceRef = repositoryRef->RepositorySourcesItemAt(0);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Sources0Code", BString("flamingo_x86_64"),
		repositorySourceRef->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Sources0Identifier", BString("repo:flamingo"),
		repositorySourceRef->Identifier());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Sources0IsSetArchitectureCode", false,
		repositorySourceRef->IsSetArchitectureCode());

	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountRepositorySourceMirrors", 1,
		repositorySourceRef->CountRepositorySourceMirrors());
	DumpExportRepositorySourceMirrorRef repositorySourceMirrorRef
		= repositorySourceRef->RepositorySourceMirrorsItemAt(0);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountryCode", BString("de"),
		repositorySourceMirrorRef->CountryCode());
}


void
DumpExportRepositoryTest::TestBuilderFromExisting()
{
	DumpExportRepositoryRef originRepositoryRef = _CreateAny();
	DumpExportRepositoryBuilder builder
		= DumpExportRepositoryBuilder(originRepositoryRef).WithCode("flamingo2");

	// ----------------------
	DumpExportRepositoryRef repositoryRef = builder.BuildRef();
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code", BString("flamingo2"), repositoryRef->Code());
		// check that the changed field has come through
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Name", BString("Flamingo"), repositoryRef->Name());
		// check that the unchanged field has come through

	// check that unmodified subordinate objects are coming through
	CPPUNIT_ASSERT_EQUAL_MESSAGE("SourcesCount", 1, repositoryRef->CountRepositorySources());
	DumpExportRepositorySourceRef repositorySourceRef = repositoryRef->RepositorySourcesItemAt(0);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Sources0Code", BString("flamingo_x86_64"),
		repositorySourceRef->Code());
}


void
DumpExportRepositoryTest::TestBuilderFromExistingNoChange()
{
	DumpExportRepositoryRef originRepositoryRef = _CreateAny();
	DumpExportRepositoryBuilder builder = DumpExportRepositoryBuilder(originRepositoryRef);

	// ----------------------
	DumpExportRepositoryRef repositoryRef = builder.BuildRef();
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code", BString("flamingo"), repositoryRef->Code());
}


void
DumpExportRepositoryTest::TestFromMessage()
{
	BMessage message = _CreateAnyMessage();

	// ----------------------
	DumpExportRepositoryRef repository(new DumpExportRepository(&message), true);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("R Code", BString("myrepo"), repository->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetCode", true, repository->IsSetCode());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetInformationUrl", false, repository->IsSetInformationUrl());
		// ^ specifically check that one of the missing fields is showing as missing.
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountRepositorySources", 1, repository->CountRepositorySources());

	DumpExportRepositorySourceRef source0 = repository->RepositorySourcesItemAt(0);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RS Code", BString("myrepo_x86_64"), source0->Code());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetCode", true, source0->IsSetCode());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountExtraIdentifiers", 2, source0->CountExtraIdentifiers());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifier_0", BString("extra_ident:myrepo_x86_64_0"),
		source0->ExtraIdentifiersItemAt(0));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifier_1", BString("extra_ident:myrepo_x86_64_1"),
		source0->ExtraIdentifiersItemAt(1));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountRepositorySourceMirrors", 1,
		source0->CountRepositorySourceMirrors());

	DumpExportRepositorySourceMirrorRef mirror0 = source0->RepositorySourceMirrorsItemAt(0);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountryCode", BString("nz"), mirror0->CountryCode());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsPrimary", true, mirror0->IsPrimary());
	CPPUNIT_ASSERT_EQUAL_MESSAGE("IsSetIsPrimary", true, mirror0->IsSetIsPrimary());
}


void
DumpExportRepositoryTest::TestToMessage()
{
	DumpExportRepositoryRef repositoryRef = _CreateAny();
	BMessage repositoryMessage;

	// ----------------------
	status_t result = repositoryRef->Archive(&repositoryMessage, true);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL_MESSAGE("Result", B_OK, result);

	// quick checks as this could get quite laborious.
	status_t status;
	BString anyString;
	bool anyBool;

	status = repositoryMessage.FindString("Code", &anyString);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code", BString("flamingo"), anyString);

	status = repositoryMessage.FindBool("RepositorySources_isSet", &anyBool);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySources_isSet Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySources_isSet", true, anyBool);

	BMessage repositorySource0Message;
	status = repositoryMessage.FindMessage("RepositorySources_0", &repositorySource0Message);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySources_0 Status", B_OK, status);

	status = repositorySource0Message.FindString("Code", &anyString);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Code", BString("flamingo_x86_64"), anyString);

	status = repositorySource0Message.FindBool("ExtraIdentifiers_isSet", &anyBool);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifiers_isSet Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifiers_isSet", true, anyBool);

	status = repositorySource0Message.FindString("ExtraIdentifiers_0", &anyString);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifiers_0 Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("ExtraIdentifiers_0", BString("repo:flamingo2"), anyString);

	status = repositorySource0Message.FindBool("RepositorySourceMirrors_isSet", &anyBool);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySourceMirrors_isSet Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySourceMirrors_isSet", true, anyBool);

	BMessage repositorySourceMirror0Message;
	status = repositorySource0Message.FindMessage("RepositorySourceMirrors_0",
		&repositorySourceMirror0Message);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("RepositorySourceMirrors_0 Status", B_OK, status);

	status = repositorySourceMirror0Message.FindString("CountryCode", &anyString);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountryCode Status", B_OK, status);
	CPPUNIT_ASSERT_EQUAL_MESSAGE("CountryCode", BString("de"), anyString);
}


/*static*/ void
DumpExportRepositoryTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DumpExportRepositoryTest");

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestEqualityTrue", &DumpExportRepositoryTest::TestEqualityTrue));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestEqualityFalse",
		&DumpExportRepositoryTest::TestEqualityFalse));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestEqualityTransientFalse",
		&DumpExportRepositoryTest::TestEqualityTransientFalse));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestBuilderFromCold",
		&DumpExportRepositoryTest::TestBuilderFromCold));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestBuilderFromExisting",
		&DumpExportRepositoryTest::TestBuilderFromExisting));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestBuilderFromExistingNoChange",
		&DumpExportRepositoryTest::TestBuilderFromExistingNoChange));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestBuilderClear", &DumpExportRepositoryTest::TestBuilderClear));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestFromMessage", &DumpExportRepositoryTest::TestFromMessage));

	suite.addTest(new CppUnit::TestCaller<DumpExportRepositoryTest>(
		"DumpExportRepositoryTest::TestToMessage", &DumpExportRepositoryTest::TestToMessage));

	parent.addTest("DumpExportRepositoryTest", &suite);
}


/*!	This is a `BMessage` that is an archive of an `DumpExportRepository` object. */
BMessage
DumpExportRepositoryTest::_CreateAnyMessage()
{
	BMessage repositorySourceMirrorMessage;

	if (repositorySourceMirrorMessage.AddString("BaseUrl", "http://www.example.com/mirror_baseurl")
		!= B_OK) {
		CPPUNIT_FAIL("unable to add the key [BaseUrl]");
	}
	if (repositorySourceMirrorMessage.AddString("CountryCode", "nz") != B_OK)
		CPPUNIT_FAIL("unable to add the key [CountryCode]");
	if (repositorySourceMirrorMessage.AddString("Description", "New Zealand mirror") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Description]");
	if (repositorySourceMirrorMessage.AddBool("IsPrimary", true) != B_OK)
		CPPUNIT_FAIL("unable to add the key [IsPrimary]");

	BMessage repositorySourceMessage;

	if (repositorySourceMessage.AddString("ArchitectureCode", "x86_64") != B_OK)
		CPPUNIT_FAIL("unable to add the key [ArchitectureCode]");
	if (repositorySourceMessage.AddString("Code", "myrepo_x86_64") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Code]");
	if (repositorySourceMessage.AddBool("ExtraIdentifiers_isSet", true) != B_OK)
		CPPUNIT_FAIL("unable to add the key [ExtraIdentifiers_isSet]");
	if (repositorySourceMessage.AddString("ExtraIdentifiers_0", "extra_ident:myrepo_x86_64_0")
		!= B_OK) {
		CPPUNIT_FAIL("unable to add the key [ExtraIdentifiers_0]");
	}
	if (repositorySourceMessage.AddString("ExtraIdentifiers_1", "extra_ident:myrepo_x86_64_1")
		!= B_OK) {
		CPPUNIT_FAIL("unable to add the key [ExtraIdentifiers_1]");
	}
	if (repositorySourceMessage.AddString("Identifier", "ident:myrepo_x86_64") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Identifier]");
	if (repositorySourceMessage.AddString("RepoInfoUrl", "http://www.example.com/myrepo_x86_64")
		!= B_OK) {
		CPPUNIT_FAIL("unable to add the key [RepoInfoUrl]");
	}
	if (repositorySourceMessage.AddBool("RepositorySourceMirrors_isSet", true) != B_OK)
		CPPUNIT_FAIL("unable to add the key [RepositorySourceMirrors_isSet]");
	if (repositorySourceMessage.AddMessage("RepositorySourceMirrors_0",
			&repositorySourceMirrorMessage)
		!= B_OK) {
		CPPUNIT_FAIL("unable to add the key [RepositorySourceMirrors_0]");
	}

	BMessage repositoryMessage;

	if (repositoryMessage.AddString("Code", "myrepo") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Code]");
	if (repositoryMessage.AddString("Description", "This is my repo") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Description]");
	// note; missing the informational url intentionally
	if (repositoryMessage.AddString("Name", "My Repo") != B_OK)
		CPPUNIT_FAIL("unable to add the key [Name]");
	if (repositoryMessage.AddBool("RepositorySources_isSet", true) != B_OK)
		CPPUNIT_FAIL("unable to add the key [RepositorySources_isSet]");
	if (repositoryMessage.AddMessage("RepositorySources_0", &repositorySourceMessage) != B_OK)
		CPPUNIT_FAIL("unable to add the key [RepositorySources_0]");

	return repositoryMessage;
}


DumpExportRepositoryRef
DumpExportRepositoryTest::_CreateAny()
{
	return DumpExportRepositoryBuilder()
		.WithCode("flamingo")
		.WithDescription("Flamingo is a bird")
		.WithInformationUrl("http://example.com")
		.WithName("Flamingo")
		.AddToRepositorySources(DumpExportRepositorySourceBuilder()
				.WithCode("flamingo_x86_64")
				.WithIdentifier("repo:flamingo")
				.AddToExtraIdentifiers("repo:flamingo2")
				.AddToRepositorySourceMirrors(DumpExportRepositorySourceMirrorBuilder()
						.WithCountryCode("de")
						.WithBaseUrl("http://www.example.com/base_url")
						.BuildRef())
				.BuildRef())
		.BuildRef();
}
