// TranslatorTestAddOn.h

#ifndef TRANSLATOR_TEST_ADD_ON_H
#define TRANSLATOR_TEST_ADD_ON_H

#include <TestCase.h>
#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include <DataIO.h>
#include <TranslationDefs.h>

bool CompareStreams(BPositionIO &a, BPositionIO &b);

void CheckTranslatorInfo(translator_info *pti,
	uint32 type, uint32 group, float quality, float capability,
	const char *name, const char *mime);
	
void TranslatorLoadAddOnTest(const char *path, BTestCase *ptest,
	const translation_format *pExpectedIns, uint32 nExpectedIns,
	const translation_format *pExpectedOuts, uint32 nExpectedOuts,
	int32 expectedVer);

#endif // #ifndef TRANSLATOR_TEST_ADD_ON_H
