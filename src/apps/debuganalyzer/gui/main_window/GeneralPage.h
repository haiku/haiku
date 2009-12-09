/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_GENERAL_PAGE_H
#define MAIN_GENERAL_PAGE_H

#include "AbstractGeneralPage.h"
#include "main_window/MainWindow.h"


class TextDataView;


class MainWindow::GeneralPage : public AbstractGeneralPage {
public:
								GeneralPage();
	virtual						~GeneralPage();

			void				SetModel(Model* model);

private:
			Model*				fModel;
			TextDataView*		fDataSourceView;
			TextDataView*		fCPUCountView;
			TextDataView*		fRunTimeView;
			TextDataView*		fIdleTimeView;
			TextDataView*		fTeamCountView;
			TextDataView*		fThreadCountView;
};



#endif	// MAIN_GENERAL_PAGE_H
