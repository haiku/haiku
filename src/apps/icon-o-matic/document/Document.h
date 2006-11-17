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

#include "Observable.h"
#include "RWLocker.h"

struct entry_ref;

class CommandStack;
class DocumentSaver;
class Icon;
class Selection;

class Document : public RWLocker,
				 public Observable {
 public:
								Document(const char* name = NULL);
	virtual						~Document();

	inline	::CommandStack*		CommandStack() const
									{ return fCommandStack; }

	inline	::Selection*		Selection() const
									{ return fSelection; }

			void				SetName(const char* name);
			const char*			Name() const;

			void				SetNativeSaver(::DocumentSaver* saver);
	inline	::DocumentSaver*	NativeSaver() const
									{ return fNativeSaver; }

			void				SetExportSaver(::DocumentSaver* saver);
	inline	::DocumentSaver*	ExportSaver() const
									{ return fExportSaver; }

			void				SetIcon(::Icon* icon);
	inline	::Icon*				Icon() const
									{ return fIcon; }

			void				MakeEmpty(bool includingSavers = true);

 private:
			::Icon*				fIcon;
			::CommandStack*		fCommandStack;
			::Selection*		fSelection;

			BString				fName;

			::DocumentSaver*	fNativeSaver;
			::DocumentSaver*	fExportSaver;
};

#endif // DOCUMENT_H
