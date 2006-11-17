/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef FILE_SAVER_H
#define FILE_SAVER_H

#include <Entry.h>

#include "DocumentSaver.h"

class FileSaver : public DocumentSaver {
 public:
								FileSaver(const entry_ref& ref);
	virtual						~FileSaver();

			void				SetRef(const entry_ref& ref);
			const entry_ref*	Ref() const
									{ return &fRef; }

 protected:
			entry_ref			fRef;
};

#endif // FILE_SAVER_H
