/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef __CALCULATOR__
#define __CALCULATOR__

#include <Application.h>

class CalculatorApp : public BApplication
{
public:
					CalculatorApp();
	virtual	void	AboutRequested();

private:
		BWindow		*wind;

};




#endif
