////////////////////////////////////////////////////////////////////////////////
//
//  File:           ListView.cpp
//
//  Description:    BListView represents a one-dimensional list view.
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#include "ListView.h"
#include <Region.h>
#include <Window.h>
#include <ClassInfo.h>
#include <iostream>
#include <ScrollView.h>



struct track_data 
{
} ;

/**
 * Initializes the new BListView. The frame, name, resizingMode, and flags
 * arguments are identical to those declared for the BView class and are
 * passed unchanged to the BView constructor.
 *
 * The list type can be either:
 * B_SINGLE_SELECTION_LIST
 * The user can select only one item in the list at a time. This is the
 * default setting.
 * B_MULTIPLE_SELECTION_LIST
 * The user can select any number of items by holding down an Option
 * key (for discontinuous selections) or a Shift key (for contiguous
 * selections).
 */
BListView::BListView(BRect frame, const char *name,
                     list_view_type type /* = B_SINGLE_SELECTION_LIST */,
                     uint32 resizeMask   /* = B_FOLLOW_LEFT | B_FOLLOW_TOP */,
                     uint32 flags /* = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE */)
 : BView(frame, name, resizeMask, flags)
{
  InitObject(type) ;
}                     


BListView::BListView(BMessage* data)
 : BView(data)
{
  // if the pointer is null the object should be initialized anyway
  if (data == NULL) {
    InitObject(B_SINGLE_SELECTION_LIST) ;
    return ;
  }

  {
    if (data->FindInt32("_lv_type", fListType) != B_OK)
      fList = B_SINGLE_SELECTION_LIST ;

    // first check if a selection message is included in the messgage
    // if there is one allocate memory for it and get it.
    type_code code ;

    if (data->GetInfo("_2nd_msg", &code) == B_OK )
    {
      if (code == B_MESSAGE_TYPE)
      {
        fSelectMessage = new BMessage() ;
        data->FindMessage("_msg", fSelectMessage) ;
        BInvoker::SetMessage(fSelectMessage) ;

        // set the fSelectMessage back to NULL
        fSelectMessage = NULL ;
      }
    }

    // check if there is an invokation message stored
    // in the archiv and set it
    if (data->GetInfo("_msg", &code) == B_OK )
    {
      if (code == B_MESSAGE_TYPE)
      {
        fSelectMessage = new BMessage() ;
        data->FindMessage("_msg", fSelectMessage) ;
      }
      else {
        fSelectMessage = NULL ;
      }
    }

    int32 count ;

    // add the item archivied in the message if it was a deep copy
    if (data->GetInfo("_l_items", &code, &count) == B_OK)
    {
      if (code == B_MESSAGE_TYPE)
      {
        BMessage msg ;
        BArchivable* unarchived ;
        BListItem* item ;

        for (int32 i = 0 ; i < count ; ++i)
        {
          data->FindMessage("_l_items", i, &msg) ;
          unarchived = instantiate_object(data) ;
          item = dynamic_cast<BListItem*>(unarchived) ;

          if (item != NULL) {
            AddItem(item) ;
          }
        }
      }
    }
  }
}

/**
 * Frees the selection and invocation messages, if any, and any memory
 * allocated to hold the list of items, but not the items themselves.
 */
BListView::~BListView()
{
  // Remove all pointers from the list
  fList.MakeEmpty() ;
 
  // delete the select message if there is one
  // the invocation message is deleted by the destructor of BInvoker 
  if (fSelectMessage != NULL)
  {
    delete fSelectMessage ;
    fSelectMessage = NULL ;
  }

  if (fTrack != NULL)
  {
    delete fTrack ;
    fTrack = NULL ;
  }
}

/**
 * Returns a new BListView object, allocated by new and created
 * with the version of the constructor that takes a BMessage
 * archive. However, if the archive message doesn't contain
 * data for a BListView object, this function returns NULL.
 *
 * @param data Pointer to a BMessge object containing the data
 *             of an archieved BListView object.
 */
BArchivable*
BListView::Instantiate(BMessage* data)
{
  if (validate_instantiation(data, "BListView"))
    return new BListView(data) ;

  return NULL ;
}

status_t
BListView::Archive(BMessage* data, bool deep /* = true */) const
{
  if (!data)
    return B_ERROR ;

  BView::Archive(data, deep) ;

  data->AddString("class", "BListView") ;
  data->AddInt32("_lv_type", fListType) ;

  if (fSelectMessage) {
    data->AddMessage("_msg", fSelectMessage) ;
  }

  if (BInvoker::Message()) {
    data->AddMessage("_2nd_msg", BInvoker::Message()) ;
  }

  if (deep)
  {
    BMessage* msg ;

    for (int i = 0 ; i < fList.CountItems() ; ++i)
    {
      msg = new BMessage() ;
      ItemAt(i)->Archive(msg, deep) ;
      data->AddMessage("_l_items", msg) ;
    }
  }

  return B_OK ;
}

/**
 * Calls upon every item in the updateRect area of the view to draw itself.
 * Draw() is called for you whenever the list view is to be updated or
 * redisplayed; you don't need to call it yourself. You also don't need to
 * reimplement it; to change the way items are drawn, define a new version
 * of DrawItem() in a class derived from BListItem. 
 */
void
BListView::Draw(BRect updateRect)
{
  SetHighColor(0, 0 ,0) ;

  if (!fList.IsEmpty())
  {
    // Find the first and the last item which has to be updated
    float tmp = 0 ;
    int32 firstIndex = 0 ;

    for ( ; firstIndex < fList.CountItems() ; ++firstIndex)
    {
      float help = (ItemAt(firstIndex))->Height() ;
      tmp += help ;
      if (tmp >= updateRect.top)
        break ;
    }

    int32 lastIndex = firstIndex ;

    for (; lastIndex < fList.CountItems() ; ++lastIndex)
    {
      //Draw the item
      BRect rect(Bounds());
      rect.top = tmp - ItemAt(lastIndex)->Height() ;
      rect.bottom = tmp ;
      rect.right = 2000 ;

//      BRegion region ;
  //    region.Include(rect) ;

//      ConstrainClippingRegion(&region) ;
      ItemAt(lastIndex)->DrawItem(this, rect);//, true) ;

      if (tmp >= updateRect.bottom)
        break ;

      tmp += ItemAt(lastIndex)->Height() ;
    }
  }
}

void
BListView::MessageReceived(BMessage *msg)
{
  switch (msg->what)
  {
  // scripting stuff
  case B_GET_SUPPORTED_SUITES:
  {
    BMessage reply(B_REPLY) ;
    reply.AddString("suites", "suite/vnd.Be-list-view") ;
    msg->SendReply(&reply) ;
    break ;
  }
  case B_COUNT_PROPERTIES:
  {
    BMessage reply(B_REPLY) ;
    reply.AddInt32("result", fList.CountItems()) ;
    msg->SendReply(&reply) ;
    break ;
  }
  case B_EXECUTE_PROPERTY:
  break ;
  case B_GET_PROPERTY:
  break ;
  case B_SET_PROPERTY:
  break ;
  case B_MOUSE_WHEEL_CHANGED:
    break ;
  }
  
  BView::MessageReceived(msg) ;
}

/**
 * Responds to B_MOUSE_DOWN messages by selecting items, invoking them
 * (if the mouse-down event is the second of a double-click), and
 * autoscrolling the list (when the user drags with a mouse button down).
 * This function also calls InitiateDrag() to give derived classes the
 * opportunity to drag items. You can implement that function; you shouldn't
 * override (or call) this one.
 */
 // The docu says shift key and option key like Windows but it does not.
void
BListView::MouseDown(BPoint where)
{
  int32 index = IndexOf(where) ;

  if (index >= 0)
  {
    bool extend = false ;

    if ((fListType == B_MULTIPLE_SELECTION_LIST) && (modifiers() & B_SHIFT_KEY))
      extend = true ;

    if (!ItemAt(index)->IsSelected())
      Select(index, extend) ;
    else {
      ItemAt(index)->Deselect() ;
      InvalidateItem(index) ;
    }
  }

  MakeFocus() ;
}

/**
 * Permits the user to operate the list using the following keys:
 *
 * Up Arrow and Down Arrow
 * Select the items that are immediately before and immediately after
 * the currently selected item.
 *
 * Page Up and Page Down
 * Select the items that are one viewful above and below the currently
 * selected item—or the first and last items if there's no item a viewful away.
 *
 * Home and End
 * Select the first and last items in the list.
 *
 * Enter and the space bar
 * Invoke the current selection.
 *
 * This function also incorporates the inherited BView version so that the
 * Tab key can navigate to another view. KeyDown() is called to report
 * B_KEY_DOWN messages when the BListView is the focus view of the active
 * window; you shouldn't call it yourself.
 */
void
BListView::KeyDown(const char *bytes, int32 numBytes)
{
  switch (*bytes)
  {
  // pressing the home key the first item in the list is selected
  // and made visible
  case B_HOME:
  {
    if (CountItems() > 0)
    {
      Select(0) ;
      ScrollToSelection() ;
    }
    break;
  }
  // if the end key is pressed the last entry is selected and
  // made visible
  case B_END:
  {
    if (CountItems() > 0)
    {
      Select(CountItems() - 1) ;
      ScrollToSelection() ;
    }
    break ;
  }
  // select the item above the first selected one
  case B_UP_ARROW:
    if (fFirstSelected > 0)
    {
      Select(fFirstSelected - 1) ;
      ScrollToSelection() ;
    }
    break ;
  // select the item below the last one
  case B_DOWN_ARROW:
    if ((fFirstSelected >= 0) && (fLastSelected < (fList.CountItems() - 1)))
    {
      Select(fLastSelected + 1) ;
      ScrollToSelection() ;
    }
    break ;
  case B_PAGE_UP:
  {
    BRect bounds(Bounds()) ;
    BPoint top(bounds.left, bounds.top) ;
    int32 index = IndexOf(top) ;

    if (index >= 0)
    {
      BRect frame(ItemFrame(index)) ;

      if ((frame.bottom - (bounds.bottom - bounds.top)) >= 0)
        ScrollTo(bounds.left, frame.bottom - (bounds.bottom - bounds.top)) ;
      else
        ScrollTo(bounds.left, 0) ;
    }

    break ;
  }
  case B_PAGE_DOWN:
  {
    BRect bounds(Bounds()) ;
    BPoint bottom(bounds.left, bounds.bottom) ;
    int32 index = IndexOf(bottom) ;

    if (index >= 0)
    {
      BRect frame(ItemFrame(index)) ;
      float height ;
      GetPreferredSize(NULL, &height) ;

      if ((bounds.bottom - bounds.top + frame.top) <= height)
      {
        ScrollTo(bounds.left, frame.top) ;
      }
      else {
        ScrollTo(bounds.left, height - (bounds.bottom - bounds.top)) ;
      }
    }

    break;
  }
  // both, the space bar and the return key invoke the current selection
  case B_SPACE:
  case B_RETURN:
  {
    if (fFirstSelected >= 0)
    {
      Invoke() ;
    }
    break ;
  }
  }
  BView::KeyDown(bytes, numBytes) ;
}

/**
 * Overrides the BView version of MakeFocus() to draw an indication that
 * the BListView has become the focus for keyboard events when the
 * focused flag is true, and to remove that indication when the flag is false.
 */
void
BListView::MakeFocus(bool state /* = true */)
{
  if (state != IsFocus())
  {
    BView::MakeFocus(state) ;
  }

  // if the list view is target of a scroll view
  // the border of the scroll view needs to be set to the
  // correct state
  if (fScrollView != NULL)
  {
    fScrollView->SetBorderHighlighted(state) ;
  }
}

/**
 * Updates the on-screen display in response to a notification that
 * the BListView's frame rectangle has been resized. In particular,
 * this function looks for a vertical scroll bar that's a sibling of
 * the BListView. It adjusts this scroll bar to reflect the way the
 * list view was resized, under the assumption that it must have the
 * BListView as its target.
 */
void
BListView::FrameResized(float newWidth, float newHeight)
{
  // set the range of the scroll bars of the scroll view
  FixupScrollBar() ;
}

void
BListView::TargetedByScrollView(BScrollView *scroller)
{
//  if (fScrollView != NULL)
  {
  //  delete fScrollView ;
  }

  fScrollView = scroller ;
}

void
BListView::ScrollTo(BPoint where)
{
  BView::ScrollTo(where) ;
}

/**
 * Adds an item to the BListView at the end of the list. If necessary,
 * additional memory is allocated to accommodate the new item. 
 * Adding an item never removes an item already in the list.
 * If the supplied pointer is NULL the function returns false. Otherwise,
 * it returns true.
 */
bool
BListView::AddItem(BListItem *item)
{
  // check if the supplied pointer is NULL
  bool bReturn = (item != NULL) ;

  if (bReturn)
  {
    if (bReturn = fList.AddItem(static_cast<void*>(item)))
    {
      // If the view is already attached to a view the item needs to be updated
      if (Parent() != NULL)
      {
        BFont font ;
        GetFont(&font) ;
        item->Update(this, &font) ;
      }
    }

    // The new item needs to be drawn if visible
    InvalidateItem(fList.CountItems() - 1) ;
  }

  return bReturn ;
}

/**
 * Adds an item to the BListView at index. If necessary, additional memory is
 * allocated to accommodate the new item. 
 * Adding an item never removes an item already in the list. If the item is
 * added at an index that's already occupied, items currently in the list are 
 * bumped down one slot to make room. 
 * If index is out of range (greater than the current item count, or less than zero), this function fails and returns false. Otherwise, it returns 
 * true
*/
bool
BListView::AddItem(BListItem *item, int32 atIndex)
{
  bool bReturn(item != NULL) ;

  // If successful invalidate all items starting at atIndex
  if (bReturn)
  {
    if (bReturn = fList.AddItem(static_cast<void*>(item), atIndex))
    {
      if (Parent() != NULL)
      {
        BFont font ;
        GetFont(&font) ;
        item->Update(this, &font) ;
      }
    }

    // The new item needs to drawn if visible
    InvalidateFrom(atIndex) ;
  }

  return bReturn ;
}

/**
 * Adds the contents of another list to this BListView. The items from
 * the BList are appended to the end of the list.
 * If the supplied pointer or one of the pointers within the list is NULL,
 * the function fails and returns false. If successful, it returns true. 
 * The BListView doesn't check to be sure that all the items it adds from the
 * list are pointers to BListItem objects. It assumes that they are; if the 
 * assumption is false, the program will crash.
 */
bool
BListView::AddList(BList* newItems)
{
  bool bReturn(newItems != NULL) ;

  if (bReturn)
  {
    for (int32 index = 0 ; index < newItems->CountItems() ; ++index)
    {
      if (newItems->ItemAt(index) == NULL)
      {
        bReturn = false ;
        break ;
      }
    }

    if (bReturn)
    {
      int32 index = CountItems() ;
      fList.AddList(newItems) ;
      InvalidateFrom(index) ;

      if (Parent() != NULL)
      {
        BFont font ;
        GetFont(&font) ;

        for (int32 index = CountItems() - fList.CountItems() ;
             index < CountItems() ; ++index)
        {
          ItemAt(index)->Update(this, &font) ;
        }
      }
    }
  }

  return bReturn ;
}

/**
 * Adds the contents of another list to this BListView. The items from the
 * BList are inserted at index.
 * If the supplied pointer or one of the pointers within the list is NULL or
 * the index is out of range, the function, fails and returns false.
 * If successful, it returns true. 
 * The BListView doesn't check to be sure that all the items it adds from
 * the list are pointers to BListItem objects. It assumes that they are; if the 
 * assumption is false, the program will crash.
 */
bool
BListView::AddList(BList *newItems, int32 atIndex)
{
  bool bReturn(newItems != NULL) ;

  if (bReturn)
  {
    for (int32 index = 0 ; index < newItems->CountItems() ; ++index)
    {
      if (newItems->ItemAt(index) == NULL)
      {
        bReturn = false ;
        break ;
      }
    }

    if (bReturn)
    {
      fList.AddList(newItems, atIndex) ;
      InvalidateFrom(atIndex) ;

      if (Parent() != NULL)
      {
        BFont font ;
        GetFont(&font) ;

        for (int32 index = CountItems() - fList.CountItems() ;
             index < CountItems() ; ++index)
        {
          ItemAt(index)->Update(this, &font) ;
        }
      }
    }
  }

  return bReturn ;
}

/**
 * Removes a single item from the BListView. If passed an index,
 * it removes the item at that index and returns it. If there's no
 * item at the index, it returns NULL. If passed an item, this
 * function looks for that particular item in the list, removes it,
 * and returns true. If it can't find the item, it returns false.
 * If the item is in the list more than once, this function removes
 * only its first occurrence.
 */
bool
BListView::RemoveItem(BListItem* item)
{
  bool bReturn = false ;
  // Remember the position of the item
  int32 index = IndexOf(item) ;
  bReturn = fList.RemoveItem(static_cast<void*>(item)) ;

  // update all items with a higher index than the removed item
  if (bReturn)
  {
    for ( ; index < fList.CountItems() ; ++index)
    {
      InvalidateItem(index) ;
    }
  }

  return bReturn ;
}

BListItem*
BListView::RemoveItem(int32 index)
{
  BListItem* item = static_cast<BListItem*>(fList.RemoveItem(index)) ;

  // update all items with a higher index than the removed item
  if (item != NULL)
  {
    for ( ; index < fList.CountItems() ; ++index)
    {
      InvalidateItem(index) ;
    }
  }

  return item ;
}

bool
BListView::RemoveItems(int32 index, int32 count)
{
  bool bReturn = fList.RemoveItems(index, count) ;

  // update all items with a higher index than the last removed item
  if (bReturn)
  {
    for ( ; index < fList.CountItems() ; ++index)
    {
      InvalidateItem(index) ;
    }
  }

  return bReturn ;
}


/**
 * These functions set, and return information about, the message that
 * a BListView sends whenever a new item is selected. They're exact
 * counterparts to the functions described above under SetInvocationMessage(),
 * except that the selection message is sent whenever an item in the list
 * is selected, rather than when invoked. It's more common to take action
 * (to initiate a message) when invoking an item than when selecting one.
 */
void
BListView::SetSelectionMessage(BMessage *message)
{
  if (fSelectMessage != NULL)
  {
    delete fSelectMessage ;
  }
  
  fSelectMessage = message ;
}

BMessage*
BListView::SelectionMessage() const
{
  return fSelectMessage ;
}

uint32
BListView::SelectionCommand() const
{
  if (fSelectMessage)
    return fSelectMessage->what ;
  else
    return 0 ;
}

void
BListView::SetInvocationMessage(BMessage *message)
{
  BInvoker::SetMessage(message) ;
}

/**
 * These functions set and return information about the BMessage
 * that the BListView sends when currently selected items are invoked.
 * SetInvocationMessage() assigns message to the BListView, freeing
 * any message previously assigned. The message becomes the responsibility
 * of the BListView object and will be freed only when it's replaced by
 * another message or the BListView is freed; you shouldn't free it
 * yourself.
 * Passing a NULL pointer to this function deletes the current message
 * without replacing it. 
 * When sending the message, the Invoke() function makes a copy of it
 * and adds two pieces of relevant information—"when" the message is
 * sent and the "source" BListView. These names should not be used for
 * any data that you add to the invocation message.
 * InvocationMessage() returns a pointer to the BMessage and
 * InvocationCommand() returns its what data member. The message 
 * belongs to the BListView; it can be altered by adding or removing
 * data, but it shouldn't be deleted. To get rid of the current message,
 * pass a NULL pointer to SetInvocationMessage().
 */
BMessage*
BListView::InvocationMessage() const
{
  return BInvoker::Message() ;
}


uint32
BListView::InvocationCommand() const
{
  return BInvoker::Command() ;
}

/**
 * These functions set and return the list type—whether or not
 * it permits multiple selections. The list_view_type must be either
 * B_SINGLE_SELECTION_LIST or B_MULTIPLE_SELECTION_LIST. The type
 * is first set when the BListView is constructed. 
 */
void
BListView::SetListType(list_view_type type)
{
  if (type != fListType)
  {
    // When the type is changed from B_MULTIPLE_SELECTION_LIST to
    // B_SINGLE_SELECTION_LIST all selected items will be deselcted.
    if (fListType == B_MULTIPLE_SELECTION_LIST)
    {
      for (int32 item = fFirstSelected ; item <= fLastSelected ; ++item)
      {
        if (ItemAt(item)->IsSelected())
        {
          ItemAt(item)->Deselect() ;
          InvalidateItem(item) ;
        }
      }
    }

    fListType = type ;
  }
}

list_view_type
BListView::ListType() const
{
  return fListType ;
}

/**
 * This function  returns the BListItem at index in the list, or NULL if the
 * list is empty.
 * The function does not alter the contents of the list—they don't remove the returned 
 * item.
 */
BListItem*
BListView::ItemAt(int32 index) const
{
  return static_cast<BListItem*>(fList.ItemAt(index)) ;
}

/**
 * Returns the index where a particular item whose display rectangle
 * includes a particular point—is located in the list.
 * To determine whether an item lies at the specified point, only
 * the y-coordinate value of the point is considered. 
 * If the item isn't in the list or the y-coordinate of the point
 * doesn't intersect with the data rectangle of the BListView, the return
 * value will be a negative number.
 */
int32
BListView::IndexOf(BPoint point) const
{
  float tmp = 0 ;

  for (int32 index = 0 ; index < fList.CountItems() ; ++index)
  {
    tmp += ItemAt(index)->Height() ;
    
    if (point.y < tmp)
    {
      return index ;
    }
  }

  return -1 ;
}

/**
 * Returns the index where a particular item is located in the list.
 * If the item is in the list more than once, the index returned will
 * be the position of its first occurrence.
 * If the item isn't in the list of the BListView, the return value
 * will be a negative number.
 */
int32
BListView::IndexOf(BListItem *item) const
{
  return fList.IndexOf(static_cast<void*>(item)) ;
}


/**
 * This function returns the very first in the list, or NULL if the list is empty.
 * The function does not alter the contents of the list—they don't remove the returned 
 * item.
 */
BListItem*
BListView::FirstItem() const
{
  return static_cast<BListItem*>(fList.FirstItem()) ;
}

/**
 * This function returns the very last in the list, or NULL if the list is empty.
 * The function does not alter the contents of the list—they don't remove the returned 
 * item.
 */

BListItem*
BListView::LastItem() const
{
  return static_cast<BListItem*>(fList.FirstItem()) ;
}

/**
 * Returns true if item is in the list, and false if not.
 */
bool
BListView::HasItem(BListItem *item) const
{
  return fList.HasItem(static_cast<void*>(item)) ;
}

/**
 * Returns the number of BListItems currently in the list.
 */
int32
BListView::CountItems() const
{
  return fList.CountItems() ;
}

/**
 * MakeEmpty() empties the BListView of all its items, without freeing
 * the BListItem objects.
 */
void
BListView::MakeEmpty()
{
  // Remove all items from the list
  fList.MakeEmpty() ;
  fFirstSelected = -1 ;
  
  // update the list view
  Invalidate() ;
}

/**
 * IsEmpty() returns true if the list is empty (if it contains no items),
 * and false otherwise.
 */
bool
BListView::IsEmpty() const
{
  return fList.IsEmpty() ;
}

/**
 * Calls the func function once for each item in the BListView.
 * BListItems are visited in order, beginning with the first one in
 * the list (index 0) and ending with the last.
 * If a call to func returns true, the iteration is stopped, even
 * if some items have not yet been visited.
 */
void
BListView::DoForEach(bool (*func)(BListItem *))
{
  fList.DoForEach(reinterpret_cast<bool (*)(void*)>(func)) ;
}

/**
 * Calls the func function once for each item in the BListView.
 * BListItems are visited in order, beginning with the first one in
 * the list (index 0) and ending with the last.
 * If a call to func returns true, the iteration is stopped, even
 * if some items have not yet been visited.
 */
void
BListView::DoForEach(bool (*func)(BListItem *, void *), void* parameter)
{
  fList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), parameter) ;
}

/**
 * Returns a pointer to the BListView's list of BListItems. You can index
 * directly into the list of items if you're certain that the index is in range:
 * BListItem *item = Items()[index] ;
 *
 * Although the practice is discouraged, you can also step through the list
 * of items by incrementing the list pointer that Items() returns. Be aware
 * that the list isn't null-terminated—you have to detect the end of the
 * list by some other means. The simplest method is to count items:
 *
 * BListItem **ptr = myListView->Items() ;
 *
 * for ( long i = myListView->CountItems(); i > 0; i-- ) 
 * {
 *   . . . 
 *   *ptr++ ;
 * }
 *
 * You should never use the items pointer to alter the contents of the list.
 */
const BListItem**
BListView::Items() const
{
  return reinterpret_cast<const BListItem**>(fList.Items()) ;
}

/**
 * Invalidates the item at index so that an update message will
 * be sent forcing the BListView to redraw it.
 */
void
BListView::InvalidateItem(int32 index)
{
  if (index <= fList.CountItems())
  {
    // Invalidate the rectangle of the list item
    Invalidate(Bounds() | ItemFrame(index)) ;
  }
}

void
BListView::ScrollToSelection()
{
  if (fFirstSelected != -1)
  {
    float tmp = 0 ;
    int32 index = 0 ;

    for ( ; index < fFirstSelected ; ++index)
    {
      tmp += ItemAt(index)->Height() ;
    }

    BRect rect (Bounds()) ;

    if (tmp < rect.top)
    {
      ScrollTo(0, tmp) ;
    }
    else if ((tmp + ItemAt(index)->Height()) > rect.bottom)
    {
      ScrollTo(0, tmp + ItemAt(index)->Height() - (rect.bottom - rect.top)) ;
    }
  }
}

/**
 * Selects the item at the index
 * if extend is false all former selections will be removed
 * if extend is true the
 */
void
BListView::Select(int32 index, bool extend /* = false */)
{
  // The index must be in range
  if (index < fList.CountItems() && (index >= 0))
  {
    // Deselect all selected items
    if ((fFirstSelected != -1) && (!extend))
    {
      for (int32 item = fFirstSelected ; item <= fLastSelected ; ++item)
      {
        if ((ItemAt(item)->IsSelected()) && (item != index))
        {
          ItemAt(item)->Deselect() ;
          InvalidateItem(item) ;
        }
      }

      // No item is selected any more
      fFirstSelected = -1 ;
    }

    if (fFirstSelected == -1)
    {
      fFirstSelected = index ;
      fLastSelected = index ;
    }
    else if (index < fFirstSelected)
      fFirstSelected = index ;
    else if (index > fLastSelected)
      fLastSelected = index ;

    if (!ItemAt(index)->IsSelected())
    {
      ItemAt(index)->Select() ;
      InvalidateItem(index) ;
    }

    SelectionChanged() ;
  }
}

void
BListView::Select(int32 from, int32 to, bool extend /* = false */)
{
  if ((from <= to) && (to < fList.CountItems()))
  {
    // Deselect all current selected items
    if (!extend)
    {
    }

    for (int32 index = from ; index <= to ; ++index)
    {
      ItemAt(index)->Select() ;
    }
  }
}

/**
 * Returns true if the item at index is currently selected, and false if it's not.
 * It also returns false if the index is not in the range an therefoe does not
 * exist.
 */
bool
BListView::IsItemSelected(int32 index) const
{
  if ((fFirstSelected >= 0) && (index >= fFirstSelected) && (index <= fLastSelected)) 
    return ItemAt(index)->IsSelected() ;

  return false ;
}

/**
 * Returns the index of a currently selected item in the list, or
 * a negative number if no item is selected.
 * The domain of the index passed as an argument is the current set
 * of selected items; the first selected item is at index 0, the second
 * at index 1, and so on, even if the selection is not  contiguous. The
 * domain of the returned index is the set of all items in the list.
 *
 * To get all currently selected items, increment the passed index until
 * the function returns a negative number.
 */
int32
BListView::CurrentSelection(int32 index /* = 0 */) const
{
  if ((index >= 0) && (fFirstSelected >= 0))
  {
    for (int32 count = (fFirstSelected > index ? fFirstSelected : index) ;
         count <= fLastSelected ; ++count)
    {
      if (ItemAt(count)->IsSelected())
      {
        --index ;

        if (index == 0)
        {
          return count ;
        }
      }
    }
  }

  return -1 ;
}

/**
 * Augments the BInvoker version of Invoke() to add three pieces of information
 * to each message the BListView sends:
 *
 * "when" - B_INT64_TYPE
 * When the message is sent, as measured by the number of microseconds
 * since 12:00:00 AM 1970.
 *
 * "source" - B_POINTER_TYPE
 * A pointer to the BListView object.
 *
 * "index" - B_INT32_TYPE
 * An array containing the index of every selected item.
 *
 * This function is called to send both the selection message and the
 * invocation message. It can also be called from application code. The default
 * target of the message (established by AttachedToWindow()) is the BWindow
 * where the BListView is located.
 * What it means to "invoke" selected items depends entirely on the
 * invocation BMessage and the receiver's response to it. This function does
 * nothing but send the message.
 */

 // check if it is done in the right way
status_t
BListView::Invoke(BMessage* msg /* = NULL */)
{
  return BInvoker::Invoke(msg) ;
}

/**
 * This function deselects all the items.
 */
void
BListView::DeselectAll()
{
  if (fFirstSelected != -1)
  {
    for (int32 index = fFirstSelected ; index <= fLastSelected ; ++index)
    {
      if (!ItemAt(index)->IsSelected())
      {
        ItemAt(index)->Deselect() ;
        InvalidateItem(index) ;
      }
    }
  }
}

/**
 * This function deselects all the items except those from index start through
 * index finish.
 */
void
BListView::DeselectExcept(int32 except_from, int32 except_to)
{
  // if no item is selected -> nothing to do
  if ((fFirstSelected != -1) && (except_from <= except_to))
  {
    for (int32 index = fFirstSelected ; index < except_from ; ++index)
    {
      if (!ItemAt(index)->IsSelected())
      {
        ItemAt(index)->Deselect() ;
        InvalidateItem(index) ;
      }
    }
    for (int32 index = except_to + 1 ; index <= fLastSelected ; ++index)
    {
      if (!ItemAt(index)->IsSelected())
      {
        ItemAt(index)->Deselect() ;
        InvalidateItem(index) ;
      }
    }

    SelectionChanged() ;
  }
}

/**
 * These functions deselect the item at index.
 */
void
BListView::Deselect(int32 index)
{
  if (fFirstSelected != -1)
  {
    if ((index < fLastSelected) && (index >= fFirstSelected))
    {
      if (!ItemAt(index)->IsSelected())
      {
        ItemAt(index)->Deselect() ;
        InvalidateItem(index) ;
        SelectionChanged() ;
      }
    }
  }
}

// Implemented by derived class
void
BListView::SelectionChanged()
{
}

void
BListView::SortItems(int (*cmp)(const void *, const void *))
{
  fList.SortItems(cmp) ;
  Invalidate() ;
}


/* These functions bottleneck through DoMiscellaneous() */
bool
BListView::SwapItems(int32 a, int32 b)
{
  bool success = true ;

  if ((a >= 0) && (a < fList.CountItems()) && (b >= 0) && (b < fList.CountItems()))
  {
    // just swap if a is not b
    if (a != b)
    {
      // ToDo chack if it is done correct
      BListItem* item = RemoveItem(a) ;
      AddItem(item, b - 1) ;
      item = RemoveItem(b) ;
      AddItem(item, a) ;
    }
  }
  else {
    success = false ;
  }

  return success ;
}

/**
 * MoveItem() moves the item located at the index from to the index
 * specified by the to argument.
 * This function returns true if the requested operation is
 * completed successfully, or false if the operation failed (for example,
 * if a specified  index is out of range).
 */

 // Is redraw neccessary????
bool
BListView::MoveItem(int32 from, int32 to)
{
  bool success = true ;

  if ((to >= 0) && (to < fList.CountItems()))
  {
    // no move required if from would be to
    if (from != to)
    {
      BListItem* item = RemoveItem(from) ;

      if (item != NULL)
        AddItem(item, to - 1) ;
      else
        success = false ;
    }
  }
  else {
    success = false ;
  }

  return success ;
}

bool
BListView::ReplaceItem(int32 index, BListItem* item)
{
  bool success = true ;

  // otherwise the "new" item would be deleted 
  if (ItemAt(index) != item)
  {
    BListItem* old = RemoveItem(index) ;

    if (old != NULL)
    {
      AddItem(item, index) ;
      delete old ;
    }
    // In this case the index is not in the range
    else {
      success = false ;
    }
  }

  return success ;
}

/**
 * Sets up the BListView and makes the BWindow to which it has become
 * attached the target for the messages it sends when items are selected
 * or invoked—provided another target hasn't already been set. In addition,
 * this function calls Update() for each item in the list to give it a
 * chance to adjust its layout. The BListView's vertical scroll bar is
 * also adjusted. 
 * This function is called for you when the BListView becomes part of a
 * window's view hierarchy. 
 */
void
BListView::AttachedToWindow()
{
  // If no target is set the window to which the ListView is attached will
  // become the target
  if (BInvoker::Target() == NULL)
  {
    BInvoker::SetTarget(BView::Window()) ;
  }

  // call the update function of all items in the list
  for (int32 index = 0 ; index < fList.CountItems() ; ++index)
  {
    BFont font ;
    BView::GetFont(&font) ;
    ItemAt(index)->Update(this, &font) ;

    if (fWidth < ItemAt(index)->Width())
      fWidth = ItemAt(index)->Width() ;
  }

  // Adjust the ScrollBars.
  FixupScrollBar() ;
}

void
BListView::FrameMoved(BPoint new_position)
{
}

/**
 * Returns the frame rectangle of the BListItem at index. The rectangle is stated
 * in the coordinate system of the BListView and defines the area where the item
 * is drawn. Items can differ in height, (but all have the same width ??).
 */
BRect
BListView::ItemFrame(int32 index)
{
  BRect rect ;
  
  if ((index < fList.CountItems()) && (index >= 0))
  {
    rect.left = 0 ;

    for (int32 item = 0 ; item < index ; ++item)
    {
      rect.top += ItemAt(item)->Height() ;
    }
    
    rect.bottom = rect.top + ItemAt(index)->Height() ;
    rect.right = Bounds().right ;
  }
  
  return rect ;
}

BHandler*
BListView::ResolveSpecifier(BMessage *msg, int32 index,
                            BMessage *specifier, int32 form,
                            const char *property)
{
  return NULL ;
}
                 
status_t
BListView::GetSupportedSuites(BMessage *data)
{
  return B_OK ;
}

status_t
BListView::Perform(perform_code d, void *arg)
{
  return B_OK ;
}

void
BListView::WindowActivated(bool state)
{
}

void
BListView::MouseUp(BPoint pt)
{
  int32 index = IndexOf(pt) ;

  if ((index > 0) && (fListType == B_MULTIPLE_SELECTION_LIST) && ((modifiers() & B_SHIFT_KEY)))
  {
    if (!ItemAt(index)->IsSelected())
      Deselect(index) ;
  }
}

void
BListView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
}

void
BListView::DetachedFromWindow()
{
}

/**
 * Implemented by derived classes to permit users to drag items.
 * This function is called from the BListView's MouseDown() function;
 * it should initiate the drag-and-drop operation and return true,
 * or refuse to do so and return false. By default, it always
 * returns false.
 * The point that's passed to InitiateDrag() is the same as the
 * point passed to MouseDown(); it's where the cursor was located
 * when the user pressed the mouse button. The index of the item
 * under the cursor (the item that would be dragged) is passed as the
 * second argument, and the wasSelected flag indicates whether or
 * not the item was selected before the mouse button went down.
 * 
 * A BListView allows users to autoscroll the list by holding the
 * mouse button down and dragging outside its frame rectangle. If a
 * derived class implements InitiateDrag() to drag an item each time
 * the user moves the mouse with a button down, it will hide this
 * autoscrolling behavior. Therefore, derived classes typically
 * permit users to drag items only if they're already selected (if
 * wasSelected is true). In other words, it takes two mouse-down events
 * to drag an item—one to select it and one to begin dragging it.
 */
bool
BListView::InitiateDrag(BPoint pt, int32 itemIndex, bool initialySelected)
{
  return false ;
}

void
BListView::ResizeToPreferred()
{
}

void
BListView::GetPreferredSize(float* width, float* height)
{
  if (height != NULL)
  {
    *height = 0 ;

    for (int32 index = 0 ; index < CountItems() ; ++index)
    {
      *height += ItemAt(index)->Height() ;
    }
  }

  if (width != NULL)
    *width = 0 ;
}

void
BListView::AllAttached()
{
}

void
BListView::AllDetached()
{
}

//---------------------------------------------------------------------------
//  protected!

bool
BListView::DoMiscellaneous(MiscCode code, MiscData* data)
{
  switch(code)
  {
  case B_NO_OP:
    break;

  case B_REPLACE_OP:
    data->replace.index ;
    data->replace.item ;
    break ;

  case B_MOVE_OP:
    data->move.from ;
    data->move.to ;
    break ;

  case B_SWAP_OP:
    data->swap.a ;
    data->swap.b ;
    break ;
  }

  return true ;
}

/*----- Private or reserved -----------------------------------------*/

void
BListView::InitObject(list_view_type type)
{
  fList.MakeEmpty() ;
  fListType = type ;
  fFirstSelected = -1 ;
/*
int32			fLastSelected;
int32			fAnchorIndex;
*/
  fWidth = 0 ;
  fSelectMessage = NULL ;
  fScrollView = NULL ;
  fTrack = NULL ;
}

/**
 * Updates the scroll bars if the list view is targetted by a scroll view
 */
void
BListView::FixupScrollBar()
{
  // set the range of the scroll bars of the scroll view
  if (fScrollView != NULL)
  {
    BScrollBar* bar ;
    float height ;
    float width ;
    BRect bounds(Bounds()) ;

    GetPreferredSize(&width, &height) ;
    height -= (bounds.bottom - bounds.top) ;
    width  -= (bounds.right - bounds.left) ;

    if (height < 0)
      height = 0 ;

    if (width < 0)
      width = 0 ;

    if ((bar = fScrollView->ScrollBar(B_HORIZONTAL)) != NULL)
    {
      bar->SetRange(0, width) ;
      bar->SetSteps(ItemAt(0)->Height(), bounds.bottom - bounds.top) ;
    }

    if ((bar = fScrollView->ScrollBar(B_VERTICAL)) != NULL)
    {
      bar->SetRange(0, height) ;
      bar->SetSteps(ItemAt(0)->Height(), bounds.bottom - bounds.top) ;
    }
  }
}

void
BListView::InvalidateFrom(int32 index)
{
  if ((index >= 0) && (index < CountItems()))
  {
    BRect frame(ItemFrame(index)) ;

    for (++index ; index < fList.CountItems() ; ++index)
      frame.bottom += ItemAt(index)->Height() ;

    // build the rect which needs an update
    BRect bounds(Bounds()) ;
    frame = frame & bounds ;
    frame = frame | bounds ;
    Invalidate(frame) ;
  }
}

inline status_t
BListView::PostMsg(BMessage* msg)
{
  return BInvoker::Invoke(msg) ;
}

void
BListView::FontChanged()
{
  // all items need to be updated
  for (int32 i = 0 ; i < fList.CountItems() ; ++i)
  {
    ItemAt(i)->Update(this, NULL) ;
  }
}

int32
BListView::RangeCheck(int32 index)
{
  return 0 ;
}

bool
BListView::_Select(int32 index, bool extend)
{
  return true ;
}

bool
BListView::_Select(int32 from, int32 to, bool extend)
{
  return true ;
}

bool
BListView::_Deselect(int32 index)
{
  return true ;
}

void
BListView::Deselect(int32 from, int32 to)
{
  for ( ; (from <= to) && (from < fList.CountItems()) ; ++from)
  {
    ItemAt(from)->Deselect() ;
  }
}

bool
BListView::_DeselectAll(int32 except_from, int32 except_to)
{
  for (int32 i = 0 ; i < CountItems() ; ++i)
  {
    if (i == except_from)
    {
      i = except_to ;
      if (i >= CountItems())
        break ;
    }
    
    ItemAt(i)->Deselect() ;
  }

  return true ;
}

void
BListView::PerformDelayedSelect()
{
}

bool
BListView::TryInitiateDrag(BPoint where)
{
  return true ;
}

int32
BListView::CalcFirstSelected(int32 after)
{
  return 0 ;
}

int32
BListView::CalcLastSelected(int32 before)
{
  return 0 ;
}

void
BListView::DrawItem(BListItem* item, BRect itemRect,
                    bool complete /* = false */)
{
}

bool
BListView::DoSwapItems(int32 a, int32 b)
{
  return true ;
}

bool
BListView::DoMoveItem(int32 from, int32 to)
{
  return true ;
}

bool
BListView::DoReplaceItem(int32 index, BListItem* item)
{
  return true ;
}

void
BListView::RescanSelection(int32 from, int32 to)
{
}

void
BListView::DoMouseUp(BPoint where)
{
}

void
BListView::DoMouseMoved(BPoint where)
{
}
