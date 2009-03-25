/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYMAP_MESSAGE_FILTER_H
#define KEYMAP_MESSAGE_FILTER_H


#include <MessageFilter.h>

class Keymap;


class KeymapMessageFilter : public BMessageFilter {
public:
							KeymapMessageFilter(
								message_delivery delivery = B_ANY_DELIVERY,
               					message_source source = B_ANY_SOURCE,
               					Keymap* keymap = NULL);
	virtual					~KeymapMessageFilter();

			void			SetKeymap(Keymap* keymap);

	virtual	filter_result	Filter(BMessage* message, BHandler** _target);

private:
			Keymap*			fKeymap;
};

#endif	// KEYMAP_MESSAGE_FILTER_H
