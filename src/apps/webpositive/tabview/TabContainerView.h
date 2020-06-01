/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TAB_CONTAINER_VIEW_H
#define TAB_CONTAINER_VIEW_H

#include <GroupView.h>


class TabView;


class TabContainerView : public BGroupView {
public:
	class Controller {
	public:
		virtual	void			UpdateSelection(int32 index) = 0;
		virtual	bool			HasFrames() = 0;
		virtual	TabView*		CreateTabView() = 0;
		virtual	void			DoubleClickOutsideTabs() = 0;
		virtual	void			UpdateTabScrollability(bool canScrollLeft,
									bool canScrollRight) = 0;
		virtual	void			SetToolTip(const BString& text) = 0;
	};

public:
								TabContainerView(Controller* controller);
	virtual						~TabContainerView();

	virtual	BSize				MinSize();

	virtual	void				MessageReceived(BMessage*);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				DoLayout();

			void				AddTab(const char* label, int32 index = -1);
			void				AddTab(TabView* tab, int32 index = -1);
			TabView*			RemoveTab(int32 index);
			TabView*			TabAt(int32 index) const;

			int32				IndexOf(TabView* tab) const;

			int32				FirstTabIndex() { return 0; };
			int32				LastTabIndex()
									{ return GroupLayout() == NULL ? -1
										: GroupLayout()->CountItems() - 1; };
			int32				SelectedTabIndex()
									{ return fSelectedTab == NULL ? -1
										: IndexOf(fSelectedTab); };

			TabView*			SelectedTab() { return fSelectedTab; };

			void				SelectTab(int32 tabIndex);
			void				SelectTab(TabView* tab);

			void				SetTabLabel(int32 tabIndex, const char* label);

			void				SetFirstVisibleTabIndex(int32 index);
			int32				FirstVisibleTabIndex() const;
			int32				MaxFirstVisibleTabIndex() const;

			bool				CanScrollLeft() const;
			bool				CanScrollRight() const;

private:
			TabView*			_TabAt(const BPoint& where) const;
			void				_MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
			void				_ValidateTabVisibility();
			void				_UpdateTabVisibility();
			float				_AvailableWidthForTabs() const;
			void				_SendFakeMouseMoved();

private:
			TabView*			fLastMouseEventTab;
			bool				fMouseDown;
			uint32				fClickCount;
			TabView*			fSelectedTab;
			Controller*			fController;
			int32				fFirstVisibleTabIndex;
};

#endif // TAB_CONTAINER_VIEW_H
