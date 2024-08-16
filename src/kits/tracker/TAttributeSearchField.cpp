#include "TAttributeSearchField.h"


#include <utility>
#include <iostream>

#include <fs_attr.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <StringView.h>

#include "TAttributeColumn.h"
#include "PopUpTextControl.h"

namespace BPrivate {

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

void
PopulateOptionsMenuWithOperators(BMenu* menu, AttrType attrType) {
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		BMenuItem* item = new BMenuItem(combinationOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		message->AddPointer("pointer", item);
		message->AddBool("combination", true);
		item->SetMessage(message);
		menu->AddItem(item);
	}
	menu->AddSeparatorItem();
	
	const char** attrOperators = NULL;
	int32 attrOperatorsCount = 0;
	
	switch (attrType) {
		case AttrType::REGULAR_ATTRIBUTE:
		{
			attrOperators = regularAttributeOperators;
			attrOperatorsCount = regularAttributeOperatorsLength;
			break;
		}
		case AttrType::SIZE:
		{
			attrOperators = sizeAttributeOperators;
			attrOperatorsCount = sizeAttributeOperatorsLength;
			break;
		}
		case AttrType::MODIFIED:
		{
			attrOperators = modifiedAttributeOperators;
			attrOperatorsCount = modifiedAttributeOperatorsLength;
			break;
		}
	}
	
	for (int32 i = 0; i < attrOperatorsCount; ++i) {
		BMenuItem* item = new BMenuItem(attrOperators[i], NULL);
		BMessage* message = new BMessage(kMenuOptionClicked);
		message->AddPointer("pointer", item);
		message->AddBool("combination", false);
		item->SetMessage(message);
		menu->AddItem(item);
	}
}


status_t
TAttributeSearchField::GetSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;

	*size = fSize;
	return B_OK;
}


status_t
TAttributeSearchField::SetSize(BSize size)
{
	fSize = size;
	ResizeTo(fSize);
	SetExplicitSize(fSize);
	SetExplicitPreferredSize(fSize);
	
	return B_OK;
}


TAttributeSearchField::TAttributeSearchField(TAttributeSearchColumn* parent,
	AttrType attrType, const char* attrName)
	:
	BView("attribute-search-field", B_WILL_DRAW),
	fParent(parent)
{
	fAttrName = attrName;

	fLabel = new BStringView("label", NULL);
	fTextControl = new PopUpTextControl("text-control", NULL, NULL, this);
	fAttrType = attrType;
	PopulateOptionsMenuWithOperators(fTextControl->PopUpMenu(), attrType);
	
	fAddButton = new BButton("+", NULL);
	BMessage* message = new BMessage(kAddSearchField);
	message->AddPointer("pointer", this);
	fAddButton->SetMessage(message);
	
	fRemoveButton = new BButton("-", NULL);
	message = new BMessage(kRemoveSearchField);
	message->AddPointer("pointer", this);
	fRemoveButton->SetMessage(message);
	
	BSize textControlSize = fTextControl->PreferredSize();
	BSize buttonSize = BSize(textControlSize.height, textControlSize.height);
	fAddButton->ResizeTo(buttonSize);
	fRemoveButton->ResizeTo(buttonSize);
	fAddButton->SetExplicitSize(buttonSize);
	fRemoveButton->SetExplicitSize(buttonSize);
	
	fTextControl->PopUpMenu()->ItemAt(0)->SetMarked(true);
	fTextControl->PopUpMenu()->ItemAt(combinationOperatorsLength + 1)->SetMarked(true);
	UpdateLabel();
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fLabel)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fTextControl)
			.Add(fAddButton)
			.Add(fRemoveButton)
		.End()
	.End();
	
	BSize size;
	GetRequiredHeight(&size);
	SetSize(size);
}


TAttributeSearchField::~TAttributeSearchField()
{
	// Empty destructor!
}


void
TAttributeSearchField::AttachedToWindow()
{
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	
	fTextControl->PopUpMenu()->SetTargetForItems(this);
	fTextControl->SetTarget(this);
}


status_t
TAttributeSearchField::HandleMenuOptionClicked(BMenu* menu, BMenuItem* item,
	bool isCombinationOperator)
{
	if (item == NULL)
		return B_BAD_VALUE;
	
	int32 startingIndex = isCombinationOperator ? 0: combinationOperatorsLength;
	int32 endingIndex = isCombinationOperator ? combinationOperatorsLength : menu->CountItems();
	
	for (int32 i = startingIndex; i < endingIndex; ++i)
		menu->ItemAt(i)->SetMarked(false);
	
	item->SetMarked(true);
	UpdateLabel();
	return B_OK;
}


status_t
TAttributeSearchField::GetAttributeOperator(query_op* attributeOperator) const
{
	if (attributeOperator == NULL)
		return B_BAD_VALUE;
	
	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	int32 attributeOperatorIndex = -1;
	for (int32 i = combinationOperatorsLength + 1; i < optionsMenu->CountItems(); ++i) {
		if (optionsMenu->ItemAt(i)->IsMarked()) {
			attributeOperatorIndex = i - combinationOperatorsLength - 1;
			break;
		}
	}
	
	if (attributeOperatorIndex == -1)
		return B_ENTRY_NOT_FOUND;
	
	const query_op* operators = fAttrType == AttrType::REGULAR_ATTRIBUTE ? regularQueryOps :
		fAttrType == AttrType::SIZE ? sizeQueryOps : modifiedQueryOps;
	*attributeOperator = operators[attributeOperatorIndex];
	return B_OK;
}


status_t
TAttributeSearchField::GetCombinationOperator(query_op* combinationOperator) const
{
	if (combinationOperator == NULL)
		return B_BAD_VALUE;
	
	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	int32 combinationOperatorIndex = -1;
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		if (optionsMenu->ItemAt(i)->IsMarked()) {
			combinationOperatorIndex = i;
			break;
		}
	}
	
	if (combinationOperatorIndex == -1)
		return B_ENTRY_NOT_FOUND;
	
	*combinationOperator = combinationQueryOps[combinationOperatorIndex];
	return B_OK;
}


status_t
TAttributeSearchField::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	if (strcmp(fTextControl->Text(), "") == 0) {
		*predicateString = "";
		return B_CANCELED;
	}
	
	BPopUpMenu* optionsMenu = fTextControl->PopUpMenu();
	int32 attributeOperatorIndex = -1;
	
	for (int32 i = combinationOperatorsLength + 1; i < optionsMenu->CountItems(); ++i) {
		if (optionsMenu->ItemAt(i)->IsMarked()) {
			attributeOperatorIndex = i - combinationOperatorsLength - 1;
			break;
		}
	}
	
	BQuery predicateStringCreatorMessage;
	predicateStringCreatorMessage.PushAttr(fAttrName);
	predicateStringCreatorMessage.PushString(fTextControl->Text(), true);
	const query_op* operators = fAttrType == AttrType::REGULAR_ATTRIBUTE ? regularQueryOps :
		fAttrType == AttrType::SIZE ? sizeQueryOps : modifiedQueryOps;
	predicateStringCreatorMessage.PushOp(operators[attributeOperatorIndex]);
	
	predicateStringCreatorMessage.GetPredicate(predicateString);
	
	return B_OK;
}


void
TAttributeSearchField::RestoreFromMessage(const BMessage* message)
{
	
}


BMessage
TAttributeSearchField::ArchiveToMessage()
{
	BMessage searchFieldMessage;
	int32 count = fTextControl->PopUpMenu()->CountItems();
	for (int32 i = 0; i < count; ++i) {
		BMenuItem* item = fTextControl->PopUpMenu()->ItemAt(i);
		if (item->IsMarked())
			searchFieldMessage.AddString("menu-item", item->Label());
	}
	
	searchFieldMessage.AddString("text-control", fTextControl->Text());
	return searchFieldMessage;
}


void
TAttributeSearchField::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kAddSearchField:
		case kRemoveSearchField:
		{
			BMessenger(fParent).SendMessage(message);
			break;
		}
		
		case kMenuOptionClicked:
		{
			BMenuItem* source = NULL;
			if (message->FindPointer("pointer", (void**)&source) != B_OK || source == NULL)
				break;
			
			bool isCombinationOperator;
			if (message->FindBool("combination", &isCombinationOperator) != B_OK)
				break;
			
			HandleMenuOptionClicked(fTextControl->PopUpMenu(), source, isCombinationOperator);
			break;
		}
		default:
		{
			__inherited::MessageReceived(message);
			break;
		}
	}
}


void
TAttributeSearchField::UpdateLabel()
{
	BPopUpMenu* menu = fTextControl->PopUpMenu();
	int32 count = menu->CountItems();
	
	BString combinationOperatorLabel = "";
	BString operatorLabel = "";
	
	for (int32 i = 0; i < combinationOperatorsLength; i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (item->IsMarked())
			combinationOperatorLabel = item->Label();
	}
	
	for (int32 i = combinationOperatorsLength + 1; i < count; i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (item->IsMarked())
			operatorLabel = item->Label();
	}
	
	if (combinationOperatorLabel == "") {
		fLabel->SetText(operatorLabel.String());
		return;
	}
	
	combinationOperatorLabel.Append(" ");
	combinationOperatorLabel.Append(operatorLabel);
	fLabel->SetText(combinationOperatorLabel.String());
}


void
TAttributeSearchField::EnableAddButton(bool enable)
{
	fAddButton->SetEnabled(enable);
}


void
TAttributeSearchField::EnableRemoveButton(bool enable)
{
	fRemoveButton->SetEnabled(enable);
}


status_t
TAttributeSearchField::GetRequiredHeight(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;
	
	float width = B_SIZE_UNSET;
	float height = 0.0f;
	height += fTextControl->PreferredSize().height;
	height += fLabel->PreferredSize().height;
	height += 2.0f;
		// Spacing in between the BTextControl and the BStringView
	height += 4.0f;
		// 2.0f padding on both the top and bottom
	
	*size = BSize(width, height);
	return B_OK;
}

}