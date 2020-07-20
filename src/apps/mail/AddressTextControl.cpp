/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include "AddressTextControl.h"

#include <Autolock.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Clipboard.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <LayoutUtils.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>

#include "MailApp.h"
#include "Messages.h"
#include "QueryList.h"
#include "TextViewCompleter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddressTextControl"


static const uint32 kMsgAddAddress = 'adad';
static const float kVerticalTextRectInset = 2.0;


class AddressTextControl::TextView : public BTextView {
private:
	static const uint32 MSG_CLEAR = 'cler';

public:
								TextView(AddressTextControl* parent);
	virtual						~TextView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focused = true);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

			const BMessage*		ModificationMessage() const;
			void				SetModificationMessage(BMessage* message);

			void				SetUpdateAutoCompleterChoices(bool update);

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset,
									const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			AddressTextControl*	fAddressTextControl;
			TextViewCompleter*	fAutoCompleter;
			BString				fPreviousText;
			bool				fUpdateAutoCompleterChoices;
			BMessage*			fModificationMessage;
};


class AddressPopUpMenu : public BPopUpMenu, public QueryListener {
public:
								AddressPopUpMenu();
	virtual						~AddressPopUpMenu();

protected:
	virtual	void				EntryCreated(QueryList& source,
									const entry_ref& ref, ino_t node);
	virtual	void				EntryRemoved(QueryList& source,
									const node_ref& nodeRef);

private:
			void				_RebuildMenu();
			void				_AddGroup(const char* label, const char* group,
									PersonList& peopleList);
			void				_AddPeople(BMenu* menu, PersonList& peopleList,
									const char* group,
									bool addSeparator = false);
			bool				_MatchesGroup(const Person& person,
									const char* group);
};


class AddressTextControl::PopUpButton : public BControl {
public:
								PopUpButton();
	virtual						~PopUpButton();

	virtual BSize				MinSize();
	virtual BSize				PreferredSize();
	virtual BSize				MaxSize();

	virtual	void				MouseDown(BPoint where);
	virtual	void				Draw(BRect updateRect);

private:
			AddressPopUpMenu*	fPopUpMenu;
};


class PeopleChoiceModel : public BAutoCompleter::ChoiceModel {
public:
	PeopleChoiceModel()
		:
		fChoices(5, true)
	{
	}

	~PeopleChoiceModel()
	{
	}

	virtual void FetchChoicesFor(const BString& pattern)
	{
		// Remove all existing choices
		fChoices.MakeEmpty();

		// Search through the people list for any matches
		PersonList& peopleList = static_cast<TMailApp*>(be_app)->People();
		BAutolock locker(peopleList);

		for (int32 index = 0; index < peopleList.CountPersons(); index++) {
			const Person* person = peopleList.PersonAt(index);

			const BString& baseText = person->Name();
			for (int32 addressIndex = 0;
					addressIndex < person->CountAddresses(); addressIndex++) {
				BString choiceText = baseText;
				choiceText << " <" << person->AddressAt(addressIndex) << ">";

				int32 match = choiceText.IFindFirst(pattern);
				if (match < 0)
					continue;

				fChoices.AddItem(new BAutoCompleter::Choice(choiceText,
					choiceText, match, pattern.Length()));
			}
		}

		locker.Unlock();
		fChoices.SortItems(_CompareChoices);
	}

	virtual int32 CountChoices() const
	{
		return fChoices.CountItems();
	}

	virtual const BAutoCompleter::Choice* ChoiceAt(int32 index) const
	{
		return fChoices.ItemAt(index);
	}

	static int _CompareChoices(const BAutoCompleter::Choice* a,
		const BAutoCompleter::Choice* b)
	{
		return a->DisplayText().Compare(b->DisplayText());
	}

private:
	BObjectList<BAutoCompleter::Choice> fChoices;
};


// #pragma mark - TextView


AddressTextControl::TextView::TextView(AddressTextControl* parent)
	:
	BTextView("mail"),
	fAddressTextControl(parent),
	fAutoCompleter(new TextViewCompleter(this,
		new PeopleChoiceModel())),
	fPreviousText(""),
	fUpdateAutoCompleterChoices(true)
{
	MakeResizable(true);
	SetStylable(true);
	fAutoCompleter->SetModificationsReported(true);
}


AddressTextControl::TextView::~TextView()
{
	delete fAutoCompleter;
}


void
AddressTextControl::TextView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR:
			SetText("");
			break;

		default:
			BTextView::MessageReceived(message);
			break;
	}
}


void
AddressTextControl::TextView::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_TAB:
			BView::KeyDown(bytes, numBytes);
			break;

		case B_ESCAPE:
			// Revert to text as it was when we received keyboard focus.
			SetText(fPreviousText.String());
			SelectAll();
			break;

		case B_RETURN:
			// Don't let this through to the text view.
			break;

		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}

void
AddressTextControl::TextView::MakeFocus(bool focus)
{
	if (focus == IsFocus())
		return;

	BTextView::MakeFocus(focus);

	if (focus) {
		fPreviousText = Text();
		SelectAll();
	}

	fAddressTextControl->Invalidate();
}


BSize
AddressTextControl::TextView::MinSize()
{
	BSize min;
	min.height = ceilf(LineHeight(0) + kVerticalTextRectInset);
		// we always add at least one pixel vertical inset top/bottom for
		// the text rect.
	min.width = min.height * 3;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), min);
}


BSize
AddressTextControl::TextView::MaxSize()
{
	BSize max(MinSize());
	max.width = B_SIZE_UNLIMITED;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


const BMessage*
AddressTextControl::TextView::ModificationMessage() const
{
	return fModificationMessage;
}


void
AddressTextControl::TextView::SetModificationMessage(BMessage* message)
{
	fModificationMessage = message;
}


void
AddressTextControl::TextView::SetUpdateAutoCompleterChoices(bool update)
{
	fUpdateAutoCompleterChoices = update;
}


void
AddressTextControl::TextView::InsertText(const char* text,
	int32 length, int32 offset, const text_run_array* runs)
{
	if (!strncmp(text, "mailto:", 7)) {
		text += 7;
		length -= 7;
		if (runs != NULL)
			runs = NULL;
	}

	// Filter all line breaks, note that text is not terminated.
	if (length == 1) {
		if (*text == '\n' || *text == '\r')
			BTextView::InsertText(" ", 1, offset, runs);
		else
			BTextView::InsertText(text, 1, offset, runs);
	} else {
		BString filteredText(text, length);
		filteredText.ReplaceAll('\n', ' ');
		filteredText.ReplaceAll('\r', ' ');
		BTextView::InsertText(filteredText.String(), length, offset,
			runs);
	}

	// TODO: change E-mail representation
/*
	// Make the base URL part bold.
	BString text(Text(), TextLength());
	int32 baseUrlStart = text.FindFirst("://");
	if (baseUrlStart >= 0)
		baseUrlStart += 3;
	else
		baseUrlStart = 0;
	int32 baseUrlEnd = text.FindFirst("/", baseUrlStart);
	if (baseUrlEnd < 0)
		baseUrlEnd = TextLength();

	BFont font;
	GetFont(&font);
	const rgb_color black = (rgb_color) { 0, 0, 0, 255 };
	const rgb_color gray = (rgb_color) { 60, 60, 60, 255 };
	if (baseUrlStart > 0)
		SetFontAndColor(0, baseUrlStart, &font, B_FONT_ALL, &gray);
	if (baseUrlEnd > baseUrlStart) {
		font.SetFace(B_BOLD_FACE);
		SetFontAndColor(baseUrlStart, baseUrlEnd, &font, B_FONT_ALL, &black);
	}
	if (baseUrlEnd < TextLength()) {
		font.SetFace(B_REGULAR_FACE);
		SetFontAndColor(baseUrlEnd, TextLength(), &font, B_FONT_ALL, &gray);
	}
*/
	fAutoCompleter->TextModified(fUpdateAutoCompleterChoices);
	fAddressTextControl->InvokeNotify(fModificationMessage,
		B_CONTROL_MODIFIED);
}


void
AddressTextControl::TextView::DeleteText(int32 fromOffset,
	int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);

	fAutoCompleter->TextModified(fUpdateAutoCompleterChoices);
	fAddressTextControl->InvokeNotify(fModificationMessage,
		B_CONTROL_MODIFIED);
}


//	#pragma mark - PopUpButton


AddressTextControl::PopUpButton::PopUpButton()
	:
	BControl(NULL, NULL, NULL, B_WILL_DRAW)
{
	fPopUpMenu = new AddressPopUpMenu();
}


AddressTextControl::PopUpButton::~PopUpButton()
{
	delete fPopUpMenu;
}


BSize
AddressTextControl::PopUpButton::MinSize()
{
	// TODO: BControlLook does not give us any size information!
	return BSize(10, 10);
}


BSize
AddressTextControl::PopUpButton::PreferredSize()
{
	return BSize(10, B_SIZE_UNSET);
}


BSize
AddressTextControl::PopUpButton::MaxSize()
{
	return BSize(10, B_SIZE_UNLIMITED);
}


void
AddressTextControl::PopUpButton::MouseDown(BPoint where)
{
	if (fPopUpMenu->Parent() != NULL)
		return;

	float width;
	fPopUpMenu->GetPreferredSize(&width, NULL);
	fPopUpMenu->SetTargetForItems(Parent());

	BPoint point(Bounds().Width() - width, Bounds().Height() + 2);
	ConvertToScreen(&point);
	fPopUpMenu->Go(point, true, true, true);
}


void
AddressTextControl::PopUpButton::Draw(BRect updateRect)
{
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;

	if (IsFocus() && Window()->IsActive())
		flags |= BControlLook::B_FOCUSED;

	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
	BRect rect = Bounds();
	be_control_look->DrawMenuFieldBackground(this, rect,
		updateRect, base, true, flags);
}


//	#pragma mark - PopUpMenu


AddressPopUpMenu::AddressPopUpMenu()
	:
	BPopUpMenu("", true)
{
	static_cast<TMailApp*>(be_app)->PeopleQueryList().AddListener(this);
}


AddressPopUpMenu::~AddressPopUpMenu()
{
	static_cast<TMailApp*>(be_app)->PeopleQueryList().RemoveListener(this);
}


void
AddressPopUpMenu::EntryCreated(QueryList& source,
	const entry_ref& ref, ino_t node)
{
	_RebuildMenu();
}


void
AddressPopUpMenu::EntryRemoved(QueryList& source,
	const node_ref& nodeRef)
{
	_RebuildMenu();
}


void
AddressPopUpMenu::_RebuildMenu()
{
	// Remove all items
	int32 index = CountItems();
	while (index-- > 0) {
		 delete RemoveItem(index);
	}

	// Rebuild contents
	PersonList& peopleList = static_cast<TMailApp*>(be_app)->People();
	BAutolock locker(peopleList);

	if (peopleList.CountPersons() > 0)
		_AddGroup(B_TRANSLATE("All people"), NULL, peopleList);

	GroupList& groupList = static_cast<TMailApp*>(be_app)->PeopleGroups();
	BAutolock groupLocker(groupList);

	for (int32 index = 0; index < groupList.CountGroups(); index++) {
		BString group = groupList.GroupAt(index);
		_AddGroup(group, group, peopleList);
	}

	groupLocker.Unlock();

	_AddPeople(this, peopleList, "", true);
}


void
AddressPopUpMenu::_AddGroup(const char* label, const char* group,
	PersonList& peopleList)
{
	BMenu* menu = new BMenu(label);
	AddItem(menu);
	menu->Superitem()->SetMessage(new BMessage(kMsgAddAddress));

	_AddPeople(menu, peopleList, group);
}


void
AddressPopUpMenu::_AddPeople(BMenu* menu, PersonList& peopleList,
	const char* group, bool addSeparator)
{
	for (int32 index = 0; index < peopleList.CountPersons(); index++) {
		const Person* person = peopleList.PersonAt(index);
		if (!_MatchesGroup(*person, group))
			continue;

		if (person->CountAddresses() != 0 && addSeparator) {
			menu->AddSeparatorItem();
			addSeparator = false;
		}

		for (int32 addressIndex = 0; addressIndex < person->CountAddresses();
				addressIndex++) {
			BString email = person->Name();
			email << " <" << person->AddressAt(addressIndex) << ">";

			BMessage* message = new BMessage(kMsgAddAddress);
			message->AddString("email", email);
			menu->AddItem(new BMenuItem(email, message));

			if (menu->Superitem() != NULL)
				menu->Superitem()->Message()->AddString("email", email);
		}
	}
}


bool
AddressPopUpMenu::_MatchesGroup(const Person& person, const char* group)
{
	if (group == NULL)
		return true;

	if (group[0] == '\0')
		return person.CountGroups() == 0;

	return person.IsInGroup(group);
}


// TODO: sort lists!
/*
void
AddressTextControl::PopUpMenu::_AddPersonItem(const entry_ref *ref, ino_t node, BString &name,
	BString &email, const char *attr, BMenu *groupMenu, BMenuItem *superItem)
{
	BString	label;
	BString	sortKey;
		// For alphabetical order sorting, usually last name.

	// if we have no Name, just use the email address
	if (name.Length() == 0) {
		label = email;
		sortKey = email;
	} else {
		// otherwise, pretty-format it
		label << name << " (" << email << ")";

		// Extract the last name (last word in the name),
		// removing trailing and leading spaces.
		const char *nameStart = name.String();
		const char *string = nameStart + strlen(nameStart) - 1;
		const char *wordEnd;

		while (string >= nameStart && isspace(*string))
			string--;
		wordEnd = string + 1; // Points to just after last word.
		while (string >= nameStart && !isspace(*string))
			string--;
		string++; // Point to first letter in the word.
		if (wordEnd > string)
			sortKey.SetTo(string, wordEnd - string);
		else // Blank name, pretend that the last name is after it.
			string = nameStart + strlen(nameStart);

		// Append the first names to the end, so that people with the same last
		// name get sorted by first name.  Note no space between the end of the
		// last name and the start of the first names, but that shouldn't
		// matter for sorting.
		sortKey.Append(nameStart, string - nameStart);
	}
}
*/

// #pragma mark - AddressTextControl


AddressTextControl::AddressTextControl(const char* name, BMessage* message)
	:
	BControl(name, NULL, message, B_WILL_DRAW),
	fRefDropMenu(NULL),
	fWindowActive(false),
	fEditable(true)
{
	fTextView = new TextView(this);
	fTextView->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
	fPopUpButton = new PopUpButton();

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.SetInsets(2)
		.Add(fTextView)
		.Add(fPopUpButton);

	SetFlags(Flags() | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	SetLowUIColor(ViewUIColor());
	SetViewUIColor(fTextView->ViewUIColor());

	SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_CENTER));

	SetEnabled(fEditable);
		// Sets the B_NAVIGABLE flag on the TextView
}


AddressTextControl::~AddressTextControl()
{
}


void
AddressTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();
	fWindowActive = Window()->IsActive();
}


void
AddressTextControl::WindowActivated(bool active)
{
	BControl::WindowActivated(active);
	if (fWindowActive != active) {
		fWindowActive = active;
		Invalidate();
	}
}


void
AddressTextControl::Draw(BRect updateRect)
{
	if (!IsEditable())
		return;

	BRect bounds(Bounds());
	rgb_color base(LowColor());
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= BControlLook::B_DISABLED;
	if (fWindowActive && fTextView->IsFocus())
		flags |= BControlLook::B_FOCUSED;
	be_control_look->DrawTextControlBorder(this, bounds, updateRect, base,
		flags);
}


void
AddressTextControl::MakeFocus(bool focus)
{
	// Forward this to the text view, we never accept focus ourselves.
	fTextView->MakeFocus(focus);
}


void
AddressTextControl::SetEnabled(bool enabled)
{
	BControl::SetEnabled(enabled);
	fTextView->MakeEditable(enabled && fEditable);
	if (enabled)
		fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	else
		fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);

	fPopUpButton->SetEnabled(enabled);

	_UpdateTextViewColors();
}


void
AddressTextControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		{
			int32 buttons = -1;
			BPoint point;
			if (message->FindInt32("buttons", &buttons) != B_OK)
				buttons = B_PRIMARY_MOUSE_BUTTON;

			if (buttons != B_PRIMARY_MOUSE_BUTTON
				&& message->FindPoint("_drop_point_", &point) != B_OK)
				return;

			BMessage forwardRefs(B_REFS_RECEIVED);
			bool forward = false;

			entry_ref ref;
			for (int32 index = 0;message->FindRef("refs", index, &ref) == B_OK; index++) {
				BFile file(&ref, B_READ_ONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					BNodeInfo info(&file);
					char type[B_FILE_NAME_LENGTH];
					info.GetType(type);

					if (!strcmp(type,"application/x-person")) {
						// add person's E-mail address to the To: field

						BString attr = "";
						if (buttons == B_PRIMARY_MOUSE_BUTTON) {
							if (message->FindString("attr", &attr) < B_OK)
								attr = "META:email";
						} else {
							BNode node(&ref);
							node.RewindAttrs();

							char buffer[B_ATTR_NAME_LENGTH];

							delete fRefDropMenu;
							fRefDropMenu = new BPopUpMenu("RecipientMenu");

							while (node.GetNextAttrName(buffer) == B_OK) {
								if (strstr(buffer, "email") == NULL)
									continue;

								attr = buffer;

								BString address;
								node.ReadAttrString(buffer, &address);
								if (address.Length() <= 0)
									continue;

								BMessage* itemMsg
									= new BMessage(kMsgAddAddress);
								itemMsg->AddString("email", address.String());

								BMenuItem* item = new BMenuItem(
									address.String(), itemMsg);
								fRefDropMenu->AddItem(item);
							}

							if (fRefDropMenu->CountItems() > 1) {
								fRefDropMenu->SetTargetForItems(this);
								fRefDropMenu->Go(point, true, true, true);
								return;
							} else {
								delete fRefDropMenu;
								fRefDropMenu = NULL;
							}
						}

						BString email;
						file.ReadAttrString(attr.String(), &email);

						// we got something...
						if (email.Length() > 0) {
							// see if we can get a username as well
							BString name;
							file.ReadAttrString("META:name", &name);

							BString	address;
							if (name.Length() == 0) {
								// if we have no Name, just use the email address
								address = email;
							} else {
								// otherwise, pretty-format it
								address << "\"" << name << "\" <" << email << ">";
							}

							_AddAddress(address);
						}
					} else {
						forward = true;
						forwardRefs.AddRef("refs", &ref);
					}
				}
			}

			if (forward) {
				// Pass on to parent
				Window()->PostMessage(&forwardRefs, Parent());
			}
			break;
		}

		case M_SELECT:
		{
			BTextView *textView = (BTextView *)ChildAt(0);
			if (textView != NULL)
				textView->Select(0, textView->TextLength());
			break;
		}

		case kMsgAddAddress:
		{
			const char* email;
			for (int32 index = 0;
					message->FindString("email", index++, &email) == B_OK;)
				_AddAddress(email);
			break;
		}

		default:
			BControl::MessageReceived(message);
			break;
	}
}


const BMessage*
AddressTextControl::ModificationMessage() const
{
	return fTextView->ModificationMessage();
}


void
AddressTextControl::SetModificationMessage(BMessage* message)
{
	fTextView->SetModificationMessage(message);
}


bool
AddressTextControl::IsEditable() const
{
	return fEditable;
}


void
AddressTextControl::SetEditable(bool editable)
{
	fTextView->MakeEditable(IsEnabled() && editable);
	fTextView->MakeSelectable(IsEnabled() && editable);
	fEditable = editable;

	if (editable && fPopUpButton->IsHidden(this))
		fPopUpButton->Show();
	else if (!editable && !fPopUpButton->IsHidden(this))
		fPopUpButton->Hide();
}


void
AddressTextControl::SetText(const char* text)
{
	if (text == NULL || Text() == NULL || strcmp(Text(), text) != 0) {
		fTextView->SetUpdateAutoCompleterChoices(false);
		fTextView->SetText(text);
		fTextView->SetUpdateAutoCompleterChoices(true);
	}
}


const char*
AddressTextControl::Text() const
{
	return fTextView->Text();
}


int32
AddressTextControl::TextLength() const
{
	return fTextView->TextLength();
}


void
AddressTextControl::GetSelection(int32* start, int32* end) const
{
	fTextView->GetSelection(start, end);
}


void
AddressTextControl::Select(int32 start, int32 end)
{
	fTextView->Select(start, end);
}


void
AddressTextControl::SelectAll()
{
	fTextView->Select(0, TextLength());
}


bool
AddressTextControl::HasFocus()
{
	return fTextView->IsFocus();
}


void
AddressTextControl::_AddAddress(const char* text)
{
	int last = fTextView->TextLength();
	if (last != 0) {
		fTextView->Select(last, last);
		// TODO: test if there is already a ','
		fTextView->Insert(", ");
	}
	fTextView->Insert(text);
}


void
AddressTextControl::_UpdateTextViewColors()
{
	BFont font;
	fTextView->GetFontAndColor(0, &font);

	rgb_color textColor;
	if (!IsEditable() || IsEnabled())
		textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	else {
		textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT);
	}

	fTextView->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	rgb_color color;
	if (!IsEditable())
		color = ui_color(B_PANEL_BACKGROUND_COLOR);
	else if (IsEnabled())
		color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	else {
		color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT);
	}

	fTextView->SetViewColor(color);
	fTextView->SetLowColor(color);
}
