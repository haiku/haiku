/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 */
#ifndef _SUB_WINDOW_LIST_H_
#define _SUB_WINDOW_LIST_H_


#include <List.h>
#include <Window.h>


class WindowLayer;

class SubWindowList : public BList {
	public:
		SubWindowList();
		virtual ~SubWindowList();

		void AddWindowLayer(WindowLayer *windowLayer);

		// special
		void AddSubWindowList(SubWindowList *list);
	
		// debugging methods
		void PrintToStream() const;
};

#endif	/* _SUB_WINDOW_LIST_H_ */
