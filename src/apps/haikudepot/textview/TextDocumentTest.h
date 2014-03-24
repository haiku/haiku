/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TEXT_DOCUMENT_TEST_H
#define TEXT_DOCUMENT_TEST_H


#include <Application.h>


class TextDocumentTest : public BApplication {
public:
								TextDocumentTest();
	virtual						~TextDocumentTest();

	virtual	void				ReadyToRun();
};


#endif // TEXT_DOCUMENT_TEST_H
