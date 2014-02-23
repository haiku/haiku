/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tri-Edge AI
 *		John Scipione, jscipione@gmail.com
 */
#ifndef GRAVITY_VIEW_H
#define GRAVITY_VIEW_H


#include <GLView.h>


class Gravity;
class GravitySource;


class GravityView : public BGLView {
public:
							GravityView(BRect frame, Gravity* parent);
							~GravityView();

			void			AttachedToWindow();

			void			DirectDraw();

private:
			Gravity*		fParent;
			GravitySource*	fGravitySource;

			int32			fSize;
			int32			fShade;
};


#endif	// GRAVITY_VIEW_H
