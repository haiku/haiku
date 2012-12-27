/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */
#ifndef _GRAVITY_VIEW_HPP_
#define _GRAVITY_VIEW_HPP_


#include <GLView.h>

#include "Particle.hpp"
#include "GravityHole.hpp"
#include "GravityScreenSaver.hpp"


class GravityScreenSaver;

class GravityView : public BGLView
{
public:
							GravityView(GravityScreenSaver* parent, BRect rect);
							~GravityView();
	
			void			AttachedToWindow();
	
			void			Draw(BRect rect);
			void			Run();
	
private:
			BRect			fRect;

			GravityScreenSaver* parent;
			GravityHole*	ghole;
};


#endif /* _GRAVITY_VIEW_HPP_ */
