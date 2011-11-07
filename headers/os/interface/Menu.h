/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MENU_H
#define _MENU_H


#include <InterfaceDefs.h>
#include <List.h>
#include <View.h>

class BMenu;
class BMenuBar;
class BMenuItem;


namespace BPrivate {
	class BMenuWindow;
	class ExtraMenuData;
	class TriggerList;
	class MenuPrivate;
}

enum menu_layout {
	B_ITEMS_IN_ROW = 0,
	B_ITEMS_IN_COLUMN,
	B_ITEMS_IN_MATRIX
};

struct menu_info {
	float		font_size;
	font_family	f_family;
	font_style	f_style;
	rgb_color	background_color;
	int32		separator;
	bool		click_to_open;
	bool		triggers_always_shown;
};

status_t get_menu_info(menu_info* info);
status_t set_menu_info(menu_info* info);

typedef bool (*menu_tracking_hook)(BMenu* menu, void* state);


class BMenu : public BView {
public:
								BMenu(const char* title,
									menu_layout layout = B_ITEMS_IN_COLUMN);
								BMenu(const char* title, float width,
									float height);
								BMenu(BMessage* archive);

	virtual						~BMenu();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();
	virtual void				AllAttached();
	virtual void				AllDetached();

	virtual void				Draw(BRect updateRect);

	virtual void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				ResizeToPreferred();
	virtual	void				DoLayout();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);
			void				InvalidateLayout();
	virtual	void				InvalidateLayout(bool descendants);

	virtual void				MakeFocus(bool focus = true);

			bool				AddItem(BMenuItem* item);
			bool				AddItem(BMenuItem* item, int32 index);
			bool				AddItem(BMenuItem* item, BRect frame);
			bool				AddItem(BMenu* menu);
			bool				AddItem(BMenu* menu, int32 index);
			bool				AddItem(BMenu* menu, BRect frame);
			bool				AddList(BList* list, int32 index);

			bool				AddSeparatorItem();

			bool				RemoveItem(BMenuItem* item);
			BMenuItem*			RemoveItem(int32 index);
			bool				RemoveItems(int32 index, int32 count,
									bool deleteItems = false);
			bool				RemoveItem(BMenu* menu);

			BMenuItem*			ItemAt(int32 index) const;
			BMenu*				SubmenuAt(int32 index) const;
			int32				CountItems() const;
			int32				IndexOf(BMenuItem* item) const;
			int32				IndexOf(BMenu* menu) const;
			BMenuItem*			FindItem(uint32 command) const;
			BMenuItem*			FindItem(const char* name) const;

	virtual	status_t			SetTargetForItems(BHandler* target);
	virtual	status_t			SetTargetForItems(BMessenger messenger);
	virtual	void				SetEnabled(bool state);
	virtual	void				SetRadioMode(bool state);
	virtual	void				SetTriggersEnabled(bool state);
	virtual	void				SetMaxContentWidth(float maxWidth);

			void				SetLabelFromMarked(bool state);
			bool				IsLabelFromMarked();
			bool				IsEnabled() const;
			bool				IsRadioMode() const;
			bool				AreTriggersEnabled() const;
			bool				IsRedrawAfterSticky() const;
			float				MaxContentWidth() const;

			BMenuItem*			FindMarked();

			BMenu*				Supermenu() const;
			BMenuItem*			Superitem() const;


	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual status_t			Perform(perform_code d, void* arg);

protected:
								BMenu(BRect frame, const char* name,
									uint32 resizeMask, uint32 flags,
									menu_layout layout, bool resizeToFit);

	virtual	BPoint				ScreenLocation();

			void				SetItemMargins(float left, float top,
									float right, float bottom);
			void				GetItemMargins(float* _left, float* _top,
									float* _right, float* _bottom) const;

			menu_layout			Layout() const;

	virtual	void				Show();
			void				Show(bool selectFirstItem);
			void				Hide();
			BMenuItem*			Track(bool startOpened = false,
									BRect* specialRect = NULL);

public:
	enum add_state {
		B_INITIAL_ADD,
		B_PROCESSING,
		B_ABORT
	};
	virtual	bool				AddDynamicItem(add_state state);
	virtual void				DrawBackground(BRect update);

			void				SetTrackingHook(menu_tracking_hook hook,
									void* state);

private:
	friend class BMenuBar;
	friend class BPrivate::MenuPrivate;
	friend status_t _init_interface_kit_();
	friend status_t	set_menu_info(menu_info* info);
	friend status_t	get_menu_info(menu_info* info);

	struct LayoutData;

	virtual	void				_ReservedMenu3();
	virtual	void				_ReservedMenu4();
	virtual	void				_ReservedMenu5();
	virtual	void				_ReservedMenu6();

			BMenu&				operator=(const BMenu& other);

			void				_InitData(BMessage* archive);
			bool				_Show(bool selectFirstItem = false,
									bool keyDown = false);
			void				_Hide();
			BMenuItem*			_Track(int* action, long start = -1);

			void				_UpdateNavigationArea(BPoint position,
									BRect& navAreaRectAbove,
									BRect& navAreaBelow);

			void				_UpdateStateOpenSelect(BMenuItem* item,
									BPoint position, BRect& navAreaRectAbove,
									BRect& navAreaBelow,
									bigtime_t& selectedTime,
									bigtime_t& navigationAreaTime);
			void				_UpdateStateClose(BMenuItem* item,
									const BPoint& where,
									const uint32& buttons);

			bool				_AddItem(BMenuItem* item, int32 index);
			bool				_RemoveItems(int32 index, int32 count,
									BMenuItem* item, bool deleteItems = false);
			bool				_RelayoutIfNeeded();
			void				_LayoutItems(int32 index);
			BSize				_ValidatePreferredSize();
			void				_ComputeLayout(int32 index, bool bestFit,
									bool moveItems, float* width,
									float* height);
			void				_ComputeColumnLayout(int32 index, bool bestFit,
									bool moveItems, BRect& outRect);
			void				_ComputeRowLayout(int32 index, bool bestFit,
									bool moveItems, BRect& outRect);		
			void				_ComputeMatrixLayout(BRect& outRect);

			BRect				_CalcFrame(BPoint where, bool* scrollOn);

protected:
			void				_DrawItems(BRect updateRect);

private:
			bool				_OverSuper(BPoint loc);
			bool				_OverSubmenu(BMenuItem* item, BPoint loc);
			BPrivate::BMenuWindow* _MenuWindow();
			void				_DeleteMenuWindow();
			BMenuItem*			_HitTestItems(BPoint where,
									BPoint slop = B_ORIGIN) const;
			BRect				_Superbounds() const;
			void				_CacheFontInfo();

			void				_ItemMarked(BMenuItem* item);
			void				_Install(BWindow* target);
			void				_Uninstall();
			void				_SelectItem(BMenuItem* item,
									bool showSubmenu = true,
									bool selectFirstItem = false,
									bool keyDown = false);
			bool				_SelectNextItem(BMenuItem* item, bool forward);
			BMenuItem*			_NextItem(BMenuItem* item, bool forward) const;
			void				_SetIgnoreHidden(bool on);
			void				_SetStickyMode(bool on);
			bool				_IsStickyMode() const;

			// Methods to get the current modifier keycode
			void				_GetShiftKey(uint32 &value) const;
			void				_GetControlKey(uint32 &value) const;
			void				_GetCommandKey(uint32 &value) const;
			void				_GetOptionKey(uint32 &value) const;
			void				_GetMenuKey(uint32 &value) const;

			void				_CalcTriggers();
			bool				_ChooseTrigger(const char* title, int32& index,
									uint32& trigger,
									BPrivate::TriggerList& triggers);
			void				_UpdateWindowViewSize(const bool &updatePosition);
			bool				_AddDynamicItems(bool keyDown = false);
			bool				_OkToProceed(BMenuItem* item,
									bool keyDown = false);

			bool				_CustomTrackingWantsToQuit();

			int					_State(BMenuItem** _item = NULL) const;
			void				_InvokeItem(BMenuItem* item, bool now = false);
			void				_QuitTracking(bool onlyThis = true);

	static	menu_info			sMenuInfo;

			// Variables to keep track of what code is currently assigned to
			// each modifier key
	static	uint32				sShiftKey;
	static	uint32				sControlKey;
	static	uint32				sOptionKey;
	static	uint32				sCommandKey;
	static	uint32				sMenuKey;

			BMenuItem*			fChosenItem;
			BList				fItems;
			BRect				fPad;
			BMenuItem*			fSelected;
			BPrivate::BMenuWindow* fCachedMenuWindow;
			BMenu*				fSuper;
			BMenuItem*			fSuperitem;
			BRect				fSuperbounds;
			float				fAscent;
			float				fDescent;
			float				fFontHeight;
			uint32				fState;
			menu_layout			fLayout;
			BRect*				fExtraRect;
			float				fMaxContentWidth;
			BPoint*				fInitMatrixSize;
			BPrivate::ExtraMenuData* fExtraMenuData;

			LayoutData*			fLayoutData;

			int32				_reserved;

			char				fTrigger;
			bool				fResizeToFit;
			bool				fUseCachedMenuLayout;
			bool				fEnabled;
			bool				fDynamicName;
			bool				fRadioMode;
			bool				fTrackNewBounds;
			bool				fStickyMode;
			bool				fIgnoreHidden;
			bool				fTriggerEnabled;
			bool				fRedrawAfterSticky;
			bool				fAttachAborted;
};

#endif // _MENU_H
