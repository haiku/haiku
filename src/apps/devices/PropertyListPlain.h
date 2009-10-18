/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Modified by Pieter Panman
 */
#ifndef PROPERTY_LIST_PLAIN_H
#define PROPERTY_LIST_PLAIN_H


#include <View.h>

#include "Device.h"


class PropertyListPlain : public BView {
public:
					PropertyListPlain(const char* name);
	virtual			~PropertyListPlain();

	virtual void	AddAttributes(const Attributes& attributes);
	virtual void	RemoveAll();

	virtual	void	MessageReceived(BMessage* message);
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
private:
	BView*			rootView;
};

#endif /* PROPERTY_LIST_PLAIN_H */
