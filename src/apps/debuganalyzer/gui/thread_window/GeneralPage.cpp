/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "thread_window/GeneralPage.h"

#include <stdio.h>

#include "util/TimeUtils.h"


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
	fPreemptionView(NULL),
	fUnspecifiedTimeView(NULL)
{
	fThreadNameView = AddDataView("Name:");
	fThreadIDView = AddDataView("ID:");
	fTeamView = AddDataView("Team:");
	fRunTimeView = AddDataView("Run time:");
	fWaitTimeView = AddDataView("Wait time:");
	fLatencyView = AddDataView("Latencies:");
	fPreemptionView = AddDataView("Preemptions:");
	fUnspecifiedTimeView = AddDataView("Unspecified time:");
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
		char timeBuffer[64];
		format_nanotime(fThread->TotalRunTime(), timeBuffer,
			sizeof(timeBuffer));
		snprintf(buffer, sizeof(buffer), "%s (%lld)", timeBuffer,
			fThread->Runs());
		fRunTimeView->SetText(buffer);

		// wait time
		format_nanotime(fThread->TotalWaitTime(), timeBuffer,
			sizeof(timeBuffer));
		snprintf(buffer, sizeof(buffer), "%s (%lld)", timeBuffer,
			fThread->Waits());
		fWaitTimeView->SetText(buffer);

		// latencies
		format_nanotime(fThread->TotalLatency(), timeBuffer,
			sizeof(timeBuffer));
		snprintf(buffer, sizeof(buffer), "%s (%lld)", timeBuffer,
			fThread->Latencies());
		fLatencyView->SetText(buffer);

		// preemptions
		format_nanotime(fThread->TotalRerunTime(), timeBuffer,
			sizeof(timeBuffer));
		snprintf(buffer, sizeof(buffer), "%s (%lld)", timeBuffer,
			fThread->Preemptions());
		fPreemptionView->SetText(buffer);

		// unspecified time
		format_nanotime(fThread->UnspecifiedWaitTime(), timeBuffer,
			sizeof(timeBuffer));
		fUnspecifiedTimeView->SetText(timeBuffer);
	} else {
		fThreadNameView->SetText("");
		fThreadIDView->SetText("");
		fTeamView->SetText("");
		fRunTimeView->SetText("");
		fWaitTimeView->SetText("");
		fLatencyView->SetText("");
		fPreemptionView->SetText("");
		fUnspecifiedTimeView->SetText("");
	}
}
