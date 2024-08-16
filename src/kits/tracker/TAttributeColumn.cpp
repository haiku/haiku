#include "TAttributeColumn.h"


#include <utility>
#include <iostream>
#include <OS.h>

#include <fs_attr.h>

#include <Box.h>
#include <Catalog.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <Size.h>
#include <StringView.h>
#include <View.h>

#include "PopUpTextControl.h"
#include "PoseView.h"
#include "TAttributeSearchField.h"
#include "TFindPanel.h"
#include "TitleView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FindPanel"

namespace BPrivate {

static const char* combinationOperators[] = {
	B_TRANSLATE_MARK("and"),
	B_TRANSLATE_MARK("or"),
};

static const int32 combinationOperatorsLength = sizeof(combinationOperators) / sizeof(
	combinationOperators[0]);

static const char* regularAttributeOperators[] = {
	B_TRANSLATE_MARK("contains"),
	B_TRANSLATE_MARK("is"),
	B_TRANSLATE_MARK("is not"),
	B_TRANSLATE_MARK("starts with"),
	B_TRANSLATE_MARK("ends with")
};

static const int32 regularAttributeOperatorsLength = sizeof(regularAttributeOperators) / 
	sizeof(regularAttributeOperators[0]);

static const char* sizeAttributeOperators[] = {
	B_TRANSLATE_MARK("greater than"),
	B_TRANSLATE_MARK("less than"),
	B_TRANSLATE_MARK("is")
};

static const int32 sizeAttributeOperatorsLength = sizeof(sizeAttributeOperators) / sizeof(
	sizeAttributeOperators[0]);

static const char* modifiedAttributeOperators[] = {
	B_TRANSLATE_MARK("before"),
	B_TRANSLATE_MARK("after")
};

static const int32 modifiedAttributeOperatorsLength = sizeof(modifiedAttributeOperators) /
	sizeof(modifiedAttributeOperators[0]);

TAttributeColumn::TAttributeColumn(BColumnTitle* title)
	:
	BView("attribute-column", B_WILL_DRAW),
	fColumnTitle(title)
{
	BRect titleBounds = title->Bounds();
	BSize size(titleBounds.Width(), B_SIZE_UNSET);
	SetSize(size);
}


TAttributeColumn::~TAttributeColumn()
{
	// Empty Destructor
}


status_t
TAttributeColumn::SetSize(BSize size)
{
	fSize = size;
	SetExplicitPreferredSize(fSize);
	SetExplicitSize(fSize);
	ResizeTo(fSize);
	Invalidate();
	
	return B_OK;
}


status_t
TAttributeColumn::GetSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;
	
	*size = fSize;
	return B_OK;
}


status_t
TAttributeColumn::SetColumnTitle(BColumnTitle* title)
{
	if (title == NULL)
		return B_BAD_VALUE;
	
	fColumnTitle = title;
	BRect bounds = title->Bounds();
	BSize size(bounds.Width(), B_SIZE_UNSET);
	
	return SetSize(size);
}


status_t
TAttributeColumn::GetColumnTitle(BColumnTitle** title) const
{
	if (title == NULL)
		return B_BAD_VALUE;

	*title = fColumnTitle;
	return B_OK;
}


status_t
TAttributeColumn::HandleColumnUpdate(float parentHeight)
{
	BRect bounds = fColumnTitle->Bounds();
	BSize size(bounds.Width(), parentHeight);
	
	status_t error;
	if ((error = SetSize(size)) != B_OK)
		return error;
	
	MoveTo(bounds.left, 0);
	return B_OK;
}


BMessage
TAttributeSearchColumn::ArchiveToMessage()
{
	BMessage searchColumnMessage;
	
	int32 attrType = static_cast<int32>(fAttrType);
	searchColumnMessage.AddInt32("attr-type", attrType);
	
	int32 count = fSearchFields->CountItems();
	for (int32 i = 0; i < count; ++i) {
		TAttributeSearchField* searchField = fSearchFields->ItemAt(i);
		BMessage searchFieldMessage = searchField->ArchiveToMessage();
		searchColumnMessage.AddMessage("search-field", &searchFieldMessage);
	}
	return searchColumnMessage;
}


void
TAttributeSearchColumn::RestoreFromMessage(const BMessage* message)
{
	if (message == NULL)
		return;
	
	int32 countOfChildren = fSearchFields->CountItems();
	for (int32 i = 0; i < countOfChildren; ++i) {
		fSearchFields->ItemAt(i)->RemoveSelf();
	}
	fSearchFields->MakeEmpty();
	
	int32 attrType;
	message->FindInt32("attr-type", &attrType);
	fAttrType = static_cast<AttrType>(attrType);
	
	int32 countOfSearchFields = -1;
	message->GetInfo("search-field", NULL, &countOfSearchFields);
	for (int32 i = 0; i < countOfSearchFields; ++i) {
		TAttributeSearchField* searchField = new TAttributeSearchField(this, fAttrType,
			fColumnTitle->Column()->AttrName());
		BMessage searchFieldMessage;
		message->FindMessage("search-field", i, &searchFieldMessage);
		
		const char* textControlLabel;
		searchFieldMessage.FindString("text-control", &textControlLabel);
		searchField->fTextControl->SetText(textControlLabel);
		
		int32 labelCounts = -1;
		searchFieldMessage.GetInfo("menu-item", NULL, &labelCounts);
		std::cout<<labelCounts<<std::endl;
		for (int32 i = 0; i < labelCounts; ++i) {
			const char* label;
			searchFieldMessage.FindString("menu-item", i, &label);
			for (int32 i = 0; i < searchField->fTextControl->PopUpMenu()->CountItems(); ++i) {
				BMenuItem* item = searchField->fTextControl->PopUpMenu()->ItemAt(i);
				if (strcmp(label, item->Label()) == 0) {
					item->SetMarked(true);
				} else {
					item->SetMarked(false);
				}
			}
		}
		
		fSearchFields->AddItem(searchField);
		fContainerView->GroupLayout()->AddView(searchField);
	}
}


void
TAttributeColumn::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kResizeHeight:
		case kMoveColumn:
		{
			float height = 0.0f;
			if (message->FindFloat("height", &height) != B_OK)
				break;
			HandleColumnUpdate(height);
			break;
		}
		
		case kRemoveColumn:
		{
			std::cout<<"I'm in the remover"<<std::endl;
			break;
		}
		
		default:
		{
			__inherited::MessageReceived(message);
			break;
		}
	}
}


TAttributeSearchColumn::TAttributeSearchColumn(BColumnTitle* title, BView* parent, AttrType type)
	:
	TAttributeColumn(title),
	fAttrType(type),
	fSearchFields(new BObjectList<TAttributeSearchField>(10, true)),
	fParent(parent),
	fContainerView(new BGroupView(B_VERTICAL, 2.0f)),
	fBox(new BBox("container-box"))
{
	fContainerView->GroupLayout()->SetInsets(2.0f, 2.0f, 2.0f, 4.0f);
	fBox->AddChild(fContainerView);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.AddGlue()
		.Add(fBox);
		// .AddGlue();
}


TAttributeSearchColumn::~TAttributeSearchColumn()
{
	fSearchFields->MakeEmpty();
	delete fSearchFields;
}


void
TAttributeSearchColumn::SetColumnCombinationMode(ColumnCombinationMode mode)
{
	fCombinationMode = mode;
}


status_t
TAttributeSearchColumn::GetColumnCombinationMode(ColumnCombinationMode* mode) const
{
	if (mode == NULL)
		return B_BAD_VALUE;
	*mode = fCombinationMode;
	return B_OK;
}


status_t
TAttributeSearchColumn::GetPredicateString(BString* predicateString) const
{
	if (predicateString == NULL)
		return B_BAD_VALUE;
	
	BString attrName = fColumnTitle->Column()->AttrName();
	attrName.ReplaceFirst("_stat/", "");
	
	BQuery columnPredicateGenerator;
	int32 numberOfSearchFields = fSearchFields->CountItems();
	for (int32 i = 0; i < numberOfSearchFields; ++i) {
		TAttributeSearchField* searchField = fSearchFields->ItemAt(i);
		
		const char* searchFieldText = searchField->TextControl()->Text();
		if (strcmp(searchFieldText, "") == 0)
			break;

		columnPredicateGenerator.PushAttr(attrName.String());
		columnPredicateGenerator.PushString(searchField->TextControl()->Text(), true);
		query_op attributeOperator;
		searchField->GetAttributeOperator(&attributeOperator);
		columnPredicateGenerator.PushOp(attributeOperator);
		
		query_op combinationOperator;
		if (searchField->GetCombinationOperator(&combinationOperator) == B_OK)
			columnPredicateGenerator.PushOp(combinationOperator);
	}
	
	columnPredicateGenerator.GetPredicate(predicateString);
	return B_OK;
}


status_t
TAttributeSearchColumn::AddSearchField(int32 index)
{
	if (index == -1)
		index = fSearchFields->CountItems();
	
	TAttributeSearchField* searchField = new TAttributeSearchField(this, fAttrType,
		fColumnTitle->Column()->AttrName());
	fSearchFields->AddItem(searchField, index);
	fContainerView->GroupLayout()->AddView(index, searchField);
	
	int32 count = fSearchFields->CountItems();
	TAttributeSearchField* firstSearchField = fSearchFields->ItemAt(0);
	firstSearchField->EnableRemoveButton(count > 1);
	for (int32 i = 0; i < combinationOperatorsLength; ++i) {
		BMenuItem* combinationMenuItem = firstSearchField->TextControl()->PopUpMenu()->ItemAt(i);
		combinationMenuItem->SetEnabled(false);
		combinationMenuItem->SetMarked(false);
	}
	firstSearchField->UpdateLabel();
	
	float height = 0.0f;
	BSize searchFieldHeight;
	status_t error;
	if ((error = firstSearchField->GetRequiredHeight(&searchFieldHeight)) != B_OK)
		return error;
	height += count * searchFieldHeight.height;
	height += count * 3.0f;
	height += 4.0f;
	
	BSize boxHeight(B_SIZE_UNSET, height);
	if ((error = ResizeBox(boxHeight)) != B_OK)
		return error;
	
	return B_OK;
}


status_t
TAttributeSearchColumn::AddSearchField(TAttributeSearchField* after)
{
	if (after == NULL)
		return AddSearchField();

	int32 index = fSearchFields->IndexOf(after);
	if (index == -1)
		return B_BAD_VALUE;
	
	return AddSearchField(index+1);
}


status_t
TAttributeSearchColumn::RemoveSearchField(TAttributeSearchField* field)
{
	if (field == NULL)
		return B_BAD_VALUE;
	
	if (fSearchFields->RemoveItem(field, false)) {
		field->RemoveSelf();
		delete field;
	
		TAttributeSearchField* firstSearchField = fSearchFields->ItemAt(0);
		int32 count = fSearchFields->CountItems();
	
		float height = 0.0f;
		BSize searchFieldHeight;
		status_t error;
		if ((error = firstSearchField->GetRequiredHeight(&searchFieldHeight)) != B_OK)
			return error;
		height += count * searchFieldHeight.height;
		height += count * 3.0f;
		height += 4.0f;
		BSize boxHeight(B_SIZE_UNSET, height);
		ResizeBox(boxHeight);
		return B_OK;
	} else {
		return B_BAD_VALUE;
	}
}


status_t
TAttributeSearchColumn::ResizeBox(BSize size)
{
	fBox->ResizeTo(size);
	fBox->SetExplicitSize(size);
	fBox->SetExplicitPreferredSize(size);
	return B_OK;
}


status_t
TAttributeSearchColumn::GetBoxSize(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;
	
	*size = fBox->PreferredSize();
	return B_OK;
}


status_t
TAttributeSearchColumn::GetRequiredHeight(BSize* size) const
{
	if (size == NULL)
		return B_BAD_VALUE;

	int32 count = fSearchFields->CountItems();
	if (count == 0) {
		*size = BSize(0.0f, 0.0f);
		return B_OK;
	}
	
	return GetBoxSize(size);
}


void
TAttributeSearchColumn::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kModifiedField:
		{
			BString predicateString;
			GetPredicateString(&predicateString);
			BMessenger(fParent).SendMessage(new BMessage(message->what));
			break;
		}
		
		case kAddSearchField:
		{
			TAttributeSearchField* source = NULL;
			if (message->FindPointer("pointer", (void**)&source) != B_OK)
				source = NULL;

			AddSearchField(source);
			BMessenger(fParent).SendMessage(kResizeHeight);
			break;
		}
		
		case kRemoveSearchField:
		{
			TAttributeSearchField* source = NULL;
			if (message->FindPointer("pointer", (void**)&source) != B_OK || source == NULL)
				break;
			
			RemoveSearchField(source);
			BMessenger(fParent).SendMessage(kResizeHeight);
			break;
		}
		
		default:
		{
			__inherited::MessageReceived(message);
			break;
		}
	}
}


TDisabledSearchColumn::TDisabledSearchColumn(BColumnTitle* title)
	:
	TAttributeColumn(title)
{
	BBox* box = new BBox("container-box");
	BGroupView* view = new BGroupView(B_VERTICAL, 0.0f);
	BLayoutBuilder::Group<>(view)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(new BStringView("label", B_TRANSLATE("Can't be queried!")))
			.AddGlue();
	box->AddChild(view);
	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(box);
}


TDisabledSearchColumn::~TDisabledSearchColumn()
{
	// Empty Destructor
}

}