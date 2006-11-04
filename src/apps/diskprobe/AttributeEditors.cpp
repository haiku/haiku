/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeEditors.h"
#include "DataEditor.h"

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <IconUtils.h>
#endif

#include <Alert.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <Slider.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <TranslationUtils.h>

#include <stdlib.h>


static const uint32 kMsgValueChanged = 'vlch';
static const uint32 kMsgScaleChanged = 'scch';
static const uint32 kMimeTypeItem = 'miti';


class StringEditor : public TypeEditorView {
	public:
		StringEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateText();
		virtual void CommitChanges();

	private:
		DataEditor	&fEditor;
		BTextView	*fTextView;
		BString		fPreviousText;
};


class MimeTypeEditor : public TypeEditorView {
	public:
		MimeTypeEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateText();
		virtual void CommitChanges();

	private:
		DataEditor		&fEditor;
		BTextControl	*fTextControl;
		BString			fPreviousText;
};


class NumberEditor : public TypeEditorView {
	public:
		NumberEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateText();
		virtual void CommitChanges();

	private:
		const char *TypeLabel();
		status_t Format(char *buffer);
		size_t Size();

		DataEditor		&fEditor;
		BTextControl	*fTextControl;
		BString			fPreviousText;
};


class BooleanEditor : public TypeEditorView {
	public:
		BooleanEditor(BRect rect, DataEditor &editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);

		void UpdateMenuField();
		virtual void CommitChanges();

	private:
		DataEditor	&fEditor;
		BMenuItem	*fFalseMenuItem;
		BMenuItem	*fTrueMenuItem;
};


class ImageView : public TypeEditorView {
	public:
		ImageView(BRect rect, DataEditor &editor);
		virtual ~ImageView();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect updateRect);

		void UpdateImage();
		virtual void CommitChanges();

	private:
		DataEditor	&fEditor;
		BBitmap		*fBitmap;
		BStringView	*fDescriptionView;
		BSlider		*fScaleSlider;
};


//-----------------


StringEditor::StringEditor(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, "String Editor", B_FOLLOW_ALL, 0),
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
StringEditor::UpdateText()
{
	BAutolock locker(fEditor);

	size_t viewSize = fEditor.ViewSize();
	// that may need some more memory...
	if (viewSize < fEditor.FileSize())
		fEditor.SetViewSize(fEditor.FileSize());

	const char *buffer;
	if (fEditor.GetViewBuffer((const uint8 **)&buffer) == B_OK) {
		fTextView->SetText(buffer);
		fPreviousText.SetTo(buffer);
	}

	// restore old view size
	fEditor.SetViewSize(viewSize);
}


void 
StringEditor::CommitChanges()
{
	if (fPreviousText != fTextView->Text()) {
		fEditor.Replace(0, (const uint8 *)fTextView->Text(),
			fTextView->TextLength() + 1);
	}
}


void
StringEditor::AttachedToWindow()
{
	fEditor.StartWatching(this);

	UpdateText();
}


void 
StringEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);

	CommitChanges();
}


void
StringEditor::MessageReceived(BMessage *message)
{
	BView::MessageReceived(message);
}


//	#pragma mark -


MimeTypeEditor::MimeTypeEditor(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, "MIME Type Editor", B_FOLLOW_LEFT_RIGHT, 0),
	fEditor(editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTextControl = new BTextControl(rect.InsetByCopy(5, 5), B_EMPTY_STRING, "MIME Type:", NULL,
							new BMessage(kMsgValueChanged), B_FOLLOW_ALL);
	fTextControl->SetDivider(StringWidth(fTextControl->Label()) + 8);

	float width, height;
	fTextControl->GetPreferredSize(&width, &height);
	fTextControl->ResizeTo(rect.Width() - 10, height);

	ResizeTo(rect.Width(), height + 10);

	AddChild(fTextControl);
}


void
MimeTypeEditor::UpdateText()
{
	BAutolock locker(fEditor);

	const char *mimeType;
	if (fEditor.GetViewBuffer((const uint8 **)&mimeType) == B_OK) {
		fTextControl->SetText(mimeType);
		fPreviousText.SetTo(mimeType);
	}
}


void 
MimeTypeEditor::CommitChanges()
{
	if (fPreviousText != fTextControl->Text()) {
		fEditor.Replace(0, (const uint8 *)fTextControl->Text(),
			strlen(fTextControl->Text()) + 1);
	}
}


void
MimeTypeEditor::AttachedToWindow()
{
	fTextControl->SetTarget(this);
	fEditor.StartWatching(this);

	UpdateText();
}


void 
MimeTypeEditor::DetachedFromWindow()
{
	fEditor.StopWatching(this);

	CommitChanges();
}


void
MimeTypeEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgValueChanged:
			fEditor.Replace(0, (const uint8 *)fTextControl->Text(),
				strlen(fTextControl->Text()) + 1);
			break;

		case kMsgDataEditorUpdate:
			UpdateText();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark -


NumberEditor::NumberEditor(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, "Number Editor", B_FOLLOW_LEFT_RIGHT, 0),
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
NumberEditor::CommitChanges()
{
	if (fPreviousText == fTextControl->Text())
		return;

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

	fEditor.Replace(0, buffer, Size());
	fPreviousText.SetTo((char *)buffer);
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

	CommitChanges();
}


void
NumberEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgValueChanged:
			CommitChanges();
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
	: TypeEditorView(rect, "Boolean Editor", B_FOLLOW_NONE, 0),
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
BooleanEditor::CommitChanges()
{
	// we're commiting the changes as they happen
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


ImageView::ImageView(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, "Image View", B_FOLLOW_NONE, B_WILL_DRAW),
	fEditor(editor),
	fBitmap(NULL),
	fScaleSlider(NULL)
{
	if (editor.Type() == 'MICN' || editor.Type() == 'ICON'
		|| (!strcmp(editor.Attribute(), "BEOS:ICON") && editor.Type() == B_RAW_TYPE)) {
		SetName("Icon View");

		if (editor.Type() == B_RAW_TYPE) {
			// vector icon
			fScaleSlider = new BSlider(BRect(0, 0, 140, 20), "", NULL,
				new BMessage(kMsgScaleChanged), 2, 16);
			fScaleSlider->SetModificationMessage(new BMessage(kMsgScaleChanged));
			fScaleSlider->ResizeToPreferred();
			fScaleSlider->SetValue(8.0);
			fScaleSlider->SetHashMarks(B_HASH_MARKS_BOTH);
			fScaleSlider->SetHashMarkCount(15);
			AddChild(fScaleSlider);
		}
	}

	fDescriptionView = new BStringView(Bounds(), "", "Could not read image", B_FOLLOW_NONE);
	fDescriptionView->SetAlignment(B_ALIGN_CENTER);

	AddChild(fDescriptionView);
}


ImageView::~ImageView()
{
	delete fBitmap;
}


void 
ImageView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fEditor.StartWatching(this);
	if (fScaleSlider != NULL)
		fScaleSlider->SetTarget(this);

	UpdateImage();
}


void 
ImageView::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void
ImageView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgDataEditorUpdate:
			UpdateImage();
			break;

		case kMsgScaleChanged:
			UpdateImage();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
ImageView::Draw(BRect updateRect)
{
	if (fBitmap != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fBitmap, BPoint((Bounds().Width() - fBitmap->Bounds().Width()) / 2, 0));
		SetDrawingMode(B_OP_COPY);
	}
}


void
ImageView::UpdateImage()
{
	// ToDo: add scroller if necessary?!

	BAutolock locker(fEditor);

	// we need all the data...

	size_t viewSize = fEditor.ViewSize();
	// that may need some more memory...
	if (viewSize < fEditor.FileSize())
		fEditor.SetViewSize(fEditor.FileSize());

	const char *data;
	if (fEditor.GetViewBuffer((const uint8 **)&data) != B_OK) {
		fEditor.SetViewSize(viewSize);
		return;
	}

	if (fBitmap != NULL && (fEditor.Type() == 'MICN' || fEditor.Type() == 'ICON')) {
		// optimize icon update...
		fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
		fEditor.SetViewSize(viewSize);
		return;
	}
	if (fBitmap != NULL && !strcmp(fEditor.Attribute(), "BEOS:ICON")
		&& fEditor.Type() == B_RAW_TYPE
		&& fScaleSlider->Value() * 8 - 1 == fBitmap->Bounds().Width()) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		if (BIconUtils::GetVectorIcon((const uint8 *)data,
				(size_t)fEditor.FileSize(), fBitmap) == B_OK) {
#endif
			fEditor.SetViewSize(viewSize);
			return;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		}
#endif
	}

	delete fBitmap;
	fBitmap = NULL;

	switch (fEditor.Type()) {
		case 'MICN':
			fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			if (fBitmap->InitCheck() == B_OK)
				fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
			break;
		case 'ICON':
			fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			if (fBitmap->InitCheck() == B_OK)
				fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
			break;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		case B_RAW_TYPE:
			if (strcmp(fEditor.Attribute(), "BEOS:ICON"))
				break;

			fBitmap = new BBitmap(BRect(0, 0, fScaleSlider->Value() * 8 - 1,
				fScaleSlider->Value() * 8 - 1), B_RGB32);
			if (fBitmap->InitCheck() != B_OK
				|| BIconUtils::GetVectorIcon((const uint8 *)data,
					(size_t)fEditor.FileSize(), fBitmap) != B_OK) {
				delete fBitmap;
				fBitmap = NULL;
			}
			break;
#endif
		case 'PNG ':
		{
			BMemoryIO stream(data, fEditor.FileSize());
			fBitmap = BTranslationUtils::GetBitmap(&stream);
			break;
		}
		case 'MSGG':
		{
			BMessage message;
			// ToDo: this could be problematic if the data is not large
			//		enough to contain the message...
			if (message.Unflatten(data) == B_OK)
				fBitmap = new BBitmap(&message);
			break;
		}
	}

	// Update the bitmap description. If no image can be displayed,
	// we will show our "unsupported" message

	if (fBitmap != NULL && fBitmap->InitCheck() != B_OK) {
		delete fBitmap;
		fBitmap = NULL;
	}

	if (fBitmap != NULL) {
		char buffer[256];
		const char *type = "Unknown Type";
		switch (fEditor.Type()) {
			case 'MICN':
			case 'ICON':
				type = "Icon";
				break;
			case B_RAW_TYPE:
				if (!strcmp(fEditor.Attribute(), "BEOS:ICON"))
					type = "Icon";
				break;
			case 'PNG ':
				type = "PNG Format";
				break;
			case 'MSGG':
				type = "Flattened Bitmap";
				break;
			default:	
				break;
		}
		const char *colorSpace;
		switch (fBitmap->ColorSpace()) {
			case B_GRAY1:
			case B_GRAY8:
				colorSpace = "Gray Scale";
				break;
			case B_CMAP8:
				colorSpace = "8 Bit Palette";
				break;
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				colorSpace = "32 Bit";
				break;
			case B_RGB15:
			case B_RGBA15:
			case B_RGB15_BIG:
			case B_RGBA15_BIG:
				colorSpace = "15 Bit";
				break;
			case B_RGB16:
			case B_RGB16_BIG:
				colorSpace = "16 Bit";
				break;
			default:
				colorSpace = "Unknown Format";
				break;
		}
		snprintf(buffer, sizeof(buffer), "%s, %g x %g, %s", type,
			fBitmap->Bounds().Width() + 1, fBitmap->Bounds().Height() + 1, colorSpace);
		fDescriptionView->SetText(buffer);
	} else
		fDescriptionView->SetText("Could not read image");

	// Update the view size to match the image and its description

	float width, height;
	fDescriptionView->GetPreferredSize(&width, &height);
	fDescriptionView->ResizeTo(width, height);

	BRect rect = fDescriptionView->Bounds();
	if (fBitmap != NULL) {
		BRect bounds = fBitmap->Bounds();
		rect.bottom += bounds.Height() + 5;

		if (fScaleSlider != NULL && rect.Width() < fScaleSlider->Bounds().Width())
			rect.right = fScaleSlider->Bounds().right;
		if (bounds.Width() > rect.Width())
			rect.right = bounds.right;

		// center description below the bitmap
		fDescriptionView->MoveTo((rect.Width() - fDescriptionView->Bounds().Width()) / 2,
			bounds.Height() + 5);

		if (fScaleSlider != NULL) {
			// center slider below description
			rect.bottom += fScaleSlider->Bounds().Height() + 5;
			fScaleSlider->MoveTo((rect.Width() - fScaleSlider->Bounds().Width()) / 2,
				fDescriptionView->Frame().bottom + 5);

			if (fScaleSlider->IsHidden())
				fScaleSlider->Show();
		}
	} else if (fScaleSlider != NULL && !fScaleSlider->IsHidden())
		fScaleSlider->Hide();

	ResizeTo(rect.Width(), rect.Height());
	if (Parent()) {
		// center within parent view
		BRect parentBounds = Parent()->Bounds();
		MoveTo((parentBounds.Width() - rect.Width()) / 2,
			(parentBounds.Height() - rect.Height()) / 2);
	}

	Invalidate();

	// restore old view size
	fEditor.SetViewSize(viewSize);
}


void
ImageView::CommitChanges()
{
	// we're not an editor, we're just displaying something
}


//	#pragma mark -


TypeEditorView *
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
		case 'MICN':
		case 'ICON':
		case 'PNG ':
		case 'MSGG':
			return new ImageView(rect, editor);
	}

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (!strcmp(editor.Attribute(), "BEOS:ICON") && editor.Type() == B_RAW_TYPE)
		return new ImageView(rect, editor);
#endif

	return NULL;
}

