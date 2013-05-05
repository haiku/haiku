/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "GroupListView.h"

#include <map>

#include <Window.h>


class RendererLayoutItem : public BAbstractLayout {
public:
	RendererLayoutItem(BView* owner, int32 index, void* item,
		ListItemRenderer*& renderer)
		:
		fOwner(owner),
		fIndex(index),
		fItem(item),
		fRenderer(renderer)
	{
	}

	ListItemRenderer* Renderer() const
	{
		fRenderer->SetTo(fOwner, fItem);
		return fRenderer;
	}

	int32 Index() const
	{
		return fIndex;
	}

	void* Item() const
	{
		return fItem;
	}

	virtual	BSize BaseMinSize()
	{
		fRenderer->SetTo(fOwner, fItem);
		return fRenderer->MinSize();
	}

	virtual	BSize BasePreferredSize()
	{
		fRenderer->SetTo(fOwner, fItem);
		return fRenderer->PreferredSize();
	}

protected:
	virtual	void DoLayout()
	{
		// shouldn't ever be called?
	}

private:
	BView*				fOwner;
	int32				fIndex;
	void*				fItem;
	ListItemRenderer*&	fRenderer;
};


GroupListView::GroupListView(const char* name, GroupListModel* model,
	enum orientation orientation, float spacing)
	:
	BView(NULL, B_WILL_DRAW, new BGroupLayout(orientation, spacing)),
	fModel(NULL),
	fItemRenderer(NULL),
	fGroupRenderer(NULL),
	fSelectionMessage(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetModel(model);
}


GroupListView::~GroupListView()
{
	delete fModel;
	delete fItemRenderer;
	delete fGroupRenderer;
	delete fSelectionMessage;
}


void
GroupListView::SetModel(GroupListModel* model)
{
	// TODO: remove all previous
	// TODO: add change mechanism
	// TODO: use a "virtual" BGroupLayout (ie. one that create its layout items
	//		on the fly).
	fModel = model;

	std::map<addr_t, BGroupLayout*> groupMap;

	int32 groupCount = model->CountGroups();
	for (int groupIndex = 0; groupIndex < groupCount; groupIndex++) {
		BGroupLayout* groupItem = new BGroupLayout(B_VERTICAL, 0);
		groupItem->SetVisible(false);
		AddChild(groupItem);

		void* group = model->GroupAt(groupIndex);
		groupMap[(addr_t)group] = groupItem;
		groupItem->AddItem(new RendererLayoutItem(this, groupIndex, group,
			fGroupRenderer));
	}

	int32 itemCount = model->CountItems();
	for (int itemIndex = 0; itemIndex < itemCount; itemIndex++) {
		void* group = model->GroupForItemAt(itemIndex);
		if (group == NULL)
			continue;

		BGroupLayout* groupItem = groupMap[(addr_t)group];
		if (groupItem == NULL)
			continue;

		groupItem->SetVisible(true);

		RendererLayoutItem* rendererItem = new RendererLayoutItem(this,
			itemIndex, model->ItemAt(itemIndex), fItemRenderer);
		groupItem->AddItem(rendererItem);
	}
}


void
GroupListView::SetItemRenderer(ListItemRenderer* renderer)
{
	fItemRenderer = renderer;
	InvalidateLayout();
}


void
GroupListView::SetGroupRenderer(ListItemRenderer* renderer)
{
	fGroupRenderer = renderer;
	InvalidateLayout();
}


void
GroupListView::SetSelectionMessage(BMessage* message, BMessenger target)
{
	fSelectionMessage = message;
	fSelectionTarget = target;
}


void
GroupListView::AttachedToWindow()
{
}


void
GroupListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
GroupListView::MouseDown(BPoint point)
{
	if (fSelectionMessage == NULL)
		return;

	BLayoutItem* item = _ItemAt(GetLayout(), point);

	if (RendererLayoutItem* rendererItem
			= dynamic_cast<RendererLayoutItem*>(item)) {
		BMessage message(*fSelectionMessage);

		int32 buttons = 0;
		if (Window()->CurrentMessage() != NULL)
			buttons = Window()->CurrentMessage()->FindInt32("buttons");
		if (buttons != 0)
			message.AddInt32("buttons", buttons);
		message.AddInt32("index", rendererItem->Index());
		message.AddPointer(rendererItem->Renderer() == fGroupRenderer
			? "group" : "item", rendererItem->Item());

		fSelectionTarget.SendMessage(&message);
	}
}


void
GroupListView::Draw(BRect updateRect)
{
	_Draw(GetLayout(), updateRect);
}


void
GroupListView::_Draw(BLayoutItem* item, BRect updateRect)
{
	if (RendererLayoutItem* rendererItem
			= dynamic_cast<RendererLayoutItem*>(item)) {
		ListItemRenderer* renderer = rendererItem->Renderer();
		renderer->Draw(this, rendererItem->Frame(), rendererItem->Index(),
			false);
	} else if (BLayout* layout = dynamic_cast<BLayout*>(item)) {
		for (int i = 0; i < layout->CountItems(); i++) {
			item = layout->ItemAt(i);
			if (!item->IsVisible() || !item->Frame().Intersects(updateRect))
				continue;

			_Draw(item, updateRect);
		}
	}
}


BLayoutItem*
GroupListView::_ItemAt(BLayoutItem* item, BPoint point)
{
	if (RendererLayoutItem* rendererItem
			= dynamic_cast<RendererLayoutItem*>(item))
		return rendererItem;

	if (BLayout* layout = dynamic_cast<BLayout*>(item)) {
		for (int i = 0; i < layout->CountItems(); i++) {
			item = layout->ItemAt(i);
			if (!item->IsVisible() || !item->Frame().Contains(point))
				continue;

			return _ItemAt(item, point);
		}
	}

	return item;
}
