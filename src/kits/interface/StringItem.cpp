////////////////////////////////////////////////////////////////////////////////
//
//  File:           StringItem.cpp
//
//  Description:    BListView represents a one-dimensional list view.
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#include "StringItem.h"
#include <Font.h>
#include <Message.h>
#include <View.h>

#ifdef USE_OPENBEOS_NAMESPACE
using namespace OpenBeOS
#endif

/*----------------------------------------------------------------*/
/*----- BStringItem class ----------------------------------------*/


BStringItem::BStringItem(const char* text, uint32 outlineLevel /* = 0 */,
                         bool expanded /* = true */)
 : BListItem(outlineLevel, expanded), fBaselineOffset(0),
   fText(new char[strlen(text) + 1])
{
  // copy the text !!!!!
  strcpy(fText, text) ;
}

BStringItem::~BStringItem()
{
  if (fText != NULL) {
    delete fText ;
    fText = NULL ;
  }
}

BStringItem::BStringItem(BMessage* data)
 : BListItem(data) //First call the contructor of the base class
{
  char* text ;
  if (data->FindString("_label", const_cast<const char**>(&text)) == B_OK)
  {
    fText = new char[strlen(text) + 1] ;
    strcpy(fText, text) ;
  }

}

BArchivable*
BStringItem::Instantiate(BMessage* data)
{
  if (validate_instantiation(data, "BStringItem"))
    return new MyStringItem(data) ;

  return NULL ;
}

status_t
BStringItem::Archive(BMessage* data, bool deep /* = true */) const
{
  status_t tReturn (BListItem::Archive(data, deep)) ;

  if (tReturn != B_OK)
    return tReturn ;

  if (data->AddString("class", "BStringItem") != B_OK)
    return tReturn ;

  return tReturn = data->AddString("_label", fText) ;
}

// The colors need to be set correct
void
BStringItem::DrawItem(BView* owner, BRect frame, bool complete /* = false */)
{
  if (IsSelected() || complete)
  {
    if (IsSelected())
    {
      owner->SetHighColor(
        tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_HIGHLIGHT_BACKGROUND_TINT)) ;
      owner->SetLowColor(owner->HighColor()) ;
    }
    else {
      owner->SetHighColor(owner->ViewColor()) ;
      owner->SetLowColor(owner->ViewColor()) ;
    }

    owner->FillRect(frame) ;
  }

  BFont font ;
  owner->GetFont(&font) ;

  // Get the height of the font to place the pen to
  // the right position
  font_height height ;
  font.GetHeight(&height) ;

  owner->MovePenTo(frame.left + 4, frame.bottom - height.descent) ;
  owner->SetHighColor(IsEnabled() ? ui_color(B_MENU_ITEM_TEXT_COLOR)
    : tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT)) ;

  owner->DrawString(fText) ;
}

void
BStringItem::SetText(const char* text)
{
  if (fText != NULL)
    delete fText ;

  if (text != NULL)
  {
    fText = new char[strlen(text) + 1] ;
    strcpy(fText, text) ; // copy text
  }
  else {
    fText = NULL ;
  }
}

inline const char*
BStringItem::Text() const
{
  return fText ;
}

void
BStringItem::Update(BView* owner, const BFont* font)
{
  BListItem::Update(owner, font) ;

  // Set the width to the width of the string
  SetWidth(font->StringWidth(fText)) ;
}

status_t
BStringItem::Perform(perform_code d, void* arg)
{
  status_t retval = B_OK ;
  return retval ;
}

