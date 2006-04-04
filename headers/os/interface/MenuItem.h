/*******************************************************************************
/
/	File:			MenuItem.h
/
/   Description:    BMenuItem represents a single item in a BMenu.
/                   BSeparatorItem is a cosmetic menu item that demarcates
/                   groups of other items.
/
/	Copyright 1994-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _MENU_ITEM_H
#define _MENU_ITEM_H
 
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Archivable.h>
#include <Invoker.h>
#include <Menu.h>			/* For convenience */

class BMessage;
class BWindow;

/*----------------------------------------------------------------*/
/*----- BMenuItem class ------------------------------------------*/

class BMenuItem : public BArchivable, public BInvoker {
public:
						BMenuItem(	const char *label,
									BMessage *message,
									char shortcut = 0,
									uint32 modifiers = 0);
						BMenuItem(BMenu *menu, BMessage *message = NULL);
						BMenuItem(BMessage *data);
virtual					~BMenuItem();
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;
	
virtual	void			SetLabel(const char *name);
virtual	void			SetEnabled(bool state);
virtual	void			SetMarked(bool state);
virtual void			SetTrigger(char ch);
virtual void			SetShortcut(char ch, uint32 modifiers);

		const char		*Label() const;
		bool			IsEnabled() const;
		bool			IsMarked() const;
		char			Trigger() const;
		char			Shortcut(uint32 *modifiers = NULL) const;
		
		BMenu			*Submenu() const;
		BMenu			*Menu() const;
		BRect			Frame() const;

protected:

virtual	void			GetContentSize(float *width, float *height);
virtual	void			TruncateLabel(float max, char *new_label);
virtual	void			DrawContent();
virtual	void			Draw();
virtual	void			Highlight(bool on);
		bool			IsSelected() const;
		BPoint			ContentLocation() const;

/*----- Private or reserved -----------------------------------------*/
private:
friend class BMenu;
friend class BPopUpMenu;
friend class BMenuBar;

virtual	void		_ReservedMenuItem1();
virtual	void		_ReservedMenuItem2();
virtual	void		_ReservedMenuItem3();
virtual	void		_ReservedMenuItem4();

					BMenuItem(const BMenuItem &);
		BMenuItem	&operator=(const BMenuItem &);

		void		InitData();
		void		InitMenuData(BMenu *menu);
		void		Install(BWindow *window);

/*----- Protected function -----------------------------------------*/
protected:
virtual	status_t	Invoke(BMessage *msg = NULL);

/*----- Private or reserved -----------------------------------------*/
private:
		void		Uninstall();
		void		SetSuper(BMenu *super);
		void		Select(bool on);
		void		DrawMarkSymbol(rgb_color bgColor);
		void		DrawShortcutSymbol();
		void		DrawSubmenuSymbol(rgb_color bgColor);
		void		_DrawControlChar(char shortcut, BPoint where);
		void		SetSysTrigger(char ch);

		char		*fLabel;
		BMenu		*fSubmenu;
		BWindow		*fWindow;
		BMenu		*fSuper;
		BRect		fBounds;
		uint32		fModifiers;
		float		fCachedWidth;
		int16		fTriggerIndex;
		char		fUserTrigger;
		char		fSysTrigger;
		char		fShortcutChar;
		bool		fMark;
		bool		fEnabled;
		bool		fSelected;

		uint32		_reserved[4];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

// We moved BSeparatorItem's declaration to its own file, but for source
// compatibility we have to export that class from here too.
#include <SeparatorItem.h>

#endif /* _MENU_ITEM_H */
