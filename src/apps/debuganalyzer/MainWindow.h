/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


class BTabView;


class MainWindow : public BWindow {
public:
								MainWindow();
	virtual						~MainWindow();

private:
			BTabView*			fMainTabView;
};



#endif	// MAIN_WINDOW_H
