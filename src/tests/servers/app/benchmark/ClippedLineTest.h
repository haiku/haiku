/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef CLIPPED_LINE_TEST_H
#define CLIPPED_LINE_TEST_H

#include <Rect.h>

#include "Test.h"

class ClippedLineTest : public Test {
public:
								ClippedLineTest();
	virtual						~ClippedLineTest();

	virtual	void				Prepare(BView* view);
	virtual	bool				RunIteration(BView* view);
	virtual	void				PrintResults();

private:
	bigtime_t					fTestDuration;
	bigtime_t					fTestStart;
	uint64						fLinesRendered;
	uint32						fLinesPerIteration;

	uint32						fIterations;
	bigtime_t					fMaxTestDuration;

	BRect						fViewBounds;
};

#endif // CLIPPED_LINE_TEST_H
