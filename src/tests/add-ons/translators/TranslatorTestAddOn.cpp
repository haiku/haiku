// TranslatorTestAddOn.cpp

#include <stdio.h>
#include <string.h>
#include <Translator.h>
#include <OS.h>
#include "TranslatorTestAddOn.h"

// ##### Include headers for your tests here #####
#include "bmptranslator/BMPTranslatorTest.h"
#include "pngtranslator/PNGTranslatorTest.h"
#include "stxttranslator/STXTTranslatorTest.h"
#include "tgatranslator/TGATranslatorTest.h"
#include "tifftranslator/TIFFTranslatorTest.h"

BTestSuite *
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("Translators");

	// ##### Add test suites here #####
	suite->addTest("BMPTranslator", BMPTranslatorTest::Suite());
	suite->addTest("PNGTranslator", PNGTranslatorTest::Suite());
	suite->addTest("STXTTranslator", STXTTranslatorTest::Suite());
	suite->addTest("TGATranslator", TGATranslatorTest::Suite());
	suite->addTest("TIFFTranslator", TIFFTranslatorTest::Suite());

	return suite;
}

// helper function used by multiple tests to
// determine if the given streams are exactly
// the same
bool
CompareStreams(BPositionIO &a, BPositionIO &b)
{
	off_t alen = 0, blen = 0;
	const uint32 kbuflen = 2048;
	uint8 abuf[kbuflen], bbuf[kbuflen];

	a.Seek(0, SEEK_END);
	alen = a.Position();
	b.Seek(0, SEEK_END);
	blen = b.Position();
	if (alen != blen)
		return false;

	if (a.Seek(0, SEEK_SET) != 0)
		return false;
	if (b.Seek(0, SEEK_SET) != 0)
		return false;

	ssize_t read = 0;
	while (alen > 0) {
		read = a.Read(abuf, kbuflen);
		if (read < 0)
			return false;
		if (b.Read(bbuf, read) != read)
			return false;

		if (memcmp(abuf, bbuf, read) != 0)
			return false;

		alen -= read;
	}

	return true;
}

// Check each member of translator_info to see that it matches
// what is expected
void
CheckTranslatorInfo(translator_info *pti, uint32 type, uint32 group,
	float quality, float capability, const char *name, const char *mime)
{
	CPPUNIT_ASSERT(pti->type == type);
	CPPUNIT_ASSERT(pti->translator != 0);
	CPPUNIT_ASSERT(pti->group == group);
	CPPUNIT_ASSERT(pti->quality > quality - 0.01f &&
		pti->quality < quality + 0.01f);
	CPPUNIT_ASSERT(pti->capability > capability - 0.01f &&
		pti->capability < capability + 0.01f);
	CPPUNIT_ASSERT(strcmp(pti->name, name) == 0);
	CPPUNIT_ASSERT(strcmp(pti->MIME, mime) == 0);
}

// Returns true if the translation_formats are
// identical (or nearly identical).  Returns false if
// they are different
bool
CompareTranslationFormat(const translation_format *pleft,
	const translation_format *pright)
{
	CPPUNIT_ASSERT(pleft->MIME);
	CPPUNIT_ASSERT(pright->MIME);
	CPPUNIT_ASSERT(pleft->name);
	CPPUNIT_ASSERT(pright->name);

	if (pleft->group != pright->group)
		return false;
	if (pleft->type != pright->type)
		return false;
	if (pleft->quality < pright->quality - 0.01f ||
		pleft->quality > pright->quality + 0.01f)
		return false;
	if (pleft->capability < pright->capability - 0.01f ||
		pleft->capability > pright->capability + 0.01f)
		return false;
	if (strcmp(pleft->MIME, pright->MIME) != 0)
		return false;
	if (strcmp(pleft->name, pright->name) != 0)
		return false;

	return true;
}

// Apply a number of tests to a BTranslator * to a TGATranslator object
void
TestBTranslator(BTestCase *ptest, BTranslator *ptran,
	const translation_format *pExpectedIns, uint32 nExpectedIns,
	const translation_format *pExpectedOuts, uint32 nExpectedOuts,
	int32 expectedVer)
{
	const uint32 knmatches = 50;
	uint8 matches[knmatches];
	CPPUNIT_ASSERT(nExpectedIns <= knmatches && nExpectedOuts <= knmatches);

	// The translator should only have one reference
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);

	// Make sure Acquire returns a BTranslator even though its
	// already been Acquired once
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);

	// Acquired twice, refcount should be 2
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);

	// Release should return ptran because it is still acquired
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);

	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);

	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Acquire() == ptran);

	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 2);

	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == ptran);

	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->ReferenceCount() == 1);

	// A name would be nice
	ptest->NextSubTest();
	const char *tranname = ptran->TranslatorName();
	CPPUNIT_ASSERT(tranname);
	printf(" {%s} ", tranname);

	// More info would be nice
	ptest->NextSubTest();
	const char *traninfo = ptran->TranslatorInfo();
	CPPUNIT_ASSERT(traninfo);
	printf(" {%s} ", traninfo);

	// What version are you?
	// (when ver == 100, that means that version is 1.00)
	ptest->NextSubTest();
	int32 ver = ptran->TranslatorVersion();
	CPPUNIT_ASSERT(ver == expectedVer);
	printf(" {0x%.8lx} ", ver);

	// Input formats?
	ptest->NextSubTest();
	{
		printf("input:");

		int32 incount = 0;
		const translation_format *pins = ptran->InputFormats(&incount);
		CPPUNIT_ASSERT((unsigned)incount == nExpectedIns);
		CPPUNIT_ASSERT(pins);

		memset(matches, 0, sizeof(uint8) * nExpectedIns);
		for (int32 i = 0; i < incount; i++) {
			bool bmatch = false;
			for (uint32 k = 0; bmatch == false && k < nExpectedIns; k++) {
				bmatch = CompareTranslationFormat(pins + i, pExpectedIns + k);
				if (bmatch)
					matches[k] = 1;
			}

			CPPUNIT_ASSERT(bmatch);
		}

		// make sure that each expected input format was matched
		for (uint32 i = 0; i < nExpectedIns; i++)
			CPPUNIT_ASSERT(matches[i]);
	}

	// Output formats?
	ptest->NextSubTest();
	{
		printf("output:");

		int32 outcount = 0;
		const translation_format *pouts = ptran->OutputFormats(&outcount);
		CPPUNIT_ASSERT((unsigned)outcount == nExpectedOuts);
		CPPUNIT_ASSERT(pouts);

		memset(matches, 0, sizeof(uint8) * nExpectedOuts);
		for (int32 i = 0; i < outcount; i++) {
			bool bmatch = false;
			for (uint32 k = 0; bmatch == false && k < nExpectedOuts; k++) {
				bmatch = CompareTranslationFormat(pouts + i, pExpectedOuts + k);
				if (bmatch)
					matches[k] = 1;
			}

			CPPUNIT_ASSERT(bmatch);
		}

		// make sure that each expected input format was matched
		for (uint32 i = 0; i < nExpectedOuts; i++)
			CPPUNIT_ASSERT(matches[i]);
	}

	// Release should return NULL because Release has been called
	// as many times as it has been acquired
	ptest->NextSubTest();
	CPPUNIT_ASSERT(ptran->Release() == NULL);
	ptran = NULL;
}

void
TranslatorLoadAddOnTest(const char *path, BTestCase *ptest,
	const translation_format *pExpectedIns, uint32 nExpectedIns,
	const translation_format *pExpectedOuts, uint32 nExpectedOuts,
	int32 expectedVer)
{
	// Make sure the add_on loads
	ptest->NextSubTest();
	image_id image = load_add_on(path);
	CPPUNIT_ASSERT(image >= 0);

	// Load in function to make the object
	ptest->NextSubTest();
	BTranslator *(*pMakeNthTranslator)(int32 n,image_id you,uint32 flags,...);
	status_t err = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void **)&pMakeNthTranslator);
	CPPUNIT_ASSERT(!err);

	// Make sure the function returns a pointer to a BTranslator
	ptest->NextSubTest();
	BTranslator *ptran = pMakeNthTranslator(0, image, 0);
	CPPUNIT_ASSERT(ptran);

	// Make sure the function only returns one BTranslator
	ptest->NextSubTest();
	CPPUNIT_ASSERT(!pMakeNthTranslator(1, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(2, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(3, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(16, image, 0));
	CPPUNIT_ASSERT(!pMakeNthTranslator(1023, image, 0));

	// Run a number of tests on the BTranslator object
	TestBTranslator(ptest, ptran, pExpectedIns, nExpectedIns,
		pExpectedOuts, nExpectedOuts, expectedVer);
		// NOTE: this function Release()s ptran
	ptran = NULL;

	// Unload Add-on
	ptest->NextSubTest();
	CPPUNIT_ASSERT(unload_add_on(image) == B_OK);
}
