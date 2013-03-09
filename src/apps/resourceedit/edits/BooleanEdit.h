/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef BOOLEAN_EDIT_H
#define BOOLEAN_EDIT_H


#include "EditView.h"


class BCheckBox;


class BooleanEdit : public EditView {
public:
					BooleanEdit();
					~BooleanEdit();

	void			AttachTo(BView* view);
	void			Edit(ResourceRow* row);
	void			Commit();

private:
	BCheckBox*		fValueCheck;

};


#endif
