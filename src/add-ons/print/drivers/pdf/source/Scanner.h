/*

Scanner.

Copyright (c) 2002 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef _SCANNER_H
#define _SCANNER_H

#include <stdio.h>
#include <String.h>

class Scanner {
	class Position {
	public:
		int32 line;
		int32 column;
	};
	
	FILE*    fFile;
	Position fCur;
	Position fPrev;

	int  GetCh();
	void UngetCh(int ch);
	
public:
	Scanner(const char* name);
	status_t InitCheck() const;
	~Scanner();

	void  SkipSpaces();
	bool  ReadName(BString* s);
	bool  ReadString(BString* s);
	bool  ReadFloat(float* f);
	bool  NextChar(char c);
	bool  IsEOF() const;
	int32 Line() const   { return fCur.line; }
	int32 Column() const { return fCur.column; }
};

#endif
