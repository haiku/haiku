/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RandomLineTest.h"

#include <stdio.h>

#include <View.h>

#include "TestSupport.h"


RandomLineTest::RandomLineTest()
	: Test(),
	  fTestDuration(0),
	  fTestStart(-1),

	  fLinesRendered(0),
	  fLinesPerIteration(100),

	  fIterations(0),
	  fMaxIterations(1500),

	  fViewBounds(0, 0, -1, -1)
{
}


RandomLineTest::~RandomLineTest()
{
}


void
RandomLineTest::Prepare(BView* view)
{
	fViewBounds = view->Bounds();

	fTestDuration = 0;
	fLinesRendered = 0;
	fIterations = 0;
	fTestStart = system_time();
}

bool
RandomLineTest::RunIteration(BView* view)
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

	return fIterations < fMaxIterations;
}


void
RandomLineTest::PrintResults(BView* view)
{
	if (fTestDuration == 0) {
		printf("Test was not run.\n");
		return;
	}
	bigtime_t timeLeak = system_time() - fTestStart - fTestDuration;

	Test::PrintResults(view);

	printf("Lines per iteration: %lu\n", fLinesPerIteration);
	printf("Total lines rendered: %llu\n", fLinesRendered);
	printf("Lines per second: %.3f\n",
		fLinesRendered * 1000000.0 / fTestDuration);
	printf("Average time between iterations: %.4f seconds.\n",
		(float)timeLeak / fIterations / 1000000);
}


Test*
RandomLineTest::CreateTest()
{
	return new RandomLineTest();
}


