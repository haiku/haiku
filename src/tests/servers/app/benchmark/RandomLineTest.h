/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RANDOM_LINE_TEST_H
#define RANDOM_LINE_TEST_H

#include <Rect.h>

#include "Test.h"

class RandomLineTest : public Test {
public:
								RandomLineTest();
	virtual						~RandomLineTest();

	virtual	void				Prepare(BView* view);
	virtual	bool				RunIteration(BView* view);
	virtual	void				PrintResults(BView* view);

	static	Test*				CreateTest();

private:
	bigtime_t					fTestDuration;
	bigtime_t					fTestStart;
	uint64						fLinesRendered;
	uint32						fLinesPerIteration;

	uint32						fIterations;
	uint32						fMaxIterations;

	BRect						fViewBounds;
};

#endif // RANDOM_LINE_TEST_H
