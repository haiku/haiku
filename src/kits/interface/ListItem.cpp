////////////////////////////////////////////////////////////////////////////////
//
//  File:           ListItem.h
//
//  Description:    BListView represents a one-dimensional list view.
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#include <Font.h>
#include <Message.h>
#include <View.h>
#include "MyListItem.h"

/*----------------------------------------------------------------*/
/*----- BListItem class ------------------------------------------*/

/**
 * The constructors create a new BListItem object. The level and
 * expanded arguments are only used if the item is added to a 
 * BOutlineListView object; see BOutlineListView::AddItem() for
 * more information. The destructor is empty.
 *
 * @param outlineLevel Level of the 
 */

BListItem::BListItem(uint32 outlineLevel /* = 0 */, bool expanded /* = true */)
 : fWidth(0), fHeight(0), fLevel(outlineLevel), fSelected(false), fEnabled(true),
   fExpanded(expanded), fHasSubItems(false), fVisible(true)
{
}

BListItem::BListItem(BMessage* data)
 : BArchivable(data), // first call the constructor of the base class
   fWidth(0), fHeight(0), fLevel(0), fSelected(false), fEnabled(true),
   fExpanded(true), fHasSubItems(false), fVisible(true)
{
  // get the data from the message

  if (data->FindBool("_sel", &fSelected) != B_OK) 
    fSelected = false ;

  if (data->FindBool("_disable", !fEnabled) != B_OK)
    fEnabled = true ;

  if (data->FindBool("_li_expanded", fExpanded) != B_OK)
    fExpanded = true ;

  if (data->FindInt32("_li_outline_level", fLevel) != B_OK)
    fLevel = 0 ;
}

BListItem::~BListItem()
{
  fWidth = 0 ;
  fHeight = 0 ;
  fLevel = 0 ;
  fSelected = false ;
  fEnabled = false ;
}

status_t
BListItem::Archive(BMessage* data, bool deep /* = true */) const
{
  status_t tReturn = BArchivable::Archive(data, deep) ;

  if (tReturn != B_OK)
    return tReturn ;

  tReturn = data->AddString("class", "BListItem") ;

  if (tReturn != B_OK)
    return tReturn ;

  if (fSelected == true)
  {
    tReturn = data->AddBool("_sel", fSelected) ;

    if (tReturn != B_OK)
      return tReturn ;
  }

  if (fEnabled == false)
  {
    tReturn = data->AddBool("_disable", !fEnabled) ;

    if (tReturn != B_OK)
      return tReturn ;
  }

  if (!fExpanded) {
    tReturn = data->AddInt32("_li_outline_level", fExpanded) ;
  }

  return tReturn ;
}

float
BListItem::Height() const
{
  return fHeight ;
}

float
BListItem::Width() const
{
  return fWidth ;
}

//inline
bool
BListItem::IsSelected() const
{
  return fSelected ;
}

//inline
void
BListItem::Select()
{
  fSelected = true ;
}

//inline
void
BListItem::Deselect()
{
  fSelected = false ;
}

//inline
void
BListItem::SetEnabled(bool on)
{
  fEnabled = on ;
}

//inline
bool
BListItem::IsEnabled() const
{
  return fEnabled ;
}

//inline void
void BListItem::SetHeight(float height)
{
  fHeight = height ;
}

//inline
void
BListItem::SetWidth(float width)
{
  fWidth = width ;
}

void
BListItem::Update(BView* owner, const BFont* font)
{
  // Set the width of the item to the width of the owner
  BRect rect(owner->Frame()) ;
  SetWidth(rect.right - rect.left) ;

  // Set the height of the item to the height of the font
  font_height height ;
  font->GetHeight(&height) ;
  SetHeight(height.leading + height.ascent + height.descent);
}

status_t
BListItem::Perform(perform_code d, void* arg)
{
  status_t tReturn = B_OK ;
  return tReturn ;
}

inline uint32
BListItem::OutlineLevel() const
{
  return fLevel ;
}
