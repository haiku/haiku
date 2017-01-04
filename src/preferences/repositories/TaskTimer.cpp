/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */


#include "TaskTimer.h"

#include <Application.h>
#include <Catalog.h>

#include "constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TaskTimer"

static int32 sAlertStackCount = 0;


TaskTimer::TaskTimer(const BMessenger& target, Task* owner)
	:
	BLooper(),
	fTimeoutMicroSeconds(kTimerTimeoutSeconds * 1000000),
	fTimerIsRunning(false),
	fReplyTarget(target),
	fMessageRunner(NULL),
	fTimeoutMessage(TASK_TIMEOUT),
	fTimeoutAlert(NULL),
	fOwner(owner)
{
	Run();

	// Messenger for the Message Runner to use to send its message to the timer
	fMessenger.SetTo(this);
	// Invoker for the Alerts to use to send their messages to the timer
	fTimeoutAlertInvoker.SetMessage(
		new BMessage(TIMEOUT_ALERT_BUTTON_SELECTION));
	fTimeoutAlertInvoker.SetTarget(this);
}


TaskTimer::~TaskTimer()
{
	if (fTimeoutAlert) {
		fTimeoutAlert->Lock();
		fTimeoutAlert->Quit();
	}
	if (fMessageRunner)
		fMessageRunner->SetCount(0);
}


bool
TaskTimer::QuitRequested()
{
	return true;
}


void
TaskTimer::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case TASK_TIMEOUT: {
			fMessageRunner = NULL;
			if (fTimerIsRunning) {
				BString text(B_TRANSLATE_COMMENT("The task for repository"
					" %name% is taking a long time to complete.",
					"Alert message.  Do not translate %name%"));
				BString nameString("\"");
				nameString.Append(fRepositoryName).Append("\"");
				text.ReplaceFirst("%name%", nameString);
				fTimeoutAlert = new BAlert("timeout", text,
					B_TRANSLATE_COMMENT("Keep trying", "Button label"),
					B_TRANSLATE_COMMENT("Cancel task", "Button label"),
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				fTimeoutAlert->SetShortcut(0, B_ESCAPE);
				// Calculate the position to correctly stack this alert
				BRect windowFrame = be_app->WindowAt(0)->Frame();
				int32 stackPos = _NextAlertStackCount();
				float xPos = windowFrame.left
					+ windowFrame.Width()/2 + stackPos * kTimerAlertOffset;
				float yPos = windowFrame.top
					+ (stackPos + 1) * kTimerAlertOffset;
				fTimeoutAlert->Go(&fTimeoutAlertInvoker);
				xPos -= fTimeoutAlert->Frame().Width()/2;
					// The correct frame for the alert is not available until
					// after Go is called
				fTimeoutAlert->MoveTo(xPos, yPos);
			}
			break;
		}
		case TIMEOUT_ALERT_BUTTON_SELECTION: {
			fTimeoutAlert = NULL;
			// Timeout alert was invoked by user and timer still has not
			// been stopped
			if (fTimerIsRunning) {
				// find which button was pressed
				int32 selection = -1;
				message->FindInt32("which", &selection);
				if (selection == 1) {
					BMessage reply(TASK_KILL_REQUEST);
					reply.AddPointer(key_taskptr, fOwner);
					fReplyTarget.SendMessage(&reply);
				} else if (selection == 0) {
					// Create new timer
					fMessageRunner = new BMessageRunner(fMessenger,
						&fTimeoutMessage, kTimerRetrySeconds * 1000000, 1);
				}
			}
			break;
		}
	}
}


void
TaskTimer::Start(const char* name)
{
	fTimerIsRunning = true;
	fRepositoryName.SetTo(name);

	// Create a message runner that will send a TASK_TIMEOUT message if the
	// timer is not stopped
	if (fMessageRunner == NULL)
		fMessageRunner = new BMessageRunner(fMessenger, &fTimeoutMessage,
			fTimeoutMicroSeconds, 1);
	else
		fMessageRunner->SetInterval(fTimeoutMicroSeconds);
}


void
TaskTimer::Stop(const char* name)
{
	fTimerIsRunning = false;

	// Reset max timeout so we can reuse the runner at the next Start call
	if (fMessageRunner != NULL)
		fMessageRunner->SetInterval(LLONG_MAX);

	// If timeout alert is showing replace it
	if (fTimeoutAlert) {
		// Remove current alert
		BRect frame = fTimeoutAlert->Frame();
		fTimeoutAlert->Quit();
		fTimeoutAlert = NULL;

		// Display new alert that won't send a message
		BString text(B_TRANSLATE_COMMENT("Good news! The task for repository "
			"%name% completed.", "Alert message.  Do not translate %name%"));
		BString nameString("\"");
		nameString.Append(name).Append("\"");
		text.ReplaceFirst("%name%", nameString);
		BAlert* newAlert = new BAlert("timeout", text, kOKLabel, NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		newAlert->SetShortcut(0, B_ESCAPE);
		newAlert->MoveTo(frame.left, frame.top);
		newAlert->Go(NULL);
	}
}


int32
TaskTimer::_NextAlertStackCount()
{
	if (sAlertStackCount > 9)
		sAlertStackCount = 0;
	return sAlertStackCount++;
}
