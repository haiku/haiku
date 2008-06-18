/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _CHARACTER_CLASSES_H
#define _CHARACTER_CLASSES_H

#define kCr '\n'
#define kLf '\r'
#define kTab '\t'
#define kEof -1

inline bool IsWhitespaceSeparator(int ch)
{
	return ch == ' ' || ch == kTab;
}

inline bool IsWhitespace(int ch)
{
	return ch == ' ' || ch == kTab || ch == kLf || ch == kCr;
}

inline bool IsIdentChar(int ch) {
	// TODO check '.' if is an identifier character
	// in one of the PPD files delivered with BeOS R5
	// '.' is used inside of an identifier
	// if (ch == '.' || ch == '/' || ch == ':') return false;
	if (ch == '/' || ch == ':') return false;
	return 33 <= ch && ch <= 126;
}

inline bool IsOptionChar(int ch)
{
	if (ch == '.') return true;
	return IsIdentChar(ch);
}

inline bool IsChar(int ch)
{
	if (ch == '"') return false;
	return 32 <= ch && ch <= 255 || IsWhitespace(ch);
}
	
inline bool IsPrintableWithoutWhitespaces(int ch)
{
	if (ch == '"') return false;
	return 33 <= ch && ch <= 126;
}
	
inline bool IsPrintableWithWhitespaces(int ch)
{
	return IsPrintableWithoutWhitespaces(ch) || IsWhitespace(ch);
}

inline bool IsStringChar(int ch)
{
	if (IsWhitespaceSeparator(ch)) return true;
	if (ch == '"') return true;
	if (ch == '/') return false;
	return IsPrintableWithoutWhitespaces(ch);
}

#endif
