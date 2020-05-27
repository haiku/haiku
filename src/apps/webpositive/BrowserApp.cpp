/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "BrowserApp.h"

#include <AboutWindow.h>
#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <UrlContext.h>
#include <debugger.h>

#include <stdio.h>

#include "BrowserWindow.h"
#include "BrowsingHistory.h"
#include "DownloadWindow.h"
#include "SettingsMessage.h"
#include "SettingsWindow.h"
#include "ConsoleWindow.h"
#include "CookieWindow.h"
#include "NetworkCookieJar.h"
#include "WebKitInfo.h"
#include "WebPage.h"
#include "WebSettings.h"
#include "WebView.h"
#include "WebViewConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WebPositive"

const char* kApplicationSignature = "application/x-vnd.Haiku-WebPositive";
const char* kApplicationName = B_TRANSLATE_SYSTEM_NAME("WebPositive");
static const uint32 PRELOAD_BROWSING_HISTORY = 'plbh';

#define ENABLE_NATIVE_COOKIES 1


BrowserApp::BrowserApp()
	:
	BApplication(kApplicationSignature),
	fWindowCount(0),
	fLastWindowFrame(50, 50, 950, 750),
	fLaunchRefsMessage(0),
	fInitialized(false),
	fSettings(NULL),
	fCookies(NULL),
	fSession(NULL),
	fContext(NULL),
	fDownloadWindow(NULL),
	fSettingsWindow(NULL),
	fConsoleWindow(NULL),
	fCookieWindow(NULL)
{
#ifdef __i386__
	// First let's check SSE2 is available
	cpuid_info info;
	get_cpuid(&info, 1, 0);

	if ((info.eax_1.features & (1 << 26)) == 0) {
		BAlert alert(B_TRANSLATE("No SSE2 support"), B_TRANSLATE("Your CPU is "
			"too old and does not support the SSE2 extensions, without which "
			"WebPositive cannot run. We recommend installing NetSurf instead."),
			B_TRANSLATE("Darn!"));
		alert.Go();
		exit(-1);
	}
#endif

#if ENABLE_NATIVE_COOKIES
	BString cookieStorePath = kApplicationName;
	cookieStorePath << "/Cookies";
	fCookies = new SettingsMessage(B_USER_SETTINGS_DIRECTORY,
		cookieStorePath.String());
	fContext = new BUrlContext();
	if (fCookies->InitCheck() == B_OK) {
		BMessage cookieArchive = fCookies->GetValue("cookies", cookieArchive);
		fContext->SetCookieJar(BNetworkCookieJar(&cookieArchive));
	}
#endif

	BString sessionStorePath = kApplicationName;
	sessionStorePath << "/Session";
	fSession = new SettingsMessage(B_USER_SETTINGS_DIRECTORY,
		sessionStorePath.String());
}


BrowserApp::~BrowserApp()
{
	delete fLaunchRefsMessage;
	delete fSettings;
	delete fCookies;
	delete fSession;
	delete fContext;
}


void
BrowserApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(kApplicationName,
		kApplicationSignature);

	// create the about window

	const char* authors[] = {
		"Andrea Anzani",
		"Stephan Aßmus",
		"Alexandre Deckner",
		"Adrien Destugues",
		"Rajagopalan Gangadharan",
		"Rene Gollent",
		"Ryan Leavengood",
		"Michael Lotz",
		"Maxime Simon",
		NULL
	};

	BString aboutText("");
	aboutText << "HaikuWebKit " << WebKitInfo::HaikuWebKitVersion();
	aboutText << "\nWebKit " << WebKitInfo::WebKitVersion();

	window->AddCopyright(2007, "Haiku, Inc.");
	window->AddAuthors(authors);
	window->AddExtraInfo(aboutText.String());

	window->Show();
}


void
BrowserApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage message(B_REFS_RECEIVED);
	for (int i = 1; i < argc; i++) {
		if (strcmp("-f", argv[i]) == 0
			|| strcmp("--fullscreen", argv[i]) == 0) {
			message.AddBool("fullscreen", true);
			continue;
		}
		const char* url = argv[i];
		BEntry entry(argv[i], true);
		BPath path;
		if (entry.Exists() && entry.GetPath(&path) == B_OK)
			url = path.Path();
		message.AddString("url", url);
	}
	// Upon program launch, it will buffer a copy of the message, since
	// ArgReceived() is called before ReadyToRun().
	RefsReceived(&message);
}


void
BrowserApp::ReadyToRun()
{
	// Since we will essentially run the GUI...
	set_thread_priority(Thread(), B_DISPLAY_PRIORITY);

	BWebPage::InitializeOnce();
	BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK
		&& path.Append(kApplicationName) == B_OK
		&& create_directory(path.Path(), 0777) == B_OK) {

		BWebSettings::SetPersistentStoragePath(path.Path());
	}

	BString mainSettingsPath(kApplicationName);
	mainSettingsPath << "/Application";
	fSettings = new SettingsMessage(B_USER_SETTINGS_DIRECTORY,
		mainSettingsPath.String());

	fLastWindowFrame = fSettings->GetValue("window frame", fLastWindowFrame);
	BRect defaultDownloadWindowFrame(-10, -10, 365, 265);
	BRect downloadWindowFrame = fSettings->GetValue("downloads window frame",
		defaultDownloadWindowFrame);
	BRect settingsWindowFrame = fSettings->GetValue("settings window frame",
		BRect());
	BRect consoleWindowFrame = fSettings->GetValue("console window frame",
		BRect(50, 50, 400, 300));
	BRect cookieWindowFrame = fSettings->GetValue("cookie window frame",
		BRect(50, 50, 400, 300));
	bool showDownloads = fSettings->GetValue("show downloads", false);

	fDownloadWindow = new DownloadWindow(downloadWindowFrame, showDownloads,
		fSettings);
	if (downloadWindowFrame == defaultDownloadWindowFrame) {
		// Initially put download window in lower right of screen.
		BRect screenFrame = BScreen().Frame();
		BMessage decoratorSettings;
		fDownloadWindow->GetDecoratorSettings(&decoratorSettings);
		float borderWidth = 0;
		if (decoratorSettings.FindFloat("border width", &borderWidth) != B_OK)
			borderWidth = 5;
		fDownloadWindow->MoveTo(screenFrame.Width()
			- fDownloadWindow->Frame().Width() - borderWidth,
			screenFrame.Height() - fDownloadWindow->Frame().Height()
			- borderWidth);
	}
	fSettingsWindow = new SettingsWindow(settingsWindowFrame, fSettings);

	BWebPage::SetDownloadListener(BMessenger(fDownloadWindow));

	fConsoleWindow = new ConsoleWindow(consoleWindowFrame);
	fCookieWindow = new CookieWindow(cookieWindowFrame, fContext->GetCookieJar());

	fInitialized = true;

	int32 pagesCreated = 0;
	bool fullscreen = false;
	if (fLaunchRefsMessage) {
		_RefsReceived(fLaunchRefsMessage, &pagesCreated, &fullscreen);
		delete fLaunchRefsMessage;
		fLaunchRefsMessage = NULL;
	}

	// If no refs led to a new open page, open new session if set
	if (fSession->InitCheck() == B_OK && pagesCreated == 0) {
		const char* kSettingsKeyStartUpPolicy = "start up policy";
		uint32 fStartUpPolicy = fSettings->GetValue(kSettingsKeyStartUpPolicy,
			(uint32)ResumePriorSession);
		if (fStartUpPolicy == StartNewSession) {
			PostMessage(NEW_WINDOW);
		} else {
			// otherwise, restore previous session
			BMessage archivedWindow;
			for (int i = 0; fSession->FindMessage("window", i, &archivedWindow)
				== B_OK; i++) {
				BRect frame = archivedWindow.FindRect("window frame");
				BString url;
				archivedWindow.FindString("tab", 0, &url);
				BrowserWindow* window = new(std::nothrow) BrowserWindow(frame,
					fSettings, url, fContext);

				if (window != NULL) {
					window->Show();
					pagesCreated++;

					for (int j = 1; archivedWindow.FindString("tab", j, &url)
						== B_OK; j++) {
						printf("Create %d:%d\n", i, j);
						_CreateNewTab(window, url, false);
						pagesCreated++;
					}
				}
			}
		}
	}

	// If previous session did not contain any window, create a new empty one.
	if (pagesCreated == 0)
		_CreateNewWindow("", fullscreen);

	PostMessage(PRELOAD_BROWSING_HISTORY);
}


void
BrowserApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case PRELOAD_BROWSING_HISTORY:
		// Accessing the default instance will load the history from disk.
		BrowsingHistory::DefaultInstance();
		break;
	case B_SILENT_RELAUNCH:
		_CreateNewPage("");
		break;
	case NEW_WINDOW: {
		BString url;
		if (message->FindString("url", &url) != B_OK)
			break;
		_CreateNewWindow(url);
		break;
	}
	case NEW_TAB: {
		BrowserWindow* window;
		if (message->FindPointer("window",
			reinterpret_cast<void**>(&window)) != B_OK)
			break;
		BString url;
		message->FindString("url", &url);
		bool select = false;
		message->FindBool("select", &select);
		_CreateNewTab(window, url, select);
		break;
	}
	case WINDOW_OPENED:
		fWindowCount++;
		fDownloadWindow->SetMinimizeOnClose(false);
		break;
	case WINDOW_CLOSED:
		fWindowCount--;
		message->FindRect("window frame", &fLastWindowFrame);
		if (fWindowCount <= 0) {
			BMessage* message = new BMessage(B_QUIT_REQUESTED);
			message->AddMessage("window", DetachCurrentMessage());
			PostMessage(message);
		}
		break;

	case SHOW_DOWNLOAD_WINDOW:
		_ShowWindow(message, fDownloadWindow);
		break;
	case SHOW_SETTINGS_WINDOW:
		_ShowWindow(message, fSettingsWindow);
		break;
	case SHOW_CONSOLE_WINDOW:
		_ShowWindow(message, fConsoleWindow);
		break;
	case SHOW_COOKIE_WINDOW:
		_ShowWindow(message, fCookieWindow);
		break;
	case ADD_CONSOLE_MESSAGE:
		fConsoleWindow->PostMessage(message);
		break;

	default:
		BApplication::MessageReceived(message);
		break;
	}
}


void
BrowserApp::RefsReceived(BMessage* message)
{
	if (!fInitialized) {
		delete fLaunchRefsMessage;
		fLaunchRefsMessage = new BMessage(*message);
		return;
	}

	_RefsReceived(message);
}


bool
BrowserApp::QuitRequested()
{
	if (fDownloadWindow->DownloadsInProgress()) {
		BAlert* alert = new BAlert(B_TRANSLATE("Downloads in progress"),
			B_TRANSLATE("There are still downloads in progress, do you really "
			"want to quit WebPositive now?"), B_TRANSLATE("Quit"),
			B_TRANSLATE("Continue downloads"));
		int32 choice = alert->Go();
		if (choice == 1) {
			if (fWindowCount == 0) {
				if (fDownloadWindow->Lock()) {
					fDownloadWindow->SetWorkspaces(1 << current_workspace());
					if (fDownloadWindow->IsHidden())
						fDownloadWindow->Show();
					else
						fDownloadWindow->Activate();
					fDownloadWindow->SetMinimizeOnClose(true);
					fDownloadWindow->Unlock();
					return false;
				}
			} else
				return false;
		}
	}

	fSession->MakeEmpty();

	/* See if we got here because the last window is already closed.
	 * In that case we only need to save that one, which is already archived */
	BMessage* message = CurrentMessage();
	BMessage windowMessage;

	status_t ret = message->FindMessage("window", &windowMessage);
	if (ret == B_OK) {
		fSession->AddMessage("window", &windowMessage);
	} else {
		for (int i = 0; BWindow* window = WindowAt(i); i++) {
			BrowserWindow* webWindow = dynamic_cast<BrowserWindow*>(window);
			if (!webWindow)
				continue;
			if (!webWindow->Lock())
				continue;

			BMessage windowArchive;
			webWindow->Archive(&windowArchive, true);
			fSession->AddMessage("window", &windowArchive);

			if (webWindow->QuitRequested()) {
				fLastWindowFrame = webWindow->WindowFrame();
				webWindow->Quit();
				i--;
			} else {
				webWindow->Unlock();
				return false;
			}
		}
	}

	BWebPage::ShutdownOnce();

	fSettings->SetValue("window frame", fLastWindowFrame);
	if (fDownloadWindow->Lock()) {
		fSettings->SetValue("downloads window frame", fDownloadWindow->Frame());
		fSettings->SetValue("show downloads", !fDownloadWindow->IsHidden());
		fDownloadWindow->Unlock();
	}
	if (fSettingsWindow->Lock()) {
		fSettings->SetValue("settings window frame", fSettingsWindow->Frame());
		fSettingsWindow->Unlock();
	}
	if (fConsoleWindow->Lock()) {
		fSettings->SetValue("console window frame", fConsoleWindow->Frame());
		fConsoleWindow->Unlock();
	}
	if (fCookieWindow->Lock()) {
		fSettings->SetValue("cookie window frame", fCookieWindow->Frame());
		fCookieWindow->Unlock();
	}

	BMessage cookieArchive;
	BNetworkCookieJar& cookieJar = fContext->GetCookieJar();
	cookieJar.PurgeForExit();
	if (cookieJar.Archive(&cookieArchive) == B_OK)
		fCookies->SetValue("cookies", cookieArchive);

	return true;
}


void
BrowserApp::_RefsReceived(BMessage* message, int32* _pagesCreated,
	bool* _fullscreen)
{
	int32 pagesCreated = 0;

	BrowserWindow* window = NULL;
	if (message->FindPointer("window", (void**)&window) != B_OK)
		window = NULL;

	bool fullscreen;
	if (message->FindBool("fullscreen", &fullscreen) != B_OK)
		fullscreen = false;

	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++) {
		BEntry entry(&ref, true);
		if (!entry.Exists())
			continue;
		BPath path;
		if (entry.GetPath(&path) != B_OK)
			continue;
		BUrl url(path);
		window = _CreateNewPage(url.UrlString(), window, fullscreen,
			pagesCreated == 0);
		pagesCreated++;
	}

	BString url;
	for (int32 i = 0; message->FindString("url", i, &url) == B_OK; i++) {
		window = _CreateNewPage(url, window, fullscreen, pagesCreated == 0);
		pagesCreated++;
	}

	if (_pagesCreated != NULL)
		*_pagesCreated = pagesCreated;
	if (_fullscreen != NULL)
		*_fullscreen = fullscreen;
}


BrowserWindow*
BrowserApp::_CreateNewPage(const BString& url, BrowserWindow* webWindow,
	bool fullscreen, bool useBlankTab)
{
	// Let's first see if we must target a specific window...
	if (webWindow && webWindow->Lock()) {
		if (useBlankTab && webWindow->IsBlankTab()) {
			if (url.Length() != 0)
				webWindow->CurrentWebView()->LoadURL(url);
		} else
			webWindow->CreateNewTab(url, true);
		webWindow->Activate();
		webWindow->CurrentWebView()->MakeFocus(true);
		webWindow->Unlock();
		return webWindow;
	}

	// Otherwise, try to find one in the current workspace
	uint32 workspace = 1 << current_workspace();

	bool loadedInWindowOnCurrentWorkspace = false;
	for (int i = 0; BWindow* window = WindowAt(i); i++) {
		webWindow = dynamic_cast<BrowserWindow*>(window);
		if (!webWindow)
			continue;

		if (webWindow->Lock()) {
			if (webWindow->Workspaces() & workspace) {
				if (useBlankTab && webWindow->IsBlankTab()) {
					if (url.Length() != 0)
						webWindow->CurrentWebView()->LoadURL(url);
				} else
					webWindow->CreateNewTab(url, true);
				webWindow->Activate();
				webWindow->CurrentWebView()->MakeFocus(true);
				loadedInWindowOnCurrentWorkspace = true;
			}
			webWindow->Unlock();
		}
		if (loadedInWindowOnCurrentWorkspace)
			return webWindow;
	}

	// Finally, if no window is available, let's create one.
	return _CreateNewWindow(url, fullscreen);
}


BrowserWindow*
BrowserApp::_CreateNewWindow(const BString& url, bool fullscreen)
{
	// Offset the window frame unless this is the first window created in the
	// session.
	if (fWindowCount > 0)
		fLastWindowFrame.OffsetBy(20, 20);
	if (!BScreen().Frame().Contains(fLastWindowFrame))
		fLastWindowFrame.OffsetTo(50, 50);

	BrowserWindow* window = new BrowserWindow(fLastWindowFrame, fSettings,
		url, fContext);
	if (fullscreen)
		window->ToggleFullscreen();
	window->Show();
	return window;
}


void
BrowserApp::_CreateNewTab(BrowserWindow* window, const BString& url,
	bool select)
{
	if (!window->Lock())
		return;
	window->CreateNewTab(url, select);
	window->Unlock();
}


void
BrowserApp::_ShowWindow(const BMessage* message, BWindow* window)
{
	BAutolock _(window);
	uint32 workspaces;
	if (message->FindUInt32("workspaces", &workspaces) == B_OK)
		window->SetWorkspaces(workspaces);
	if (window->IsHidden())
		window->Show();
	else
		window->Activate();
}


// #pragma mark -


int
main(int, char**)
{
	try {
		new BrowserApp();
		be_app->Run();
		delete be_app;
	} catch (...) {
		debugger("Exception caught.");
	}

	return 0;
}

