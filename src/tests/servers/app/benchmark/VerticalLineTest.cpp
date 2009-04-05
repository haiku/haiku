/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VerticalLineTest.h"

#include <stdio.h>

#include <View.h>

#include "TestSupport.h"


VerticalLineTest::VerticalLineTest()
	: Test(),
	  fTestDuration(0),
	  fTestStart(-1),

	  fLinesRendered(0),

	  fIterations(0),
	  fMaxIterations(1500),

	  fViewBounds(0, 0, -1, -1)
{
}


VerticalLineTest::~VerticalLineTest()
{
}


void
VerticalLineTest::Prepare(BView* view)
{
	fViewBounds = view->Bounds();

	fTestDuration = 0;
	fLinesRendered = 0;
	fIterations = 0;
	fTestStart = system_time();
}

bool
VerticalLineTest::RunIteration(BView* view)
{
	float x = 1;

	bigtime_t now = system_time();

	while (true) {
		view->StrokeLine(BPoint(x, fViewBounds.top + 1),
			BPoint(x, fViewBounds.bottom - 1));

		fLinesRendered++;

		// offset text location
		x += 2;
		if (x > fViewBounds.right)
			break;
	}

	view->Sync();

	fTestDuration += system_time() - now;
	fIterations++;

	return fIterations < fMaxIterations;
}


void
VerticalLineTest::PrintResults(BView* view)
{
	if (fTestDuration == 0) {
		printf("Test was not run.\n");
		return;
	}
	bigtime_t timeLeak = system_time() - fTestStart - fTestDuration;

	Test::PrintResults(view);

	printf("Line height: %ld\n", fViewBounds.IntegerHeight() + 1 - 2);
	printf("Lines per iteration: %ld\n", fViewBounds.IntegerWidth() / 2);
	printf("Total lines rendered: %llu\n", fLinesRendered);
	printf("Lines per second: %.3f\n",
		fLinesRendered * 1000000.0 / fTestDuration);
	printf("Average time between iterations: %.4f seconds.\n",
		(float)timeLeak / fIterations / 1000000);
}


Test*
VerticalLineTest::CreateTest()
{
	return new VerticalLineTest();
}

