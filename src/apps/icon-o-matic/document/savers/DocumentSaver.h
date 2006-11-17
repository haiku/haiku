/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef DOCUMENT_SAVER_H
#define DOCUMENT_SAVER_H

#include <SupportDefs.h>

class Document;

class DocumentSaver {
 public:
								DocumentSaver();
	virtual						~DocumentSaver();

	virtual	status_t			Save(Document* document) = 0;
};

#endif // DOCUMENT_SAVER_H
