/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "TypeEditors.h"
#include "DataEditor.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Catalog.h>
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <IconUtils.h>
#endif
#include <Locale.h>
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
#include <TranslatorFormats.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TypeEditors"

static const uint32 kMsgValueChanged = 'vlch';
static const uint32 kMsgScaleChanged = 'scch';
static const uint32 kMimeTypeItem = 'miti';


class StringEditor : public TypeEditorView {
	public:
		StringEditor(BRect rect, DataEditor& editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);

		virtual void CommitChanges();

	private:
		void _UpdateText();

		BTextView*		fTextView;
		BString			fPreviousText;
};


class MimeTypeEditor : public TypeEditorView {
	public:
		MimeTypeEditor(BRect rect, DataEditor& editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);

		virtual void CommitChanges();
		virtual bool TypeMatches();

	private:
		void _UpdateText();

		BTextControl*	fTextControl;
		BString			fPreviousText;
};


class NumberEditor : public TypeEditorView {
	public:
		NumberEditor(BRect rect, DataEditor& editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);

		virtual void CommitChanges();
		virtual bool TypeMatches();

	private:
		void _UpdateText();
		const char* _TypeLabel();
		status_t _Format(char* buffer);
		size_t _Size();

		BTextControl*	fTextControl;
		BString			fPreviousText;
};


class BooleanEditor : public TypeEditorView {
	public:
		BooleanEditor(BRect rect, DataEditor& editor);

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);

		virtual void CommitChanges();
		virtual bool TypeMatches();

	private:
		void _UpdateMenuField();

		BMenuItem*		fFalseMenuItem;
		BMenuItem*		fTrueMenuItem;
};


class ImageView : public TypeEditorView {
	public:
		ImageView(BRect rect, DataEditor &editor);
		virtual ~ImageView();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect updateRect);

	private:
		void _UpdateImage();

		BBitmap*		fBitmap;
		BStringView*	fDescriptionView;
		BSlider*		fScaleSlider;
};


class MessageView : public TypeEditorView {
	public:
		MessageView(BRect rect, DataEditor& editor);
		virtual ~MessageView();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);

		void SetTo(BMessage& message);

	private:
		BString _TypeToString(type_code type);
		void _UpdateMessage();

		BTextView*		fTextView;
};


//	#pragma mark - TypeEditorView


TypeEditorView::TypeEditorView(BRect rect, const char *name,
		uint32 resizingMode, uint32 flags, DataEditor& editor)
	: BView(rect, name, resizingMode, flags),
	fEditor(editor)
{
}


TypeEditorView::~TypeEditorView()
{
}


void
TypeEditorView::CommitChanges()
{
	// the default just does nothing here
}


bool
TypeEditorView::TypeMatches()
{
	// the default is to accept anything that easily fits into memory

	system_info info;
	get_system_info(&info);

	return fEditor.FileSize() / B_PAGE_SIZE < info.max_pages / 2;
}


//	#pragma mark - StringEditor


StringEditor::StringEditor(BRect rect, DataEditor& editor)
	: TypeEditorView(rect, B_TRANSLATE("String editor"), B_FOLLOW_ALL, 0, editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView *stringView = new BStringView(BRect(5, 5, rect.right, 20),
		B_EMPTY_STRING, B_TRANSLATE("Contents:"));
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect = Bounds();
	rect.top = stringView->Frame().bottom + 5;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	fTextView = new BTextView(rect, B_EMPTY_STRING,
		rect.OffsetToCopy(B_ORIGIN).InsetByCopy(5, 5),
		B_FOLLOW_ALL, B_WILL_DRAW);

	BScrollView* scrollView = new BScrollView("scroller", fTextView,
		B_FOLLOW_ALL, B_WILL_DRAW, true, true);
	AddChild(scrollView);
}


void
StringEditor::_UpdateText()
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

	_UpdateText();
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


//	#pragma mark - MimeTypeEditor


MimeTypeEditor::MimeTypeEditor(BRect rect, DataEditor& editor)
	: TypeEditorView(rect, B_TRANSLATE("MIME type editor"), B_FOLLOW_LEFT_RIGHT, 0, editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTextControl = new BTextControl(rect.InsetByCopy(5, 5), B_EMPTY_STRING,
		B_TRANSLATE("MIME type:"), NULL, new BMessage(kMsgValueChanged), B_FOLLOW_ALL);
	fTextControl->SetDivider(StringWidth(fTextControl->Label()) + 8);

	float width, height;
	fTextControl->GetPreferredSize(&width, &height);
	fTextControl->ResizeTo(rect.Width() - 10, height);

	ResizeTo(rect.Width(), height + 10);

	AddChild(fTextControl);
}


void
MimeTypeEditor::_UpdateText()
{
	BAutolock locker(fEditor);

	const char* mimeType;
	if (fEditor.GetViewBuffer((const uint8 **)&mimeType) == B_OK) {
		fTextControl->SetText(mimeType);
		fPreviousText.SetTo(mimeType);
	}
}


void
MimeTypeEditor::CommitChanges()
{
	if (fPreviousText != fTextControl->Text()) {
		fEditor.Replace(0, (const uint8*)fTextControl->Text(),
			strlen(fTextControl->Text()) + 1);
	}
}


bool
MimeTypeEditor::TypeMatches()
{
	// TODO: check contents?
	return fEditor.FileSize() <= B_MIME_TYPE_LENGTH;
}


void
MimeTypeEditor::AttachedToWindow()
{
	fTextControl->SetTarget(this);
	fEditor.StartWatching(this);

	_UpdateText();
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
			_UpdateText();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark - NumberEditor


NumberEditor::NumberEditor(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, B_TRANSLATE("Number editor"), B_FOLLOW_LEFT_RIGHT, 0, editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTextControl = new BTextControl(rect.InsetByCopy(5, 5), B_EMPTY_STRING,
		_TypeLabel(), NULL, new BMessage(kMsgValueChanged), B_FOLLOW_ALL);
	fTextControl->SetDivider(StringWidth(fTextControl->Label()) + 8);
	fTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_RIGHT);
	ResizeTo(rect.Width(), 30);

	AddChild(fTextControl);
}


void
NumberEditor::_UpdateText()
{
	if (fEditor.Lock()) {
		const char* number;
		if (fEditor.GetViewBuffer((const uint8**)&number) == B_OK) {
			char buffer[64];
			char format[16];
			switch (fEditor.Type()) {
				case B_FLOAT_TYPE:
				{
					float value = *(float*)number;
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
					_Format(format);
					switch (_Size()) {
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
					_Format(format);
					switch (_Size()) {
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


bool
NumberEditor::TypeMatches()
{
	return fEditor.FileSize() >= _Size();
		// we only look at as many bytes we need to
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

	fEditor.Replace(0, buffer, _Size());
	fPreviousText.SetTo((char *)buffer);
}


const char*
NumberEditor::_TypeLabel()
{
	switch (fEditor.Type()) {
		case B_INT8_TYPE:
			return B_TRANSLATE("8 bit signed value:");
		case B_UINT8_TYPE:
			return B_TRANSLATE("8 bit unsigned value:");
		case B_INT16_TYPE:
			return B_TRANSLATE("16 bit signed value:");
		case B_UINT16_TYPE:
			return B_TRANSLATE("16 bit unsigned value:");
		case B_INT32_TYPE:
			return B_TRANSLATE("32 bit signed value:");
		case B_UINT32_TYPE:
			return B_TRANSLATE("32 bit unsigned value:");
		case B_INT64_TYPE:
			return B_TRANSLATE("64 bit signed value:");
		case B_UINT64_TYPE:
			return B_TRANSLATE("64 bit unsigned value:");
		case B_FLOAT_TYPE:
			return B_TRANSLATE("Floating-point value:");
		case B_DOUBLE_TYPE:
			return B_TRANSLATE("Double precision floating-point value:");
		case B_SSIZE_T_TYPE:
			return B_TRANSLATE("32 bit size or status:");
		case B_SIZE_T_TYPE:
			return B_TRANSLATE("32 bit unsigned size:");
		case B_OFF_T_TYPE:
			return B_TRANSLATE("64 bit signed offset:");
		case B_POINTER_TYPE:
			return B_TRANSLATE("32 bit unsigned pointer:");
		default:
			return B_TRANSLATE("Number:");
	}
}


size_t
NumberEditor::_Size()
{
	switch (fEditor.Type()) {
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			return 1;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			return 2;
		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_POINTER_TYPE:
		case B_UINT32_TYPE:
			return 4;
		case B_INT64_TYPE:
		case B_OFF_T_TYPE:
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
NumberEditor::_Format(char *buffer)
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

	_UpdateText();
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
			_UpdateText();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark - BooleanEditor


BooleanEditor::BooleanEditor(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, B_TRANSLATE("Boolean editor"), B_FOLLOW_NONE, 0, editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BPopUpMenu *menu = new BPopUpMenu("bool");
	BMessage *message;
	menu->AddItem(fFalseMenuItem = new BMenuItem("false",
		new BMessage(kMsgValueChanged)));
	menu->AddItem(fTrueMenuItem = new BMenuItem("true",
		message = new BMessage(kMsgValueChanged)));
	message->AddInt8("value", 1);

	BMenuField *menuField = new BMenuField(rect.InsetByCopy(5, 5),
		B_EMPTY_STRING, B_TRANSLATE("Boolean value:"), menu, B_FOLLOW_LEFT_RIGHT);
	menuField->SetDivider(StringWidth(menuField->Label()) + 8);
	menuField->ResizeToPreferred();
	ResizeTo(menuField->Bounds().Width() + 10,
		menuField->Bounds().Height() + 10);

	_UpdateMenuField();

	AddChild(menuField);
}


bool
BooleanEditor::TypeMatches()
{
	// we accept everything: we just look at the first byte, anyway
	return true;
}


void
BooleanEditor::_UpdateMenuField()
{
	if (fEditor.FileSize() != 1)
		return;

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
			_UpdateMenuField();
			break;

		default:
			BView::MessageReceived(message);
	}
}


//	#pragma mark - ImageView


ImageView::ImageView(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, B_TRANSLATE_COMMENT("Image view", "Image means "
		"here a picture file, not a disk image."), B_FOLLOW_NONE, 
	B_WILL_DRAW, editor),
	fBitmap(NULL),
	fScaleSlider(NULL)
{
	if (editor.Type() == B_MINI_ICON_TYPE 
		|| editor.Type() == B_LARGE_ICON_TYPE
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		|| editor.Type() == B_VECTOR_ICON_TYPE
#endif
		)
		SetName(B_TRANSLATE("Icon view"));

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (editor.Type() == B_VECTOR_ICON_TYPE) {
		// vector icon
		fScaleSlider = new BSlider(BRect(0, 0, 195, 20), "", NULL,
			new BMessage(kMsgScaleChanged), 2, 32);
		fScaleSlider->SetModificationMessage(new BMessage(kMsgScaleChanged));
		fScaleSlider->ResizeToPreferred();
		fScaleSlider->SetValue(8);
		fScaleSlider->SetHashMarks(B_HASH_MARKS_BOTH);
		fScaleSlider->SetHashMarkCount(15);
		AddChild(fScaleSlider);
	}
#endif

	fDescriptionView = new BStringView(Bounds(), "", 
		B_TRANSLATE_COMMENT("Could not read image", "Image means "
		"here a picture file, not a disk image."), B_FOLLOW_NONE);
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

	_UpdateImage();
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
			_UpdateImage();
			break;

		case kMsgScaleChanged:
			_UpdateImage();
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
ImageView::_UpdateImage()
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

	if (fBitmap != NULL && (fEditor.Type() == B_MINI_ICON_TYPE
			|| fEditor.Type() == B_LARGE_ICON_TYPE)) {
		// optimize icon update...
		fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
		fEditor.SetViewSize(viewSize);
		return;
	}
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	if (fBitmap != NULL && fEditor.Type() == B_VECTOR_ICON_TYPE
		&& fScaleSlider->Value() * 8 - 1 == fBitmap->Bounds().Width()) {
		if (BIconUtils::GetVectorIcon((const uint8 *)data,
				(size_t)fEditor.FileSize(), fBitmap) == B_OK) {
			fEditor.SetViewSize(viewSize);
			return;
		}
	}
#endif

	delete fBitmap;
	fBitmap = NULL;

	switch (fEditor.Type()) {
		case B_MINI_ICON_TYPE:
			fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			if (fBitmap->InitCheck() == B_OK)
				fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
			break;
		case B_LARGE_ICON_TYPE:
			fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			if (fBitmap->InitCheck() == B_OK)
				fBitmap->SetBits(data, fEditor.FileSize(), 0, B_CMAP8);
			break;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		case B_VECTOR_ICON_TYPE:
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
		case B_PNG_FORMAT:
		{
			BMemoryIO stream(data, fEditor.FileSize());
			fBitmap = BTranslationUtils::GetBitmap(&stream);
			break;
		}
		case B_MESSAGE_TYPE:
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
		const char *type = B_TRANSLATE("Unknown type");
		switch (fEditor.Type()) {
			case B_MINI_ICON_TYPE:
			case B_LARGE_ICON_TYPE:
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			case B_VECTOR_ICON_TYPE:
#endif
				type = B_TRANSLATE("Icon");
				break;
			case B_PNG_FORMAT:
				type = B_TRANSLATE("PNG format");
				break;
			case B_MESSAGE_TYPE:
				type = B_TRANSLATE("Flattened bitmap");
				break;
			default:
				break;
		}
		const char *colorSpace;
		switch (fBitmap->ColorSpace()) {
			case B_GRAY1:
			case B_GRAY8:
				colorSpace = B_TRANSLATE("Grayscale");
				break;
			case B_CMAP8:
				colorSpace = B_TRANSLATE("8 bit palette");
				break;
			case B_RGB32:
			case B_RGBA32:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
				colorSpace = B_TRANSLATE("32 bit");
				break;
			case B_RGB15:
			case B_RGBA15:
			case B_RGB15_BIG:
			case B_RGBA15_BIG:
				colorSpace = B_TRANSLATE("15 bit");
				break;
			case B_RGB16:
			case B_RGB16_BIG:
				colorSpace = B_TRANSLATE("16 bit");
				break;
			default:
				colorSpace = B_TRANSLATE("Unknown format");
				break;
		}
		snprintf(buffer, sizeof(buffer), "%s, %g x %g, %s", type,
			fBitmap->Bounds().Width() + 1, fBitmap->Bounds().Height() + 1, 
			colorSpace);
		fDescriptionView->SetText(buffer);
	} else
		fDescriptionView->SetText(B_TRANSLATE_COMMENT("Could not read image", 
			"Image means here a picture file, not a disk image."));

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


//	#pragma mark - MessageView


MessageView::MessageView(BRect rect, DataEditor &editor)
	: TypeEditorView(rect, B_TRANSLATE("Message View"), B_FOLLOW_ALL, 0, editor)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	rect = Bounds().InsetByCopy(10, 10);
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	fTextView = new BTextView(rect, B_EMPTY_STRING,
		rect.OffsetToCopy(B_ORIGIN).InsetByCopy(5, 5),
		B_FOLLOW_ALL, B_WILL_DRAW);
	fTextView->SetViewColor(ViewColor());
	fTextView->SetLowColor(ViewColor());

	BScrollView *scrollView = new BScrollView("scroller", fTextView,
		B_FOLLOW_ALL, B_WILL_DRAW, true, true);
	AddChild(scrollView);
}


MessageView::~MessageView()
{
}


BString
MessageView::_TypeToString(type_code type)
{
	// TODO: move this to a utility function (it's a copy from something
	// similar in ProbeView.cpp)
	char text[32];
	for (int32 i = 0; i < 4; i++) {
		text[i] = type >> (24 - 8 * i);
		if (text[i] < ' ' || text[i] == 0x7f) {
			snprintf(text, sizeof(text), "0x%04lx", type);
			break;
		} else if (i == 3)
			text[4] = '\0';
	}

	return BString(text);
}


void
MessageView::SetTo(BMessage& message)
{
	// TODO: when we have a real column list/tree view, redo this using
	// it with nice editors as well.

	fTextView->SetText("");

	char text[512];
	snprintf(text, sizeof(text), B_TRANSLATE_COMMENT("what: '%.4s'\n\n", 
		"'What' is a message specifier that defines the type of the message."),
		(char*)&message.what);
	fTextView->Insert(text);

	type_code type;
	int32 count;
#ifdef HAIKU_TARGET_PLATFORM_DANO
	const
#endif
	char* name;
	for (int32 i = 0; message.GetInfo(B_ANY_TYPE, i, &name, &type, &count)
			== B_OK; i++) {
		snprintf(text, sizeof(text), "%s\t", _TypeToString(type).String());
		fTextView->Insert(text);

		text_run_array array;
		array.count = 1;
		array.runs[0].offset = 0;
		array.runs[0].font.SetFace(B_BOLD_FACE);
		array.runs[0].color = (rgb_color){0, 0, 0, 255};

		fTextView->Insert(name, &array);

		array.runs[0].font = be_plain_font;
		fTextView->Insert("\n", &array);

		for (int32 j = 0; j < count; j++) {
			const char* data;
			ssize_t size;
			if (message.FindData(name, type, j, (const void**)&data, &size)
					!= B_OK)
				continue;

			text[0] = '\0';

			switch (type) {
				case B_INT64_TYPE:
					snprintf(text, sizeof(text), "%Ld", *(int64*)data);
					break;
				case B_UINT64_TYPE:
					snprintf(text, sizeof(text), "%Lu", *(uint64*)data);
					break;
				case B_INT32_TYPE:
					snprintf(text, sizeof(text), "%ld", *(int32*)data);
					break;
				case B_UINT32_TYPE:
					snprintf(text, sizeof(text), "%lu", *(uint32*)data);
					break;
				case B_BOOL_TYPE:
					if (*(uint8*)data)
						strcpy(text, "true");
					else
						strcpy(text, "false");
					break;
				case B_STRING_TYPE:
				case B_MIME_STRING_TYPE:
				{
					snprintf(text, sizeof(text), "%s", data);
					break;
				}
			}

			if (text[0]) {
				fTextView->Insert("\t\t");
				if (count > 1) {
					char index[16];
					snprintf(index, sizeof(index), "%ld.\t", j);
					fTextView->Insert(index);
				}
				fTextView->Insert(text);
				fTextView->Insert("\n");
			}
		}
	}
}


void
MessageView::_UpdateMessage()
{
	BAutolock locker(fEditor);

	size_t viewSize = fEditor.ViewSize();
	// that may need some more memory...
	if (viewSize < fEditor.FileSize())
		fEditor.SetViewSize(fEditor.FileSize());

	const char *buffer;
	if (fEditor.GetViewBuffer((const uint8 **)&buffer) == B_OK) {
		BMessage message;
		message.Unflatten(buffer);
		SetTo(message);
	}

	// restore old view size
	fEditor.SetViewSize(viewSize);
}


void
MessageView::AttachedToWindow()
{
	fEditor.StartWatching(this);

	_UpdateMessage();
}


void
MessageView::DetachedFromWindow()
{
	fEditor.StopWatching(this);
}


void
MessageView::MessageReceived(BMessage* message)
{
	BView::MessageReceived(message);
}


//	#pragma mark -


TypeEditorView*
GetTypeEditorFor(BRect rect, DataEditor& editor)
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
		case B_MESSAGE_TYPE:
			// TODO: check for archived bitmaps!!!
			return new MessageView(rect, editor);
		case B_MINI_ICON_TYPE:
		case B_LARGE_ICON_TYPE:
		case B_PNG_FORMAT:
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		case B_VECTOR_ICON_TYPE:
#endif
			return new ImageView(rect, editor);
	}

	return NULL;
}


status_t
GetNthTypeEditor(int32 index, const char** _name)
{
	static const char* kEditors[] = {
		B_TRANSLATE_COMMENT("Text", "This is the type of editor"),
		B_TRANSLATE_COMMENT("Number", "This is the type of editor"),
		B_TRANSLATE_COMMENT("Boolean", "This is the type of editor"),
		B_TRANSLATE_COMMENT("Message", "This is the type of view"),
		B_TRANSLATE_COMMENT("Image", "This is the type of view")
	};

	if (index < 0 || index >= int32(sizeof(kEditors) / sizeof(kEditors[0])))
		return B_BAD_VALUE;

	*_name = kEditors[index];
	return B_OK;
}


TypeEditorView*
GetTypeEditorAt(int32 index, BRect rect, DataEditor& editor)
{
	TypeEditorView* view = NULL;

	switch (index) {
		case 0:
			view = new StringEditor(rect, editor);
			break;
		case 1:
			view = new NumberEditor(rect, editor);
			break;
		case 2:
			view = new BooleanEditor(rect, editor);
			break;
		case 3:
			view = new MessageView(rect, editor);
			break;
		case 4:
			view = new ImageView(rect, editor);
			break;

		default:
			return NULL;
	}

	if (view == NULL)
		return NULL;

	if (!view->TypeMatches()) {
		delete view;
		return NULL;
	}

	return view;
}

