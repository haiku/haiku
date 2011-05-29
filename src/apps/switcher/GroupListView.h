/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GROUP_LIST_VIEW_H
#define GROUP_LIST_VIEW_H


#include <GroupView.h>
#include <Roster.h>


class GroupListModel {
public:
	virtual	int32				CountItems() = 0;
	virtual void*				ItemAt(int32 index) = 0;

	virtual int32				CountGroups() = 0;
	virtual void*				GroupAt(int32 index) = 0;
	virtual void*				GroupForItemAt(int32 index) = 0;
};


class ListItemRenderer {
public:
	virtual void				SetTo(BView* owner, void* item) = 0;

	virtual BSize				MinSize() = 0;
	virtual BSize				MaxSize() = 0;
	virtual BSize				PreferredSize() = 0;

	virtual void				Draw(BView* owner, BRect frame, int32 index,
									bool selected) = 0;
};


class GroupListView : public BView {
public:
								GroupListView(const char* name,
									GroupListModel* model = NULL,
									enum orientation orientation = B_VERTICAL,
									float spacing = 0);
	virtual						~GroupListView();

			GroupListModel*		Model() const
									{ return fModel; }
	virtual void				SetModel(GroupListModel* model);

			ListItemRenderer*	ItemRenderer() const
									{ return fItemRenderer; }
	virtual	void				SetItemRenderer(ListItemRenderer* renderer);
			ListItemRenderer*	GroupRenderer() const
									{ return fGroupRenderer; }
	virtual	void				SetGroupRenderer(ListItemRenderer* renderer);

			BMessage*			SelectionMessage() const
									{ return fSelectionMessage; }
	virtual void				SetSelectionMessage(BMessage* message,
									BMessenger target);

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				MouseDown(BPoint point);
	virtual	void				Draw(BRect updateRect);

private:
			void				_Draw(BLayoutItem* item, BRect updateRect);
			BLayoutItem*		_ItemAt(BLayoutItem* item, BPoint point);

private:
			GroupListModel*		fModel;
			ListItemRenderer*	fItemRenderer;
			ListItemRenderer*	fGroupRenderer;

			BMessage*			fSelectionMessage;
			BMessenger			fSelectionTarget;
};


#endif	// GROUP_LIST_VIEW_H
