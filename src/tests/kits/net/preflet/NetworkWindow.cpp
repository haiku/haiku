#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app/Application.h>
#include <app/Roster.h>
#include <InterfaceKit.h>
#include <StorageKit.h>
#include <SupportKit.h>

#include <BoneyardAddOn.h>

#include "NetworkWindow.h"

// --------------------------------------------------------------
NetworkWindow::NetworkWindow(const char *title)
	: BWindow(BRect(100, 100, 600, 600), title, B_TITLED_WINDOW,
				B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BMenu 		*menu;
	BBox 		*box, *bb;
	BButton		*button;
	BRect		r;
	float		w, h, divider;
	// float		min_w, min_h;

#define H_MARGIN	12
#define V_MARGIN	12
#define LINE_MARGIN	4

	divider = MAX(be_bold_font->StringWidth(PROFILE_LABEL "#"),
				  be_bold_font->StringWidth(SHOW_LABEL "#"));

	box = new BBox(Bounds(), NULL, B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(box); 

	r = box->Bounds();
	r.InsetBy(H_MARGIN, V_MARGIN);

	// ---- Profiles section
	menu = new BPopUpMenu("<none>");
	fProfilesMenu = new BMenuField(r, "profiles_menu", PROFILE_LABEL, menu); 

	fProfilesMenu->SetFont(be_bold_font);
	fProfilesMenu->SetDivider(divider);
	box->AddChild(fProfilesMenu);
	fProfilesMenu->ResizeToPreferred();
	fProfilesMenu->GetPreferredSize(&w, &h);

	BuildProfilesMenu(menu, SELECT_PROFILE_MSG);

	button = new BButton(r, "apply_now", APPLY_NOW_LABEL,
					new BMessage(APPLY_NOW_MSG),
					B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	button->GetPreferredSize(&w, &h);
	button->ResizeToPreferred();
	button->MoveBy(r.right - w - H_MARGIN, 0);
	box->AddChild(button);

	fApplyButton = button;
	r.top = button->Frame().bottom + V_MARGIN;

	// ---- Show settings section
	bb = new BBox(r, "show_box", B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_FANCY_BORDER);

	box->AddChild(bb);
	
	menu = new BPopUpMenu("<nothing>");
	
	fShowMenu = new BMenuField(r, "show_menu", SHOW_LABEL, menu,
									B_FOLLOW_TOP | B_FOLLOW_LEFT,
									B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	// fShowMenu->SetFont(be_bold_font);
	// fShowMenu->SetDivider(be_bold_font->StringWidth(SHOW_LABEL "#"));
	fShowMenu->SetDivider(be_plain_font->StringWidth(SHOW_LABEL "#"));
	bb->SetLabel(fShowMenu);
	fShowMenu->ResizeToPreferred();
	fShowMenu->GetPreferredSize(&w, &h);

	BuildShowMenu(menu, SHOW_MSG);

	r = bb->Bounds();
	r.top += h / 2;
	r.InsetBy(H_MARGIN, V_MARGIN);

	fPanel = new BBox(r, "showview_box", B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_NO_BORDER);
	bb->AddChild(fPanel);

	fShowRect = fPanel->Bounds();
	fShowView = NULL;

}


// --------------------------------------------------------------
NetworkWindow::~NetworkWindow()
{
}



// --------------------------------------------------------------
bool NetworkWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// --------------------------------------------------------------
void NetworkWindow::MessageReceived
	(
	BMessage *	msg
	)
{
	switch (msg->what) {
	case NEW_PROFILE_MSG:
		break;
		
	case RENAME_PROFILE_MSG:
		break;
		
	case DELETE_PROFILE_MSG: {
		BString text;
		
		text << "Are you sure you want to remove '";
		text << fProfilesMenu->MenuItem()->Label();
		text << "' configuration?";  
		BAlert *alert = new BAlert("Hum...", text.String() ,"Cancel", "Delete",
										NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go();
		break;
	}
	
	case SELECT_PROFILE_MSG: {
		BPath name;
		const char *path;
		bool is_default;
		BMenuItem *item;
		BMenu *menu;
		
		if (msg->FindString("path", &path) != B_OK)
			break;
			
		name.SetTo(path);

		is_default = (strcmp(name.Leaf(), "default") == 0);

		menu = fProfilesMenu->Menu();
		item = menu->FindItem(RENAME_PROFILE_LABEL);
		if (item)
			item->SetEnabled(!is_default);
		item = menu->FindItem(DELETE_PROFILE_LABEL);
		if (item)
			item->SetEnabled(!is_default);
		break;
	}
	
	case SHOW_MSG: {
		BYAddon *by;

		if (fShowView)
			fShowView->RemoveSelf();
		
		fShowView = NULL;
		
		if (msg->FindPointer("byaddon", (void **) &by) != B_OK)
			break;
		
		fShowRect = fPanel->Bounds();
		fShowView = by->CreateView(&fShowRect);
		if (fShowView) {
			fPanel->AddChild(fShowView);
			// fShowView->SetViewColor((rand() % 256), (rand() % 256), (rand() % 256));
			fShowView->ResizeTo(fPanel->Bounds().Width(), fPanel->Bounds().Height());
		};
		
		break;
	}

	default:
		inherited::MessageReceived(msg);
	};
}


// --------------------------------------------------------------
void NetworkWindow::BuildProfilesMenu
	(
	BMenu *menu,
	int32 msg_what
	)
{
	BMenuItem *	item;
	char current_profile[256] = { 0 };
	
	menu->SetRadioMode(true);

	BDirectory dir("/etc/network/profiles");

	if (dir.InitCheck() == B_OK) {
		BEntry entry;
		BMessage *msg;

		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			BPath name;
			entry.GetPath(&name);

			if (entry.IsSymLink() &&
				strcmp("current", name.Leaf()) == 0) {
				BSymLink symlink(&entry);
			
				if (symlink.IsAbsolute())
					// oh oh, sorry, wrong symlink...
					continue;
				
				symlink.ReadLink(current_profile, sizeof(current_profile));
				continue;	
			};

			if (!entry.IsDirectory())
				continue;

			msg = new BMessage(msg_what);
			msg->AddString("path", name.Path());
			
			item = new BMenuItem(name.Leaf(), msg);
			menu->AddItem(item);
		};
	};
	
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(NEW_PROFILE_LABEL, new BMessage(NEW_PROFILE_MSG)));
	menu->AddItem(new BMenuItem(RENAME_PROFILE_LABEL, new BMessage(RENAME_PROFILE_MSG)));
	menu->AddItem(new BMenuItem(DELETE_PROFILE_LABEL, new BMessage(DELETE_PROFILE_MSG)));

	if (strlen(current_profile)) {
		item = menu->FindItem(current_profile);
		if (item) {
			BString label;
			bool is_default = (strcmp(current_profile, "default") == 0);

			label << item->Label();
			label << " (current)";
			item->SetLabel(label.String());
			item->SetMarked(true);

			item = menu->FindItem(RENAME_PROFILE_LABEL);
			if (item)
				item->SetEnabled(!is_default);
			item = menu->FindItem(DELETE_PROFILE_LABEL);
			if (item)
				item->SetEnabled(!is_default);
		};
		
	};

}

// --------------------------------------------------------------
void NetworkWindow::BuildShowMenu
	(
	BMenu *menu,
	int32 msg_what
	)
{
	menu->SetRadioMode(true);
/*
	menu->AddItem(item = new BMenuItem("vt86c100/0 interface", 0));
	item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("rtl8136/0 interface", 0));
	menu->AddItem(item = new BMenuItem("Dialup", 0));
	menu->AddItem(item = new BMenuItem("Identity", 0));
	menu->AddItem(item = new BMenuItem("Services", 0));
*/
	
	BPath path;
	BPath addon_path;
	BDirectory dir;
	BEntry entry;
	char * search_paths;
	char * search_path;
	char * next_path_token; 

	search_paths = getenv("ADDON_PATH");
	if (!search_paths)
		// Nowhere to search addons!!!
		return;
		
	search_paths = strdup(search_paths);
	search_path = strtok_r(search_paths, ":", &next_path_token);
	
	while (search_path) {
		if (strncmp(search_path, "%A/", 3) == 0) {
			// compute "%A/..." path
			app_info ai;
			
			be_app->GetAppInfo(&ai);
			entry.SetTo(&ai.ref);
			entry.GetPath(&path);
			path.GetParent(&path);
			path.Append(search_path + 3);
		} else {
			path.SetTo(search_path);
		};

		search_path = strtok_r(NULL, ":", &next_path_token);

		path.Append("boneyard");
		printf("Looking into %s\n", path.Path());
		
		dir.SetTo(path.Path());
		if (dir.InitCheck() != B_OK)
			continue;
		
		dir.Rewind();
		while (dir.GetNextEntry(&entry) >= 0) {
			if (entry.IsDirectory())
				continue;

			entry.GetPath(&addon_path);
			image_id addon_id = load_add_on(addon_path.Path());
			if (addon_id < 0) {
				printf("Failed to load %s addon: %s.\n", addon_path.Path(), strerror(addon_id));
				continue;
			};
	
			printf("Addon %s loaded.\n", addon_path.Path());
		
			by_instantiate_func func;
			status_t status;
			
			status = get_image_symbol(addon_id, "instantiate", B_SYMBOL_TYPE_TEXT, (void **) &func);
			if (status != B_OK) {
				//  No "modules" symbol found in this addon
				printf("Symbol \"instantiate\" not found in %s addon: not a module addon!\n", addon_path.Path());
			} else {
				BYAddon *by = func();
				BMessage *msg = new BMessage(msg_what);
				msg->AddInt32("image_id", addon_id);
				msg->AddString("addon_path", addon_path.Path());
				msg->AddPointer("byaddon", by);
				menu->AddItem(new BMenuItem(by->Name(), msg));
			};
		};
		
	};

	free(search_paths);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Status", new BMessage(msg_what)));
	menu->AddItem(new BMenuItem("Very long label of network stuff to set/display", new BMessage(msg_what)));
}	

// #pragma mark Boneyard add-ons support

_EXPORT void boneyard_do_save() {}
_EXPORT void boneyard_do_revert() {}

BYDataEntry::BYDataEntry(const char *name, const char *value, const char *comment)
{
	m_name 		= (char *) name;
	m_value 	= (char *) value;
	m_comment 	= (char *) comment;
}
	
BYDataEntry::~BYDataEntry() {}

const char * BYDataEntry::Name() 	{ return m_name; }
const char * BYDataEntry::Value() 	{ return m_value; }
const char * BYDataEntry::Comment()	{ return m_comment; }
void BYDataEntry::SetValue(char *value) 		{ m_value = value; }
void BYDataEntry::SetComment(char *comment) 	{ m_comment = comment; }

void BYDataEntry::_ReservedDataEntry1() {}
void BYDataEntry::_ReservedDataEntry2() {}

BYAddon::BYAddon() {}
BYAddon::~BYAddon() {}
BView * BYAddon::CreateView(BRect *bounds) { return NULL; }
void BYAddon::Revert() {}
void BYAddon::Save() {}

const char *BYAddon::Name() { return "noname"; }
const char *BYAddon::Section() { return "noname"; }
const char *BYAddon::Description() { return "noname"; }
const BList *BYAddon::FileList() { return NULL; }
int BYAddon::Importance() { return 10; }

const BList *BYAddon::ObtainSectionData() { return NULL; }			// get all BYDataEntrys for my section.
const char *BYAddon::GetValue(const char *name) { return ""; }		// get a value from your section.
void BYAddon::SetValue(const char *name,const char *data) {}		// set a value in your section.
void BYAddon::ClearValue(const char *name) {}			// remove an entry from your section.
void BYAddon::ClearSectionData() {}					// remove all entries from your section.
bool BYAddon::IsDirty() { return false; }								// query dirty status
void BYAddon::SetDirty(bool _dirty) {}			// set dirty status
const BList *BYAddon::GetAddonList() { return NULL; }				// get all add-ons in the cradle.

void BYAddon::_ReservedAddon1() {}
void BYAddon::_ReservedAddon2() {}
void BYAddon::_ReservedAddon3() {}
void BYAddon::_ReservedAddon4() {}
void BYAddon::_ReservedAddon5() {}
void BYAddon::_ReservedAddon6() {}
void BYAddon::_ReservedAddon7() {}
void BYAddon::_ReservedAddon8() {}

// void BYAddon::GetAddonList();

void save_config();
