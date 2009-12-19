/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef __INLINEINPUT_H
#define __INLINEINPUT_H

#include <Messenger.h>
#include <String.h>

struct clause;

class InlineInput {
public:
	InlineInput(BMessenger);
	~InlineInput();
	
	const BMessenger *Method() const;

	const char *String() const;
	void SetString(const char *string);	

	bool IsActive() const;
	void SetActive(bool active);
			
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
	BString fString;

	bool fActive;
		
	int32 fSelectionOffset;
	int32 fSelectionLength;
	
	int32 fNumClauses;
	clause *fClauses;
};

#endif //__INLINEINPUT_H
