#include "Application.h"
#include "Resources.h"
#include "Window.h"
#include "View.h"
#include "Region.h"
#include "Button.h"
#include "CheckBox.h"
#include "TextControl.h"
#include "Bitmap.h"
#include "Mime.h"
#include "FilePanel.h"
#include "Menu.h"
#include "PopUpMenu.h"
#include "MenuItem.h"
#include "MenuField.h"
#include "String.h"

#include "dirent.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "ListControl.h"
#include "PickUser.h"
#include "GroupProperties.h"
#include "Icons.h"
#include "besure_auth.h"

const int32 MSG_GROUP_OK		= 'GOky';
const int32 MSG_GROUP_CANCEL	= 'GCan';
const int32 MSG_GROUP_REFRESH	= 'GRef';
const int32 MSG_GROUP_ADD		= 'GAdd';
const int32 MSG_GROUP_REMOVE	= 'GRem';
const int32 MSG_MEMBER_SELECT	= 'MSel';


// ----- MemberItem ------------------------------------------------------------

MemberItem::MemberItem(BView *headerView, const char *text0, const char *text1, const char *text2) :
  ListItem(headerView, text0, text1, text2)
{
	BBitmap *icon;
	BMessage archive;
	size_t size;

	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_USER_SMALL, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
		{
			icon = new BBitmap(&archive);

			SetAssociatedHeader(headerView);
			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, text1);
			SetColumnContent(3, text2);
		}
}

MemberItem::~MemberItem()
{
}

void MemberItem::ItemSelected()
{
	GroupPropertiesView *view = (GroupPropertiesView *) GetAssociatedHeader();
	view->GetRemoveButton()->SetEnabled(true);
}

void MemberItem::ItemDeselected()
{
	GroupPropertiesView *view = (GroupPropertiesView *) GetAssociatedHeader();
	view->GetRemoveButton()->SetEnabled(false);
}

// ----- GroupPropertiesView -----------------------------------------------------

GroupPropertiesView::GroupPropertiesView(BRect rect, const char *name) :
  BView(rect, "GroupInfoView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	newGroup = name == NULL;
	strcpy(group, newGroup ? "unknown" : name);
	getGroupDesc(group, desc, sizeof(desc));

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Teldar-Group");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(10, 52, 200, 72);
	editName = new BTextControl(r, "GroupName", "Name:", group, NULL);
	editName->SetDivider(70);
	editName->SetEnabled(newGroup);
	AddChild(editName);

	r.Set(10, 77, 250, 97);
	editDesc = new BTextControl(r, "Description", "Description:", desc, NULL);
	editDesc->SetDivider(70);
	AddChild(editDesc);

	r.Set(205, 320, 275, 340);
	BButton *okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_GROUP_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(285, 320, 360, 340);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_GROUP_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

	r.Set(285, 125, 360, 145);
	AddChild(new BButton(r, "AddUserBtn", "Add", new BMessage(MSG_GROUP_ADD), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

	r.Set(285, 155, 360, 175);
	removeBtn = new BButton(r, "RemoveBtn", "Remove", new BMessage(MSG_GROUP_REMOVE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	removeBtn->SetEnabled(false);
	AddChild(removeBtn);

	// Now add the membership list.
	CLVContainerView *listContView;
	r.Set(13, 125, 280 - B_V_SCROLL_BAR_WIDTH, 305);
	listView = new SmartColumnListView(r, &listContView, NULL, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
		false, false, true, false, B_FANCY_BORDER);

	listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
		CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
	listView->AddColumn(new CLVColumn("User", 85.0, CLV_SORT_KEYABLE, 50.0));
	listView->AddColumn(new CLVColumn("Full Name", 130.0, CLV_SORT_KEYABLE));

	listView->SetSelectionMessage(new BMessage(MSG_MEMBER_SELECT));
	listView->SetInvocationMessage(new BMessage(MSG_LIST_DESELECT));

	AddGroupMembers(listView);
	AddChild(listContView);
}

GroupPropertiesView::~GroupPropertiesView()
{
}

void GroupPropertiesView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	BString string(group);
	string += " Properties";
	DrawString(string.String());

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("Groups are collections of users that will share certain properties,");
	MovePenTo(55, 40);
	DrawString("such as access to files or network resources.");

	MovePenTo(13, 121);
	DrawString("Current Members:");

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}

void GroupPropertiesView::UpdateInfo()
{
	if (newGroup)
		strcpy(group, editName->Text());

	strcpy(desc, editDesc->Text());
}

void GroupPropertiesView::AddGroupMembers(ColumnListView* listView)
{
	DIR *groupList;
	struct dirent *member;

	groupList = OpenGroup(group);
	if (groupList)
	{
		while ((member = ReadGroupMember(groupList)) != NULL)
		{
			char desc[64];
			getUserFullName(member->d_name, desc, sizeof(desc));
			listView->AddItem(new MemberItem(this, member->d_name, desc, ""));
		}

		CloseGroup(groupList);
	}
}

char *GroupPropertiesView::GetSelectedMember()
{
	if (listView)
	{
		int index = listView->CurrentSelection();
		if (index >= 0)
		{
			MemberItem *item = (MemberItem *) listView->ItemAt(index);
			return ((char *) item->GetColumnContentText(1));
		}
	}

	return NULL;
}

void GroupPropertiesView::ItemSelected()
{
	removeBtn->SetEnabled(true);
}

void GroupPropertiesView::ItemDeselected()
{
	removeBtn->SetEnabled(false);
}

void GroupPropertiesView::AddNewMember(char *user)
{
	char desc[64];

	addUserToGroup(user, group);
	getUserFullName(user, desc, sizeof(desc));

	listView->LockLooper();
	listView->AddItem(new MemberItem(this, user, desc, ""));
	listView->UnlockLooper();
}

void GroupPropertiesView::RemoveSelectedMember()
{
	int index = listView->CurrentSelection();
	if (index >= 0)
	{
		MemberItem *item = (MemberItem *) listView->ItemAt(index);
		if (item)
		{
			removeUserFromGroup((char *) item->GetColumnContentText(1), group);

			listView->LockLooper();
			listView->RemoveItem(item);
			listView->UnlockLooper();
		}
	}
}


// ----- GroupPropertiesPanel ----------------------------------------------------------------------

GroupPropertiesPanel::GroupPropertiesPanel(BRect frame, const char *name, BWindow *parent) :
  BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	newGroup = name == NULL;
	if (!newGroup)
		strcpy(group, name);

	shareWin = parent;
	cancelled = true;

	BRect r = Bounds();
	infoView = new GroupPropertiesView(r, name == NULL ? NULL : group);
	AddChild(infoView);

	Show();
}

// MessageReceived()
//
void GroupPropertiesPanel::MessageReceived(BMessage *msg)
{
	PickUserPanel *picker;
	BRect frame;
	status_t result;
	char *user;

	switch (msg->what)
	{
		case MSG_GROUP_OK:
//			relay = new BMessage(MSG_GROUP_REFRESH);
//			relay->AddInt32("share", share);
//			shareWin->PostMessage(relay);
			cancelled = false;
			infoView->UpdateInfo();
			if (newGroup)
			{
				strcpy(group, infoView->getGroup());
				createGroup(group);
			}

			strcpy(desc, infoView->getDesc());
			setGroupDesc(group, desc);
			BWindow::Quit();
			break;

		case MSG_GROUP_CANCEL:
			cancelled = true;
			BWindow::Quit();
			break;

		case MSG_MEMBER_SELECT:
			infoView->ItemSelected();
			break;

		case (int32) 'CLV0': //MSG_LIST_DESELECT:
			infoView->ItemDeselected();
			break;

		case MSG_GROUP_ADD:
			frame = Frame();
			frame.left += 100;
			frame.top += 25;
			frame.right = frame.left + 285;
			frame.bottom = frame.top + 340;
			picker = new PickUserPanel(frame, group);
			wait_for_thread(picker->Thread(), &result);
			user =  picker->GetUser();
			if (user)
				infoView->AddNewMember(user);
			break;

		case MSG_GROUP_REMOVE:
			infoView->RemoveSelectedMember();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
