/*
 * A BStringItem modified such that it holds
 * the BEntry object it corresponds with
 */
 
#include "KeymapListItem.h"


KeymapListItem::KeymapListItem( BEntry *keymap )
	:	BStringItem( "" )
{
	char	name[B_FILE_NAME_LENGTH];
	
	fKeymap = keymap;
	keymap->GetName( name );
	
	SetText( name );
}

KeymapListItem::~KeymapListItem()
{
	delete fKeymap;
}

BEntry* KeymapListItem::KeymapEntry()
{
	return fKeymap;
}
