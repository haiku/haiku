// ClipboardTree.cpp

#include <Message.h>
#include <Clipboard.h>

#include "ClipboardTree.h"

/*!
	\class ClipboardTree
	\brief Implements a tree containing clipboards and their data
*/

// constructor
/*!	\brief Creates and initializes a ClipboardTree.
*/
ClipboardTree::ClipboardTree()
{
  fName = "";
  fCount = 0;
  fLeftChild = NULL;
  fRightChild = NULL;
}

// destructor
/*!	\brief Frees all resources associate with this object.
*/
ClipboardTree::~ClipboardTree()
{
  if ( fLeftChild )
    delete fLeftChild;
  if ( fRightChild )
    delete fRightChild;
}

void ClipboardTree::AddNode(BString name)
{
  if ( fName == "" )
  {
    fName = name;
    return;
  }
  if ( fName == name )
    return;
  if ( name < fName )
  {
    if ( !fLeftChild )
      fLeftChild = new ClipboardTree;
    fLeftChild->AddNode(name);
    return;
  }
  if ( !fRightChild )
    fRightChild = new ClipboardTree;
  fRightChild->AddNode(name);
}

ClipboardTree* ClipboardTree::GetNode(BString name)
{
  if ( fName == "" )
    return NULL;
  if ( fName == name )
    return this;
  if ( name < fName )
  {
    if ( !fLeftChild )
      return NULL;
    return fLeftChild->GetNode(name);
  }
  if ( !fRightChild )
    return NULL;
  return fRightChild->GetNode(name);
}

uint32 ClipboardTree::GetCount()
{
  return fCount;
}

uint32 ClipboardTree::IncrementCount()
{
  fCount++;
  return fCount;
}

BMessage* ClipboardTree::GetData()
{
  return &fData;
}

void ClipboardTree::SetData(BMessage *data)
{
  fData = *data;
}

BMessenger* ClipboardTree::GetDataSource()
{
  return &fDataSource;
}

void ClipboardTree::SetDataSource(BMessenger *dataSource)
{
  fDataSource = *dataSource;
}

bool ClipboardTree::AddWatcher(BMessenger *watcher)
{
  return fWatchingService.AddWatcher(*watcher);
}

bool ClipboardTree::RemoveWatcher(BMessenger *watcher)
{
  return fWatchingService.RemoveWatcher(*watcher,true);
}

void ClipboardTree::NotifyWatchers()
{
  BMessage message(B_CLIPBOARD_CHANGED);
  fWatchingService.NotifyWatchers(&message,NULL);
}

