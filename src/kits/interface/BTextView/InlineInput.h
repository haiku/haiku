//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Haiku, Inc.
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
//	File Name:		InlineInput.h
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	Helper class to handle input method requests
//------------------------------------------------------------------------------
#ifndef __INLINEINPUT_H
#define __INLINEINPUT_H

#include <Messenger.h>

struct clause;
class _BInlineInput_ {
public:
	_BInlineInput_(BMessenger);
	~_BInlineInput_();
	
	const BMessenger *Method() const;
	
	bool IsActive() const;
	void SetActive(bool active);
	
	int32 Length() const;
	void SetLength(int32 length);
	
	int32 Offset() const;
	void SetOffset(int32 offset);
		
	int32 SelectionLength() const;
	void SetSelectionLength(int32);
	
	int32 SelectionOffset() const;
	void SetSelectionOffset(int32 offset);
	
	void AddClause(int32, int32);
	bool GetClause(int32 index, int32 *start, int32 *end) const;
	int32 CountClauses() const;
	
	void ResetClauses();
	
private:
	const BMessenger fMessenger;

	bool fActive;
		
	int32 fOffset;
	int32 fLength;
	
	int32 fSelectionOffset;
	int32 fSelectionLength;
	
	int32 fNumClauses;
	clause *fClauses;
};

#endif //__INLINEINPUT_H
