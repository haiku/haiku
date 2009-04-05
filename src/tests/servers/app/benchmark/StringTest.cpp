/*
 * Copyright (C) 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StringTest.h"

#include <stdio.h>

#include <String.h>
#include <View.h>

#include "TestSupport.h"


StringTest::StringTest()
	: Test(),
	  fTestDuration(0),
	  fTestStart(-1),
	  fGlyphsRendered(0),
	  fGlyphsPerLine(500),
	  fIterations(0),
	  fMaxIterations(1500),

	  fStartHeight(11.0),
	  fLineHeight(15.0)
{
}


StringTest::~StringTest()
{
}


void
StringTest::Prepare(BView* view)
{
//	SetupClipping(view);

	font_height fh;
	view->GetFontHeight(&fh);
	fLineHeight = ceilf(fh.ascent) + ceilf(fh.descent)
		+ ceilf(fh.leading);
	fStartHeight = ceilf(fh.ascent) + ceilf(fh.descent);
	fViewBounds = view->Bounds();

	BString string;
	string.Append('M', fGlyphsPerLine);
	while (view->StringWidth(string.String()) < fViewBounds.Width() - 10)
		string.Append('M', 1);
	while (view->StringWidth(string.String()) > fViewBounds.Width() - 10)
		string.Remove(string.Length() - 1, 1);

	fGlyphsPerLine = 60; //string.Length();
	fTestDuration = 0;
	fGlyphsRendered = 0;
	fIterations = 0;
	fTestStart = system_time();
}

bool
StringTest::RunIteration(BView* view)
{
	BPoint textLocation;
	textLocation.x = 5;
	textLocation.y = random_number_between(fStartHeight,
		fStartHeight + fLineHeight / 2);

	char buffer[fGlyphsPerLine + 1];
	buffer[fGlyphsPerLine] = 0;

	bigtime_t now = system_time();

	while (true) {
		// fill string with random chars
		for (uint32 j = 0; j < fGlyphsPerLine; j++)
			buffer[j] = 'A' + rand() % ('z' - 'A');

		view->DrawString(buffer, textLocation);

		fGlyphsRendered += fGlyphsPerLine;

		// offset text location
		textLocation.y += fLineHeight;
		if (textLocation.y > fViewBounds.bottom)
			break;
	}

	view->Sync();

	fTestDuration += system_time() - now;
	fIterations++;

	return fIterations < fMaxIterations;
}


void
StringTest::PrintResults(BView* view)
{
	if (fTestDuration == 0) {
		printf("Test was not run.\n");
		return;
	}
	bigtime_t timeLeak = system_time() - fTestStart - fTestDuration;

	Test::PrintResults(view);

	printf("Glyphs per DrawString() call: %ld\n", fGlyphsPerLine);
	printf("Glyphs per second: %.3f\n",
		fGlyphsRendered * 1000000.0 / fTestDuration);
	printf("Average time between iterations: %.4f seconds.\n",
		(float)timeLeak / fIterations / 1000000);
}


Test*
StringTest::CreateTest()
{
	return new StringTest();
}

