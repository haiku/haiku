/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EDIT_VIEW_H
#define EDIT_VIEW_H


#include <View.h>


class ResourceRow;


class EditView : public BView {
public:
							EditView(const char* name);
	virtual					~EditView();

	virtual void			AttachTo(BView* view);
	virtual	void			Edit(ResourceRow* row);
	virtual	void			Commit();

protected:
			ResourceRow*	fRow;

};


#endif

