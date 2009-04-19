/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


class BTabView;
class DataSource;
class MainModel;
class MainModelLoader;


class MainWindow : public BWindow {
public:
								MainWindow(DataSource* dataSource);
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Quit();

	virtual	void				Show();

private:
			void				_SetModel(MainModel* model);

private:
			BTabView*			fMainTabView;
			MainModel*			fModel;
			MainModelLoader*	fModelLoader;
};



#endif	// MAIN_WINDOW_H
