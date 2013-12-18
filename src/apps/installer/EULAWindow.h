/*
 * Copyright 2013,	Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EULA_WINDOW_H
#define EULA_WINDOW_H


#include <Window.h>


class EULAWindow : public BWindow {
public:
								EULAWindow();
	virtual bool				QuitRequested();

private:

};

#endif // EULA_WINDOW_H
