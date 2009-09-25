/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *  FILE:	glutMenu.cpp
 *
 *	DESCRIPTION:	code for popup menu handling
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>
#include "glutint.h"
#include "glutState.h"

/***********************************************************
 *	Private variables
 ***********************************************************/
static GlutMenu **menuList = 0;
static int menuListSize = 0;

/***********************************************************
 *	FUNCTION:	getUnusedMenuSlot
 *
 *	DESCRIPTION:  helper function to get a new menu slot
 ***********************************************************/
GlutMenu *__glutGetMenuByNum(int menunum)
{
  if (menunum < 1 || menunum > menuListSize) {
    return NULL;
  }
  return menuList[menunum - 1];
}

/***********************************************************
 *	FUNCTION:	getUnusedMenuSlot
 *
 *	DESCRIPTION:  helper function to get a new menu slot
 ***********************************************************/
static int
getUnusedMenuSlot(void)
{
  int i;

  /* Look for allocated, unused slot. */
  for (i = 0; i < menuListSize; i++) {
    if (!menuList[i]) {
      return i;
    }
  }
  /* Allocate a new slot. */
  menuListSize++;
  menuList = (GlutMenu **)
      realloc(menuList, menuListSize * sizeof(GlutMenu *));
  if (!menuList)
    __glutFatalError("out of memory.");
  menuList[menuListSize - 1] = NULL;
  return menuListSize - 1;
}

/***********************************************************
 *	FUNCTION:	glutCreateMenu (6.1)
 *
 *	DESCRIPTION:  create a new menu
 ***********************************************************/
int APIENTRY 
glutCreateMenu(GLUTselectCB selectFunc)
{
  GlutMenu *menu;
  int menuid;

  menuid = getUnusedMenuSlot();
  menu = new GlutMenu(menuid, selectFunc);	// constructor sets up members
  menuList[menuid] = menu;
  gState.currentMenu = menu;
  return menuid + 1;
}

/***********************************************************
 *	FUNCTION:	glutSetMenu (6.2)
 *				glutGetMenu
 *
 *	DESCRIPTION:  set and get the current menu
 ***********************************************************/
int APIENTRY 
glutGetMenu(void)
{
  if (gState.currentMenu) {
    return gState.currentMenu->id + 1;
  } else {
    return 0;
  }
}

void APIENTRY 
glutSetMenu(int menuid)
{
  GlutMenu *menu;

  if (menuid < 1 || menuid > menuListSize) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  menu = menuList[menuid - 1];
  if (!menu) {
    __glutWarning("glutSetMenu attempted on bogus menu.");
    return;
  }
  gState.currentMenu = menu;
}

/***********************************************************
 *	FUNCTION:	glutDestroyMenu (6.3)
 *
 *	DESCRIPTION:  destroy the specified menu
 ***********************************************************/
void APIENTRY 
glutDestroyMenu(int menunum)
{
  GlutMenu *menu = __glutGetMenuByNum(menunum);
  menuList[menunum - 1] = 0;
  if (gState.currentMenu == menu) {
    gState.currentMenu = 0;
  }
  delete menu;
}

/***********************************************************
 *	FUNCTION:	glutAddMenuEntry (6.4)
 *
 *	DESCRIPTION:  add a new menu item
 ***********************************************************/
void
glutAddMenuEntry(const char *label, int value)
{
	new GlutMenuItem(gState.currentMenu, false, value, label);
}

/***********************************************************
 *	FUNCTION:	glutAddSubMenu (6.5)
 *
 *	DESCRIPTION:  add a new submenu
 ***********************************************************/
void
glutAddSubMenu(const char *label, int menu)
{
	new GlutMenuItem(gState.currentMenu, true, menu-1, label);
}

/***********************************************************
 *	FUNCTION:	glutChangeToMenuEntry (6.6)
 *
 *	DESCRIPTION:  change menuitem into a menu entry
 ***********************************************************/
void
glutChangeToMenuEntry(int num, const char *label, int value)
{
  GlutMenuItem *item;
  int i;

  i = gState.currentMenu->num;
  item = gState.currentMenu->list;
  while (item) {
    if (i == num) {
      free(item->label);
      item->label = strdup(label);
      item->isTrigger = false;
      item->value = value;
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

/***********************************************************
 *	FUNCTION:	glutChangeToSubMenu (6.7)
 *
 *	DESCRIPTION:  change menuitem into a submenu
 ***********************************************************/
void
glutChangeToSubMenu(int num, const char *label, int menu)
{
  GlutMenuItem *item;
  int i;

  i = gState.currentMenu->num;
  item = gState.currentMenu->list;
  while (item) {
    if (i == num) {
      free(item->label);
      item->label = strdup(label);
      item->isTrigger = true;
      item->value = menu-1;
      return;
    }
    i--;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

/***********************************************************
 *	FUNCTION:	glutRemoveMenuItem (6.8)
 *
 *	DESCRIPTION:  remove a menu item
 ***********************************************************/
void
glutRemoveMenuItem(int num)
{
  GlutMenuItem *item, **prev;
  int i;

  i = gState.currentMenu->num;
  prev = &gState.currentMenu->list;
  item = gState.currentMenu->list;

  while (item) {
    if (i == num) {
      gState.currentMenu->num--;

      /* Patch up menu's item list. */
      *prev = item->next;

      free(item->label);
      delete item;
      return;
    }
    i--;
    prev = &item->next;
    item = item->next;
  }
  __glutWarning("Current menu has no %d item.", num);
}

/***********************************************************
 *	FUNCTION:	glutAttachMenu (6.9)
 *				glutDetachMenu
 *
 *	DESCRIPTION:  attach and detach menu from view
 ***********************************************************/
void
glutAttachMenu(int button)
{
	gState.currentWindow->menu[button] = gState.currentMenu->id + 1;
}

void
glutDetachMenu(int button)
{
	gState.currentWindow->menu[button] = 0;
}

/***********************************************************
 *	CLASS:		GlutMenu
 *
 *	FUNCTION:	CreateBMenu
 *
 *	DESCRIPTION:  construct a BPopupMenu for this menu
 ***********************************************************/
BMenu *GlutMenu::CreateBMenu(bool toplevel) {
	BMenu *bpopup;
	if(toplevel) {
		bpopup = new GlutPopUp(id+1);
	} else {
		bpopup = new BMenu("");
	}
	GlutMenuItem *item = list;
	while (item) {
		GlutBMenuItem *bitem;
		if(item->isTrigger) {
			// recursively call CreateBMenu
			bitem = new GlutBMenuItem(menuList[item->value]->CreateBMenu(false));
			bitem->SetLabel(item->label);
			bitem->menu = 0;	// real menu items start at 1
			bitem->value = 0;
		} else {
			bitem = new GlutBMenuItem(item->label);
			bitem->menu = id + 1;
			bitem->value = item->value;
		}
		bpopup->AddItem(bitem, 0);
		item = item->next;
	}
	return bpopup;
}

/***********************************************************
 *	CLASS:		GlutMenu
 *
 *	FUNCTION:	(destructor)
 *
 *	DESCRIPTION:  destroy the menu and its items (but not submenus!)
 ***********************************************************/
GlutMenu::~GlutMenu() {
	while (list) {
		GlutMenuItem *next = list->next;
		delete list;
		list = next;
	}
}

/***********************************************************
 *	CLASS:		GlutMenuItem
 *
 *	FUNCTION:	(constructor)
 *
 *	DESCRIPTION:  construct the new menu item and add to parent
 ***********************************************************/
GlutMenuItem::GlutMenuItem(GlutMenu *n_menu, bool n_trig, int n_value, const char *n_label)
{
	menu = n_menu;
	isTrigger = n_trig;
	value = n_value;
	label = strdup(n_label);
	next = menu->list;
	menu->list = this;
	menu->num++;
}
