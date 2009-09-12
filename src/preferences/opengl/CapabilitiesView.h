/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef CAPABILITIES_VIEW_H
#define CAPABILITIES_VIEW_H


#include <View.h>

class CapabilitiesView : public BView {
public:
    CapabilitiesView();
    virtual ~CapabilitiesView();

	virtual	void MessageReceived(BMessage* message);
	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
};

#endif /* CAPABILITIES_VIEW_H */
