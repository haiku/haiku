/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "AttributeWindow.h"
#include "ProbeView.h"

#include <MenuBar.h>
#include <MenuItem.h>
#include <TabView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <ScrollView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Alert.h>
#include <Autolock.h>
#include <Mime.h>
#include <fs_attr.h>

#include <stdio.h>


static const uint32 kMsgRemoveAttribute = 'rmat';
static const uint32 kMsgValueChanged = 'vlch';
static const uint32 kMimeTypeItem = 'miti';


class EditorTabView : public BTabView {
	public:
		EditorTabView(BRect frame, const char *name, button_width width = B_WIDTH_AS_USUAL,
			uint32 resizingMode = B_FOLLOW_ALL, uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);

		virtual void FrameResized(float width, float height);
		virtual void Select(int32 tab);

		void AddRawEditorTab(BView *view);
		void SetTypeEditorTab(BView *view);

	private:
		BView		*fRawEditorView;
		BView		*fTypeEditorView;
		BStringView	*fNoEditorView;
		int32		fRawTab;
};


class StringEditor : public BView {
	public:
		StringEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		DataEditor	&fEditor;
		BTextView	*fTextView;
};


class MimeTypeEditor : public BView {
	public:
		MimeTypeEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateText();

	private:
		void AddMimeTypesToMenu();

		DataEditor		&fEditor;
		BMenu			*fMimeTypeMenu;
		BTextControl	*fTextControl;
		BString			fPreviousText;
};


class NumberEditor : public BView {
	public:
		NumberEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateText();
		void UpdateNumber();

	private:
		const char *TypeLabel();
		status_t Format(char *buffer);
		size_t Size();

		DataEditor		&fEditor;
		BTextControl	*fTextControl;
		BString			fPreviousText;
};


class BooleanEditor : public BView {
	public:
		BooleanEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateMenuField();

	private:
		DataEditor	&fEditor;
		BMenuItem	*fFalseMenuItem;
		BMenuItem	*fTrueMenuItem;
};


//-----------------


static BView *
GetTypeEditorFor(BRect rect, DataEditor &editor)
{
	switch (editor.Type()) {
		case B_STRING_TYPE:
			return new StringEditor(rect, editor);
		case B_MIME_STRING_TYPE:
			return new MimeTypeEditor(rect, editor);
		case B_BOOL_TYPE:
			return new BooleanEditor(rect, editor);
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
		case B_SSIZE_T_TYPE:
		case B_SIZE_T_TYPE:
		case B_OFF_T_TYPE:
		case B_POINTER_TYPE:
			return new NumberEditor(rect, editor);
	}

	return NULL;
}


//	#pragma mark -


EditorTabView::EditorTabView(BRect frame, const char *name, button_width width,
	uint32 resizingMode, uint32 flags)
	: BTabView(frame, name, width, resizingMode, flags),
	fRawEditorView(NULL),
	fRawTab(-1)
{
	ContainerView()->MoveBy(-ContainerView()->Frame().left,
						TabHeight() + 1 - ContainerView()->Frame().top);
	fNoEditorView = new BStringView(ContainerView()->Bounds(), "Type Editor",
							"No type editor available", B_FOLLOW_NONE);
	fNoEditorView->ResizeToPreferred();
	fNoEditorView->SetAlignment(B_ALIGN_CENTER);
	fTypeEditorView = fNoEditorView;

	FrameResized(0, 0);

	SetTypeEditorTab(NULL);
}


void 
EditorTabView::FrameResized(float width, float height)
{
	BRect rect = Bounds();
	rect.top = ContainerView()->Frame().top;

	ContainerView()->ResizeTo(rect.Width(), rect.Height());

	BView *view = fTypeEditorView;
	if (view == NULL)
		view = fNoEditorView;

	BPoint point = view->Frame().LeftTop();
	if ((view->ResizingMode() & B_FOLLOW_RIGHT) == 0)
		point.x = (rect.Width() - view->Bounds().Width()) / 2;
	if ((view->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
		point.y = (rect.Height() - view->Bounds().Height()) / 2;

	view->MoveTo(point);
}


void 
EditorTabView::Select(int32 tab)
{
	if (tab != fRawTab && fRawEditorView != NULL && !fRawEditorView->IsHidden(fRawEditorView))
		fRawEditorView->Hide();

	BTabView::Select(tab);

	BView *view;
	if (tab == fRawTab && fRawEditorView != NULL) {
		if (fRawEditorView->IsHidden(fRawEditorView))
			fRawEditorView->Show();
		view = fRawEditorView;
	} else
		view = ViewForTab(Selection());

	if (view != NULL && (view->ResizingMode() & (B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM)) != 0) {
		BRect rect = ContainerView()->Bounds();

		BRect frame = view->Frame();
		rect.left = frame.left;
		rect.top = frame.top;
		if ((view->ResizingMode() & B_FOLLOW_RIGHT) == 0)
			rect.right = frame.right;
		if ((view->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
			rect.bottom = frame.bottom;

		view->ResizeTo(rect.Width(), rect.Height());
	}
}


void
EditorTabView::AddRawEditorTab(BView *view)
{
	fRawEditorView = view;
	if (view != NULL)
		ContainerView()->AddChild(view);

	fRawTab = CountTabs();

	view = new BView(BRect(0, 0, 5, 5), "Raw Editor", B_FOLLOW_NONE, 0);
	view->SetViewColor(ViewColor());
	AddTab(view);
}


void 
EditorTabView::SetTypeEditorTab(BView *view)
{
	if (fTypeEditorView == view)
		return;

	BTab *tab = TabAt(0);
	if (tab != NULL)
		tab->SetView(NULL);

	fTypeEditorView = view;

	if (view == NULL)
		view = fNoEditorView;

	if (CountTabs() == 0)
		AddTab(view);
	else
		tab->SetView(view);

	FrameResized(0, 0);
	Select(0);
}


//	#pragma mark -


StringEditor::StringEditor(BRect rect, DataEditor &editor)
	: BView(rect, "String Editor", B_FOLLOW_ALL, 0),
	fEditor(editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView *stringView = new BStringView(BRect(5, 5, rect.right, 20),
										B_EMPTY_STRING, "Contents:");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = Bounds();
	rect.top = stringView->Frame().bottom + 5;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	fTextView = new BTextView(rect, B_EMPTY_STRING, rect.OffsetToCopy(B_ORIGIN).InsetByCopy(5, 5),
						B_FOLLOW_ALL, B_WILL_DRAW);

	if (fEditor.Lock()) {
		size_t viewSize = fEditor.ViewSize();
		// that may need some more memory...
		if (viewSize < fEditor.FileSize())
			fEditor.SetViewSize(fEditor.FileSize());

		const char *buffer;
		if (fEditor.GetViewBuffer((const uint8 **)&buffer) == B_OK)
			fTextView->SetText(buffer);

		// restore old view size
		fEditor.SetViewSize(viewSize);

		fEditor.Unlock();
	}
#if 0
	char *data = (char *)malloc(info.size);
	if (data != NULL) {
		if (fNode.ReadAttr(attribute, info.type, 0LL, data, info.size) >= B_OK)
			fTextView->SetText(data);

		free(data);
	}
#endif

	BScrollView *scrollView = new BScrollView("scroller", fTextView, B_FOLLOW_ALL, B_WILL_DRAW, true, true);
	AddChild(scrollView);
}


void
StringEditor::AttachedToWindow()
{
	fEditor.StartWatching(this);
}


void 
StringEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void
StringEditor::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}


//	#pragma mark -


MimeTypeEditor::MimeTypeEditor(BRect rect, DataEditor &editor)
	: BView(rect, "MIME Type Editor", B_FOLLOW_NONE, 0),
	fEditor(editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	rect.right -= 100;
	fTextControl = new BTextControl(rect.InsetByCopy(5, 5), B_EMPTY_STRING, "MIME Type:", NULL,
							new BMessage(kMsgValueChanged), B_FOLLOW_ALL);
	fTextControl->SetDivider(StringWidth(fTextControl->Label()) + 8);

	fMimeTypeMenu = new BMenu(B_EMPTY_STRING);
	rect = fTextControl->Frame();
	rect.left = rect.right + 5;
	rect.right = Bounds().right;
	//rect.top++;
	BMenuField *menuField = new BMenuField(rect, NULL, NULL, fMimeTypeMenu);
	menuField->SetDivider(0);
	menuField->ResizeToPreferred();
	AddChild(menuField);

	ResizeTo(fTextControl->Bounds().Width() + menuField->Bounds().Width() + 10,
		menuField->Bounds().Height() + 10);

	AddChild(fTextControl);
}


void 
MimeTypeEditor::UpdateText()
{
	if (fEditor.Lock()) {
		const char *mimeType;
		if (fEditor.GetViewBuffer((const uint8 **)&mimeType) == B_OK) {
			fTextControl->SetText(mimeType);
			fPreviousText.SetTo(mimeType);
		}

		fEditor.Unlock();
	}
}


void
MimeTypeEditor::AttachedToWindow()
{
	fTextControl->SetTarget(this);
	fEditor.StartWatching(this);

	AddMimeTypesToMenu();
	UpdateText();
}


void 
MimeTypeEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);
	
	if (fPreviousText != fTextControl->Text()) {
		BAutolock locker(fEditor);
		fEditor.Replace(0, (const uint8 *)fTextControl->Text(),
			strlen(fTextControl->Text()) + 1);
	}
}


void
MimeTypeEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgValueChanged:
		{
			BAutolock locker(fEditor);
			fEditor.Replace(0, (const uint8 *)fTextControl->Text(),
				strlen(fTextControl->Text()) + 1);
			break;
		}

		case kMsgDataEditorUpdate:
			UpdateText();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
MimeTypeEditor::AddMimeTypesToMenu()
{
	// add MIME type tree list

	BMessage types;
	if (BMimeType::GetInstalledSupertypes(&types) == B_OK) {
		const char *superType;
		int32 index = 0;

		while (types.FindString("super_types", index++, &superType) == B_OK) {
			BMenu *superMenu = new BMenu(superType);

			// ToDo: there are way too many "application" types... (need to lay off to another thread)
			if (!strcmp(superType, "application"))
				continue;

			BMessage subTypes;
			if (BMimeType::GetInstalledTypes(superType, &subTypes) == B_OK) {
				const char *type;
				int32 subIndex = 0;
				while (subTypes.FindString("types", subIndex++, &type) == B_OK) {
					BMessage *message = new BMessage(kMimeTypeItem);
					message->AddString("super_type", superType);
					message->AddString("mime_type", type);
					superMenu->AddItem(new BMenuItem(strchr(type, '/') + 1, message));
				}
		 	}
		 	if (superMenu->CountItems() != 0) {
				fMimeTypeMenu->AddItem(new BMenuItem(superMenu));

				// the MimeTypeMenu's font is not correct at this time
				superMenu->SetFont(be_plain_font);
				superMenu->SetTargetForItems(this);
			} else
				delete superMenu;

		}
	}
}


//	#pragma mark -


NumberEditor::NumberEditor(BRect rect, DataEditor &editor)
	: BView(rect, "Number Editor", B_FOLLOW_LEFT_RIGHT, 0),
	fEditor(editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTextControl = new BTextControl(rect.InsetByCopy(5, 5), B_EMPTY_STRING, TypeLabel(), NULL,
							new BMessage(kMsgValueChanged), B_FOLLOW_ALL);
	fTextControl->SetDivider(StringWidth(fTextControl->Label()) + 8);
	fTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	ResizeTo(rect.Width(), 30);

	AddChild(fTextControl);
}


void 
NumberEditor::UpdateText()
{
	if (fEditor.Lock()) {
		const char *number;
		if (fEditor.GetViewBuffer((const uint8 **)&number) == B_OK) {
			char buffer[64];
			char format[16];
			switch (fEditor.Type()) {
				case B_FLOAT_TYPE:
				{
					float value = *(float *)number;
					snprintf(buffer, sizeof(buffer), "%g", value);
					break;
				}
				case B_DOUBLE_TYPE:
				{
					double value = *(double *)number;
					snprintf(buffer, sizeof(buffer), "%g", value);
					break;
				}
				case B_SSIZE_T_TYPE:
				case B_INT8_TYPE:
				case B_INT16_TYPE:
				case B_INT32_TYPE:
				case B_INT64_TYPE:
				case B_OFF_T_TYPE:
				{
					Format(format);
					switch (Size()) {
						case 1:
						{
							int8 value = *(int8 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 2:
						{
							int16 value = *(int16 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 4:
						{
							int32 value = *(int32 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 8:
						{
							int64 value = *(int64 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
					}
					break;
				}
				default:
				{
					Format(format);
					switch (Size()) {
						case 1:
						{
							uint8 value = *(uint8 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 2:
						{
							uint16 value = *(uint16 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 4:
						{
							uint32 value = *(uint32 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
						case 8:
						{
							uint64 value = *(uint64 *)number;
							snprintf(buffer, sizeof(buffer), format, value);
							break;
						}
					}
					break;
				}
			}
			fTextControl->SetText(buffer);
			fPreviousText.SetTo(buffer);
		}

		fEditor.Unlock();
	}
}


void
NumberEditor::UpdateNumber()
{
	const char *number = fTextControl->Text();
	uint8 buffer[8];

	switch (fEditor.Type()) {
		case B_FLOAT_TYPE:
		{
			float value = strtod(number, NULL);
			*(float *)buffer = value;
			break;
		}
		case B_DOUBLE_TYPE:
		{
			double value = strtod(number, NULL);
			*(double *)buffer = value;
			break;
		}
		case B_INT8_TYPE:
		{
			int64 value = strtoll(number, NULL, 0);
			if (value > CHAR_MAX)
				value = CHAR_MAX;
			else if (value < CHAR_MIN)
				value = CHAR_MIN;
			*(int8 *)buffer = (int8)value;
			break;
		}
		case B_UINT8_TYPE:
		{
			int64 value = strtoull(number, NULL, 0);
			if (value > UCHAR_MAX)
				value = UCHAR_MAX;
			*(uint8 *)buffer = (uint8)value;
			break;
		}
		case B_INT16_TYPE:
		{
			int64 value = strtoll(number, NULL, 0);
			if (value > SHRT_MAX)
				value = SHRT_MAX;
			else if (value < SHRT_MIN)
				value = SHRT_MIN;
			*(int16 *)buffer = (int16)value;
			break;
		}
		case B_UINT16_TYPE:
		{
			int64 value = strtoull(number, NULL, 0);
			if (value > USHRT_MAX)
				value = USHRT_MAX;
			*(uint16 *)buffer = (uint16)value;
			break;
		}
		case B_INT32_TYPE:
		case B_SSIZE_T_TYPE:
		{
			int64 value = strtoll(number, NULL, 0);
			if (value > LONG_MAX)
				value = LONG_MAX;
			else if (value < LONG_MIN)
				value = LONG_MIN;
			*(int32 *)buffer = (int32)value;
			break;
		}
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_POINTER_TYPE:
		{
			uint64 value = strtoull(number, NULL, 0);
			if (value > ULONG_MAX)
				value = ULONG_MAX;
			*(uint32 *)buffer = (uint32)value;
			break;
		}
		case B_INT64_TYPE:
		case B_OFF_T_TYPE:
		{
			int64 value = strtoll(number, NULL, 0);
			*(int64 *)buffer = value;
			break;
		}
		case B_UINT64_TYPE:
		{
			uint64 value = strtoull(number, NULL, 0);
			*(uint64 *)buffer = value;
			break;
		}
		default:
			return;
	}

	BAutolock locker(fEditor);
	fEditor.Replace(0, buffer, Size());
}


const char *
NumberEditor::TypeLabel()
{
	switch (fEditor.Type()) {
		case B_INT8_TYPE:
			return "8 Bit Signed Value:";
		case B_UINT8_TYPE:
			return "8 Bit Unsigned Value:";
		case B_INT16_TYPE:
			return "16 Bit Signed Value:";
		case B_UINT16_TYPE:
			return "16 Bit Unsigned Value:";
		case B_INT32_TYPE:
			return "32 Bit Signed Value:";
		case B_UINT32_TYPE:
			return "32 Bit Unsigned Value:";
		case B_INT64_TYPE:
			return "64 Bit Signed Value:";
		case B_UINT64_TYPE:
			return "64 Bit Unsigned Value:";
		case B_FLOAT_TYPE:
			return "Floating-Point Value:";
		case B_DOUBLE_TYPE:
			return "Double Precision Floating-Point Value:";
		case B_SSIZE_T_TYPE:
			return "32 Bit Size or Status:";
		case B_SIZE_T_TYPE:
			return "32 Bit Unsigned Size:";
		case B_OFF_T_TYPE:
			return "64 Bit Signed Offset:";
		case B_POINTER_TYPE:
			return "32 Bit Unsigned Pointer:";
		default:
			return "Number:";
	}
}


size_t 
NumberEditor::Size()
{
	switch (fEditor.Type()) {
		case B_INT8_TYPE:
			return 1;
		case B_UINT8_TYPE:
			return 1;
		case B_INT16_TYPE:
			return 2;
		case B_UINT16_TYPE:
			return 2;
		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
			return 4;
		case B_SIZE_T_TYPE:
		case B_POINTER_TYPE:
		case B_UINT32_TYPE:
			return 4;
		case B_INT64_TYPE:
		case B_OFF_T_TYPE:
			return 8;
		case B_UINT64_TYPE:
			return 8;
		case B_FLOAT_TYPE:
			return 4;
		case B_DOUBLE_TYPE:
			return 8;
		default:
			return 0;
	}
}


status_t
NumberEditor::Format(char *buffer)
{
	switch (fEditor.Type()) {
		case B_INT8_TYPE:
			strcpy(buffer, "%hd");
			return B_OK;
		case B_UINT8_TYPE:
			strcpy(buffer, "%hu");
			return B_OK;
		case B_INT16_TYPE:
			strcpy(buffer, "%hd");
			return B_OK;
		case B_UINT16_TYPE:
			strcpy(buffer, "%hu");
			return B_OK;
		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
			strcpy(buffer, "%ld");
			return B_OK;
		case B_SIZE_T_TYPE:
		case B_POINTER_TYPE:
		case B_UINT32_TYPE:
			strcpy(buffer, "%lu");
			return B_OK;
		case B_INT64_TYPE:
		case B_OFF_T_TYPE:
			strcpy(buffer, "%Ld");
			return B_OK;
		case B_UINT64_TYPE:
			strcpy(buffer, "%Lu");
			return B_OK;
		case B_FLOAT_TYPE:
			strcpy(buffer, "%g");
			return B_OK;
		case B_DOUBLE_TYPE:
			strcpy(buffer, "%lg");
			return B_OK;

		default:
			return B_ERROR;
	}
}


void
NumberEditor::AttachedToWindow()
{
	fTextControl->SetTarget(this);
	fEditor.StartWatching(this);

	UpdateText();
}


void 
NumberEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);

	if (fPreviousText != fTextControl->Text())
		UpdateNumber();
}


void
NumberEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgValueChanged:
			UpdateNumber();
			break;
		case kMsgDataEditorUpdate:
			UpdateText();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark -


BooleanEditor::BooleanEditor(BRect rect, DataEditor &editor)
	: BView(rect, "Boolean Editor", B_FOLLOW_NONE, 0),
	fEditor(editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BPopUpMenu *menu = new BPopUpMenu("bool");
	BMessage *message;
	menu->AddItem(fFalseMenuItem = new BMenuItem("false", new BMessage(kMsgValueChanged)));
	menu->AddItem(fTrueMenuItem = new BMenuItem("true", message = new BMessage(kMsgValueChanged)));
	message->AddInt8("value", 1);

	BMenuField *menuField = new BMenuField(rect.InsetByCopy(5, 5), B_EMPTY_STRING,
						"Boolean Value:", menu, B_FOLLOW_LEFT_RIGHT);
	menuField->SetDivider(StringWidth(menuField->Label()) + 8);
	menuField->ResizeToPreferred();
	ResizeTo(menuField->Bounds().Width() + 10, menuField->Bounds().Height() + 10);

	UpdateMenuField();

	AddChild(menuField);
}


void 
BooleanEditor::UpdateMenuField()
{
	if (fEditor.Lock()) {
		const char *buffer;
		if (fEditor.GetViewBuffer((const uint8 **)&buffer) == B_OK)
			(buffer[0] != 0 ? fTrueMenuItem : fFalseMenuItem)->SetMarked(true);

		fEditor.Unlock();
	}

}


void 
BooleanEditor::AttachedToWindow()
{
	fTrueMenuItem->SetTarget(this);
	fFalseMenuItem->SetTarget(this);

	fEditor.StartWatching(this);
}


void 
BooleanEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void 
BooleanEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgValueChanged:
		{
			BAutolock locker(fEditor);
			uint8 boolean = message->FindInt8("value");
			fEditor.Replace(0, (const uint8 *)&boolean, 1);
			break;
		}
		case kMsgDataEditorUpdate:
			UpdateMenuField();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark -


AttributeWindow::AttributeWindow(BRect rect, entry_ref *ref, const char *attribute,
	const BMessage *settings)
	: ProbeWindow(rect, ref),
	fAttribute(strdup(attribute))
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%s: %s", ref->name, attribute);
	SetTitle(buffer);

	// add the menu

	BMenuBar *menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu *menu = new BMenu("Attribute");

	menu->AddItem(new BMenuItem("Save", NULL, 'S', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Remove from File", new BMessage(kMsgRemoveAttribute)));
	menu->AddSeparatorItem();

	// the ProbeView file menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;

	BView *view = new BView(rect, "main", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds();
	rect.top += 3;

	EditorTabView *tabView = new EditorTabView(rect, "tabView");

	rect = tabView->ContainerView()->Bounds();
	rect.top += 3;
	fProbeView = new ProbeView(rect, ref, attribute, settings);
	fProbeView->AddFileMenuItems(menu, menu->CountItems() - 2);
	tabView->AddRawEditorTab(fProbeView);

	view->AddChild(tabView);

	BView *editor = GetTypeEditorFor(rect, fProbeView->Editor());
	if (editor != NULL)
		tabView->SetTypeEditorTab(editor);
	else {
		// show the raw editor if we don't have a specialised type editor
		tabView->Select(1);
	}

	fProbeView->UpdateSizeLimits();
}


AttributeWindow::~AttributeWindow()
{
	free(fAttribute);
}


void 
AttributeWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgRemoveAttribute:
		{
			char buffer[1024];
			snprintf(buffer, sizeof(buffer),
				"Do you really want to remove the attribute \"%s\" from the file \"%s\"?\n\n"
				"The contents of the attribute will get lost if you click on \"Remove\".",
				fAttribute, Ref().name);

			int32 chosen = (new BAlert("DiskProbe request",
				buffer, "Remove", "Cancel", NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			if (chosen == 0) {
				BNode node(&Ref());
				if (node.InitCheck() == B_OK)
					node.RemoveAttr(fAttribute);

				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}

		default:
			ProbeWindow::MessageReceived(message);
			break;
	}
}


bool
AttributeWindow::Contains(const entry_ref &ref, const char *attribute)
{
	return ref == Ref() && attribute != NULL && !strcmp(attribute, fAttribute);
}

