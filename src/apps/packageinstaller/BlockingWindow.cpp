/*
 * Copyright (c) 2007-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "PackageImageViewer.h"


BlockingWindow::BlockingWindow(BRect frame, const char* title, uint32 flags)
	:
	BWindow(frame, title, B_MODAL_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE | flags),
	fSemaphore(-1),
	fReturnValue(0)
{
}


BlockingWindow::~BlockingWindow()
{
}


bool
BlockingWindow::QuitRequested()
{
	ReleaseSem(0);
	return true;
}


int32
BlockingWindow::Go()
{
	int32 returnValue = 0;

	// Since this class can be thought of as a modified BAlert window, no use
	// to reinvent a well fledged wheel. This concept has been borrowed from
	// the current BAlert implementation
	fSemaphore = create_sem(0, "PackageInstaller BlockingWindow");
	if (fSemaphore < B_OK) {
		Quit();
		return returnValue;
	}

	thread_id callingThread = find_thread(NULL);
	BWindow* window = dynamic_cast<BWindow*>(BLooper::LooperForThread(
		callingThread));
	Show();

	if (window != NULL) {
		// Make sure calling window thread, which is blocked here, is updating
		// the window from time to time.
		status_t ret;
		for (;;) {
			do {
				ret = acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, 50000);
			} while (ret == B_INTERRUPTED);

			if (ret == B_BAD_SEM_ID)
				break;
			window->UpdateIfNeeded();
		}
	} else {
		// Since there are no spinlocks, wait until the semaphore is free
		while (acquire_sem(fSemaphore) == B_INTERRUPTED) {
		}
	}

	returnValue = fReturnValue;

	if (Lock())
		Quit();

	return returnValue;
}


void
BlockingWindow::ReleaseSem(int32 returnValue)
{
	if (fSemaphore >= B_OK) {
		delete_sem(fSemaphore);
		fSemaphore = -1;
		fReturnValue = returnValue;
	}
}
