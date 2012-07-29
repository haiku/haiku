/*
 * Copyright 2006-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MENU_ITEM_H
#define _MENU_ITEM_H
 

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Invoker.h>
#include <Menu.h>

class BMessage;
class BWindow;


class BMenuItem : public BArchivable, public BInvoker {
public:
								BMenuItem(const char* label, BMessage* message,
									char shortcut = 0, uint32 modifiers = 0);
								BMenuItem(BMenu* menu,
									BMessage* message = NULL);
								BMenuItem(BMessage* data);
	virtual						~BMenuItem();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				SetLabel(const char* name);
	virtual	void				SetEnabled(bool enabled);
	virtual	void				SetMarked(bool marked);
	virtual	void				SetTrigger(char trigger);
	virtual	void				SetShortcut(char shortcut, uint32 modifiers);

			const char*			Label() const;
			bool				IsEnabled() const;
			bool				IsMarked() const;
			char				Trigger() const;
			char				Shortcut(uint32* _modifiers = NULL) const;

			BMenu*				Submenu() const;
			BMenu*				Menu() const;
			BRect				Frame() const;

protected:
	virtual	void				GetContentSize(float* _width, float* _height);
	virtual	void				TruncateLabel(float maxWidth, char* newLabel);
	virtual	void				DrawContent();
	virtual	void				Draw();
	virtual	void				Highlight(bool enabled);
			bool				IsSelected() const;
			BPoint				ContentLocation() const;

private:
	friend class BMenu;
	friend class BPopUpMenu;
	friend class BMenuBar;

	virtual	void				_ReservedMenuItem1();
	virtual	void				_ReservedMenuItem2();
	virtual	void				_ReservedMenuItem3();
	virtual	void				_ReservedMenuItem4();

			void				Install(BWindow* window);
			void				Uninstall();
			void				SetSuper(BMenu* superMenu);
			void				Select(bool select);
			void				SetAutomaticTrigger(int32 index,
									uint32 trigger);

protected:
	virtual	status_t			Invoke(BMessage* message = NULL);

private:
								BMenuItem(const BMenuItem& other);
			BMenuItem&			operator=(const BMenuItem& other);

			void				_InitData();
			void				_InitMenuData(BMenu* menu);

			void				_DrawMarkSymbol();
			void				_DrawShortcutSymbol();
			void				_DrawSubmenuSymbol();
			void				_DrawControlChar(char shortcut, BPoint where);

private:
			char*				fLabel;
			BMenu*				fSubmenu;
			BWindow*			fWindow;
			BMenu*				fSuper;
			BRect				fBounds;
			uint32				fModifiers;
			float				fCachedWidth;
			int16				fTriggerIndex;
			char				fUserTrigger;
			char				fShortcutChar;
			bool				fMark;
			bool				fEnabled;
			bool				fSelected;
			uint32				fTrigger;

			uint32				_reserved[3];
};

// BSeparatorItem now has its own declaration file, but for source
// compatibility we're exporting that class from here too.
#include <SeparatorItem.h>

#endif // _MENU_ITEM_H
