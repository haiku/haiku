/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ClippedLineTest.h"

#include <stdio.h>

#include <Region.h>
#include <View.h>

#include "TestSupport.h"


ClippedLineTest::ClippedLineTest()
	: Test(),
	  fTestDuration(0),
	  fTestStart(-1),

	  fLinesRendered(0),
	  fLinesPerIteration(20),

	  fIterations(0),
	  fMaxTestDuration(5000000),

	  fViewBounds(0, 0, -1, -1)
{
}


ClippedLineTest::~ClippedLineTest()
{
}


void
ClippedLineTest::Prepare(BView* view)
{
	fViewBounds = view->Bounds();

	SetupClipping(view);

	fTestDuration = 0;
	fLinesRendered = 0;
	fIterations = 0;
	fTestStart = system_time();
}

bool
ClippedLineTest::RunIteration(BView* view)
{
	bigtime_t now = system_time();

	float vMiddle = (fViewBounds.top + fViewBounds.bottom) / 2;

	for (uint32 i = 0; i < fLinesPerIteration; i++) {
		view->SetHighColor(rand() % 255, rand() % 255, rand() % 255);

		BPoint a;
		a.x = random_number_between(fViewBounds.left, fViewBounds.right);
		a.y = random_number_between(fViewBounds.top, vMiddle);
		BPoint b;
		b.x = random_number_between(fViewBounds.left, fViewBounds.right);
		b.y = random_number_between(vMiddle, fViewBounds.bottom);

		view->StrokeLine(a, b);

		fLinesRendered++;
	}

	view->Sync();

	fTestDuration += system_time() - now;
	fIterations++;

	return system_time() - fTestStart < fMaxTestDuration;
}


void
ClippedLineTest::PrintResults()
{
	if (fTestDuration == 0) {
		printf("Test was not run.\n");
		return;
	}
	bigtime_t timeLeak = system_time() - fTestStart - fTestDuration;
	printf("Lines per iteration: %lu\n", fLinesPerIteration);
	printf("Clipping rects: %ld\n", fClippingRegion.CountRects());
	printf("Total lines rendered: %llu\n", fLinesRendered);
	printf("Lines per second: %.3f\n",
		fLinesRendered * 1000000.0 / fTestDuration);
	printf("Average time between iterations: %.4f seconds.\n",
		(float)timeLeak / fIterations / 1000000);
}

