/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef TAB_MANAGER_H
#define TAB_MANAGER_H

#include <Messenger.h>
#include <TabView.h>

enum {
	TAB_CHANGED = 'tcha',
	CLOSE_TAB = 'cltb'
};

class BBitmap;
class BCardLayout;
class BGroupView;
class BMenu;
class TabContainerGroup;
class TabContainerView;
class TabManagerController;

class TabManager {
public:
								TabManager(const BMessenger& target,
									BMessage* newTabMessage);
	virtual						~TabManager();

			void				SetTarget(const BMessenger& target);
			const BMessenger&	Target() const;

#if INTEGRATE_MENU_INTO_TAB_BAR
			BMenu*				Menu() const;
#endif

			BView*				TabGroup() const;
			BView*				GetTabContainerView() const;
			BView*				ContainerView() const;

			BView*				ViewForTab(int32 tabIndex) const;
			int32				TabForView(const BView* containedView) const;
			bool				HasView(const BView* containedView) const;

			void				SelectTab(int32 tabIndex);
			void				SelectTab(const BView* containedView);
			int32				SelectedTabIndex() const;
			void				CloseTab(int32 tabIndex);

			void				AddTab(BView* view, const char* label,
									int32 index = -1);
			BView*				RemoveTab(int32 index);
			int32				CountTabs() const;

			void				SetTabLabel(int32 tabIndex, const char* label);
	const	BString&			TabLabel(int32);
			void				SetTabIcon(const BView* containedView,
									const BBitmap* icon);
			void				SetCloseButtonsAvailable(bool available);

private:
#if INTEGRATE_MENU_INTO_TAB_BAR
			BMenu*				fMenu;
#endif
			TabContainerGroup*	fTabContainerGroup;
			TabContainerView*	fTabContainerView;
			BView*				fContainerView;
			BCardLayout*		fCardLayout;
			TabManagerController* fController;

			BMessenger			fTarget;
};

#endif // TAB_MANAGER_H
