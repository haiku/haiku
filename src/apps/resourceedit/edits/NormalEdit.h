/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef NORMAL_EDIT_H
#define NORMAL_EDIT_H


#include "EditView.h"


class BTextControl;

// TODO: Templatize this class and rename to NumericEdit.
//template<typename $type>
class NormalEdit : public EditView {
public:
					NormalEdit();
					~NormalEdit();

	void			AttachTo(BView* view);
	void			Edit(ResourceRow* row);
	void			Commit();

private:
	BTextControl*	fValueText;

};


#endif
