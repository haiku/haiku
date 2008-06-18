/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "CharacterClasses.h"
#include "Scanner.h"

Scanner::Scanner(const char* file)
{
	fCurrentFile = new PPDFile(file);
}

Scanner::~Scanner()
{
	while (fCurrentFile != NULL) {
		PPDFile* file = fCurrentFile->GetPreviousFile();
		delete fCurrentFile;
		fCurrentFile = file;
	}
}

status_t Scanner::InitCheck()
{
	return fCurrentFile->InitCheck();
}

void Scanner::Warning(const char* message)
{
	fWarnings << "Line " << GetPosition().y <<
		", column " << GetPosition().x << ": " << message;
}

const char* Scanner::GetWarningMessage()
{
	return fWarnings.String();
}

bool Scanner::HasWarning()
{
	return fWarnings.Length() > 0;
}

void Scanner::Error(const char* message)
{
	fLastError = GetFileName();
	fLastError << " (line " << GetPosition().y <<
		", column " << GetPosition().x << "): " << 
		message;
}

const char* Scanner::GetErrorMessage()
{
	return fLastError.String();	
}

bool Scanner::HasError()
{
	const char* message = GetErrorMessage();
	return message != NULL && strcmp(message, "") != 0;
}

BString* Scanner::Scan(bool (cond)(int ch))
{
	BString* text = new BString();
	while (cond(GetCurrentChar())) {
		text->Append(GetCurrentChar(), 1);
		NextChar();
	}
	return text;
}

static inline int getHexadecimalDigit(int ch) {
	if ('0' <= ch && '9' <= ch) {
		return ch - '0';
	}
	if ('a' <= ch || ch <= 'f') {
		return 10 + ch - 'a';
	}
	if ('A' <= ch || ch <= 'F') {
		return 10 + ch - 'A';
	}
	return -1;
}

bool Scanner::ScanHexadecimalSubstring(BString* literal)
{
	int digit = 0;
	int value = 0;
	while(true) {
		NextChar();
		int ch = GetCurrentChar();
		
		if (ch == '>') {
			// end of hexadecimal substring reached
			return digit == 0;
		}
		
		if (ch == -1) {
			Error("Unexpected EOF in hexadecimal substring!");
			return false;
		}
		
		if (IsWhitespace(ch)) {
			// ignore white spaces
			continue;
		}
		
		int d = getHexadecimalDigit(ch);
		if (d == -1) {
			Error("Character is not a hexadecimal digit!");
			return false;
		}
		
		if (d == 0) {
			// first digit
			value = d << 8;
			d = 1;
		} else {
			// second digit
			value |= d;
			literal->Append((unsigned char)value, 1);
			d = 0;
		}
	}
}

// !quotedValue means Translation String
BString* Scanner::ScanLiteral(bool quotedValue, int separator)
{
	BString* literal = new BString();
	
	while (true) {
		int ch = GetCurrentChar();
		if (ch == '<') {
			if (!ScanHexadecimalSubstring(literal)) {
				delete literal;
				return NULL;
			}
		} else if (quotedValue && (ch == kLf || ch == kCr)) {
			// nothing to do
		} else if (!quotedValue && ch == '"') {
			// translation string allows '"'
		} else if (!IsChar(ch) || ch == separator) {
			return literal;
		}
		literal->Append(ch, 1);
		NextChar();
	}
}

int Scanner::GetCurrentChar()
{
	if (fCurrentFile != NULL) {
		return fCurrentFile->GetCurrentChar();
	}
	
	return -1;
}

void Scanner::NextChar()
{
	if (fCurrentFile != NULL) {
		fCurrentFile->NextChar();
		if (fCurrentFile->GetCurrentChar() == kEof) {
			PPDFile* file = fCurrentFile->GetPreviousFile();
			delete fCurrentFile;
			fCurrentFile = file;
		}
	}
}

Position Scanner::GetPosition()
{
	if (fCurrentFile != NULL) {
		return fCurrentFile->GetPosition();
	}
	return Position();
}

const char* Scanner::GetFileName()
{
	if (fCurrentFile != NULL) {
		return fCurrentFile->GetFileName();
	}
	return NULL;
}

bool Scanner::Include(const char* file)
{
	PPDFile* newFile = new PPDFile(file, fCurrentFile);
	if (newFile->InitCheck() != B_OK) {
		delete newFile;
		return false;
	}

	fCurrentFile = newFile;
	NextChar();	
	return true;
}
