// Clipboard.cpp

#include <app/Clipboard.h>

#include "Clipboard.h"

/*!
	\class Clipboard
	\brief Server-side representation of a clipboard.
*/

// constructor
/*!	\brief Creates and initializes a Clipboard.
	\param name The name of the clipboard.
*/
Clipboard::Clipboard(const char *name)
	: fName(name),
	  fData(B_SIMPLE_DATA),
	  fDataSource(),
	  fCount(0),
	  fWatchingService()
{
}

// destructor
/*!	\brief Frees all resources associate with this object.
*/
Clipboard::~Clipboard()
{
}

// SetData
/*!	\brief Sets the clipboard's data.

	Also notifies all watchers that the clipboard data have changed.

	\param data The new clipboard data.
	\param dataSource The clipboards new data source.
*/
void
Clipboard::SetData(const BMessage *data, BMessenger dataSource)
{
	fData = *data;
	fDataSource = dataSource;
	fCount++;
	NotifyWatchers();
}

// Data
/*!	\brief Returns the clipboard's data.
	\return The clipboard's data.
*/
const BMessage *
Clipboard::Data() const
{
	return &fData;
}

// DataSource
/*!	\brief Returns the clipboard's data source.
	\return The clipboard's data source.
*/
BMessenger
Clipboard::DataSource() const
{
	return fDataSource;
}

// Count
int32
Clipboard::Count() const
{
	return fCount;
}


// AddWatcher
/*!	\brief Adds a new watcher for this clipboard.
	\param watcher The messenger referring to the new watcher.
	\return \c true, if the watcher could be added successfully,
			\c false otherwise.
*/
bool
Clipboard::AddWatcher(BMessenger watcher)
{
	return fWatchingService.AddWatcher(watcher);
}

// RemoveWatcher
/*!	\brief Removes a watcher from this clipboard.
	\param watcher The watcher to be removed.
	\return \c true, if the supplied watcher was watching the clipboard,
			\c false otherwise.
*/
bool
Clipboard::RemoveWatcher(BMessenger watcher)
{
	return fWatchingService.RemoveWatcher(watcher);
}

// NotifyWatchers
/*!	\brief Sends a notification message that the clipboard data have changed
		   to all associated watchers.
*/
void
Clipboard::NotifyWatchers()
{
	BMessage message(B_CLIPBOARD_CHANGED);
	fWatchingService.NotifyWatchers(&message, NULL);
}

