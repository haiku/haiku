/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "main_window/GeneralPage.h"

#include <stdio.h>


MainWindow::GeneralPage::GeneralPage()
	:
	AbstractGeneralPage(),
	fModel(NULL),
	fDataSourceView(NULL),
	fRunTimeView(NULL),
	fTeamCountView(NULL),
	fThreadCountView(NULL)
{
	fDataSourceView = AddDataView("Data Source:");
	fRunTimeView = AddDataView("Run Time:");
	fTeamCountView = AddDataView("Teams:");
	fThreadCountView = AddDataView("Threads:");
}


MainWindow::GeneralPage::~GeneralPage()
{
}


void
MainWindow::GeneralPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	fModel = model;

	if (fModel != NULL) {
		// data source
		fDataSourceView->SetText(fModel->DataSourceName());

		// run time
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%lld μs", fModel->LastEventTime());
		fRunTimeView->SetText(buffer);

		// team count
		snprintf(buffer, sizeof(buffer), "%ld", fModel->CountTeams());
		fTeamCountView->SetText(buffer);

		// threads
		snprintf(buffer, sizeof(buffer), "%ld", fModel->CountThreads());
		fThreadCountView->SetText(buffer);
	} else {
		fDataSourceView->SetText("");
		fRunTimeView->SetText("");
		fTeamCountView->SetText("");
		fThreadCountView->SetText("");
	}
}
