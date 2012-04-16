/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef PREFERENCES_WINDOW_h
#define PREFERENCES_WINDOW_h


#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <GroupView.h>
#include <Locale.h>
#include <MessageFilter.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <Window.h>


#if DEBUG
#	define LOG(text...) PRINT((text))
#else
#	define LOG(text...)
#endif


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Pref Window"

// messages PrefFileWatcher
const uint32 kUpdatedPreferences = '&UdP';


template<typename Preferences>
class PreferencesStorage {
public:
								PreferencesStorage(const char* file,
									const Preferences& defaultPreferences);
								~PreferencesStorage();

			void				Revert();
			void				Defaults();

			BPoint 				WindowPosition() {return fWindowPosition; }
			void				SetWindowPosition(BPoint position)
									{ fWindowPosition = position; }

			Preferences*		GetPreferences() { return &fPreferences; }

			status_t			LoadPreferences();
			status_t			SavePreferences();

			BString&			PreferencesFile() {	return fPreferencesFile; }
			status_t			GetPreferencesPath(BPath &path);

			bool				DefaultsSet();
			bool				StartPrefsSet();

private:
			BString				fPreferencesFile;

			Preferences			fPreferences;
			Preferences			fStartPreferences;
			const Preferences&	fDefaultPreferences;
			BPoint				fWindowPosition;
};


template<typename Preferences>
class PrefFileWatcher : public BMessageFilter {
public:
								PrefFileWatcher(
									PreferencesStorage<Preferences>* storage,
									BHandler* target);
	virtual						~PrefFileWatcher();

	virtual filter_result		Filter(BMessage* message, BHandler** _target);

private:
			PreferencesStorage<Preferences>* fPreferencesStorage;
			node_ref			fPreferencesNode;
			node_ref			fPreferencesDirectoryNode;

			BHandler*			fTarget;
};


template<typename Preferences>
class PreferencesWindow : public BWindow,
	public PreferencesStorage<Preferences> {
public:
								PreferencesWindow(const char* title,
									const char* file,
									const Preferences& defaultPreferences);
	virtual						~PreferencesWindow();
	virtual void				MessageReceived(BMessage *msg);
	virtual bool				QuitRequested();

	virtual bool				SetPreferencesView(BView* prefView);

private:
			void				_MoveToPosition();
			void				_UpdateButtons();

			BView*				fPreferencesView;
			BButton*			fRevertButton;
			BButton*			fDefaultButton;
			BGroupLayout*		fRootLayout;
};


const uint32 kDefaultMsg = 'dems';
const uint32 kRevertMsg = 'rems';
const uint32 kConfigChangedMsg = '&cgh';


template<typename Preferences>
PreferencesStorage<Preferences>::PreferencesStorage(const char* file,
	const Preferences& defaultPreferences)
	:
	fDefaultPreferences(defaultPreferences)
{
	// default center position
	fWindowPosition.x = -1;
	fWindowPosition.y = -1;

	fPreferencesFile = file;
	if (LoadPreferences() != B_OK)
		Defaults();
	fStartPreferences = fPreferences;
}


template<typename Preferences>
PreferencesStorage<Preferences>::~PreferencesStorage()
{
	SavePreferences();
}


template<typename Preferences>
void
PreferencesStorage<Preferences>::Revert()
{
	fPreferences = fStartPreferences;
}


template<typename Preferences>
void
PreferencesStorage<Preferences>::Defaults()
{
	fPreferences = fDefaultPreferences;
}


template<typename Preferences>
status_t
PreferencesStorage<Preferences>::GetPreferencesPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	return path.Append(fPreferencesFile.String());
}


template<typename Preferences>
status_t
PreferencesStorage<Preferences>::LoadPreferences()
{
	BPath path;
	status_t status = GetPreferencesPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_ONLY);
	status = settingsFile.InitCheck();
	if (status != B_OK)
		return status;

	if (settingsFile.Read(&fWindowPosition, sizeof(BPoint))
			!= sizeof(BPoint)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	if (settingsFile.Read(&fPreferences, sizeof(Preferences))
			!= sizeof(Preferences)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	return B_OK;
}


template<typename Preferences>
status_t
PreferencesStorage<Preferences>::SavePreferences()
{
	BPath path;
	status_t status = GetPreferencesPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = settingsFile.InitCheck();
	if (status != B_OK) {
		LOG("InitCheck() settings file failed \n");
		return status;
	}

	if (settingsFile.Write(&fWindowPosition, sizeof(BPoint))
			!= sizeof(BPoint)) {
		LOG("can't save window position\n");
		return B_ERROR;
	}

	if (settingsFile.Write(&fPreferences, sizeof(Preferences))
			!= sizeof(Preferences)) {
		LOG("can't save settings\n");
		return B_ERROR;
	}

	return B_OK;
}


template<typename Preferences>
bool
PreferencesStorage<Preferences>::DefaultsSet()
{
	return fPreferences.IsEqual(fDefaultPreferences);
}


template<typename Preferences>
bool
PreferencesStorage<Preferences>::StartPrefsSet()
{
	return fPreferences.IsEqual(fStartPreferences);
}


template<typename Preferences>
PrefFileWatcher<Preferences>::PrefFileWatcher(
	PreferencesStorage<Preferences>* storage, BHandler* target)
	:
	BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE),
	fPreferencesStorage(storage),
	fTarget(target)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);

	BEntry entry(path.Path());
	if (entry.GetNodeRef(&fPreferencesDirectoryNode) == B_OK)
		watch_node(&fPreferencesDirectoryNode, B_WATCH_DIRECTORY, fTarget);

	path.Append(fPreferencesStorage->PreferencesFile().String());
	entry.SetTo(path.Path());

	if (entry.GetNodeRef(&fPreferencesNode) == B_OK)
		watch_node(&fPreferencesNode, B_WATCH_STAT, fTarget);
}


template<typename Preferences>
PrefFileWatcher<Preferences>::~PrefFileWatcher()
{
	stop_watching(fTarget);
}


template<typename Preferences>
filter_result
PrefFileWatcher<Preferences>::Filter(BMessage *msg, BHandler **target)
{
	const char *name;
	ino_t dir = -1;
	filter_result result = B_DISPATCH_MESSAGE;
	int32 opcode;
	BPath path;
	node_ref nref;

	if (msg->what != B_NODE_MONITOR
			|| msg->FindInt32("opcode", &opcode) != B_OK)
		return result;

	switch (opcode) {
		case B_ENTRY_MOVED:
			msg->FindInt64("to directory", dir);
			if (dir != fPreferencesDirectoryNode.node)
				break;
			// supposed to fall through

		case B_ENTRY_CREATED:
			msg->FindString("name", &name);
			fPreferencesStorage->GetPreferencesPath(path);
			if (path.Path() == name) {
				msg->FindInt32("device", &fPreferencesNode.device);
				msg->FindInt64("node", &fPreferencesNode.node);
				watch_node(&fPreferencesNode, B_WATCH_STAT, fTarget);
			}
			fPreferencesStorage->LoadPreferences();
			msg->what = kUpdatedPreferences;
			break;

		case B_ENTRY_REMOVED:
			msg->FindInt32("device", &nref.device);
			msg->FindInt64("node", &nref.node);
			if (fPreferencesNode == nref) {
				// stop all watching
				stop_watching(fTarget);
				// and start watching the directory again
				watch_node(&fPreferencesDirectoryNode, B_WATCH_DIRECTORY,
					fTarget);
				msg->what = kUpdatedPreferences;
			}
			break;

		case B_STAT_CHANGED:
			msg->FindInt32("device", &nref.device);
			msg->FindInt64("node", &nref.node);
			if (fPreferencesNode == nref) {
				fPreferencesStorage->LoadPreferences();
				msg->what = kUpdatedPreferences;
			}
			break;
	}
	return result;
}


template<typename Preferences>
PreferencesWindow<Preferences>::PreferencesWindow(const char* title,
	const char* file, const Preferences& defaultPreferences)
	:
	BWindow(BRect(50, 50, 400, 350), title, B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	PreferencesStorage<Preferences>(file, defaultPreferences),
	fPreferencesView(NULL)
{
	BGroupView* buttonView = new BGroupView(B_HORIZONTAL);
	fDefaultButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kDefaultMsg));

	buttonView->AddChild(fDefaultButton);
	buttonView->GetLayout()->AddItem(
		BSpaceLayoutItem::CreateHorizontalStrut(7));
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kRevertMsg));

	buttonView->AddChild(fRevertButton);
	buttonView->GetLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	_UpdateButtons();

	SetLayout(new BGroupLayout(B_VERTICAL));
	fRootLayout = new BGroupLayout(B_VERTICAL);
	fRootLayout->SetInsets(10, 10, 10, 10);
	fRootLayout->SetSpacing(10);
	BView* rootView = new BView("root view", 0, fRootLayout);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fRootLayout->AddView(buttonView);

	BSize size = fRootLayout->PreferredSize();
	ResizeTo(size.width, size.height);
	_MoveToPosition();
}


template<typename Preferences>
PreferencesWindow<Preferences>::~PreferencesWindow()
{
	PreferencesStorage<Preferences>::SetWindowPosition(Frame().LeftTop());
}


template<typename Preferences>
void
PreferencesWindow<Preferences>::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case kConfigChangedMsg:
			_UpdateButtons();
			break;

		case kDefaultMsg:
			PreferencesStorage<Preferences>::Defaults();
			_UpdateButtons();
			if (fPreferencesView)
				PostMessage(kDefaultMsg, fPreferencesView);
			break;

		case kRevertMsg:
			PreferencesStorage<Preferences>::Revert();
			_UpdateButtons();
			if (fPreferencesView)
				PostMessage(kRevertMsg, fPreferencesView);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


template<typename Preferences>
bool
PreferencesWindow<Preferences>::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


template<typename Preferences>
bool
PreferencesWindow<Preferences>::SetPreferencesView(BView* prefView)
{
	if (fPreferencesView)
		return false;

	fPreferencesView = prefView;
	fRootLayout->AddView(0, fPreferencesView);

	BSize size = fRootLayout->PreferredSize();
	ResizeTo(size.width, size.height);
	_MoveToPosition();

	return true;
}


template<typename Preferences>
void
PreferencesWindow<Preferences>::_MoveToPosition()
{
	BPoint position = PreferencesStorage<Preferences>::WindowPosition();
	// center window on screen if it had a bad position
	if (position.x < 0 && position.y < 0){
		BRect rect = BScreen().Frame();
		BRect windowFrame = Frame();
		position.x = (rect.Width() - windowFrame.Width()) / 2;
		position.y = (rect.Height() - windowFrame.Height()) / 2;
	}
	MoveTo(position);
}


template<typename Preferences>
void
PreferencesWindow<Preferences>::_UpdateButtons()
{
	if (!PreferencesStorage<Preferences>::DefaultsSet())
		fDefaultButton->SetEnabled(true);
	else
		fDefaultButton->SetEnabled(false);
	if (!PreferencesStorage<Preferences>::StartPrefsSet())
		fRevertButton->SetEnabled(true);
	else
		fRevertButton->SetEnabled(false);
}


#undef B_TRANSLATION_CONTEXT

#endif	// PREFERENCES_WINDOW_h
