/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DOCUMENT_H
#define DOCUMENT_H


#include "Observable.h"
#include "RWLocker.h"

#include <String.h>

struct entry_ref;

namespace BPrivate {
namespace Icon {

class Icon;

}	// namespace Icon
}	// namespace BPrivate

class CommandStack;
class DocumentSaver;
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

			void				SetIcon(BPrivate::Icon::Icon* icon);
	inline	BPrivate::Icon::Icon* Icon() const
									{ return fIcon; }

			void				MakeEmpty(bool includingSavers = true);

 private:
			BPrivate::Icon::Icon* fIcon;
			::CommandStack*		fCommandStack;
			::Selection*		fSelection;

			BString				fName;

			::DocumentSaver*	fNativeSaver;
			::DocumentSaver*	fExportSaver;
};

#endif // DOCUMENT_H
