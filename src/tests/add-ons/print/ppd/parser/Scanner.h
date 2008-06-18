/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _SCANNER_H
#define _SCANNER_H

#include "CharacterClasses.h"
#include "PPDFile.h"

class Scanner {
private:	
	PPDFile* fCurrentFile;
	BString  fLastError;
	BString  fWarnings;
	
	BString* Scan(bool (cond)(int ch));
	bool ScanHexadecimalSubstring(BString* literal);
	BString* ScanLiteral(bool quotedValue, int separator);
	
public:
	Scanner(const char* file);
	virtual ~Scanner();
	
	status_t InitCheck();
	
	// Returns the current character or -1 if on EOF.
	int GetCurrentChar();
	// Reads the next character. Use GetChar to read the current 
	void NextChar();
	// Returns the position of the current character
	Position GetPosition();
	// Returns the file name of the current character
	const char* GetFileName();
	
	BString* ScanIdent()            { return Scan(IsIdentChar); }
	BString* ScanOption()           { return Scan(IsOptionChar); }
	BString* ScanSymbolValue()      { return Scan(IsPrintableWithoutWhitespaces); }
	BString* ScanInvocationValue()  { return Scan(IsPrintableWithWhitespaces); }
	BString* ScanStringValue()      { return Scan(IsStringChar); }
	BString* ScanTranslationValue(int separator = kEof) 
									{ return ScanLiteral(false, separator); }
	BString* ScanQuotedValue()      { return ScanLiteral(true, kEof); }
	
	// Include the file at the current position and read the first char.
	bool Include(const char* file);

	void Warning(const char* message);
	const char* GetWarningMessage();
	bool HasWarning();
	
	void Error(const char* message);	
	const char* GetErrorMessage();
	bool HasError();
};

#endif
