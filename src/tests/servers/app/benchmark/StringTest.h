/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef STRING_TEST_H
#define STRING_TEST_H

#include <Rect.h>

#include "Test.h"

class StringTest : public Test {
public:
								StringTest();
	virtual						~StringTest();

	virtual	void				Prepare(BView* view);
	virtual	bool				RunIteration(BView* view);
	virtual	void				PrintResults(BView* view);

	static	Test*				CreateTest();

private:
	bigtime_t					fTestDuration;
	bigtime_t					fTestStart;
	uint64						fGlyphsRendered;
	uint32						fGlyphsPerLine;
	uint32						fIterations;
	uint32						fMaxIterations;

	float						fStartHeight;
	float						fLineHeight;
	BRect						fViewBounds;
};

#endif // STRING_TEST_H
