// MultiTest.h

#ifndef MULTI_TEST_H
#define MULTI_TEST_H

#include <Application.h>
#include <TranslatorRoster.h>

class MultiTestApplication : public BApplication {
public:
	MultiTestApplication();
	virtual ~MultiTestApplication();
	
	BTranslatorRoster *GetTranslatorRoster() const;
	
private:
	BTranslatorRoster *pRoster;
};

#endif