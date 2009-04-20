/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>


class BTabView;
class DataSource;
class ModelLoader;
class Model;


class MainWindow : public BWindow {
public:
								MainWindow(DataSource* dataSource);
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Quit();

	virtual	void				Show();

private:
			void				_SetModel(Model* model);

private:
			BTabView*			fMainTabView;
			Model*				fModel;
			ModelLoader*		fModelLoader;
};



#endif	// MAIN_WINDOW_H
