/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <List.h>

void TestScanner();
void TestParser();
void TestPPDParser(bool all, bool verbose = true);
void TestExtractUI();

const char* gPPDFile = "aptollw1.ppd";

static BList gArgs;

bool enabled(const char* name, const char* arg)
{
	gArgs.AddItem((void*)name);
	if (arg == NULL) return false;
	if (strcmp(arg, "all") == 0) return true;
	return strcmp(arg, name) == 0;
}

void printArgs(const char* programName)
{
	fprintf(stderr, "%s: argument\n", programName);
	fprintf(stderr, "Argument is missing. The available arguments are:\n");
	fprintf(stderr, "  all\n");
	for (int i = 0; i < gArgs.CountItems(); i ++) {
		fprintf(stderr, "  %s\n", (const char*)gArgs.ItemAt(i));
	}
}

int main(int argc, char* argv[])
{
	const char* arg = argc >= 2 ? argv[1] : NULL;
	
	if (argc >= 3) {
		gPPDFile = argv[2];
	}
	
	if (enabled("scanner", arg)) {
		TestScanner();
	}
	if (enabled("parser", arg)) {
		TestParser();
	}
	if (enabled("ppd", arg)) {
		TestPPDParser(true);
	}
	if (enabled("header", arg)) {
		TestPPDParser(false);
	}
	if (enabled("ui", arg)) {
		TestExtractUI();
	}
	if (enabled("ppd-timing", arg)) {
		TestPPDParser(true, false);
	}
	if (enabled("header-timing", arg)) {
		TestPPDParser(false, false);
	}
	
	if (arg == NULL) {
		printArgs(argv[0]);
	}
}
