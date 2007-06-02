/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef NATIVE_SAVER_H
#define NATIVE_SAVER_H

#include "AttributeSaver.h"
#include "SimpleFileSaver.h"

class NativeSaver : public DocumentSaver {
 public:
								NativeSaver(const entry_ref& ref);
	virtual						~NativeSaver();

	virtual	status_t			Save(Document* document);

 protected:
			AttributeSaver		fAttrSaver;
			SimpleFileSaver		fFileSaver;
};

#endif // NATIVE_SAVER_H
