/*
 * Copyright 2015-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ReportUserInterface.h"

#include <stdio.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include <AutoLocker.h>

#include "MessageCodes.h"
#include "UiUtils.h"


ReportUserInterface::ReportUserInterface(thread_id targetThread,
	const char* reportPath)
	:
	fTeam(NULL),
	fListener(NULL),
	fTargetThread(targetThread),
	fReportPath(reportPath),
	fShowSemaphore(-1),
	fReportSemaphore(-1),
	fShown(false),
	fTerminating(false)
{
}


ReportUserInterface::~ReportUserInterface()
{
	if (fShowSemaphore >= 0)
		delete_sem(fShowSemaphore);

	fTeam->RemoveListener(this);
}


const char*
ReportUserInterface::ID() const
{
	return "ReportUserInterface";
}


status_t
ReportUserInterface::Init(Team* team, UserInterfaceListener* listener)
{
	fShowSemaphore = create_sem(0, "show report");
	if (fShowSemaphore < 0)
		return fShowSemaphore;

	fReportSemaphore = create_sem(0, "report generator wait");
	if (fReportSemaphore < 0)
		return fReportSemaphore;

	fTeam = team;
	fListener = listener;

	fTeam->AddListener(this);

	return B_OK;
}


void
ReportUserInterface::Show()
{
	fShown = true;
	release_sem(fShowSemaphore);
}


void
ReportUserInterface::Terminate()
{
	fTerminating = true;
}


UserInterface*
ReportUserInterface::Clone() const
{
	// the report interface does not support cloning, since
	// it won't ever be asked to interactively restart.
	return NULL;
}


bool
ReportUserInterface::IsInteractive() const
{
	return false;
}


status_t
ReportUserInterface::LoadSettings(const TeamUiSettings* settings)
{
	return B_OK;
}


status_t
ReportUserInterface::SaveSettings(TeamUiSettings*& settings) const
{
	return B_OK;
}


void
ReportUserInterface::NotifyUser(const char* title, const char* message,
	user_notification_type type)
{
}


void
ReportUserInterface::NotifyBackgroundWorkStatus(const char* message)
{
}


int32
ReportUserInterface::SynchronouslyAskUser(const char* title,
	const char* message, const char* choice1, const char* choice2,
	const char* choice3)
{
	return -1;
}


status_t
ReportUserInterface::SynchronouslyAskUserForFile(entry_ref* _ref)
{
	return B_UNSUPPORTED;
}


void
ReportUserInterface::Run()
{
	// Wait for the Show() semaphore to be released.
	status_t error;
	do {
		error = acquire_sem(fShowSemaphore);
	} while (error == B_INTERRUPTED);

	if (error != B_OK)
		return;

	bool waitNeeded = false;
	if (fTargetThread > 0) {
		AutoLocker< ::Team> teamLocker(fTeam);
		::Thread* thread = fTeam->ThreadByID(fTargetThread);
		if (thread == NULL)
			waitNeeded = true;
		else if (thread->State() != THREAD_STATE_STOPPED) {
			waitNeeded = true;
			fListener->ThreadActionRequested(fTargetThread, MSG_THREAD_STOP);
		}
	}

	if (waitNeeded) {
		do {
			error = acquire_sem(fShowSemaphore);
		} while (error == B_INTERRUPTED);

		if (error != B_OK)
			return;
	}

	entry_ref ref;
	if (fReportPath != NULL && fReportPath[0] == '/') {
		error = get_ref_for_path(fReportPath, &ref);
	} else {
		char filename[B_FILE_NAME_LENGTH];
		if (fReportPath != NULL)
			strlcpy(filename, fReportPath, sizeof(filename));
		else
			UiUtils::ReportNameForTeam(fTeam, filename, sizeof(filename));

		BPath path;
		error = find_directory(B_DESKTOP_DIRECTORY, &path);
		if (error == B_OK)
			error = path.Append(filename);
		if (error == B_OK)
			error = get_ref_for_path(path.Path(), &ref);
	}

	if (error != B_OK)
		printf("Unable to get ref for report path %s\n", strerror(error));
	else {
		fListener->DebugReportRequested(&ref);

		do {
			error = acquire_sem(fReportSemaphore);
		} while (error == B_INTERRUPTED);
	}

	fListener->UserInterfaceQuitRequested(
		UserInterfaceListener::QUIT_OPTION_ASK_KILL_TEAM);
}


void
ReportUserInterface::ThreadAdded(const Team::ThreadEvent& event)
{
	::Thread* thread = event.GetThread();
	if (thread->ID() != fTargetThread)
		return;

	if (thread->State() != THREAD_STATE_STOPPED)
		fListener->ThreadActionRequested(thread->ID(), MSG_THREAD_STOP);
	else
		release_sem(fShowSemaphore);
}


void
ReportUserInterface::ThreadStateChanged(const Team::ThreadEvent& event)
{
	::Thread* thread = event.GetThread();
	if (thread->ID() != fTargetThread)
		return;
	else if (thread->State() == THREAD_STATE_STOPPED)
		release_sem(fShowSemaphore);
}


void
ReportUserInterface::DebugReportChanged(const Team::DebugReportEvent& event)
{
	if (event.GetFinalStatus() == B_OK)
		printf("Debug report saved to %s\n", event.GetReportPath());
	else {
		fprintf(stderr, "Failed to write debug report: %s\n", strerror(
				event.GetFinalStatus()));
	}
	release_sem(fReportSemaphore);
}
