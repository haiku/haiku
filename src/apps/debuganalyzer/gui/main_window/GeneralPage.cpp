/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "main_window/GeneralPage.h"

#include <stdio.h>

#include "util/TimeUtils.h"


MainWindow::GeneralPage::GeneralPage()
	:
	AbstractGeneralPage(),
	fModel(NULL),
	fDataSourceView(NULL),
	fCPUCountView(NULL),
	fRunTimeView(NULL),
	fIdleTimeView(NULL),
	fTeamCountView(NULL),
	fThreadCountView(NULL)
{
	fDataSourceView = AddDataView("Data source:");
	fCPUCountView = AddDataView("Number of CPUs:");
	fRunTimeView = AddDataView("Total time:");
	fIdleTimeView = AddDataView("Idle time:");
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

		// cpu count
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%" B_PRId32, fModel->CountCPUs());
		fCPUCountView->SetText(buffer);

		// run time
		nanotime_t runtime = fModel->LastEventTime();
		fRunTimeView->SetText(format_nanotime(runtime, buffer, sizeof(buffer)));

		// idle time
		if (runtime == 0)
			runtime = 1;
		double idlePercentage = (double)fModel->IdleTime()
			/ (runtime * fModel->CountCPUs()) * 100;
		char timeBuffer[64];
		format_nanotime(fModel->IdleTime(), timeBuffer, sizeof(timeBuffer));
		snprintf(buffer, sizeof(buffer), "%s (%.2f %%)", timeBuffer,
			idlePercentage);
		fIdleTimeView->SetText(buffer);

		// team count
		snprintf(buffer, sizeof(buffer), "%ld", fModel->CountTeams());
		fTeamCountView->SetText(buffer);

		// threads
		snprintf(buffer, sizeof(buffer), "%ld", fModel->CountThreads());
		fThreadCountView->SetText(buffer);
	} else {
		fDataSourceView->SetText("");
		fCPUCountView->SetText("");
		fRunTimeView->SetText("");
		fIdleTimeView->SetText("");
		fTeamCountView->SetText("");
		fThreadCountView->SetText("");
	}
}
