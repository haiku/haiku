//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// ProtocolsAddon saves the loaded settings.
// ProtocolsView saves the current settings.
//-----------------------------------------------------------------------

#include "ProtocolsAddon.h"

#include "InterfaceUtils.h"
#include "MessageDriverSettingsUtils.h"

#include <Button.h>
#include <ListView.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringItem.h>

#include <PPPDefs.h>


#define MSG_ADD_PROTOCOL			'ADDP'
#define MSG_REMOVE_PROTOCOL			'REMP'
#define MSG_SHOW_PREFERENCES		'SHOW'
#define MSG_UPDATE_BUTTONS			'UBTN'


#define PROTOCOLS_TAB_PROTOCOLS		"Protocols"


ProtocolsAddon::ProtocolsAddon(BMessage *addons)
	: DialUpAddon(addons),
	fProtocolsCount(0),
	fSettings(NULL),
	fProfile(NULL),
	fProtocolsView(NULL)
{
}


ProtocolsAddon::~ProtocolsAddon()
{
}


bool
ProtocolsAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fProtocolsCount = 0;
	fSettings = settings;
	fProfile = profile;
	
	if(fProtocolsView)
		fProtocolsView->Reload();
			// reset all views (empty settings)
	
	if(!settings || !profile || isNew)
		return true;
	
	// ask protocols to load their settings
	BMessage parameter;
	for(int32 index = 0; FindMessageParameter(PPP_PROTOCOL_KEY, *fProfile, parameter,
			&index); index++)
		if(!LoadProtocolSettings(parameter))
			return false;
				// error: some protocol did not accept its settings
	
	if(fProtocolsView)
		fProtocolsView->Reload();
			// reload new settings
	
	return true;
}


bool
ProtocolsAddon::LoadProtocolSettings(const BMessage& parameter)
{
	// get protocol and ask it to load its settings
	BString name;
	if(parameter.FindString(MDSU_VALUES, &name) == B_OK) {
		DialUpAddon *protocol;
		if(!GetProtocol(name, &protocol))
			return false;
				// fatal error: we do not know how to handle this protocol
		
		if(!protocol->LoadSettings(fSettings, fProfile, false))
			return false;
				// error: protocol did not accept its settings
		
		fSettings->AddPointer(PROTOCOLS_TAB_PROTOCOLS, protocol);
		++fProtocolsCount;
	}
	
	return true;
}


bool
ProtocolsAddon::HasTemporaryProfile() const
{
	if(!fProtocolsView)
		return false;
	
	DialUpAddon *protocol;
	for(int32 index = 0; index < fProtocolsView->CountProtocols(); index++) {
		protocol = fProtocolsView->ProtocolAt(index);
		if(protocol && protocol->HasTemporaryProfile())
			return true;
	}
	
	return false;
}


void
ProtocolsAddon::IsModified(bool& settings, bool& profile) const
{
	settings = profile = false;
	
	if(!fSettings || !fProtocolsView)
		return;
	
	if(CountProtocols() != fProtocolsView->CountProtocols()) {
		settings = profile = true;
		return;
	}
	
	DialUpAddon *protocol;
	bool protocolSettingsChanged, protocolProfileChanged;
		// for current protocol
	for(int32 index = 0; fSettings->FindPointer(PROTOCOLS_TAB_PROTOCOLS, index,
			reinterpret_cast<void**>(&protocol)) == B_OK; index++) {
		if(!protocol->KernelModuleName() // this is actually an error
				|| !fProtocolsView->HasProtocol(protocol->KernelModuleName())) {
			settings = profile = true;
			return;
		}
		
		protocol->IsModified(protocolSettingsChanged, protocolProfileChanged);
		if(protocolSettingsChanged)
			settings = true;
		if(protocolProfileChanged)
			profile = true;
	}
}


bool
ProtocolsAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;
	
	DialUpAddon *protocol;
	for(int32 index = 0; index < fProtocolsView->CountProtocols(); index++) {
		protocol = fProtocolsView->ProtocolAt(index);
		if(!protocol->SaveSettings(settings, profile, saveTemporary))
			return false;
	}
	
	return true;
}


bool
ProtocolsAddon::GetPreferredSize(float *width, float *height) const
{
	BRect rect;
	if(Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect) != B_OK)
		rect.Set(0, 0, 200, 300);
			// set default values
	
	if(width)
		*width = rect.Width();
	if(height)
		*height = rect.Height();
	
	return true;
}


BView*
ProtocolsAddon::CreateView(BPoint leftTop)
{
	if(!fProtocolsView) {
		BRect rect;
		Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect);
		fProtocolsView = new ProtocolsView(this, rect);
	}
	
	fProtocolsView->MoveTo(leftTop);
	fProtocolsView->Reload();
	return fProtocolsView;
}


bool
ProtocolsAddon::GetProtocol(const BString& moduleName, DialUpAddon **protocol) const
{
	if(!protocol)
		return false;
	
	for(int32 index = 0; Addons()->FindPointer(DUN_PROTOCOL_ADDON_TYPE, index,
			reinterpret_cast<void**>(protocol)) == B_OK; index++)
		if((*protocol)->KernelModuleName()
				&& moduleName == (*protocol)->KernelModuleName())
			return true;
	
	return false;
}


// we need a simple BListItem that can hold a pointer to the DialUpAddon it represents
class AddonItem : public BStringItem {
	public:
		AddonItem(const char *label, DialUpAddon *addon) : BStringItem(label),
			fAddon(addon) {}
		
		DialUpAddon *Addon() const
			{ return fAddon; }

	private:
		DialUpAddon *fAddon;
};


ProtocolsView::ProtocolsView(ProtocolsAddon *addon, BRect frame)
	: BView(frame, "Protocols", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(10, 10);
	rect.bottom = 200;
	BRect listViewRect(rect);
	listViewRect.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new BListView(listViewRect, "Protocols");
	fListView->SetInvocationMessage(new BMessage(MSG_SHOW_PREFERENCES));
	fListView->SetSelectionMessage(new BMessage(MSG_UPDATE_BUTTONS));
	
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	float buttonWidth = (rect.Width() - 20) / 3;
	rect.right = rect.left + buttonWidth;
	fAddButton = new BButton(rect, "AddButton", "Add Protocol",
		new BMessage(MSG_ADD_PROTOCOL));
	rect.left = rect.right + 10;
	rect.right = rect.left + buttonWidth;
	fRemoveButton = new BButton(rect, "RemoveButton", "Remove",
		new BMessage(MSG_REMOVE_PROTOCOL));
	rect.left = rect.right + 10;
	rect.right = rect.left + buttonWidth;
	fPreferencesButton = new BButton(rect, "PreferencesButton", "Preferences",
		new BMessage(MSG_SHOW_PREFERENCES));
	
	AddChild(new BScrollView("ScrollView", fListView, B_FOLLOW_NONE, 0, false, true));
	AddChild(fAddButton);
	AddChild(fRemoveButton);
	AddChild(fPreferencesButton);
	
	fProtocolsMenu = new BPopUpMenu("UnregisteredProtocols", false, false);
	AddAddonsToMenu(Addon()->Addons(), fProtocolsMenu, DUN_PROTOCOL_ADDON_TYPE, 0);
}


ProtocolsView::~ProtocolsView()
{
	delete fProtocolsMenu;
	
	AddonItem *item;
	while(fListView->CountItems() > 0) {
		item = dynamic_cast<AddonItem*>(fListView->RemoveItem((int32) 0));
		delete item;
	}
}


void
ProtocolsView::Reload()
{
	// move all protocols back to the menu
	while(CountProtocols() > 0)
		UnregisterProtocol(0);
	
	if(!Addon()->Settings())
		return;
	
	// move all registered protocols to the list
	DialUpAddon *protocol;
	for(int32 index = 0; Addon()->Settings()->FindPointer(PROTOCOLS_TAB_PROTOCOLS,
			index, reinterpret_cast<void**>(&protocol)) == B_OK; index++)
		RegisterProtocol(protocol, false);
	
	fListView->Select(0);
		// XXX: unfortunately, this does not work when the BListView is detached
	
	UpdateButtons();
}


void
ProtocolsView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fListView->SetTarget(this);
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	fPreferencesButton->SetTarget(this);
	
	// XXX: a workaround for the bug in BListView that causes Select() only to work
	// when it is attached to a window
	if(fListView->CurrentSelection() < 0)
		fListView->Select(0);
}


void
ProtocolsView::DetachedFromWindow()
{
	// XXX: Is this a bug in BeOS? While BListView is detached the index for the
	// currently selected item does not get updated when it is removed.
	// Workaround: call DeselectAll() before it gets detached
	fListView->DeselectAll();
}


void
ProtocolsView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_ADD_PROTOCOL: {
			BMenuItem *selected = fProtocolsMenu->Go(fAddButton->ConvertToScreen(
				fAddButton->Bounds().RightTop()), false, true);
			
			int32 index = RegisterProtocol(fProtocolsMenu->IndexOf(selected));
			UpdateButtons();
			
			if(index > 0)
				fListView->Select(index);
		} break;
		
		case MSG_REMOVE_PROTOCOL: {
			UnregisterProtocol(fListView->CurrentSelection());
			UpdateButtons();
			
			fListView->Select(0);
		} break;
		
		case MSG_SHOW_PREFERENCES: {
			AddonItem *selected = dynamic_cast<AddonItem*>(
				fListView->ItemAt(fListView->CurrentSelection()));
			
			if(selected) {
				DialUpAddon *addon = selected->Addon();
				addon->CreateView(ConvertToScreen(Bounds().LeftTop()));
					// show the preferences window
			}
		} break;
		
		case MSG_UPDATE_BUTTONS:
			UpdateButtons();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


DialUpAddon*
ProtocolsView::ProtocolAt(int32 index) const
{
	AddonItem *item = dynamic_cast<AddonItem*>(fListView->ItemAt(index));
	if(item)
		return item->Addon();
	
	return NULL;
}


bool
ProtocolsView::HasProtocol(const BString& moduleName) const
{
	AddonItem *item;
	for(int32 index = 0; index < CountProtocols(); index++) {
		item = dynamic_cast<AddonItem*>(fListView->ItemAt(index));
		if(item && moduleName == item->Addon()->KernelModuleName())
			return true;
	}
	
	return false;
}


int32
ProtocolsView::RegisterProtocol(const DialUpAddon *protocol, bool reload = true)
{
	if(!protocol)
		return -1;
	
	DialUpAddon *addon;
	BMenuItem *item;
	for(int32 index = 0; index < fProtocolsMenu->CountItems(); index++) {
		item = fProtocolsMenu->ItemAt(index);
		if(item && item->Message()->FindPointer("Addon",
				reinterpret_cast<void**>(&addon)) == B_OK && addon == protocol)
			return RegisterProtocol(index, reload);
	}
	
	return -1;
}


int32
ProtocolsView::RegisterProtocol(int32 index, bool reload = true)
{
	DialUpAddon *addon;
	BMenuItem *remove = fProtocolsMenu->ItemAt(index);
	if(!remove || remove->Message()->FindPointer("Addon",
			reinterpret_cast<void**>(&addon)) != B_OK)
		return -1;
	
	const char *label = remove->Label();
	AddonItem *item = new AddonItem(label, addon);
	
	index = FindNextListInsertionIndex(fListView, label);
	fListView->AddItem(item, index);
	fProtocolsMenu->RemoveItem(remove);
	delete remove;
	
	addon->LoadSettings(Addon()->Settings(), Addon()->Profile(), reload);
	
	return index;
}


void
ProtocolsView::UnregisterProtocol(int32 index)
{
	AddonItem *remove = dynamic_cast<AddonItem*>(fListView->RemoveItem(index));
	if(!remove)
		return;
	
	const char *label = remove->Text();
	BMessage *message = new BMessage(MSG_ADD_PROTOCOL);
		// the 'what' field is merely set to get around the compiler warning
	message->AddPointer("Addon", remove->Addon());
	BMenuItem *item = new BMenuItem(label, message);
	
	index = FindNextMenuInsertionIndex(fProtocolsMenu, label);
	fProtocolsMenu->AddItem(item, index);
	delete remove;
}


void
ProtocolsView::UpdateButtons()
{
	AddonItem *item = dynamic_cast<AddonItem*>(fListView->ItemAt(
		fListView->CurrentSelection()));
	
	if(fProtocolsMenu->CountItems() == 0)
		fAddButton->SetEnabled(false);
	else
		fAddButton->SetEnabled(true);
	
	if(!item)
		fRemoveButton->SetEnabled(false);
	else
		fRemoveButton->SetEnabled(true);
	
	float width, height;
	if(!item || !item->Addon()->GetPreferredSize(&width, &height))
		fPreferencesButton->SetEnabled(false);
	else
		fPreferencesButton->SetEnabled(true);
}
