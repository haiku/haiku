/*
 * A BStringItem modified such that it holds
 * the BEntry object it corresponds with
 */

#ifndef OBOS_KEYMAP_LIST_ITEM_H
#define OBOS_KEYMAP_LIST_ITEM_H

#include <be/interface/ListItem.h>
#include <be/storage/Entry.h>


class KeymapListItem : public BStringItem {
public:
	KeymapListItem( BEntry *keymap );
	~KeymapListItem();
	BEntry	*KeymapEntry();

protected:
	BEntry	*fKeymap;

};

#endif //OBOS_KEYMAP_LIST_ITEM_H