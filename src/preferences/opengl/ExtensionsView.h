/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef EXTENSIONS_VIEW_H
#define EXTENSIONS_VIEW_H

#include <View.h>

class ExtensionsList;

class ExtensionsView : public BView {
public:
    ExtensionsView();
    virtual ~ExtensionsView();

	virtual	void MessageReceived(BMessage* message);
	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	
private:
			void _AddExtensionsList(ExtensionsList *extList, char* stringList);

};


#endif /* EXTENSIONS_VIEW_H */
