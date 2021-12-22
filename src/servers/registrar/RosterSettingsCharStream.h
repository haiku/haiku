//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RecentApps.cpp
//	Author:			Tyler Dauwalder (tyler@dauwalder.net)
//	Description:	Recently launched apps list
//------------------------------------------------------------------------------
/*!
	\file sniffer/RosterSettingsCharStream.h
	Character stream class
*/
#ifndef _ROSTER_SETTINGS_CHAR_STREAM_H
#define _ROSTER_SETTINGS_CHAR_STREAM_H

#include <sniffer/CharStream.h>
#include <SupportDefs.h>

#include <string>

//! A character stream manager designed for use with RosterSettings files
/*! The purpose for this class is to make reading through RosterSettings files,
	which contain a number of different types of strings (some with escapes),
	as well as comments, as painless as possible.
	
	The main idea is that GetString() will return \c B_OK and a new string
	until it hits an error or the end of the line, at which point an error
	code is returned. The next call to GetString() starts the cycle over on
	the next line. When the end of the stream is finally hit, kEndOfStream is
	returned.
	
	SkipLine() simply moves the position of the stream to the next line, and is
	used when a valid string containing invalid data is discovered in the middle
	of a line.
*/
class RosterSettingsCharStream : public BPrivate::Storage::Sniffer::CharStream {
public:
	RosterSettingsCharStream(const std::string &string);
	RosterSettingsCharStream();
	virtual ~RosterSettingsCharStream();
	
	status_t GetString(char *result);
	status_t SkipLine();
	
	static const status_t kEndOfLine				= B_ERRORS_END+1;
	static const status_t kEndOfStream				= B_ERRORS_END+2;
	static const status_t kInvalidEscape			= B_ERRORS_END+3;
	static const status_t kUnterminatedQuotedString	= B_ERRORS_END+4;
	static const status_t kComment					= B_ERRORS_END+5;
	static const status_t kUnexpectedState			= B_ERRORS_END+6;
	static const status_t kStringTooLong			= B_ERRORS_END+7;
private:
	RosterSettingsCharStream(const RosterSettingsCharStream &ref);
	RosterSettingsCharStream& operator=(const RosterSettingsCharStream &ref);
};

#endif // _ROSTER_SETTINGS_CHAR_STREAM_H
