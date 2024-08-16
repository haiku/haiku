#include "PopUpTextControl.h"


#include <Catalog.h>
#include <ControlLook.h>
#include <TextControl.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "TAttributeSearchField.h"
#include "TAttributeColumn.h"
#include "TFindPanel.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PopUpTextControl"


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


PopUpTextControl::PopUpTextControl(const char* name, const char* label, const char* value,
	BHandler* target)
	:
	BTextControl(name, label, value, nullptr, B_WILL_DRAW | B_NAVIGABLE | B_DRAW_ON_CHILDREN),
	fOptionsMenu(new BPopUpMenu("options-menu",false, false)),
	fTarget(target)
{
	// Empty Constructor
}


void
PopUpTextControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMenuOptionClicked:
		{
			BMessenger(fTarget).SendMessage(message);
			break;
		}
		default:
			BTextControl::MessageReceived(message);
			break;
	}
}


void
PopUpTextControl::LayoutChanged()
{
	BRect frame = TextView()->Frame();
	frame.left = BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING) + 2;
	TextView()->MoveTo(frame.left, frame.top);
	TextView()->ResizeTo(Bounds().Width() - frame.left - 2, frame.Height());
	
	BTextControl::LayoutChanged();
}


void
PopUpTextControl::DrawAfterChildren(BRect update)
{
	BRect frame = Bounds();
	frame.right = frame.left + BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
	frame.top++;
	frame.bottom --;
	be_control_look->DrawMenuFieldBackground(this, frame, update,
		ui_color(B_PANEL_BACKGROUND_COLOR), true);
}


void
PopUpTextControl::MouseDown(BPoint where)
{
	BRect frame = Bounds();
	if (where.x < BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING)) {
		BMenuItem* item = fOptionsMenu->Go(TextView()->ConvertToScreen(
			TextView()->Bounds().LeftTop()));
		if (item) {
			BMessage* message = item->Message();
			MessageReceived(message);
		}
	} else {
		BTextControl::MouseDown(where);
	}
}

}
