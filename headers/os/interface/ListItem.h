////////////////////////////////////////////////////////////////////////////////
//
//  File:           ListItem.h
//
//  Description:    BListView represents a one-dimensional list view.
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _LIST_ITEM_H
#define _LIST_ITEM_H

#include <BeBuild.h>
#include <Archivable.h>
#include <Rect.h>

class BFont ;
class BMessage ;
class BOutlineListView ;
class BView ;

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS
{
#endif

/*----------------------------------------------------------------*/
/*----- BListItem class ------------------------------------------*/

/**
 * A BListItem represents a single item in a BListView (or
 * BOutlineListView). The BListItem object provides drawing
 * instructions that can draw the item (through DrawItem()), and
 * keeps track of the item's state. To use a BListItem, you must
 * add it to a BListView through BListView::AddItem()
 * (BOutlineListView provides additional item-adding functions).
 * The BListView object provides the drawing context for the
 * BListItem's DrawItem() function.
 *
 * BListItem is abstract; each subclass must implement DrawItem().
 * BStringItem—the only BListItem subclass provided by
 * Be—implements the function to draw the item as a line of text.
 */

class BListItem : public BArchivable
{
public:
        BListItem(uint32 outlineLevel = 0, bool expanded = true) ;
        BListItem(BMessage* data) ;
virtual ~BListItem() ;

virtual status_t  Archive(BMessage* data, bool deep = true) const ;

        float     Height() const ;
        float     Width() const ;
        bool      IsSelected() const ;
        void      Select() ;
        void      Deselect() ;

virtual void      SetEnabled(bool on) ;
        bool      IsEnabled() const ;

        void      SetHeight(float height) ;
        void      SetWidth(float width) ;
virtual void      DrawItem(BView* owner, BRect bounds, bool complete = false) = 0 ;
virtual void      Update(BView* owner, const BFont* font) ;

virtual status_t  Perform(perform_code d, void* arg) ;

        bool      IsExpanded() const ;
        void      SetExpanded(bool expanded) ;
        uint32    OutlineLevel() const ;

/*----- Private or reserved -----------------------------------------*/
private:
friend class BOutlineListView ;

        BListItem(const BListItem&) ;
        
        BListItem& operator=(const BListItem&) ;

  float   fWidth ;
  float   fHeight ;
  uint32  fLevel ;
  bool    fSelected ;
  bool    fEnabled ;

  bool fExpanded ;
  bool fHasSubItems ;
  bool fVisible ;
} ;

// in the interface kit the BStringItem class is declared within ListItem.h
#ifndef USE_OPENBEOS_NAMESPACE
#include "StringItem.h"
#endif

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif /* _LIST_ITEM_H */
