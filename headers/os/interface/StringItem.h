////////////////////////////////////////////////////////////////////////////////
//
//  File:           StringItem.h
//
//  Description:
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _STRING_ITEM_H
#define _STRING_ITEM_H

#include <ListItem.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS
{
#endif

/*----------------------------------------------------------------*/
/*----- BStringItem class ----------------------------------------*/

class BStringItem : public BListItem
{
public:
  BStringItem(const char* text, uint32 outlineLevel = 0, bool expanded = true) ;
  BStringItem(BMessage* data) ;
virtual ~MyStringItem() ;

static  BArchivable	*Instantiate(BMessage* data);
virtual status_t    Archive(BMessage* data, bool deep = true) const ;

virtual	void        DrawItem(BView* owner, BRect frame, bool complete = false) ;
virtual	void        SetText(const char* text) ;
		const char* Text() const ;
virtual	void        Update(BView* owner, const BFont* font) ;

virtual status_t    Perform(perform_code d, void* arg);

private:

  BStringItem(const BStringItem &) ;
  BStringItem& operator=(const BStringItem &) ;

  char*   fText;
  float   fBaselineOffset ;
} ;

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif /* _STRING_ITEM_H */
