/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "HorizontalLineTest.h"

#include <stdio.h>

#include <View.h>

#include "TestSupport.h"


HorizontalLineTest::HorizontalLineTest()
	: Test(),
	  fTestDuration(0),
	  fTestStart(-1),

	  fLinesRendered(0),

	  fIterations(0),
	  fMaxIterations(1500),

	  fViewBounds(0, 0, -1, -1)
{
}


HorizontalLineTest::~HorizontalLineTest()
{
}


void
HorizontalLineTest::Prepare(BView* view)
{
	fViewBounds = view->Bounds();

	fTestDuration = 0;
	fLinesRendered = 0;
	fIterations = 0;
	fTestStart = system_time();
}

bool
HorizontalLineTest::RunIteration(BView* view)
{
	float y = 1;

	bigtime_t now = system_time();

	while (true) {
		view->StrokeLine(BPoint(fViewBounds.left + 1, y),
			BPoint(fViewBounds.right - 1, y));

		fLinesRendered++;

		// offset text location
		y += 2;
		if (y > fViewBounds.bottom)
			break;
	}

	view->Sync();

	fTestDuration += system_time() - now;
	fIterations++;

	return fIterations < fMaxIterations;
}


void
HorizontalLineTest::PrintResults(BView* view)
{
	if (fTestDuration == 0) {
		printf("Test was not run.\n");
		return;
	}
	bigtime_t timeLeak = system_time() - fTestStart - fTestDuration;

	Test::PrintResults(view);

	printf("Line width: %ld\n", fViewBounds.IntegerWidth() + 1 - 2);
	printf("Lines per iteration: %ld\n", fViewBounds.IntegerHeight() / 2);
	printf("Total lines rendered: %llu\n", fLinesRendered);
	printf("Lines per second: %.3f\n",
		fLinesRendered * 1000000.0 / fTestDuration);
	printf("Average time between iterations: %.4f seconds.\n",
		(float)timeLeak / fIterations / 1000000);
}


Test*
HorizontalLineTest::CreateTest()
{
	return new HorizontalLineTest();
}

