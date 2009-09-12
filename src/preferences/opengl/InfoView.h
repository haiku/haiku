/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef INFO_VIEW_H
#define INFO_VIEW_H


#include <View.h>

class InfoView : public BView {
public:
    InfoView();
    virtual ~InfoView();

	virtual	void MessageReceived(BMessage* message);
	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
};

#endif /* INFO_VIEW_H */
