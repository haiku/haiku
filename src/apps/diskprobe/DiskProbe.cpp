/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "DiskProbe.h"

#include <stdio.h>
#include <string.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutUtils.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <TextView.h>

#include "DataEditor.h"
#include "DataView.h"
#include "FileWindow.h"
#include "AttributeWindow.h"
#include "OpenWindow.h"
#include "FindWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskProbe"


const char* kSignature = "application/x-vnd.Haiku-DiskProbe";

static const uint32 kMsgDiskProbeSettings = 'DPst';
static const uint32 kCascadeOffset = 20;


struct disk_probe_settings {
	BRect	window_frame;
	int32	base_type;
	int32	font_size;
	int32	flags;
};


enum disk_probe_flags {
	kCaseSensitive	= 0x01,
		// this flag alone is R5 DiskProbe settings compatible
	kHexFindMode	= 0x02,
};


class Settings {
public:
								Settings();
								~Settings();

			const BMessage&		Message() const { return fMessage; }
			void				UpdateFrom(BMessage* message);

private:
			status_t			Open(BFile* file, int32 mode);

private:
			BMessage			fMessage;
			bool				fUpdated;
};


class DiskProbe : public BApplication {
public:
								DiskProbe();
	virtual						~DiskProbe();

	virtual	void				ReadyToRun();

	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				MessageReceived(BMessage* message);

	virtual bool				QuitRequested();

private:
			status_t			Probe(BEntry& entry,
									const char* attribute = NULL);

private:
			Settings			fSettings;
			BFilePanel*			fFilePanel;
			BWindow*			fOpenWindow;
			FindWindow*			fFindWindow;
			uint32				fWindowCount;
			BRect				fWindowFrame;
			BMessenger			fFindTarget;
};


//-----------------


Settings::Settings()
	:
	fMessage(kMsgDiskProbeSettings),
	fUpdated(false)
{
	float fontSize = be_plain_font->Size();
	int32 windowWidth = DataView::WidthForFontSize(fontSize) + 20;
		// TODO: make scrollbar width variable

	BScreen screen;
	fMessage.AddRect("window_frame", BLayoutUtils::AlignInFrame(screen.Frame(),
		BSize(windowWidth, windowWidth),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER)));
	fMessage.AddInt32("base_type", kHexBase);
	fMessage.AddFloat("font_size", fontSize);
	fMessage.AddBool("case_sensitive", true);
	fMessage.AddInt8("find_mode", kAsciiMode);

	BFile file;
	if (Open(&file, B_READ_ONLY) != B_OK)
		return;

	// TODO: load/save settings as flattened BMessage - but not yet,
	//		since that will break compatibility with R5's DiskProbe

	disk_probe_settings settings;
	if (file.Read(&settings, sizeof(settings)) == sizeof(settings)) {
#if B_HOST_IS_BENDIAN
		// settings are saved in little endian
		settings.window_frame.left = B_LENDIAN_TO_HOST_FLOAT(
			settings.window_frame.left);
		settings.window_frame.top = B_LENDIAN_TO_HOST_FLOAT(
			settings.window_frame.top);
		settings.window_frame.right = B_LENDIAN_TO_HOST_FLOAT(
			settings.window_frame.right);
		settings.window_frame.bottom = B_LENDIAN_TO_HOST_FLOAT(
			settings.window_frame.bottom);
#endif
		// check if the window frame is on screen at all
		BScreen screen;
		if (screen.Frame().Contains(settings.window_frame.LeftTop())
			&& settings.window_frame.Width() < screen.Frame().Width()
			&& settings.window_frame.Height() < screen.Frame().Height())
			fMessage.ReplaceRect("window_frame", settings.window_frame);

		if (settings.base_type == kHexBase
			|| settings.base_type == kDecimalBase)
			fMessage.ReplaceInt32("base_type",
				B_LENDIAN_TO_HOST_INT32(settings.base_type));
		if (settings.font_size >= 0 && settings.font_size <= 72)
			fMessage.ReplaceFloat("font_size",
				float(B_LENDIAN_TO_HOST_INT32(settings.font_size)));

		fMessage.ReplaceBool("case_sensitive",
			settings.flags & kCaseSensitive);
		fMessage.ReplaceInt8("find_mode",
			settings.flags & kHexFindMode ? kHexMode : kAsciiMode);
	}
}


Settings::~Settings()
{
	// only save the settings if something has changed
	if (!fUpdated)
		return;

	BFile file;
	if (Open(&file, B_CREATE_FILE | B_WRITE_ONLY) != B_OK)
		return;

	disk_probe_settings settings;

	settings.window_frame = fMessage.FindRect("window_frame");
#if B_HOST_IS_BENDIAN
	// settings are saved in little endian
	settings.window_frame.left = B_HOST_TO_LENDIAN_FLOAT(
		settings.window_frame.left);
	settings.window_frame.top = B_HOST_TO_LENDIAN_FLOAT(
		settings.window_frame.top);
	settings.window_frame.right = B_HOST_TO_LENDIAN_FLOAT(
		settings.window_frame.right);
	settings.window_frame.bottom = B_HOST_TO_LENDIAN_FLOAT(
		settings.window_frame.bottom);
#endif

	settings.base_type = B_HOST_TO_LENDIAN_INT32(
		fMessage.FindInt32("base_type"));
	settings.font_size = B_HOST_TO_LENDIAN_INT32(
		int32(fMessage.FindFloat("font_size") + 0.5f));
	settings.flags = B_HOST_TO_LENDIAN_INT32(
		(fMessage.FindBool("case_sensitive") ? kCaseSensitive : 0)
		| (fMessage.FindInt8("find_mode") == kHexMode ? kHexFindMode : 0));

	file.Write(&settings, sizeof(settings));
}


status_t
Settings::Open(BFile* file, int32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("DiskProbe_data");

	return file->SetTo(path.Path(), mode);
}


void
Settings::UpdateFrom(BMessage* message)
{
	BRect frame;
	if (message->FindRect("window_frame", &frame) == B_OK)
		fMessage.ReplaceRect("window_frame", frame);

	int32 baseType;
	if (message->FindInt32("base_type", &baseType) == B_OK)
		fMessage.ReplaceInt32("base_type", baseType);

	float fontSize;
	if (message->FindFloat("font_size", &fontSize) == B_OK)
		fMessage.ReplaceFloat("font_size", fontSize);

	bool caseSensitive;
	if (message->FindBool("case_sensitive", &caseSensitive) == B_OK)
		fMessage.ReplaceBool("case_sensitive", caseSensitive);

	int8 findMode;
	if (message->FindInt8("find_mode", &findMode) == B_OK)
		fMessage.ReplaceInt8("find_mode", findMode);

	fUpdated = true;
}


//	#pragma mark -


DiskProbe::DiskProbe()
	: BApplication(kSignature),
	fOpenWindow(NULL),
	fFindWindow(NULL),
	fWindowCount(0)
{
	fFilePanel = new BFilePanel();
	fWindowFrame = fSettings.Message().FindRect("window_frame");
}


DiskProbe::~DiskProbe()
{
	delete fFilePanel;
}


void
DiskProbe::ReadyToRun()
{
	// are there already windows open?
	if (CountWindows() != 1)
		return;

	// if not, ask the user to open a file
	PostMessage(kMsgOpenOpenWindow);
}


/*!	Opens a window containing the file pointed to by the entry_ref.
	This function will fail if that file doesn't exist or could not
	be opened.
	It will check if there already is a window that probes the
	file in question and will activate it in that case.
	This function must be called with the application looper locked.
*/
status_t
DiskProbe::Probe(BEntry& entry, const char* attribute)
{
	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	ProbeWindow* lastWindow(NULL);

	// Do we already have that window open?
	for (int32 i = CountWindows(); i-- > 0; ) {
		ProbeWindow* window = dynamic_cast<ProbeWindow*>(WindowAt(i));
		if (window == NULL)
			continue;

		if (window->Contains(ref, attribute)) {
			window->Activate(true);
			return B_OK;
		}
		if (lastWindow == NULL)
			lastWindow = window;
	}

	// Does the file really exist?
	if (!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	entry.GetRef(&ref);

	// cascade window
	BRect rect;
	if (lastWindow != NULL)
		rect = lastWindow->Frame();
	else
		rect = fWindowFrame;

	rect.OffsetBy(kCascadeOffset, kCascadeOffset);

	BWindow* window;
	if (attribute != NULL) {
		window = new AttributeWindow(rect, &ref, attribute,
			&fSettings.Message());
	} else
		window = new FileWindow(rect, &ref, &fSettings.Message());

	window->Show();

	// Adjust the cascading... we can only do this after the window was created
	// to adjust to the real size.
	rect.right = window->Frame().right;
	rect.bottom = window->Frame().bottom;

	BScreen screen;
	BRect screenBorder = screen.Frame();

	float left = rect.left;
	if (left + rect.Width() > screenBorder.right)
		left = 7;

	float top = rect.top;
	if (top + rect.Height() > screenBorder.bottom)
		top = 26;

	rect.OffsetTo(BPoint(left, top));
	window->MoveTo(BPoint(left, top));

	fWindowCount++;

	return B_OK;
}


void
DiskProbe::RefsReceived(BMessage* message)
{
	bool traverseLinks = (modifiers() & B_SHIFT_KEY) == 0;

	int32 index = 0;
	entry_ref ref;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		const char* attribute = NULL;
		if (message->FindString("attributes", index - 1, &attribute) == B_OK)
			traverseLinks = false;

		BEntry entry;
		status_t status = entry.SetTo(&ref, traverseLinks);

		if (status == B_OK)
			status = Probe(entry, attribute);

		if (status != B_OK) {
			char buffer[1024];
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE_COMMENT("Could not open \"%s\":\n"
				"%s", "Opening of entry reference buffer for a DiskProbe "
				"request Alert message. The name of entry reference and "
				"error message is shown."),
				ref.name, strerror(status));

			BAlert* alert = new BAlert(B_TRANSLATE("DiskProbe request"),
				buffer, B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
		}
	}
}


void
DiskProbe::ArgvReceived(int32 argc, char** argv)
{
	BMessage* message = CurrentMessage();

	BDirectory currentDirectory;
	if (message)
		currentDirectory.SetTo(message->FindString("cwd"));

	BMessage refs;

	for (int i = 1 ; i < argc ; i++) {
		BPath path;
		if (argv[i][0] == '/')
			path.SetTo(argv[i]);
		else
			path.SetTo(&currentDirectory, argv[i]);

		status_t status;
		entry_ref ref;
		BEntry entry;

		if ((status = entry.SetTo(path.Path(), false)) != B_OK
			|| (status = entry.GetRef(&ref)) != B_OK) {
			fprintf(stderr, B_TRANSLATE("Could not open file \"%s\": %s\n"),
				path.Path(), strerror(status));
			continue;
		}

		refs.AddRef("refs", &ref);
	}

	RefsReceived(&refs);
}


void
DiskProbe::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenOpenWindow:
			if (fOpenWindow == NULL) {
				fOpenWindow = new OpenWindow();
				fOpenWindow->Show();
				fWindowCount++;
			} else
				fOpenWindow->Activate(true);
			break;

		case kMsgOpenWindowClosed:
			fOpenWindow = NULL;
			// supposed to fall through
		case kMsgWindowClosed:
			if (--fWindowCount == 0 && !fFilePanel->IsShowing())
				PostMessage(B_QUIT_REQUESTED);
			break;

		case kMsgSettingsChanged:
			fSettings.UpdateFrom(message);
			break;

		case kMsgFindWindowClosed:
			fFindWindow = NULL;
			break;
		case kMsgFindTarget:
		{
			BMessenger target;
			if (message->FindMessenger("target", &target) != B_OK)
				break;

			if (fFindWindow != NULL && fFindWindow->Lock()) {
				fFindWindow->SetTarget(target);
				fFindWindow->Unlock();
			}
			break;
		}
		case kMsgOpenFindWindow:
		{
			BMessenger target;
			if (message->FindMessenger("target", &target) != B_OK)
				break;

			if (fFindWindow == NULL) {
				// open it!
				fFindWindow = new FindWindow(fWindowFrame.OffsetByCopy(80, 80),
					*message, target, &fSettings.Message());
				fFindWindow->Show();
			} else
				fFindWindow->Activate();
			break;
		}

		case kMsgOpenFilePanel:
			fFilePanel->Show();
			break;
		case B_CANCEL:
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
DiskProbe::QuitRequested()
{
	return true;
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	DiskProbe probe;

	probe.Run();
	return 0;
}
