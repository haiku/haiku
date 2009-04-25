/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "thread_window/GeneralPage.h"

#include <stdio.h>


ThreadWindow::GeneralPage::GeneralPage()
	:
	AbstractGeneralPage(),
	fModel(NULL),
	fThread(NULL),
	fThreadNameView(NULL),
	fThreadIDView(NULL),
	fTeamView(NULL),
	fRunTimeView(NULL),
	fWaitTimeView(NULL),
	fLatencyView(NULL),
	fPreemptionView(NULL)
{
	fThreadNameView = AddDataView("Name:");
	fThreadIDView = AddDataView("ID:");
	fTeamView = AddDataView("Team:");
	fRunTimeView = AddDataView("Run Time:");
	fWaitTimeView = AddDataView("Wait Time:");
	fLatencyView = AddDataView("Latencies:");
	fPreemptionView = AddDataView("Preemptions:");
}


ThreadWindow::GeneralPage::~GeneralPage()
{
}


void
ThreadWindow::GeneralPage::SetModel(Model* model, Model::Thread* thread)
{
	if (model == fModel && thread == fThread)
		return;

	fModel = model;
	fThread = thread;

	if (fThread != NULL) {
		// name
		fThreadNameView->SetText(fThread->Name());

		// ID
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "%ld", fThread->ID());
		fThreadIDView->SetText(buffer);

		// team
		fTeamView->SetText(thread->GetTeam()->Name());

		// run time
		snprintf(buffer, sizeof(buffer), "%lld μs (%lld)",
			fThread->TotalRunTime(), fThread->Runs());
		fRunTimeView->SetText(buffer);

		// wait time
		fWaitTimeView->SetText("");
// TODO:...
//		snprintf(buffer, sizeof(buffer), "%lld μs (%lld)",
//			fThread->TotalRunTime(), fThread->Runs());
//		fWaitTimeView->SetText(buffer);

		// latencies
		snprintf(buffer, sizeof(buffer), "%lld μs (%lld)",
			fThread->TotalLatency(), fThread->Latencies());
		fLatencyView->SetText(buffer);

		// preemptions
		snprintf(buffer, sizeof(buffer), "%lld μs (%lld)",
			fThread->TotalRerunTime(), fThread->Preemptions());
		fPreemptionView->SetText(buffer);
	} else {
		fThreadNameView->SetText("");
		fThreadIDView->SetText("");
		fTeamView->SetText("");
		fRunTimeView->SetText("");
		fWaitTimeView->SetText("");
		fLatencyView->SetText("");
		fPreemptionView->SetText("");
	}
}
