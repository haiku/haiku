/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "PPDConfigView.h"
#include "PPDParser.h"
#include "StatementListVisitor.h"
#include "UIUtils.h"

#include <Box.h>
#include <CheckBox.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

// margin
const float kLeftMargin = 3.0;
const float kRightMargin = 3.0;
const float kTopMargin = 3.0;
const float kBottomMargin = 3.0;

// space between views
const float kHorizontalSpace = 8.0;
const float kVerticalSpace = 8.0;

// Message what values
const uint32 kMsgBooleanChanged   = 'bool';
const uint32 kMsgStringChanged    = 'strc';

#include <stdio.h>

class DefaultValueExtractor : public StatementListVisitor
{
	BMessage fDefaultValues;
	
public:
	void DoDefault(Statement* statement)
	{
		const char* keyword = statement->GetKeywordString();
		const char* value = statement->GetValueString();
		if (keyword != NULL && value != NULL) {
			fDefaultValues.AddString(keyword, value);
		}
	}
	
	const BMessage& GetDefaultValues()
	{
		return fDefaultValues;
	}
};

class CategoryBuilder : public StatementListVisitor
{
	BOutlineListView* fCategories;
	
public:
	CategoryBuilder(BOutlineListView* categories) 
		: fCategories(categories)
	{}

	void AddStatement(const char* text, Statement* statement)
	{
		if (text != NULL) {
			BStringItem* item = new CategoryItem(text, statement, GetLevel());
			fCategories->AddItem(item);
		}
	}
	
	void BeginGroup(GroupStatement* group)
	{
		const char* translation = group->GetGroupTranslation();
		const char* name = group->GetGroupName();
		
		const char* text = NULL;
		if (translation != NULL) {
			text = translation;
		} else {
			text = name;
		}
		
		AddStatement(text, group->GetStatement());
	}
};


/*
DetailsBuilder adds views to the details view and sets the
value to the one specified in the settings.

The group type determines the view to be used for user input:
 Type     Input
 ---------------------
 Boolean  BRadioButton
 PickOne  BMenuField
 PickMany BCheckBox
 Unknown  BCheckBox
*/
class DetailsBuilder : public StatementListVisitor
{
	BView*          fParent;
	BView*          fDetails;
	BRect           fBounds;
	const char*     fKeyword;
	const char*     fValue;
	GroupStatement  fGroup;
	BMenu*          fMenu;
	BMenuField*     fMenuField;
	const BMessage& fSettings;

	void AddView(BView* view);
	
	BMessage* GetMessage(uint32 what, const char* option);
	
public:
	DetailsBuilder(BView* parent, BView* details, BRect bounds, Statement* group, const BMessage& settings);

	BRect GetBounds() { return fBounds; }
	
	void Visit(StatementList* list);

	void DoValue(Statement* statement);
};

void DetailsBuilder::AddView(BView* view)
{
	if (view != NULL) {
		fDetails->AddChild(view);
		view->ResizeToPreferred();
		
		fBounds.OffsetBy(0, view->Bounds().Height()+1);
		
		BControl* control = dynamic_cast<BControl*>(view);
		if (control != NULL) {
			control->SetTarget(fParent);
		}
	}
}

DetailsBuilder::DetailsBuilder(BView* parent, BView* details, BRect bounds, Statement* group, const BMessage& settings)
	: fParent(parent)
	, fDetails(details)
	, fBounds(bounds)
	, fGroup(group)
	, fMenu(NULL)
	, fMenuField(NULL)
	, fSettings(settings)
{
	fKeyword = fGroup.GetGroupName();

	if (fKeyword == NULL) return;

	fValue = settings.FindString(fKeyword);
		
	const char* label = fGroup.GetGroupTranslation();
	if (label == NULL) {
		label = fKeyword;
	}

	BView* view = NULL;
	if (fGroup.GetType() == GroupStatement::kPickOne) {
		fMenu = new BMenu("<pick one>");
		fMenu->SetRadioMode(true);
		fMenu->SetLabelFromMarked(true);
		fMenuField = new BMenuField(fBounds, "menuField", label, fMenu);
		view = fMenuField;
	} else if (fGroup.GetType() == GroupStatement::kBoolean) {
		BMessage* message = GetMessage(kMsgBooleanChanged, "");
		BCheckBox* cb = new BCheckBox(fBounds, "", label, message);
		view = cb;
		cb->SetValue((fValue != NULL && strcmp(fValue, "True") == 0)
			? B_CONTROL_ON
			: B_CONTROL_OFF);
	}

	AddView(view);
}

void DetailsBuilder::Visit(StatementList* list)
{
	if (fKeyword == NULL) return;
	
	StatementListVisitor::Visit(list);	
}

BMessage* DetailsBuilder::GetMessage(uint32 what, const char* option)
{
	BMessage* message = new BMessage(what);
	message->AddString("keyword", fKeyword);

	if (option != NULL) {
		message->AddString("option", option);
	}
	return message;
} 

void DetailsBuilder::DoValue(Statement* statement)
{
	if (GetLevel() != 0) return;
	if (strcmp(fKeyword, statement->GetKeywordString()) != 0) return;
	
	const char* text = NULL;
	const char* option = statement->GetOptionString();
	if (statement->GetTranslationString() != NULL) {
		text = statement->GetTranslationString();
	} else if (option != NULL) {
		text = option;
	}
	
	if (text == NULL) return;
	
	BView* view = NULL;
	BMessage* message = NULL;
		
	if (fGroup.GetType() == GroupStatement::kPickMany ||
		fGroup.GetType() == GroupStatement::kUnknown) {
		message = GetMessage(kMsgStringChanged, option);
		view = new BCheckBox(fBounds, "", text, message);
	} else if (fGroup.GetType() == GroupStatement::kPickOne) {
		message = GetMessage(kMsgStringChanged, option);
		BMenuItem* item = new BMenuItem(text, message);
		item->SetTarget(fParent);
		fMenu->AddItem(item);
		if (fValue != NULL && option != NULL && strcmp(fValue, option) == 0) {
			item->SetMarked(true);
		}
	}
	
	AddView(view);
}

#define kBoxHeight       20
#define kBoxBottomMargin 4
#define kBoxLeftMargin   8
#define kBoxRightMargin  8

#define kItemLeftMargin   5
#define kItemRightMargin  5

class PPDBuilder : public StatementListVisitor
{
	BView*    fParent;
	BView*    fView;
	BRect     fBounds;
	BMessage& fSettings;
	BList     fNestedBoxes;
	
	bool IsTop() 
	{
		return fNestedBoxes.CountItems() == 0;
	}
	
	void Push(BView* view) 
	{
		fNestedBoxes.AddItem(view);
	}

	void Pop() 
	{
		fNestedBoxes.RemoveItem((int32)fNestedBoxes.CountItems()-1);
	}
	
	BView* GetView()
	{
		if (IsTop()) {
			return fView;
		} else {
			return (BView*)fNestedBoxes.ItemAt(fNestedBoxes.CountItems()-1);
		}
	}
	
	
	
	BRect GetControlBounds()
	{
		if (IsTop()) {
			BRect bounds(fBounds);
			bounds.left  += kItemLeftMargin /** GetLevel()*/;
			bounds.right -= kItemRightMargin /** GetLevel()*/;
			return bounds;
		}
		
		BView* box = GetView();
		BRect bounds(box->Bounds());
		bounds.top = bounds.bottom - kBoxBottomMargin;
		bounds.bottom = bounds.top + kBoxHeight;
		bounds.left += kBoxLeftMargin;
		bounds.right -= kBoxRightMargin;
		return bounds;
	}
	
	bool IsUIGroup(GroupStatement* group)
	{
		return group->IsUIGroup() || group->IsJCL();
	}
	
	void UpdateParentHeight(float height)
	{
		if (IsTop()) {
			fBounds.OffsetBy(0, height);
		} else {
			BView* parent = GetView();
			parent->ResizeBy(0, height);
		}
	}
	
public:
	PPDBuilder(BView* parent, BView* view, BMessage& settings) 
		: fParent(parent)
		, fView(view)
		, fBounds(view->Bounds())
		, fSettings(settings)
	{
		RemoveChildren(view);
		fBounds.OffsetTo(0, 0);
		fBounds.left += kLeftMargin;
		fBounds.top += kTopMargin;
		fBounds.right -= kRightMargin;
	}
	
	BRect GetBounds() 
	{
		return BRect(0, 0, fView->Bounds().Width(), fBounds.top);
	}

	void AddUIGroup(const char* text, Statement* statement)
	{
		if (statement->GetChildren() == NULL) return;
		DetailsBuilder builder(fParent, GetView(), GetControlBounds(), statement, fSettings);
		builder.Visit(statement->GetChildren());
		
		if (IsTop()) {
			fBounds.OffsetTo(fBounds.left, builder.GetBounds().top);
		} else {
			BView* box = GetView();
			box->ResizeTo(box->Bounds().Width(), builder.GetBounds().top + kBoxBottomMargin);
		}
	}
	
	void OpenGroup(const char* text)
	{
		if (text != NULL) {
			BBox* box = new BBox(GetControlBounds(), text);
			box->SetLabel(text);
			GetView()->AddChild(box);
			Push(box);
			box->ResizeTo(box->Bounds().Width(), kBoxHeight);
		}
	}
	
	void CloseGroup()
	{
		if (!IsTop()) {
			BView* box = GetView();
			Pop();
			UpdateParentHeight(box->Bounds().Height());
		}
	}
	
	void BeginGroup(GroupStatement* group)
	{
		const char* translation = group->GetGroupTranslation();
		const char* name = group->GetGroupName();
		
		const char* text = NULL;
		if (translation != NULL) {
			text = translation;
		} else {
			text = name;
		}
		
		if (IsUIGroup(group)) {
			AddUIGroup(text, group->GetStatement());
		} else {
			OpenGroup(text);
		}	
	}
	
	void EndGroup(GroupStatement* group)
	{
		if (!IsUIGroup(group)) {
			CloseGroup();
		}	
	}
};

PPDConfigView::PPDConfigView(BRect bounds, const char *name, uint32 resizeMask, uint32 flags) 
	: BView(bounds, name, resizeMask, flags) 
	, fPPD(NULL)
{	
	// add category outline list view	
	bounds.OffsetTo(0, 0);
	BRect listBounds(bounds.left + kLeftMargin, bounds.top + kTopMargin, 
		bounds.right - kHorizontalSpace, bounds.bottom - kBottomMargin);
	listBounds.right -= B_V_SCROLL_BAR_WIDTH;
	listBounds.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BStringView* label = new BStringView(listBounds, "printer-settings", "Printer Settings:");
	AddChild(label);
	label->ResizeToPreferred();
	
	listBounds.top += label->Bounds().bottom + 5;

	// add details view
	fDetails = new BView(listBounds, "details", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fDetails->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BScrollView* scrollView = new BScrollView("details-scroll-view", 
		fDetails, B_FOLLOW_ALL_SIDES, 0, true, true);

	AddChild(scrollView);
}

void SetScrollBar(BScrollBar* scroller, float contents, float client)
{
	if (scroller != NULL) {
		float extent = contents - client;
		if (extent >= 0) {
			scroller->SetRange(0, extent);
			scroller->SetProportion(1-extent / contents);
			scroller->SetSteps(20, client);
		} else {
			scroller->SetRange(0, 0);
		}
	}
}

void PPDConfigView::FillCategories()
{
	if (fPPD == NULL) return;

	PPDBuilder builder(this, fDetails, fSettings);
	builder.Visit(fPPD);
	BScrollBar* scroller = fDetails->ScrollBar(B_VERTICAL);
	SetScrollBar(scroller, builder.GetBounds().Height(), fDetails->Bounds().Height());
	scroller = fDetails->ScrollBar(B_HORIZONTAL);
	SetScrollBar(scroller, builder.GetBounds().Width(), fDetails->Bounds().Width());
}

void PPDConfigView::FillDetails(Statement* statement)
{
	RemoveChildren(fDetails);
	
	if (statement == NULL) {
		return;
	}
	
	StatementList* children= statement->GetChildren();
	if (children == NULL) {
		return;
	}

	BRect bounds(fDetails->Bounds());
	bounds.OffsetTo(kLeftMargin, kTopMargin);
	DetailsBuilder builder(this, fDetails, bounds, statement, fSettings);
	builder.Visit(children);
}

void PPDConfigView::BooleanChanged(BMessage* msg)
{
	const char* keyword = msg->FindString("keyword");

	int32 value;
	if (msg->FindInt32("be:value", &value) == B_OK) {
		const char* option;
		if (value) {
			option = "True";
		} else {
			option = "False";
		}
		fSettings.ReplaceString(keyword, option);			
	}	
}

void PPDConfigView::StringChanged(BMessage* msg)
{
	const char* keyword = msg->FindString("keyword");
	const char* option = msg->FindString("option");

	if (keyword != NULL && keyword != NULL) {
		fSettings.ReplaceString(keyword, option);
	}
}

void PPDConfigView::MessageReceived(BMessage* msg)
{	
	switch (msg->what) {
		case kMsgBooleanChanged: BooleanChanged(msg);
			break;
		case kMsgStringChanged: StringChanged(msg);
			break;
	}

	BView::MessageReceived(msg);
}

void PPDConfigView::SetupSettings(const BMessage& currentSettings)
{
	DefaultValueExtractor extractor;
	extractor.Visit(fPPD);
	
	const BMessage &defaultValues(extractor.GetDefaultValues());
	fSettings.MakeEmpty();
	
	char* name;
	type_code code;
	for (int32 index = 0; defaultValues.GetInfo(B_STRING_TYPE, index, &name, &code) == B_OK; index ++) {
		const char* value = currentSettings.FindString(name);
		if (value == NULL) {
			value = defaultValues.FindString(name);
		}
		if (value != NULL) {
			fSettings.AddString(name, value);
		}
	}
}

void PPDConfigView::Set(const char* file, const BMessage& currentSettings)
{
	delete fPPD;
	
	PPDParser parser(file);
	fPPD = parser.ParseAll();
	if (fPPD == NULL) {
		fprintf(stderr, "Parsing error (%s): %s\n", file, parser.GetErrorMessage());
	}
	
	SetupSettings(currentSettings);
	FillCategories();
}
