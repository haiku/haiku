#include "TFindPanel.h"


#include <utility>
#include <iostream>

#include <fs_attr.h>

#include <AutoLock.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Locker.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <OS.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "Commands.h"
#include "IconMenuItem.h"
#include "MimeTypeList.h"
#include "MimeTypes.h"
#include "QueryContainerWindow.h"
#include "QueryPoseView.h"
#include "TAttributeColumn.h"
#include "TAttributeSearchField.h"
#include "Tracker.h"
#include "ViewState.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FindPanel"


namespace BPrivate {


static const char* kAllMimeTypes = "mimes/ALLTYPES";


static const char* combinationOperators[] = {
	B_TRANSLATE_MARK("and"),
	B_TRANSLATE_MARK("or"),
};

static const int32 combinationOperatorsLength = sizeof(combinationOperators) / sizeof(
	combinationOperators[0]);

static const query_op combinationQueryOps[] = {
	B_AND,
	B_OR
};

static const char* regularAttributeOperators[] = {
	B_TRANSLATE_MARK("contains"),
	B_TRANSLATE_MARK("is"),
	B_TRANSLATE_MARK("is not"),
	B_TRANSLATE_MARK("starts with"),
	B_TRANSLATE_MARK("ends with")
};

static const query_op regularQueryOps[] = {
	B_CONTAINS,
	B_EQ,
	B_NE,
	B_BEGINS_WITH,
	B_ENDS_WITH
};

static const int32 regularAttributeOperatorsLength = sizeof(regularAttributeOperators) / 
	sizeof(regularAttributeOperators[0]);

static const char* sizeAttributeOperators[] = {
	B_TRANSLATE_MARK("greater than"),
	B_TRANSLATE_MARK("less than"),
	B_TRANSLATE_MARK("is")
};

static const query_op sizeQueryOps[] = {
	B_GT,
	B_LT,
	B_EQ
};

static const int32 sizeAttributeOperatorsLength = sizeof(sizeAttributeOperators) / sizeof(
	sizeAttributeOperators[0]);

static const char* modifiedAttributeOperators[] = {
	B_TRANSLATE_MARK("before"),
	B_TRANSLATE_MARK("after")
};

static const query_op modifiedQueryOps[] = {
	B_LT,
	B_GT
};

static const int32 modifiedAttributeOperatorsLength = sizeof(modifiedAttributeOperators) /
	sizeof(modifiedAttributeOperators[0]);


class TMostUsedNames {
public:
								TMostUsedNames(const char* fileName, const char* directory,
									int32 maxCount = 5);
								~TMostUsedNames();

			bool				ObtainList(BList* list);
			void				ReleaseList();

			void 				AddName(const char*);

protected:
			struct list_entry {
				char* name;
				int32 count;
			};

		static int CompareNames(const void* a, const void* b);
		void LoadList();
		void UpdateList();

		const char*	fFileName;
		const char*	fDirectory;
		bool		fLoaded;
		mutable Benaphore fLock;
		BList		fList;
		int32		fCount;
};

TMostUsedNames gTMostUsedMimeTypes("MostUsedMimeTypes", "Tracker");


BEntry
GetEntryFromEntryRef(const entry_ref* ref)
{
	if (ref == NULL) {
		return BEntry();
	} else {
		BEntry entry(ref);
		if (entry.InitCheck() != B_OK)
			return BEntry();
		else
			return entry;
	}
}


void
TryReadingFileFromEntry(BFile* file, const BEntry* entry)
{
	if (entry == NULL || entry->InitCheck() != B_OK)
		return;
	
	file = new BFile(entry, B_READ_WRITE | B_CREATE_FILE);
}


BPath
CreateTemporaryFile()
{
	BPath userHomeDirectoryPath;
	if (find_directory(B_USER_DIRECTORY, &userHomeDirectoryPath) != B_OK)
		userHomeDirectoryPath.SetTo("/boot/home");
	
	BPath temporaryQueriesDirectoryPath = userHomeDirectoryPath;
	temporaryQueriesDirectoryPath.Append("queries/temporary");
	if (BEntry(temporaryQueriesDirectoryPath.Path()).Exists() == false) {
		BPath queriesDirectoryPath;
		temporaryQueriesDirectoryPath.GetParent(&queriesDirectoryPath);
		BDirectory(temporaryQueriesDirectoryPath.Path()).CreateDirectory("temporary", NULL);
	}
	
	BDirectory temporaryQueriesDirectory(temporaryQueriesDirectoryPath.Path());
	int32 countOfFiles = temporaryQueriesDirectory.CountEntries();
	
	BString temporaryFileName = "temporary";
	temporaryFileName.Append(std::to_string(countOfFiles).c_str());
	
	BPath temporaryFilePath = temporaryQueriesDirectoryPath;
	temporaryFilePath.Append(temporaryFileName);
	
	return temporaryFilePath;
}


TFindPanel::TFindPanel(BQueryContainerWindow* window, BQueryPoseView* poseView, BLooper* looper,
	entry_ref* ref)
	:
	BView("find-panel", B_WILL_DRAW),
	fContainerWindow(window),
	fPoseView(poseView),
	fLooper(looper),
	fEntry(NULL),
	fFile(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());

	if (ref == NULL) {
		BPath temporaryFilePath = CreateTemporaryFile();
		fFile = new BFile(temporaryFilePath.Path(), B_READ_WRITE | B_CREATE_FILE);
		if (fFile->InitCheck() != B_OK)
			debugger("");
		fEntry = new BEntry(temporaryFilePath.Path());
		BNodeInfo(fFile).SetType(B_QUERY_MIMETYPE);
	}

	// Building the mime-type menu
	fMimeTypeMenu = new BPopUpMenu("MimeTypeMenu");
	fMimeTypeMenu->SetRadioMode(true);
	AddMimeTypesToMenu(fMimeTypeMenu);
	fMimeTypeField = new BMenuField("MimeTypeMenu", "", fMimeTypeMenu);
	fMimeTypeField->SetDivider(0.0f);
	fMimeTypeField->MenuItem()->SetLabel(B_TRANSLATE("All files and folders"));

	// Building the volume/directory selection Menu
	fVolMenu = new BPopUpMenu("VolumeMenu", false, false);
	BMenuField* volumeField = new BMenuField("", B_TRANSLATE("In"), fVolMenu);
	volumeField->SetDivider(volumeField->StringWidth(volumeField->Label())+8);
	AddVolumes(fVolMenu);
	
	fColumnsContainer = new BView("columns-container", B_WILL_DRAW);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(0, B_USE_WINDOW_SPACING, 0, 0)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fMimeTypeField)
			.Add(volumeField)
			.Add((fPauseButton = new BButton("Pause", new BMessage('1234'))))
			.AddGlue()
		.End()
		.Add(new BSeparatorView())
		.Add(fColumnsContainer)
	.End();
	
	TFindPanel::ResizeMenuField(fMimeTypeField);
	TFindPanel::ResizeMenuField(volumeField);
}


TFindPanel::~TFindPanel()
{
	fEntry->Remove();
	delete fEntry;
	delete fFile;
}


void
TFindPanel::AttachedToWindow()
{
	BMessenger target(this);
	fVolMenu->SetTargetForItems(this);
	
	int32 count = fMimeTypeMenu->CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem* item = fMimeTypeMenu->ItemAt(i);
		BMenu* subMenu = item->Submenu();
		if (subMenu == NULL) {
			item->SetTarget(target);
			continue;
		}
		
		subMenu->SetTargetForItems(target);
	}
	
	fPauseButton->SetTarget(target);
}

void
TFindPanel::ResizeMenuField(BMenuField* menuField)
{
	BSize size;
	menuField->GetPreferredSize(&size.width, &size.height);

	BMenu* menu = menuField->Menu();

	float padding = 0.0f;
	float width = 0.0f;

	BMenuItem* markedItem = menu->FindMarked();
	if (markedItem != NULL) {
		if (markedItem->Submenu() != NULL) {
			BMenuItem* markedSubItem = markedItem->Submenu()->FindMarked();
			if (markedSubItem != NULL && markedSubItem->Label() != NULL) {
				float labelWidth
					= menuField->StringWidth(markedSubItem->Label());
				padding = size.width - labelWidth;
			}
		} else if (markedItem->Label() != NULL) {
			float labelWidth = menuField->StringWidth(markedItem->Label());
			padding = size.width - labelWidth;
		}
	}

	for (int32 index = menu->CountItems(); index-- > 0; ) {
		BMenuItem* item = menu->ItemAt(index);
		if (item->Label() != NULL)
			width = std::max(width, menuField->StringWidth(item->Label()));

		BMenu* submenu = item->Submenu();
		if (submenu != NULL) {
			for (int32 subIndex = submenu->CountItems(); subIndex-- > 0; ) {
				BMenuItem* subItem = submenu->ItemAt(subIndex);
				if (subItem->Label() == NULL)
					continue;

				width = std::max(width,
					menuField->StringWidth(subItem->Label()));
			}
		}
	}

	float maxWidth = be_control_look->DefaultItemSpacing() * 20;
	size.width = std::min(width + padding, maxWidth);
	menuField->SetExplicitSize(size);
}


static void
PopUpMenuSetTitle(BMenu* menu, const char* title)
{
	// This should really be in BMenuField
	BMenu* bar = menu->Supermenu();

	ASSERT(bar);
	ASSERT(bar->ItemAt(0));
	if (bar == NULL || !bar->ItemAt(0))
		return;

	bar->ItemAt(0)->SetLabel(title);
}


status_t
TFindPanel::SetCurrentMimeType(BMenuItem* item)
{
	// unmark old MIME type (in most used list, and the tree)

	BMenuItem* marked = CurrentMimeType();
	if (marked != NULL) {
		marked->SetMarked(false);

		if ((marked = MimeTypeMenu()->FindMarked()) != NULL)
			marked->SetMarked(false);
	}

	// mark new MIME type (in most used list, and the tree)

	if (item != NULL) {
		item->SetMarked(true);
		fMimeTypeField->MenuItem()->SetLabel(item->Label());

		BMenuItem* search;
		for (int32 i = 2; (search = MimeTypeMenu()->ItemAt(i)) != NULL; i++) {
			if (item == search || search->Label() == NULL)
				continue;

			if (strcmp(item->Label(), search->Label()) == 0) {
				search->SetMarked(true);
				break;
			}

			BMenu* submenu = search->Submenu();
			if (submenu == NULL)
				continue;

			for (int32 j = submenu->CountItems(); j-- > 0;) {
				BMenuItem* sub = submenu->ItemAt(j);
				if (strcmp(item->Label(), sub->Label()) == 0) {
					sub->SetMarked(true);
					break;
				}
			}
		}
	}

	return B_OK;
}


BMenuItem*
TFindPanel::CurrentMimeType(const char** type) const
{
	// search for marked item in the list
	BMenuItem* item = MimeTypeMenu()->FindMarked();

	if (item != NULL && MimeTypeMenu()->IndexOf(item) != 0
		&& item->Submenu() == NULL) {
		// if it's one of the most used items, ignore it
		item = NULL;
	}

	if (item == NULL) {
		for (int32 index = MimeTypeMenu()->CountItems(); index-- > 0;) {
			BMenu* submenu = MimeTypeMenu()->ItemAt(index)->Submenu();
			if (submenu != NULL && (item = submenu->FindMarked()) != NULL)
				break;
		}
	}

	if (type != NULL && item != NULL) {
		BMessage* message = item->Message();
		if (message == NULL)
			return NULL;

		if (message->FindString("mimetype", type) != B_OK)
			return NULL;
	}
	return item;
}


status_t
TFindPanel::SetCurrentMimeType(const char* label)
{
	// unmark old MIME type (in most used list, and the tree)

	BMenuItem* marked = CurrentMimeType();
	if (marked != NULL) {
		marked->SetMarked(false);

		if ((marked = MimeTypeMenu()->FindMarked()) != NULL)
			marked->SetMarked(false);
	}

	// mark new MIME type (in most used list, and the tree)

	fMimeTypeField->MenuItem()->SetLabel(label);
	bool found = false;

	for (int32 index = MimeTypeMenu()->CountItems(); index-- > 0;) {
		BMenuItem* item = MimeTypeMenu()->ItemAt(index);
		BMenu* submenu = item->Submenu();
		if (submenu != NULL && !found) {
			for (int32 subIndex = submenu->CountItems(); subIndex-- > 0;) {
				BMenuItem* subItem = submenu->ItemAt(subIndex);
				if (subItem->Label() != NULL
					&& strcmp(label, subItem->Label()) == 0) {
					subItem->SetMarked(true);
					found = true;
				}
			}
		}

		if (item->Label() != NULL && strcmp(label, item->Label()) == 0) {
			item->SetMarked(true);
			return B_OK;
		}
	}

	return found ? B_OK : B_ENTRY_NOT_FOUND;
}


void
TFindPanel::AddMimeTypesToMenu(BPopUpMenu* menu)
{
	BMessenger target(this);

	const char* kAllMimeTypes = "mime/ALLTYPES";

	BMessage* itemMessage = new BMessage(kMIMETypeItem);
	itemMessage->AddString("mimetype", kAllMimeTypes);

	IconMenuItem* firstItem = new IconMenuItem(
		B_TRANSLATE("All files and folders"), itemMessage,
		static_cast<BBitmap*>(NULL));
	fMimeTypeMenu->AddItem(firstItem);
	fMimeTypeMenu->AddSeparatorItem();

	// add recent MIME types

	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	ASSERT(tracker != NULL);

	BList list;
	if (tracker != NULL && gTMostUsedMimeTypes.ObtainList(&list)) {
		int32 count = 0;
		for (int32 index = 0; index < list.CountItems(); index++) {
			const char* name = (const char*)list.ItemAt(index);

			MimeTypeList* mimeTypes = tracker->MimeTypes();
			if (mimeTypes != NULL) {
				const ShortMimeInfo* info = mimeTypes->FindMimeType(name);
				if (info == NULL)
					continue;

				BMessage* message = new BMessage(kMIMETypeItem);
				message->AddString("mimetype", info->InternalName());

				fMimeTypeMenu->AddItem(new BMenuItem(name, message));
				count++;
			}
		}
		if (count != 0)
			fMimeTypeMenu->AddSeparatorItem();

		gTMostUsedMimeTypes.ReleaseList();
	}

	// add MIME type tree list

	BMessage types;
	if (BMimeType::GetInstalledSupertypes(&types) == B_OK) {
		const char* superType;
		int32 index = 0;

		while (types.FindString("super_types", index++, &superType) == B_OK) {
			BMenu* superMenu = new BMenu(superType);

			BMessage* message = new BMessage(kMIMETypeItem);
			message->AddString("mimetype", superType);

			fMimeTypeMenu->AddItem(new IconMenuItem(superMenu, message,
				superType));

			// the MimeTypeMenu's font is not correct at this time
			superMenu->SetFont(be_plain_font);
		}
	}

	if (tracker != NULL) {
		tracker->MimeTypes()->EachCommonType(
			&TFindPanel::AddOneMimeTypeToMenu, fMimeTypeMenu);
	}

	// remove empty super type menus (and set target)

	for (int32 index = fMimeTypeMenu->CountItems(); index-- > 2;) {
		BMenuItem* item = fMimeTypeMenu->ItemAt(index);
		BMenu* submenu = item->Submenu();
		if (submenu == NULL) {
			item->SetTarget(target);
			continue;
		}

		if (submenu->CountItems() == 0) {
			fMimeTypeMenu->RemoveItem(item);
			delete item;
		} else
			submenu->SetTargetForItems(BMessenger(this));
	}
}


static void
AddSubtype(BString& text, const BMimeType& type)
{
	text.Append(" (");
	text.Append(strchr(type.Type(), '/') + 1);
		// omit the slash
	text.Append(")");
}


void
TFindPanel::PushMimeType(BQuery* query) const
{
	const char* type;
	if (CurrentMimeType(&type) == NULL)
		return;

	if (strcmp(kAllMimeTypes, type)) {
		// add an asterisk if we are searching for a supertype
		char buffer[B_FILE_NAME_LENGTH];
		if (strchr(type, '/') == NULL) {
			strlcpy(buffer, type, sizeof(buffer));
			strlcat(buffer, "/*", sizeof(buffer));
			type = buffer;
		}

		query->PushAttr(kAttrMIMEType);
		query->PushString(type);
		query->PushOp(B_EQ);
	}
}


bool
TFindPanel::AddOneMimeTypeToMenu(const ShortMimeInfo* info, void* castToMenu)
{
	BPopUpMenu* menu = static_cast<BPopUpMenu*>(castToMenu);

	BMimeType type(info->InternalName());
	BMimeType super;
	type.GetSupertype(&super);
	if (super.InitCheck() < B_OK)
		return false;

	BMenuItem* superItem = menu->FindItem(super.Type());
	if (superItem != NULL) {
		BMessage* message = new BMessage(kMIMETypeItem);
		message->AddString("mimetype", info->InternalName());

		// check to ensure previous item's name differs
		BMenu* menu = superItem->Submenu();
		BMenuItem* previous = menu->ItemAt(menu->CountItems() - 1);
		BString text = info->ShortDescription();
		if (previous != NULL
			&& strcasecmp(previous->Label(), info->ShortDescription()) == 0) {
			AddSubtype(text, type);

			// update the previous item as well
			BMimeType type(previous->Message()->GetString("mimetype", NULL));
			BString label = ShortMimeInfo(type).ShortDescription();
			AddSubtype(label, type);
			previous->SetLabel(label.String());
		}

		menu->AddItem(new IconMenuItem(text.String(), message,
			info->InternalName()));
	}

	return false;
}


void
TFindPanel::AddVolumes(BMenu* menu)
{
	// ToDo: add calls to this to rebuild the menu when a volume gets mounted

	BMessage* message = new BMessage(kTVolumeItem);
	message->AddInt32("device", -1);
	menu->AddItem(new BMenuItem(B_TRANSLATE("All disks"), message));
	menu->AddSeparatorItem();
	PopUpMenuSetTitle(menu, B_TRANSLATE("All disks"));

	BVolumeRoster roster;
	BVolume volume;
	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsPersistent() && volume.KnowsQuery()) {
			BDirectory root;
			if (volume.GetRootDirectory(&root) != B_OK)
				continue;

			BEntry entry;
			root.GetEntry(&entry);

			Model model(&entry, true);
			if (model.InitCheck() != B_OK)
				continue;

			message = new BMessage(kTVolumeItem);
			message->AddInt32("device", volume.Device());
			
			menu->AddItem(new ModelMenuItem(&model, model.Name(), message));
		}
	}

	if (menu->ItemAt(0))
		menu->ItemAt(0)->SetMarked(true);
}


void
TFindPanel::ShowVolumeMenuLabel()
{
	if (fVolMenu->ItemAt(0)->IsMarked()) {
		// "all disks" selected
		PopUpMenuSetTitle(fVolMenu, fVolMenu->ItemAt(0)->Label());
		return;
	}

	// find out if more than one items are marked
	int32 count = fVolMenu->CountItems();
	int32 countSelected = 0;
	BMenuItem* tmpItem = NULL;
	for (int32 index = 2; index < count; index++) {
		BMenuItem* item = fVolMenu->ItemAt(index);
		if (item->IsMarked()) {
			countSelected++;
			tmpItem = item;
		}
	}


	if (countSelected == 0) {
		// no disk selected, for now revert to search all disks
		// ToDo:
		// show no disks here and add a check that will not let the
		// query go if the user doesn't pick at least one
		fVolMenu->ItemAt(0)->SetMarked(true);
		PopUpMenuSetTitle(fVolMenu, fVolMenu->ItemAt(0)->Label());
	} else if (countSelected > 1)
		// if more than two disks selected, don't use the disk name
		// as a label
		PopUpMenuSetTitle(fVolMenu,	B_TRANSLATE("multiple disks"));
	else {
		ASSERT(tmpItem);
		PopUpMenuSetTitle(fVolMenu, tmpItem->Label());
	}
}


float
TFindPanel::FindMaximumHeightOfColumns()
{
	float height = 0.0f;
	int32 count = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < count; ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
		if (searchColumn == NULL)
			continue;
		
		BSize size;
		if (searchColumn->GetRequiredHeight(&size) != B_OK)
			continue;
		
		if (size.height > height)
			height = size.height;
	}
	
	return height;
}


void
TFindPanel::HandleResizingColumnsContainer()
{
	float height = FindMaximumHeightOfColumns();
	BSize size(B_SIZE_UNSET, height);
	BSize containerSize(size);
	containerSize.height += be_control_look->DefaultItemSpacing();
	fColumnsContainer->SetExplicitSize(containerSize);
	fColumnsContainer->SetExplicitPreferredSize(containerSize);
	fColumnsContainer->ResizeTo(containerSize);
	fColumnsContainer->Invalidate();

	int32 count = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < count; ++i) {
		TAttributeColumn* column = dynamic_cast<TAttributeColumn*>(fColumnsContainer->ChildAt(i));
		if (column == NULL)
			continue;
		BMessage* message = new BMessage(kResizeHeight);
		message->AddFloat("height", size.height);
		BMessenger(column).SendMessage(message);
	}
}


void
TFindPanel::HandleMovingAColumn()
{
	float height = FindMaximumHeightOfColumns();
	int32 count = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < count; ++i) {
		TAttributeColumn* column = dynamic_cast<TAttributeColumn*>(fColumnsContainer->ChildAt(i));
		if (column == NULL)
			continue;
		
		BMessage* message = new BMessage(kMoveColumn);
		message->AddFloat("height", height);
		BMessenger(column).SendMessage(message);
	}
}


bool
CheckIfSearchColumnShouldBeAdded(const char* label)
{
	return !(!strcmp(label, B_TRANSLATE("Real name")) || !strcmp(label, B_TRANSLATE("Created")) ||
		!strcmp(label, B_TRANSLATE("Kind")) || !strcmp(label, B_TRANSLATE("Location")) ||
		!strcmp(label, B_TRANSLATE("Permissions")));
}


AttrType
GetAttrTypeFromInt(int32 type)
{
	if (type == B_OFF_T_TYPE || type == B_INT32_TYPE)
		return AttrType::SIZE;
	else if (type == B_TIME_TYPE)
		return AttrType::MODIFIED;
	else
		return AttrType::REGULAR_ATTRIBUTE;
}


status_t
TFindPanel::AddAttributeColumn(BColumn* column)
{
	if (column == NULL)
		return B_BAD_VALUE;
	
	BColumnTitle* title = fPoseView->ColumnTitleView()->FindColumnTitle(column);
	if (title == NULL)
		return B_BAD_VALUE;
	
	TAttributeColumn* attributeColumn = NULL;
	if (CheckIfSearchColumnShouldBeAdded(column->Title())) {
		AttrType type = GetAttrTypeFromInt(column->AttrType());
		attributeColumn = new TAttributeSearchColumn(title, fColumnsContainer, type);
		fColumnsContainer->AddChild(attributeColumn);
		BMessenger(attributeColumn).SendMessage(new BMessage(kAddSearchField));
	} else {
		attributeColumn = new TDisabledSearchColumn(title);
		fColumnsContainer->AddChild(attributeColumn);
		BMessenger(this).SendMessage(new BMessage(kResizeHeight));
	}
	
	return B_OK;
}


status_t
TFindPanel::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;

	int32 numberOfColumns = fColumnsContainer->CountChildren();
	int32 count = 0;
	
	BString findPanelPredicate = "";

	for (int32 i = 0; i < numberOfColumns; ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
		if (searchColumn == NULL)
			continue;
		
		BString columnPredicateString;
		searchColumn->GetPredicateString(&columnPredicateString);
		
		if (columnPredicateString == "")
			continue;
		
		findPanelPredicate.Append("(");
		findPanelPredicate.Append(columnPredicateString);
		findPanelPredicate.Append(")");
		
		if (count != 1)
			findPanelPredicate.Append("&&");
		else
			++count;
	}
	
	int32 length = findPanelPredicate.Length();
	if (length >= 2 && findPanelPredicate[length - 1] == '&')
		findPanelPredicate.Truncate(length -2);
	
	*predicateString = findPanelPredicate;
	return B_OK;
}


void
TFindPanel::RestoreColumnsFromFile()
{
	ASSERT(fFile != NULL);
	
	attr_info info;
	if (fFile->GetAttrInfo("columns", &info) != B_OK) {
		std::cout<<"There is no attr info"<<std::endl;
		debugger("");
	}
	
	BString buffer;
	char* bufferString = buffer.LockBuffer(info.size);
	if (fFile->ReadAttr("columns", B_MESSAGE_TYPE, 0, bufferString, info.size) != info.size) {
		std::cout<<"Huge Issue"<<std::endl;
		debugger("");
	}
	
	BMessage columnsMessage;
	columnsMessage.Unflatten(bufferString);
	
	int count = 0;
	
	for (int32 i = 0; i < fColumnsContainer->CountChildren(); ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
	
		if (searchColumn == NULL)
			continue;
		
		BMessage searchColumnMessage;
		columnsMessage.FindMessage("columns", count++, &searchColumnMessage);
		searchColumn->RestoreFromMessage(&searchColumnMessage);
		
		std::cout<<"Search Column Restored"<<std::endl;
	}
}


void
TFindPanel::WriteColumnsToFile()
{
	ASSERT(fFile != NULL);
	
	BMessage columnsMessage;
	int32 countOfChildren = fColumnsContainer->CountChildren();
	for (int32 i = 0; i < countOfChildren; ++i) {
		TAttributeSearchColumn* searchColumn = dynamic_cast<TAttributeSearchColumn*>(
			fColumnsContainer->ChildAt(i));
		if (searchColumn == NULL)
			continue;
		
		BMessage searchColumnMessage = searchColumn->ArchiveToMessage();
		columnsMessage.AddMessage("columns", &searchColumnMessage);
		// std::cout<<"Added One Column"<<std::endl;
	}
	
	ssize_t flattenedSize = columnsMessage.FlattenedSize();
	BString buffer;
	char* bufferString = buffer.LockBuffer(flattenedSize);
	if (columnsMessage.Flatten(bufferString, flattenedSize) != B_OK) {
		// std::cout<<"There is a huge issue"<<std::endl;
		debugger("");
	}
	
	int32 countOfSearchFields = -1;
	columnsMessage.GetInfo("columns", NULL, &countOfSearchFields);
	// std::cout<<countOfSearchFields<<std::endl;
	
	if (fFile->WriteAttr("columns", B_MESSAGE_TYPE, 0, bufferString, flattenedSize) != flattenedSize) {
		// std::cout<<"There is a huge issue 2"<<std::endl;
		debugger("");
	}
}


void
TFindPanel::SaveQueryAsAttributesToFile()
{
	ASSERT(fFile != NULL);
	
	// Save volumes to the query
	WriteVolumesToFile();
	
	BString predicateString;
	GetPredicateString(&predicateString);
	if (predicateString == "")
		predicateString = "(name==\"**\")";
	
	BString mimeString;
	BQuery mimeQuery;
	PushMimeType(&mimeQuery);
	mimeQuery.GetPredicate(&mimeString);
	if (mimeString != "" && mimeString != "(BEOS:TYPE==\"mime/ALLTYPES\")") {
		predicateString.Append("&&");
		predicateString.Append(mimeString);
	}
	
	WritePredicateToFile(&predicateString);
	std::cout<<predicateString<<std::endl;
}


int32
GetNumberOfVolumes()
{
	static int32 volumeCount = 0;
	static bool volumesFound = false;
	
	BVolume volume;
	BVolumeRoster roster;
	while (roster.GetNextVolume(&volume) == B_OK && volumesFound == false) {
		if (volume.KnowsQuery() && volume.IsPersistent() && volume.KnowsAttr())
			++volumeCount;
	}
	
	volumesFound = true;
	return volumeCount;
}


void
TFindPanel::WriteVolumesToFile()
{
	bool addAllVolumes = fVolMenu->ItemAt(0)->IsMarked();
	int32 numberOfVolumes = GetNumberOfVolumes();
	BMessage messageContainingVolumeInfo;
	for (int32 i = 2;i < numberOfVolumes + 2; ++i) {
		BMenuItem* volumeMenuItem = fVolMenu->ItemAt(i);
		BMessage* messageOfVolumeMenuItem = volumeMenuItem->Message();
		dev_t device;
		if (messageOfVolumeMenuItem->FindInt32("device", &device) != B_OK)
			continue;
		
		if (volumeMenuItem->IsMarked() || addAllVolumes) {
			BVolume volume(device);
			EmbedUniqueVolumeInfo(&messageContainingVolumeInfo, &volume);
		}
		
		ssize_t flattenedSize = messageContainingVolumeInfo.FlattenedSize();
		if (flattenedSize > 0) {
			BString bufferString;
			char* buffer = bufferString.LockBuffer(flattenedSize);
			messageContainingVolumeInfo.Flatten(buffer, flattenedSize);
			if (fFile->WriteAttr("_trk/qryvol1", B_MESSAGE_TYPE, 0, buffer,
					static_cast<size_t>(flattenedSize))
				!= flattenedSize) {
				return;
			}
		}
	}
}


void
TFindPanel::WritePredicateToFile(const BString* predicateString)
{
	if (predicateString == NULL)
		return;

	fFile->WriteAttr("_trk/qrystr", B_STRING_TYPE, 0, predicateString->String(),
		predicateString->Length());
}

void
TFindPanel::SendPoseViewUpdateMessage()
{
	BMessenger messenger(fLooper);
	BMessage* message = new BMessage(kUpdatePoseView);
	entry_ref ref;
	fEntry->GetRef(&ref);
	message->AddRef("refs", &ref);
	messenger.SendMessage(message);
}


void
TFindPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kOpenQuery:
		{
			RestoreColumnsFromFile();
			break;
		}
	
		case '1234':
		{
			if (strcmp(fPauseButton->Label(), "Pause") == 0) {
				fPauseButton->SetLabel("Search");
				BMessenger(fPoseView).SendMessage(new BMessage('1234'));
			} else {
				fPauseButton->SetLabel("Pause");
				BMessenger(this).SendMessage(new BMessage(kModifiedField));
			}
			break;
		}
		
		case '2345':
		{
			fPauseButton->SetLabel("Search");
			break;
		}
	
		case kAddColumn:
		{
			BColumn* column = NULL;
			if (message->FindPointer("pointer", (void**)&column) != B_OK || column == NULL)
				break;

			AddAttributeColumn(column);
			break;
		}
		
		case kRemoveColumn:
		{
			std::cout<<"This is a test"<<std::endl;
			BColumn* column;
			if (message->FindPointer("pointer", reinterpret_cast<void**>(&column)) == B_OK) {
				int32 columnsCount = fColumnsContainer->CountChildren();
				for (int32 i = 0; i < columnsCount; ++i) {
					TAttributeColumn* attributeColumn = dynamic_cast<TAttributeColumn*>(
						fColumnsContainer->ChildAt(i));
					if (attributeColumn->fColumnTitle->Column() == column) {
						attributeColumn->RemoveSelf();
						delete attributeColumn;
						break;
					}
				}
				HandleMovingAColumn();
			}
			break;
		}
		
		case kResizeHeight:
		{
			HandleResizingColumnsContainer();
			break;
		}
		
		case kMoveColumn:
		{
			HandleMovingAColumn();
			break;
		}
		
		case kTVolumeItem:
		{
			// volume changed
			BMenuItem* invokedItem;
			dev_t dev;
			if (message->FindPointer("source", (void**)&invokedItem) != B_OK)
				return;

			if (message->FindInt32("device", &dev) != B_OK)
				break;

			BMenu* menu = invokedItem->Menu();
			ASSERT(menu);

			if (dev == -1) {
				// all disks selected, uncheck everything else
				int32 count = menu->CountItems();
				for (int32 index = 2; index < count; index++)
					menu->ItemAt(index)->SetMarked(false);

				// make all disks the title and check it
				PopUpMenuSetTitle(menu, menu->ItemAt(0)->Label());
				menu->ItemAt(0)->SetMarked(true);
			} else {
				// a specific volume selected, unmark "all disks"
				menu->ItemAt(0)->SetMarked(false);

				// toggle mark on invoked item
				int32 count = menu->CountItems();
				for (int32 index = 2; index < count; index++) {
					BMenuItem* item = menu->ItemAt(index);

					if (invokedItem == item) {
						// we just selected this
						bool wasMarked = item->IsMarked();
						item->SetMarked(!wasMarked);
					}
				}
			}
			// make sure the right label is showing
			ShowVolumeMenuLabel();

			break;
		}
		
		case kMIMETypeItem:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) == B_OK) {
				// don't add the "All files and folders" to the list
				if (fMimeTypeMenu->IndexOf(item) != 0)
					gTMostUsedMimeTypes.AddName(item->Label());

				SetCurrentMimeType(item);
			}
			break;
		}
		
		case kModifiedField:
		{
			SaveQueryAsAttributesToFile();
			SendPoseViewUpdateMessage();
			break;
		}
		
		default:
		{
			__inherited::MessageReceived(message);
			break;
		}
	}
}


TMostUsedNames::TMostUsedNames(const char* fileName, const char* directory,
	int32 maxCount)
	:
	fFileName(fileName),
	fDirectory(directory),
	fLoaded(false),
	fCount(maxCount)
{
}


TMostUsedNames::~TMostUsedNames()
{
	// only write back settings when we've been used
	if (!fLoaded)
		return;

	// write most used list to file

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK
		|| path.Append(fDirectory) != B_OK || path.Append(fFileName) != B_OK) {
		return;
	}

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK) {
		for (int32 i = 0; i < fList.CountItems(); i++) {
			list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));

			char line[B_FILE_NAME_LENGTH + 5];

			// limit upper bound to react more dynamically to changes
			if (--entry->count > 20)
				entry->count = 20;

			// if the item hasn't been chosen in a while, remove it
			// (but leave at least one item in the list)
			if (entry->count < -10 && i > 0)
				continue;

			sprintf(line, "%" B_PRId32 " %s\n", entry->count, entry->name);
			if (file.Write(line, strlen(line)) < B_OK)
				break;
		}
	}
	file.Unset();

	// free data

	for (int32 i = fList.CountItems(); i-- > 0;) {
		list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));
		free(entry->name);
		delete entry;
	}
}


bool
TMostUsedNames::ObtainList(BList* list)
{
	if (list == NULL)
		return false;

	if (!fLoaded)
		UpdateList();

	fLock.Lock();

	list->MakeEmpty();
	for (int32 i = 0; i < fCount; i++) {
		list_entry* entry = static_cast<list_entry*>(fList.ItemAt(i));
		if (entry == NULL)
			return true;

		list->AddItem(entry->name);
	}
	return true;
}


void
TMostUsedNames::ReleaseList()
{
	fLock.Unlock();
}


void
TMostUsedNames::AddName(const char* name)
{
	fLock.Lock();

	if (!fLoaded)
		LoadList();

	// remove last entry if there are more than
	// 2*fCount entries in the list

	list_entry* entry = NULL;

	if (fList.CountItems() > fCount * 2) {
		entry = static_cast<list_entry*>(
			fList.RemoveItem(fList.CountItems() - 1));

		// is this the name we want to add here?
		if (strcmp(name, entry->name)) {
			free(entry->name);
			delete entry;
			entry = NULL;
		} else
			fList.AddItem(entry);
	}

	if (entry == NULL) {
		for (int32 i = 0;
				(entry = static_cast<list_entry*>(fList.ItemAt(i))) != NULL; i++) {
			if (strcmp(entry->name, name) == 0)
				break;
		}
	}

	if (entry == NULL) {
		entry = new list_entry;
		entry->name = strdup(name);
		entry->count = 1;

		fList.AddItem(entry);
	} else if (entry->count < 0)
		entry->count = 1;
	else
		entry->count++;

	fLock.Unlock();
	UpdateList();
}


int
TMostUsedNames::CompareNames(const void* a,const void* b)
{
	list_entry* entryA = *(list_entry**)a;
	list_entry* entryB = *(list_entry**)b;

	if (entryA->count == entryB->count)
		return strcasecmp(entryA->name,entryB->name);

	return entryB->count - entryA->count;
}


void
TMostUsedNames::LoadList()
{
	if (fLoaded)
		return;
	fLoaded = true;

	// load the most used names list

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK
		|| path.Append(fDirectory) != B_OK || path.Append(fFileName) != B_OK) {
		return;
	}

	FILE* file = fopen(path.Path(), "r");
	if (file == NULL)
		return;

	char line[B_FILE_NAME_LENGTH + 5];
	while (fgets(line, sizeof(line), file) != NULL) {
		int32 length = (int32)strlen(line) - 1;
		if (length >= 0 && line[length] == '\n')
			line[length] = '\0';

		int32 count = atoi(line);

		char* name = strchr(line, ' ');
		if (name == NULL || *(++name) == '\0')
			continue;

		list_entry* entry = new list_entry;
		entry->name = strdup(name);
		entry->count = count;

		fList.AddItem(entry);
	}
	fclose(file);
}


void
TMostUsedNames::UpdateList()
{
	AutoLock<Benaphore> locker(fLock);

	if (!fLoaded)
		LoadList();

	// sort list items

	fList.SortItems(TMostUsedNames::CompareNames);
}

}