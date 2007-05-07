/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef MESSAGED_ITEM_H
#define MESSAGED_ITEM_H


#include <Window.h>
#include <ListItem.h>
#include <Message.h>
#include <ListItem.h>


class MessagedItem : public BStringItem {
	public:
		MessagedItem(const char* label, BMessage* information) : BStringItem(label)
		{
			fMessage = information;
		}

		~MessagedItem()
		{
			delete fMessage;
		}

		BMessage* getMessage() 
		{
			return fMessage;
		}

	protected:
		BMessage*   fMessage;

};


#endif	/* MESSAGED_ITEM_H */

