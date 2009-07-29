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


// #pragma mark - HeaderRenderer


HeaderRenderer::~HeaderRenderer()
{
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
		return 15;
			// TODO:...
	} else
		return 0;
}


void
DefaultHeaderRenderer::DrawHeader(BView* view, BRect frame, BRect updateRect,
	const Header* header, uint32 flags)
{
printf("DefaultHeaderRenderer::DrawHeader(): frame: (%f, %f) - (%f, %f), header: %p\n",
frame.left, frame.top, frame.right, frame.bottom, header);
	BVariant value;
	if (!header->GetValue(value))
		return;

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


// #pragma mark - HeaderView


HeaderView::HeaderView()
 	:
 	BView("header view", B_WILL_DRAW),
 	fModel(NULL),
 	fHeaderEntries(10, true),
 	fLayoutValid(false)
{
 	HeaderModel* model = new(std::nothrow) HeaderModel;
 	Reference<HeaderModel> modelReference(model, true);
 	SetModel(model);
}


HeaderView::~HeaderView()
{
	SetModel(NULL);
}


void
HeaderView::Draw(BRect updateRect)
{
	if (fModel == NULL)
		return;

	_ValidateHeadersLayout();

	DefaultHeaderRenderer defaultRenderer;
	float bottom = Bounds().Height();

	for (int32 i = 0; HeaderEntry* entry = fHeaderEntries.ItemAt(i); i++) {
		if (Header* header = fModel->HeaderAt(i)) {
			HeaderRenderer* renderer = header->GetHeaderRenderer();
			if (renderer == NULL)
				renderer = &defaultRenderer;

			BRect frame(entry->position, 0, entry->position + entry->width - 1,
				bottom);
			renderer->DrawHeader(this, frame, updateRect, header, 0);
				// TODO: flags!
		}
	}
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


status_t
HeaderView::SetModel(HeaderModel* model)
{
	if (model == fModel)
		return B_OK;
printf("HeaderView::SetModel(%p)\n", model);

	if (fModel != NULL) {
		// remove all headers
		for (int32 i = 0; HeaderEntry* entry = fHeaderEntries.ItemAt(i); i++)
{
printf("  unsetting entry %p, header: %p\n", entry, entry->header);
			entry->header->RemoveListener(this);
}
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
printf("HeaderView::HeaderAdded(%p, %ld): header: %p, entry: %p\n", model, index, entry->header, entry);
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


// #pragma mark - HeaderViewListener


HeaderViewListener::~HeaderViewListener()
{
}
