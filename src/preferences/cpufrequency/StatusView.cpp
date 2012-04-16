/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "StatusView.h"

#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Deskbar.h>
#include <GroupLayout.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <TextView.h>

#include "CPUFrequencyView.h"


extern "C" _EXPORT BView *instantiate_deskbar_item(void);


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Status view"
#define MAX_FREQ_STRING "9999MHz"


/* This file is used both by the preference panel and the deskbar replicant.
 * This make it needs two different BCatalogs depending on the context.
 * As this is not really possible with the current way BCatalog are built,
 * we get the catalog by hand. This makes us unable to use the TR macros,
 * and then collectcatkeys will not see the strings in the file.
 * So we mark them explicitly here.
 */
B_TRANSLATE_MARK_VOID("Dynamic performance");
B_TRANSLATE_MARK_VOID("High performance");
B_TRANSLATE_MARK_VOID("Low energy");
B_TRANSLATE_MARK_VOID("Set state");
B_TRANSLATE_MARK_VOID("CPUFrequency\n"
	"\twritten by Clemens Zeidler\n"
	"\tCopyright 2009, Haiku, Inc.\n");
B_TRANSLATE_MARK_VOID("Ok");
B_TRANSLATE_MARK_VOID("Open Speedstep preferences" B_UTF8_ELLIPSIS);
B_TRANSLATE_MARK_VOID("Quit");

// messages FrequencySwitcher
const uint32 kMsgDynamicPolicyPulse = '&dpp';

// messages menu
const uint32 kMsgPolicyDynamic = 'pody';
const uint32 kMsgPolicyPerformance = 'popr';
const uint32 kMsgPolicyLowEnergy = 'pole';
const uint32 kMsgPolicySetState = 'poss';

// messages StatusView
const uint32 kMsgOpenSSPreferences = 'ossp';
const char* kDeskbarItemName = "CPUFreqStatusView";


FrequencySwitcher::FrequencySwitcher(CPUFreqDriverInterface* interface,
		BHandler* target)
	:
	BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE),
	fDriverInterface(interface),
	fTarget(target),
	fMessageRunner(NULL),
	fCurrentFrequency(NULL),
	fDynamicPolicyStarted(false)
{
}


FrequencySwitcher::~FrequencySwitcher()
{
	freq_preferences dummyPref;
	_StartDynamicPolicy(false, dummyPref);
}


filter_result
FrequencySwitcher::Filter(BMessage* message, BHandler** target)
{
	filter_result result = B_DISPATCH_MESSAGE;
	if (message->what == kMsgDynamicPolicyPulse) {
		_CalculateDynamicState();
		result = B_SKIP_MESSAGE;
	}
	return result;
}


void
FrequencySwitcher::SetMode(const freq_preferences& pref)
{
	int16 stateCount = fDriverInterface->GetNumberOfFrequencyStates();
	StateList* list = fDriverInterface->GetCpuFrequencyStates();
	freq_info* currentState = fDriverInterface->GetCurrentFrequencyState();
	freq_info* state = NULL;
	bool isDynamic = false;

	switch (pref.mode) {
		case DYNAMIC:
			isDynamic = true;
			fSteppingThreshold = pref.stepping_threshold;
			if (fMessageRunner && fIntegrationTime != pref.integration_time) {
				fIntegrationTime = pref.integration_time;
				fMessageRunner->SetInterval(fIntegrationTime);
			}
			if (!fDynamicPolicyStarted)
				_StartDynamicPolicy(true, pref);
			break;

		case PERFORMANCE:
			state = list->ItemAt(int32(0));
			if (state != currentState)
				fDriverInterface->SetFrequencyState(state);
			break;

		case LOW_ENERGIE:
			state = list->ItemAt(stateCount - 1);
			if (state != currentState)
				fDriverInterface->SetFrequencyState(state);
			break;

		case CUSTOM:
			if (pref.custom_stepping < stateCount) {
				state = list->ItemAt(pref.custom_stepping);
				fDriverInterface->SetFrequencyState(state);
			}
			break;
	}

	if (!isDynamic && fDynamicPolicyStarted) {
		fDynamicPolicyStarted = false;
		_StartDynamicPolicy(false, pref);
	}
}


void
FrequencySwitcher::_CalculateDynamicState()
{
	system_info sysInfo;
	get_system_info(&sysInfo);
	bigtime_t now = system_time();
	bigtime_t activeTime = sysInfo.cpu_infos[0].active_time;

	// if the dynamic mode is not started first init the prev values
	if (!fDynamicPolicyStarted) {
		fPrevActiveTime = activeTime;
		fPrevTime = now;
		fDynamicPolicyStarted = true;
	} else {
		float usage = (float)(activeTime - fPrevActiveTime )
						/ (now - fPrevTime);
		if (usage >= 1.0)
			usage = 0.9999999;

		int32 numberOfStates = fDriverInterface->GetNumberOfFrequencyStates();
		for (int i = 0; i < numberOfStates; i++) {
			float usageOfStep = ColorStepView::UsageOfStep(i, numberOfStates,
				fSteppingThreshold);

			if (usage < usageOfStep) {
				StateList* list = fDriverInterface->GetCpuFrequencyStates();
				freq_info* newState = list->ItemAt(numberOfStates - 1 - i);
				if (newState != fCurrentFrequency) {
					LOG("change freq\n");
					fDriverInterface->SetFrequencyState(newState);
					fCurrentFrequency = newState;
				}
				break;
			}
		}
		fPrevActiveTime = activeTime;
		fPrevTime = now;
	}
}


void
FrequencySwitcher::_StartDynamicPolicy(bool start, const freq_preferences& pref)
{
	if (start) {
		if (!fMessageRunner) {
			fIntegrationTime = pref.integration_time;
			fMessageRunner = new BMessageRunner(fTarget,
				new BMessage(kMsgDynamicPolicyPulse), pref.integration_time, -1);
			fCurrentFrequency = fDriverInterface->GetCurrentFrequencyState();
		}
	} else {
		delete fMessageRunner;
		fMessageRunner = NULL;
	}
}


//	#pragma mark -


FrequencyMenu::FrequencyMenu(BMenu* menu, BHandler* target,
		PreferencesStorage<freq_preferences>* storage,
		CPUFreqDriverInterface* interface)
	: BMessageFilter(B_PROGRAMMED_DELIVERY, B_LOCAL_SOURCE),
	fTarget(target),
	fStorage(storage),
	fInterface(interface)
{
	fDynamicPerformance = new BMenuItem(
		B_TRANSLATE("Dynamic performance"),
		new BMessage(kMsgPolicyDynamic));
	fHighPerformance = new BMenuItem(
		B_TRANSLATE("High performance"),
		new BMessage(kMsgPolicyPerformance));
	fLowEnergie = new BMenuItem(B_TRANSLATE("Low energy"),
		new BMessage(kMsgPolicyLowEnergy));

	menu->AddItem(fDynamicPerformance);
	menu->AddItem(fHighPerformance);
	menu->AddItem(fLowEnergie);

	fCustomStateMenu = new BMenu(B_TRANSLATE("Set state"));

	StateList* stateList = fInterface->GetCpuFrequencyStates();
	for (int i = 0; i < stateList->CountItems(); i++) {
		freq_info* info = stateList->ItemAt(i);
		BString label;
		label << info->frequency;
		label += " MHz";
		fCustomStateMenu->AddItem(new BMenuItem(label.String(),
			new BMessage(kMsgPolicySetState)));
	}

	menu->AddItem(fCustomStateMenu);

	// set the target of the items
	fDynamicPerformance->SetTarget(fTarget);
	fHighPerformance->SetTarget(fTarget);
	fLowEnergie->SetTarget(fTarget);

	fCustomStateMenu->SetTargetForItems(fTarget);
}


inline void
FrequencyMenu::_SetL1MenuLabelFrom(BMenuItem* item)
{
	BMenuItem* superItem, *markedItem;
	superItem = item->Menu()->Superitem();
	if (superItem)
		superItem->SetLabel(item->Label());
	markedItem = item->Menu()->FindMarked();
	if (markedItem)
		markedItem->SetMarked(false);
	item->SetMarked(true);
}


filter_result
FrequencyMenu::Filter(BMessage* msg, BHandler** target)
{
	filter_result result = B_DISPATCH_MESSAGE;

	BMenuItem* item, *superItem, *markedItem;
	msg->FindPointer("source", (void**)&item);
	if (!item)
		return result;

	bool safeChanges = false;
	freq_preferences* pref = fStorage->GetPreferences();

	switch (msg->what) {
		case kMsgPolicyDynamic:
			pref->mode = DYNAMIC;
			_SetL1MenuLabelFrom(item);
			safeChanges = true;
			msg->what = kUpdatedPreferences;
			break;

		case kMsgPolicyPerformance:
			pref->mode = PERFORMANCE;
			_SetL1MenuLabelFrom(item);
			safeChanges = true;
			msg->what = kUpdatedPreferences;
			break;

		case kMsgPolicyLowEnergy:
			pref->mode = LOW_ENERGIE;
			_SetL1MenuLabelFrom(item);
			safeChanges = true;
			msg->what = kUpdatedPreferences;
			break;

		case kMsgPolicySetState:
			pref->mode = CUSTOM;
			pref->custom_stepping = item->Menu()->IndexOf(item);

			superItem = item->Menu()->Supermenu()->Superitem();
			if (superItem)
				superItem->SetLabel(item->Label());
			markedItem = item->Menu()->Supermenu()->FindMarked();
			if (markedItem)
				markedItem->SetMarked(false);
			markedItem = item->Menu()->FindMarked();
			if (markedItem)
				markedItem->SetMarked(false);
			item->SetMarked(true);

			safeChanges = true;
			msg->what = kUpdatedPreferences;
			break;
	}

	if (safeChanges)
		fStorage->SavePreferences();

	return result;
}


void
FrequencyMenu::UpdateMenu()
{
	freq_preferences* pref = fStorage->GetPreferences();
	switch (pref->mode) {
		case DYNAMIC:
			_SetL1MenuLabelFrom(fDynamicPerformance);
			break;

		case PERFORMANCE:
			_SetL1MenuLabelFrom(fHighPerformance);
			break;

		case LOW_ENERGIE:
			_SetL1MenuLabelFrom(fLowEnergie);
			break;

		case CUSTOM:
		{
			BMenuItem* markedItem = fCustomStateMenu->FindMarked();
			if (markedItem)
				markedItem->SetMarked(false);
			BMenuItem* customItem
				= fCustomStateMenu->ItemAt(pref->custom_stepping);
			if (customItem)
				customItem->SetMarked(true);
			BMenuItem* superItem = fCustomStateMenu->Supermenu()->Superitem();
			if (superItem && customItem)
				superItem->SetLabel(customItem->Label());
			break;
		}
	}
}


//	#pragma mark -


StatusView::StatusView(BRect frame,	bool inDeskbar,
	PreferencesStorage<freq_preferences>* storage)
	:
	BView(frame, kDeskbarItemName, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fInDeskbar(inDeskbar),
	fCurrentFrequency(NULL),
	fDragger(NULL)
{
	if (!inDeskbar) {
		// we were obviously added to a standard window - let's add a dragger
		BRect bounds = Bounds();
		bounds.top = bounds.bottom - 7;
		bounds.left = bounds.right - 7;
		fDragger = new BDragger(bounds, this,
			B_FOLLOW_NONE);
		AddChild(fDragger);
	}

	if (storage) {
		fOwningStorage = false;
		fStorage = storage;
	} else {
		fOwningStorage = true;
		fStorage = new PreferencesStorage<freq_preferences>("CPUFrequency",
			kDefaultPreferences);
	}

	_Init();
}


StatusView::StatusView(BMessage* archive)
	: BView(archive),
	fInDeskbar(false),
	fCurrentFrequency(NULL),
	fDragger(NULL)
{
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK
		&& !strcasecmp(info.signature, "application/x-vnd.Be-TSKB"))
		fInDeskbar = true;

	fOwningStorage = true;
	fStorage = new PreferencesStorage<freq_preferences>(kPreferencesFileName,
		kDefaultPreferences);
	_Init();
}


StatusView::~StatusView()
{
	if (fOwningStorage)
		delete fStorage;
}


void
StatusView::_AboutRequested()
{
	BAlert *alert = new BAlert("about", B_TRANSLATE("CPUFrequency\n"
			"\twritten by Clemens Zeidler\n"
			"\tCopyright 2009, Haiku, Inc.\n"),
		B_TRANSLATE("Ok"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 13, &font);

	alert->Go();
}


void
StatusView::_Quit()
{
	if (fInDeskbar) {
		BDeskbar deskbar;
		deskbar.RemoveItem(kDeskbarItemName);
	} else
		be_app->PostMessage(B_QUIT_REQUESTED);
}


StatusView*
StatusView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "StatusView"))
		return NULL;

	return new StatusView(archive);
}


status_t
StatusView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kPrefSignature);
	if (status == B_OK)
		status = archive->AddString("class", "StatusView");

	return status;
}


void
StatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());

	// watching if the driver change the frequency
	fDriverInterface.StartWatching(this);

	// monitor preferences file
	fPrefFileWatcher = new PrefFileWatcher<freq_preferences>(fStorage, this);
	AddFilter(fPrefFileWatcher);

	// FrequencySwitcher
	fFrequencySwitcher = new FrequencySwitcher(&fDriverInterface, this);
	fFrequencySwitcher->SetMode(*(fStorage->GetPreferences()));
	AddFilter(fFrequencySwitcher);

	// perferences menu
	fPreferencesMenu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	fPreferencesMenuFilter = new FrequencyMenu(fPreferencesMenu, this,
		fStorage, &fDriverInterface);

	fPreferencesMenu->SetFont(be_plain_font);

	fPreferencesMenu->AddSeparatorItem();
	fOpenPrefItem = new BMenuItem(B_TRANSLATE(
			"Open Speedstep preferences" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOpenSSPreferences));
	fPreferencesMenu->AddItem(fOpenPrefItem);
	fOpenPrefItem->SetTarget(this);

	if (fInDeskbar) {
		fQuitItem= new BMenuItem(B_TRANSLATE("Quit"),
			new BMessage(B_QUIT_REQUESTED));
		fPreferencesMenu->AddItem(fQuitItem);
		fQuitItem->SetTarget(this);
	}
	AddFilter(fPreferencesMenuFilter);

	fPreferencesMenuFilter->UpdateMenu();
}


void
StatusView::DetachedFromWindow()
{
	fDriverInterface.StopWatching();

	if (RemoveFilter(fPrefFileWatcher))
		delete fPrefFileWatcher;
	if (RemoveFilter(fFrequencySwitcher))
		delete fFrequencySwitcher;
	if (RemoveFilter(fPreferencesMenuFilter))
		delete fPreferencesMenuFilter;
	delete fPreferencesMenu;
}


void
StatusView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenSSPreferences:
			_OpenPreferences();
			break;

		case kMSGFrequencyChanged:
			message->FindPointer("freq_info", (void**)&fCurrentFrequency);
			_SetupNewFreqString();
			Invalidate();
			break;

		case kUpdatedPreferences:
			fFrequencySwitcher->SetMode(*(fStorage->GetPreferences()));
			fPreferencesMenuFilter->UpdateMenu();
			break;

		case B_ABOUT_REQUESTED:
			_AboutRequested();
			break;

		case B_QUIT_REQUESTED:
			_Quit();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
StatusView::FrameResized(float width, float height)
{
	Invalidate();
}


void
StatusView::Draw(BRect updateRect)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float height = fontHeight.ascent + fontHeight.descent;

	if (be_control_look != NULL)
		be_control_look->DrawLabel(this, fFreqString.String(),
			this->ViewColor(), 0, BPoint(0, height));
	else {
		MovePenTo(0, height);
		DrawString(fFreqString.String());
	}
}




void
StatusView::MouseDown(BPoint point)
{
	if (fShowPopUpMenu) {
		ConvertToScreen(&point);
		fPreferencesMenu->Go(point, true, false, true);
	}
}


void
StatusView::GetPreferredSize(float *width, float *height)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	*height = fontHeight.ascent + fontHeight.descent;
	if (!fInDeskbar)
		*height += 7;

	*width = StringWidth(MAX_FREQ_STRING);
}


void
StatusView::ResizeToPreferred(void)
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}


void
StatusView::ShowPopUpMenu(bool show)
{
	fShowPopUpMenu = show;
}


void
StatusView::UpdateCPUFreqState()
{
	fFrequencySwitcher->SetMode(*(fStorage->GetPreferences()));
}


void
StatusView::_Init()
{
	fShowPopUpMenu = true;
	fCurrentFrequency = fDriverInterface.GetCurrentFrequencyState();

	_SetupNewFreqString();
}


void
StatusView::_SetupNewFreqString()
{
	if (fCurrentFrequency) {
		fFreqString = ColorStepView::CreateFrequencyString(
			fCurrentFrequency->frequency);
	} else
		fFreqString = "? MHz";

	ResizeToPreferred();

	if (fDragger) {
		BRect bounds = Bounds();
		fDragger->MoveTo(bounds.right - 7, bounds.bottom - 7);
	}
}

void
StatusView::_OpenPreferences()
{
	status_t ret = be_roster->Launch(kPrefSignature);
	if (ret == B_ALREADY_RUNNING) {
		app_info info;
		ret = be_roster->GetAppInfo(kPrefSignature, &info);
		if (ret == B_OK)
			ret = be_roster->ActivateApp(info.team);
	}
	if (ret < B_OK) {
		BString errorMessage(B_TRANSLATE(
			"Launching the CPU frequency preflet failed.\n\nError: "));
		errorMessage << strerror(ret);
		BAlert* alert = new BAlert("launch error", errorMessage.String(),
			"Ok");
		// asynchronous alert in order to not block replicant host
		// application
		alert->Go(NULL);
	}
}


//	#pragma mark -


extern "C" _EXPORT BView*
instantiate_deskbar_item(void)
{
	return new StatusView(BRect(0, 0, 15, 15), true, NULL);
}
