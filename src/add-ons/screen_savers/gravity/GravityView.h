/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GRAVITY_VIEW_H
#define GRAVITY_VIEW_H


#include <GLView.h>


class Gravity;
class GravitySource;

class GravityView : public BGLView
{
public:
					GravityView(Gravity* parent, BRect rect);
					~GravityView();

	void			AttachedToWindow();

	void			DirectDraw();

private:
	Gravity*		fParent;
	GravitySource*	fGravSource;

};


#endif
