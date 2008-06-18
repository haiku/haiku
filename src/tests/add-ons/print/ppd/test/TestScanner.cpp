/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "CharacterClasses.h"
#include "Scanner.h"

#include <stdio.h>

void Print(Scanner* scanner)
{
	Position position = scanner->GetPosition();
	const char* filename = scanner->GetFileName();
	int ch = scanner->GetCurrentChar();
	printf("[%d, %d] (%s) %c\n", position.x, position.y, filename, ch);
}

void TestScanner()
{
	Scanner scanner("main.ppd");
	if (scanner.InitCheck() != B_OK) {
		fprintf(stderr, "Could not open file main.ppd\n");
		return;
	}
	
	scanner.NextChar();
	for (int i = 0; i < 10; i ++) {
		int ch = scanner.GetCurrentChar();
		if (ch == kEof) {
			fprintf(stderr, "Unexpected end of file!\n");
			return;
		}
		Print(&scanner);
		scanner.NextChar();
	}
	
	if (!scanner.Include("include.ppd")) {
		fprintf(stderr, "Could not include file include.ppd\n");
		return;
	}
	
	while (scanner.GetCurrentChar() != kEof) {
		Print(&scanner);
		scanner.NextChar();
	}
	
	BString string;
	string.Append('a', 1);
	printf("%d\n", (int)string.Length());
	string.Append((char)0, 1);
	string.Append('b', 1);
	printf("%d\n", (int)string.Length());	
	for (int i = 0; i < string.Length(); i ++) {
		printf("%c ", string.String()[i]);
	}
	
	printf("%d\n", '"');
}
