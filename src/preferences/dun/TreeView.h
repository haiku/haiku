/*

The Amazing TreeView Header !

Author: Misza (misza@ihug.com.au)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

#include <iostream>
#include <stdio.h>
#include <Application.h>
#include <InterfaceKit.h>

class TreeView : public BView
{
	public:
		TreeView(BRect r);
		virtual void Draw(BRect updateRect);//Draws the little triangle depending on the state
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		bool TreeViewStatus() { return status;}
		void SetTreeViewStatus(bool condition);
	private:
		bool status;//0 == off && 1 == on
		bool down;
};
#endif
