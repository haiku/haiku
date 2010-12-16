/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2009, Stephan Aßmus, superstippi@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "HeaderView.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Looper.h>


// #pragma mark - HeaderRenderer


HeaderRenderer::~HeaderRenderer()
{
}


void
HeaderRenderer::DrawHeaderBackground(BView* view, BRect frame, BRect updateRect,
	uint32 flags)
{
	BRect bgRect = frame;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));
	view->StrokeLine(bgRect.LeftBottom(), bgRect.RightBottom());

	bgRect.bottom--;
	bgRect.right--;

//	if ((flags & CLICKED) != 0)
//		base = tint_color(base, B_DARKEN_1_TINT);

	be_control_look->DrawButtonBackground(view, bgRect, updateRect, base, 0,
		BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER);

	view->StrokeLine(frame.RightTop(), frame.RightBottom());
}


// #pragma mark - DefaultHeaderRenderer


DefaultHeaderRenderer::DefaultHeaderRenderer()
{
}


DefaultHeaderRenderer::~DefaultHeaderRenderer()
{
}


float
DefaultHeaderRenderer::HeaderHeight(BView* view, const Header* header)
{
	BVariant value;
	if (!header->GetValue(value))
		return 0;

	if (value.Type() == B_STRING_TYPE) {
		font_height fontHeight;
		view->GetFontHeight(&fontHeight);
		return ceilf((fontHeight.ascent + fontHeight.descent) * 1.2f) + 2.0f;
	} else {
		// TODO: Support more value types.
		return 0;
	}
}


float
DefaultHeaderRenderer::PreferredHeaderWidth(BView* view, const Header* header)
{
	BVariant value;
	if (!header->GetValue(value))
		return 0;

	if (value.Type() == B_STRING_TYPE) {
		float width = view->StringWidth(value.ToString());
		return width + be_control_look->DefaultLabelSpacing() * 2.0f;
	} else {
		// TODO: Support more value types.
		return 0;
	}
}


void
DefaultHeaderRenderer::DrawHeader(BView* view, BRect frame, BRect updateRect,
	const Header* header, uint32 flags)
{
	DrawHeaderBackground(view, frame, updateRect, flags);

	BVariant value;
	if (!header->GetValue(value))
		return;

	frame.InsetBy(be_control_look->DefaultLabelSpacing(), 0);

	if (value.Type() == B_STRING_TYPE) {
		be_control_look->DrawLabel(view, value.ToString(), frame, updateRect,
			view->LowColor(), 0);
	}
}


// #pragma mark - HeaderListener


HeaderListener::~HeaderListener()
{
}


void
HeaderListener::HeaderWidthChanged(Header* header)
{
}


void
HeaderListener::HeaderWidthRestrictionsChanged(Header* header)
{
}


void
HeaderListener::HeaderValueChanged(Header* header)
{
}


void
HeaderListener::HeaderRendererChanged(Header* header)
{
}


// #pragma mark - Header


Header::Header(int32 modelIndex)
	:
	fWidth(100),
	fMinWidth(0),
	fMaxWidth(10000),
	fPreferredWidth(100),
	fValue(),
	fRenderer(NULL),
	fModelIndex(modelIndex),
	fResizable(true)
{
}


Header::Header(float width, float minWidth, float maxWidth,
	float preferredWidth, int32 modelIndex)
	:
	fWidth(width),
	fMinWidth(minWidth),
	fMaxWidth(maxWidth),
	fPreferredWidth(preferredWidth),
	fValue(),
	fRenderer(NULL),
	fModelIndex(modelIndex),
	fResizable(true)
{
}


float
Header::Width() const
{
	return fWidth;
}


float
Header::MinWidth() const
{
	return fMinWidth;
}


float
Header::MaxWidth() const
{
	return fMaxWidth;
}


float
Header::PreferredWidth() const
{
	return fPreferredWidth;
}


void
Header::SetWidth(float width)
{
	if (width != fWidth) {
		fWidth = width;
		NotifyWidthChanged();
	}
}


void
Header::SetMinWidth(float width)
{
	if (width != fMinWidth) {
		fMinWidth = width;
		NotifyWidthRestrictionsChanged();
	}
}


void
Header::SetMaxWidth(float width)
{
	if (width != fMaxWidth) {
		fMaxWidth = width;
		NotifyWidthRestrictionsChanged();
	}
}


void
Header::SetPreferredWidth(float width)
{
	if (width != fPreferredWidth) {
		fPreferredWidth = width;
		NotifyWidthRestrictionsChanged();
	}
}


bool
Header::IsResizable() const
{
	return fResizable;
}


void
Header::SetResizable(bool resizable)
{
	if (resizable != fResizable) {
		fResizable = resizable;
		NotifyWidthRestrictionsChanged();
	}
}


bool
Header::GetValue(BVariant& _value) const
{
	_value = fValue;
	return true;
}


void
Header::SetValue(const BVariant& value)
{
	fValue = value;
	NotifyValueChanged();
}


int32
Header::ModelIndex() const
{
	return fModelIndex;
}


void
Header::SetModelIndex(int32 index)
{
	fModelIndex = index;
}


HeaderRenderer*
Header::GetHeaderRenderer() const
{
	return fRenderer;
}


void
Header::SetHeaderRenderer(HeaderRenderer* renderer)
{
	if (renderer != fRenderer) {
		fRenderer = renderer;
		NotifyRendererChanged();
	}
}


void
Header::AddListener(HeaderListener* listener)
{
	fListeners.AddItem(listener);
}


void
Header::RemoveListener(HeaderListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
Header::NotifyWidthChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderListener* listener = fListeners.ItemAt(i);
		listener->HeaderWidthChanged(this);
	}
}


void
Header::NotifyWidthRestrictionsChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderListener* listener = fListeners.ItemAt(i);
		listener->HeaderWidthRestrictionsChanged(this);
	}
}


void
Header::NotifyValueChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderListener* listener = fListeners.ItemAt(i);
		listener->HeaderValueChanged(this);
	}
}


void
Header::NotifyRendererChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderListener* listener = fListeners.ItemAt(i);
		listener->HeaderRendererChanged(this);
	}
}


// #pragma mark - HeaderModelListener


HeaderModelListener::~HeaderModelListener()
{
}


void
HeaderModelListener::HeaderAdded(HeaderModel* model, int32 index)
{
}


void
HeaderModelListener::HeaderRemoved(HeaderModel* model, int32 index)
{
}


void
HeaderModelListener::HeaderMoved(HeaderModel* model, int32 fromIndex,
	int32 toIndex)
{
}


// #pragma mark - HeaderModel


HeaderModel::HeaderModel()
{
}


HeaderModel::~HeaderModel()
{
}


int32
HeaderModel::CountHeaders() const
{
	return fHeaders.CountItems();
}


Header*
HeaderModel::HeaderAt(int32 index) const
{
	return fHeaders.ItemAt(index);
}


int32
HeaderModel::IndexOfHeader(Header* header) const
{
	return fHeaders.IndexOf(header);
}


bool
HeaderModel::AddHeader(Header* header)
{
	if (!fHeaders.AddItem(header))
		return false;

	NotifyHeaderAdded(fHeaders.CountItems() - 1);

	return true;
}


Header*
HeaderModel::RemoveHeader(int32 index)
{
	Header* header = fHeaders.RemoveItemAt(index);
	if (header != NULL)
		return NULL;

	NotifyHeaderRemoved(index);

	return header;
}


void
HeaderModel::RemoveHeader(Header* header)
{
	RemoveHeader(fHeaders.IndexOf(header));
}


bool
HeaderModel::MoveHeader(int32 fromIndex, int32 toIndex)
{
	int32 headerCount = fHeaders.CountItems();
	if (fromIndex < 0 || fromIndex >= headerCount
		|| toIndex < 0 || toIndex >= headerCount) {
		return false;
	}

	if (fromIndex == toIndex)
		return true;

	Header* header = fHeaders.RemoveItemAt(fromIndex);
	fHeaders.AddItem(header, toIndex);
		// TODO: Might fail.

	NotifyHeaderMoved(fromIndex, toIndex);

	return true;
}


void
HeaderModel::AddListener(HeaderModelListener* listener)
{
	fListeners.AddItem(listener);
}


void
HeaderModel::RemoveListener(HeaderModelListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
HeaderModel::NotifyHeaderAdded(int32 index)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderModelListener* listener = fListeners.ItemAt(i);
		listener->HeaderAdded(this, index);
	}
}


void
HeaderModel::NotifyHeaderRemoved(int32 index)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderModelListener* listener = fListeners.ItemAt(i);
		listener->HeaderRemoved(this, index);
	}
}


void
HeaderModel::NotifyHeaderMoved(int32 fromIndex, int32 toIndex)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		HeaderModelListener* listener = fListeners.ItemAt(i);
		listener->HeaderMoved(this, fromIndex, toIndex);
	}
}


// #pragma mark - HeaderEntry


struct HeaderView::HeaderEntry {
	Header*	header;
	float	position;
	float	width;

	HeaderEntry(Header* header)
		:
		header(header)
	{
	}
};


// #pragma mark - States


class HeaderView::State {
public:
	State(HeaderView* parent)
		:
		fParent(parent)
	{
	}

	virtual ~State()
	{
	}

	virtual void Entering(State* previousState)
	{
	}

	virtual void Leaving(State* nextState)
	{
	}

	virtual void MouseDown(BPoint where, uint32 buttons, uint32 modifiers)
	{
	}

	virtual void MouseUp(BPoint where, uint32 buttons, uint32 modifiers)
	{
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
	}

protected:
	HeaderView*	fParent;
};


class HeaderView::DefaultState : public State {
public:
								DefaultState(HeaderView* parent);

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									uint32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
};


class HeaderView::ResizeState : public State {
public:
	virtual	void				Entering(State* previousState);
	virtual	void				Leaving(State* nextState);

								ResizeState(HeaderView* parent,
									int32 headerIndex, BPoint startPoint);

	virtual	void				MouseUp(BPoint where, uint32 buttons,
									uint32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

private:
			Header*				fHeader;
			float				fStartX;
			float				fStartWidth;
};


// #pragma mark - DefaultState


HeaderView::DefaultState::DefaultState(HeaderView* parent)
	:
	State(parent)
{
}


void
HeaderView::DefaultState::MouseDown(BPoint where, uint32 buttons,
	uint32 modifiers)
{
	HeaderModel* model = fParent->Model();
	if (model == NULL)
		return;

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
		return;

	int32 headerIndex = fParent->HeaderIndexAt(where);
	if (headerIndex < 0) {
		int32 headerCount = model->CountHeaders();
		if (headerCount == 0)
			return;

		headerIndex = headerCount - 1;
	}

	// Check whether the mouse is close to the left or the right side of the
	// header.
	BRect headerFrame = fParent->HeaderFrame(headerIndex);
	if (fabs(headerFrame.left - where.x) <= 3) {
		if (headerIndex == 0)
			return;
		headerIndex--;
	} else if (fabs(headerFrame.right + 1 - where.x) > 3)
		return;

	// start resizing the header
	fParent->_SwitchState(new ResizeState(fParent, headerIndex, where));
}


void
HeaderView::DefaultState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
}


// #pragma mark - ResizeState


void
HeaderView::ResizeState::Entering(State* previousState)
{
	fParent->SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
HeaderView::ResizeState::Leaving(State* nextState)
{
	fParent->SetEventMask(0, 0);
}


HeaderView::ResizeState::ResizeState(HeaderView* parent, int32 headerIndex,
	BPoint startPoint)
	:
	State(parent),
	fHeader(parent->Model()->HeaderAt(headerIndex)),
	fStartX(startPoint.x),
	fStartWidth(fHeader->Width())
{
}


void
HeaderView::ResizeState::MouseUp(BPoint where, uint32 buttons,
	uint32 modifiers)
{
	if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
		fParent->_SwitchState(NULL);
}


void
HeaderView::ResizeState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	float width = fStartWidth + where.x - fStartX;
	width = std::max(width, fHeader->MinWidth());
	width = std::min(width, fHeader->MaxWidth());
	fHeader->SetWidth(width);
}


// #pragma mark - HeaderView


HeaderView::HeaderView()
 	:
 	BView("header view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
 	fModel(NULL),
 	fHeaderEntries(10, true),
 	fLayoutValid(false),
 	fDefaultState(new DefaultState(this)),
 	fState(fDefaultState)
{
 	HeaderModel* model = new(std::nothrow) HeaderModel;
 	BReference<HeaderModel> modelReference(model, true);
 	SetModel(model);

	SetViewColor(B_TRANSPARENT_32_BIT);
}


HeaderView::~HeaderView()
{
	SetModel(NULL);

	_SwitchState(NULL);
	delete fDefaultState;
}


void
HeaderView::Draw(BRect updateRect)
{
	if (fModel == NULL)
		return;

	_ValidateHeadersLayout();

	DefaultHeaderRenderer defaultRenderer;
	float bottom = Bounds().Height();
	float left = 0.0f;

	for (int32 i = 0; HeaderEntry* entry = fHeaderEntries.ItemAt(i); i++) {
		if (Header* header = fModel->HeaderAt(i)) {
			HeaderRenderer* renderer = header->GetHeaderRenderer();
			if (renderer == NULL)
				renderer = &defaultRenderer;

			BRect frame(entry->position, 0, entry->position + entry->width - 1,
				bottom);
			renderer->DrawHeader(this, frame, updateRect, header, 0);
				// TODO: flags!

			left = entry->position + entry->width;
		}
	}

	// TODO: We are not using any custom renderer here.
	defaultRenderer.DrawHeaderBackground(this,
		BRect(left, 0, Bounds().right + 1, bottom), updateRect, 0);
}


BSize
HeaderView::MinSize()
{
	_ValidateHeadersLayout();

	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		BSize(100, fPreferredHeight - 1));
}


BSize
HeaderView::MaxSize()
{
	_ValidateHeadersLayout();

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, fPreferredHeight - 1));
}


BSize
HeaderView::PreferredSize()
{
	_ValidateHeadersLayout();

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		BSize(fPreferredWidth - 1, fPreferredHeight - 1));
}


HeaderModel*
HeaderView::Model() const
{
	return fModel;
}


void
HeaderView::MouseDown(BPoint where)
{
	// get buttons and modifiers
	BMessage* message = Looper()->CurrentMessage();
	int32 buttons;
	if (message == NULL || message->FindInt32("buttons", &buttons) != B_OK)
		buttons = 0;
	int32 modifiers;
	if (message == NULL || message->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = 0;

	fState->MouseDown(where, buttons, modifiers);
}


void
HeaderView::MouseUp(BPoint where)
{
	// get buttons and modifiers
	BMessage* message = Looper()->CurrentMessage();
	int32 buttons;
	if (message == NULL || message->FindInt32("buttons", &buttons) != B_OK)
		buttons = 0;
	int32 modifiers;
	if (message == NULL || message->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = 0;

	fState->MouseUp(where, buttons, modifiers);
}


void
HeaderView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	fState->MouseMoved(where, transit, dragMessage);
}


status_t
HeaderView::SetModel(HeaderModel* model)
{
	if (model == fModel)
		return B_OK;

	_SwitchState(NULL);

	if (fModel != NULL) {
		// remove all headers
		for (int32 i = 0; HeaderEntry* entry = fHeaderEntries.ItemAt(i); i++)
			entry->header->RemoveListener(this);
		fHeaderEntries.MakeEmpty();

		fModel->RemoveListener(this);
		fModel->ReleaseReference();
	}

	fModel = model;

	if (fModel != NULL) {
		fModel->AcquireReference();
		fModel->AddListener(this);

		// create header entries
		int32 headerCount = fModel->CountHeaders();
		for (int32 i = 0; i < headerCount; i++) {
			HeaderEntry* entry = new(std::nothrow) HeaderEntry(
				fModel->HeaderAt(i));
			if (entry == NULL || !fHeaderEntries.AddItem(entry)) {
				delete entry;
				return B_NO_MEMORY;
			}

			entry->header->AddListener(this);
		}
	}

	_InvalidateHeadersLayout(0);
	Invalidate();

	return B_OK;
}


BRect
HeaderView::HeaderFrame(int32 index) const
{
	float bottom = Bounds().Height();

	if (HeaderEntry* entry = fHeaderEntries.ItemAt(index)) {
		return BRect(entry->position, 0, entry->position + entry->width - 1,
			bottom);
	}

	return BRect();
}


int32
HeaderView::HeaderIndexAt(BPoint point) const
{
	float x = point.x;

	for (int32 i = 0; HeaderEntry* entry = fHeaderEntries.ItemAt(i); i++) {
		if (x >= entry->position && x < entry->position + entry->width)
			return i;
	}

	return -1;
}


void
HeaderView::AddListener(HeaderViewListener* listener)
{
	fListeners.AddItem(listener);
}


void
HeaderView::RemoveListener(HeaderViewListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
HeaderView::HeaderAdded(HeaderModel* model, int32 index)
{
	if (Header* header = fModel->HeaderAt(index)) {
		HeaderEntry* entry = new(std::nothrow) HeaderEntry(
			fModel->HeaderAt(index));
		if (entry == NULL || !fHeaderEntries.AddItem(entry)) {
			delete entry;
			return;
		}

		header->AddListener(this);
		_InvalidateHeadersLayout(index);
	}
}


void
HeaderView::HeaderRemoved(HeaderModel* model, int32 index)
{
	if (HeaderEntry* entry = fHeaderEntries.RemoveItemAt(index)) {
		entry->header->RemoveListener(this);
		_InvalidateHeadersLayout(index);
	}
}


void
HeaderView::HeaderMoved(HeaderModel* model, int32 fromIndex, int32 toIndex)
{
	_InvalidateHeadersLayout(std::min(fromIndex, toIndex));
}


void
HeaderView::HeaderWidthChanged(Header* header)
{
	_HeaderPropertiesChanged(header, true, true);
}


void
HeaderView::HeaderWidthRestrictionsChanged(Header* header)
{
	// TODO:...
}


void
HeaderView::HeaderValueChanged(Header* header)
{
	_HeaderPropertiesChanged(header, true, false);
}


void
HeaderView::HeaderRendererChanged(Header* header)
{
	_HeaderPropertiesChanged(header, true, true);
}


void
HeaderView::_HeaderPropertiesChanged(Header* header, bool redrawNeeded,
	bool relayoutNeeded)
{
	if (!redrawNeeded && !relayoutNeeded)
		return;

	int32 index = fModel->IndexOfHeader(header);

	if (relayoutNeeded)
		_InvalidateHeadersLayout(index);
	else if (redrawNeeded)
		_InvalidateHeaders(index, index + 1);
}


void
HeaderView::_InvalidateHeadersLayout(int32 firstIndex)
{
	if (!fLayoutValid)
		return;

	fLayoutValid = false;
	InvalidateLayout();
	Invalidate();
}


void
HeaderView::_InvalidateHeaders(int32 firstIndex, int32 endIndex)
{
	Invalidate();
		// TODO: Be less lazy!
}


void
HeaderView::_ValidateHeadersLayout()
{
	if (fLayoutValid)
		return;

	DefaultHeaderRenderer defaultRenderer;

	int32 headerCount = fHeaderEntries.CountItems();
	float position = 0;
	fPreferredWidth = 0;
	fPreferredHeight = 0;

	for (int32 i = 0; i < headerCount; i++) {
		HeaderEntry* entry = fHeaderEntries.ItemAt(i);
		entry->position = position;
		if (Header* header = fModel->HeaderAt(i)) {
			entry->width = header->Width();
			fPreferredWidth += header->PreferredWidth();
		} else
			entry->width = 0;

		position = entry->position + entry->width;

		if (Header* header = fModel->HeaderAt(i)) {
			HeaderRenderer* renderer = header->GetHeaderRenderer();
			if (renderer == NULL)
				renderer = &defaultRenderer;

			float height = renderer->HeaderHeight(this, header);
			if (height > fPreferredHeight)
				fPreferredHeight = height;
		}
	}

	fLayoutValid = true;
}


void
HeaderView::_SwitchState(State* newState)
{
	if (newState == NULL)
		newState = fDefaultState;

	fState->Leaving(newState);
	newState->Entering(fState);

	if (fState != fDefaultState)
		delete fState;

	fState = newState;
}


// #pragma mark - HeaderViewListener


HeaderViewListener::~HeaderViewListener()
{
}
