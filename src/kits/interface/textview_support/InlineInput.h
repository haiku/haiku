/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#ifndef __INLINEINPUT_H
#define __INLINEINPUT_H

#include <Messenger.h>
#include <TextView.h>

struct clause;

class BTextView::InlineInput {
public:
	InlineInput(BMessenger);
	~InlineInput();
	
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
	
	bool AddClause(int32, int32);
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
