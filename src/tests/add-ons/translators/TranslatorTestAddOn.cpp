// TranslatorTestAddOn.cpp

#include <stdio.h>
#include "TranslatorTestAddOn.h"

// ##### Include headers for your tests here #####
#include "bmptranslator/BMPTranslatorTest.h"
#include "stxttranslator/STXTTranslatorTest.h"
#include "tifftranslator/TIFFTranslatorTest.h"

// helper function used by multiple tests to
// determine if the given streams are exactly
// the same
bool
CompareStreams(BPositionIO &a, BPositionIO &b)
{
	off_t alen = 0, blen = 0;
	uint8 *abuf = NULL, *bbuf = NULL;
	
	a.Seek(0, SEEK_END);
	alen = a.Position();
	b.Seek(0, SEEK_END);
	blen = b.Position();
	
	if (alen != blen)
		return false;

	bool bresult = false;		
	abuf = new uint8[alen];
	bbuf = new uint8[blen];
	if (a.ReadAt(0, abuf, alen) == alen) {
		if (b.ReadAt(0, bbuf, blen) == blen) {
			if (memcmp(abuf, bbuf, alen) == 0)
				bresult = true;
			else
				bresult = false;
		}
	}
		
	delete[] abuf;
	abuf = NULL;
	delete[] bbuf;
	bbuf = NULL;
	
	return bresult;
}

BTestSuite *
getTestSuite()
{
	BTestSuite *suite = new BTestSuite("Translators");

	// ##### Add test suites here #####
	suite->addTest("BMPTranslator", BMPTranslatorTest::Suite());
	suite->addTest("STXTTranslator", STXTTranslatorTest::Suite());
	suite->addTest("TIFFTranslator", TIFFTranslatorTest::Suite());

	return suite;
}
