/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <String.h>

#include "RWLocker.h"

struct entry_ref;

class CommandStack;
class Selection;

class Document : public RWLocker {
 public:
								Document(const char* name = NULL);
	virtual						~Document();

			::CommandStack*		CommandStack() const
									{ return fCommandStack; }

			::Selection*		Selection() const
									{ return fSelection; }

			void				SetName(const char* name);
			const char*			Name() const;

			void				SetRef(const entry_ref& ref);
			const entry_ref*	Ref() const
									{ return fRef; }

 private:
			::CommandStack*		fCommandStack;
			::Selection*		fSelection;

			BString				fName;
			entry_ref*			fRef;
};

#endif // DOCUMENT_H
