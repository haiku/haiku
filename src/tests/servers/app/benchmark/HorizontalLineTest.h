/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef HORIZONTAL_LINE_TEST_H
#define HORIZONTAL_LINE_TEST_H

#include <Rect.h>

#include "Test.h"

class HorizontalLineTest : public Test {
public:
								HorizontalLineTest();
	virtual						~HorizontalLineTest();

	virtual	void				Prepare(BView* view);
	virtual	bool				RunIteration(BView* view);
	virtual	void				PrintResults(BView* view);

	static	Test*				CreateTest();

private:
	bigtime_t					fTestDuration;
	bigtime_t					fTestStart;
	uint64						fLinesRendered;

	uint32						fIterations;
	uint32						fMaxIterations;

	BRect						fViewBounds;
};

#endif // HORIZONTAL_LINE_TEST_H
