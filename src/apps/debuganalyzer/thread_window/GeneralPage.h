/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_GENERAL_PAGE_H
#define THREAD_GENERAL_PAGE_H

#include "AbstractGeneralPage.h"
#include "thread_window/ThreadWindow.h"


class TextDataView;


class ThreadWindow::GeneralPage : public AbstractGeneralPage {
public:
								GeneralPage();
	virtual						~GeneralPage();

			void				SetModel(Model* model, Model::Thread* thread);

private:
			Model*				fModel;
			Model::Thread*		fThread;
			TextDataView*		fThreadNameView;
			TextDataView*		fThreadIDView;
			TextDataView*		fTeamView;
			TextDataView*		fRunTimeView;
			TextDataView*		fWaitTimeView;
			TextDataView*		fLatencyView;
			TextDataView*		fPreemptionView;
			TextDataView*		fUnspecifiedTimeView;
};



#endif	// THREAD_GENERAL_PAGE_H
