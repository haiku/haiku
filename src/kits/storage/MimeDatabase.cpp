// MimeDatabase.cpp

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <MimeType.h>
#include <Node.h>
#include <TypeConstants.h>

#include <ctype.h>	// For tolower()
#include <stdio.h>
#include <string>
#include <iostream>

#include "MimeDatabase.h"

//#define DBG(x) x
#define DBG(x)
#define OUT printf

namespace BPrivate {

//const char* MimeDatabase::kDefaultDatabaseDir = "/boot/home/config/settings/beos_mime";
const char* MimeDatabase::kDefaultDatabaseDir = "/boot/home/config/settings/obos_mime";

//! Converts every character in \c str to lowercase and returns the result
std::string
to_lower(const char *str) {
	if (str) {
		std::string result = "";
		for (uint i = 0; i < strlen(str); i++)
			result += tolower(str[i]);
		return result;
	} else
		return "(null)";
}

/*!
	\class MimeDatabase
	\brief MimeDatabase is the master of the MIME data base.

	All write and non-atomic read accesses are carried out by this class.
	
	\note No error checking (other than checks for NULL pointers) is performed
	      by this class on the mime type strings passed to it. It's assumed
	      that this sort of checking has been done beforehand.
*/

// constructor
/*!	\brief Creates and initializes a MimeDatabase.
*/
MimeDatabase::MimeDatabase(const char *databaseDir)
	: fDatabaseDir(databaseDir)
	, fCStatus(B_NO_INIT)
{
	// Do some really minor error checking
	BEntry entry(fDatabaseDir.c_str());
	fCStatus = entry.Exists() ? B_OK : B_BAD_VALUE; 
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
MimeDatabase::~MimeDatabase()
{
}

// InitCheck
/*! \brief Returns the initialization status of the object.
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
MimeDatabase::InitCheck()
{
	return fCStatus;
}

// Install
/*!	\brief Installs the given type in the database
	\note The R5 version of this call returned an unreliable result if the
	      MIME type was already installed. Ours simply returns B_OK.
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new long description
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
MimeDatabase::Install(const char *type)
{
	BEntry entry;
	status_t err = (type ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = entry.SetTo(TypeToFilename(type).c_str());
	if (!err) {
		if (entry.Exists())
			err = B_FILE_EXISTS;
		else {
			BNode node;
			err = OpenOrCreateType(type, &node);
		}
	}
	return err;
}

// Delete
/*!	\brief Removes the given type from the database
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
MimeDatabase::Delete(const char *type)
{
	BEntry entry;
	status_t err = (type ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = entry.SetTo(TypeToFilename(type).c_str());		 
	if (!err)
		err = entry.Remove();
	return err;
}


bool
MimeDatabase::IsInstalled(const char *type) const {
	BNode node;
	return OpenType(type, &node) == B_OK;
}

// GetShortDescription
//!	Fetches the short description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH+1 is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the short
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No short description exists for the given type
	- "error code": Failure
*/
status_t
MimeDatabase::GetShortDescription(const char *type, char *description) const {
///	DBG(OUT("MimeDatabase::GetShortDescription()\n"));
	ssize_t err = ReadMimeAttr(type, "META:S:DESC", description, B_MIME_TYPE_LENGTH, B_RAW_TYPE);
	return err >= 0 ? B_OK : err ;
}

// GetLongDescription
//!	Fetches the long description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the long description; a length of \c B_MIME_TYPE_LENGTH+1 is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the long
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No long description exists for the given type
	- "error code": Failure
*/
status_t
MimeDatabase::GetLongDescription(const char *type, char *description) const {
//	DBG(OUT("MimeDatabase::GetLongDescription()\n"));
	ssize_t err = ReadMimeAttr(type, "META:L:DESC", description, B_MIME_TYPE_LENGTH, B_RAW_TYPE);
	return err >= 0 ? B_OK : err ;
}

// SetShortDescription
/*!	\brief Sets the short description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new short description
*/
status_t
MimeDatabase::SetShortDescription(const char *type, const char *description)
{
	DBG(OUT("MimeDatabase::SetShortDescription()\n"));	
	status_t err = (type && description && strlen(description) < B_MIME_TYPE_LENGTH) ? B_OK : B_BAD_VALUE;
	if (!err)
		WriteMimeAttr(type, "META:S:DESC", description, strlen(description)+1, B_RAW_TYPE);
	if (!err)
		err = SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type);
	return err;
}

// SetLongDescription
/*!	\brief Sets the long description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new long description
*/
status_t
MimeDatabase::SetLongDescription(const char *type, const char *description)
{
	DBG(OUT("MimeDatabase::SetLongDescription()\n"));
	status_t err = (type && description && strlen(description) < B_MIME_TYPE_LENGTH) ? B_OK : B_BAD_VALUE;
	if (!err)
		WriteMimeAttr(type, "META:L:DESC", description, strlen(description)+1, B_RAW_TYPE);
	if (!err)
		err = SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type);
	return err;
}

status_t
MimeDatabase::StartWatching(BMessenger target)
{
	DBG(OUT("MimeDatabase::StartWatching()\n"));
	status_t err = target.IsValid() ? B_OK : B_BAD_VALUE;
	if (!err) 
		fMonitorSet.insert(target);
	return err;	
}

status_t
MimeDatabase::StopWatching(BMessenger target)
{
	DBG(OUT("MimeDatabase::StopWatching()\n"));
	status_t err = fMonitorSet.find(target) != fMonitorSet.end() ? B_OK : B_BAD_VALUE;
	if (!err)
		fMonitorSet.erase(target);
	return err;	
}

// ReadMimeAttr
/*! \brief Reads up to \c len bytes of the given data from the given attribute
	       for the given MIME type.
	
	If no entry for the given type exists in the database, the function fails,
	and the contents of \c data are undefined.
	       
	\param type The MIME type
	\param attr The attribute name
	\param data Pointer to a memory buffer into which the data should be copied
	\param len The maximum number of bytes to read
	\param datatype The expected data type
	\return If successful, the number of bytes read is returned, otherwise, an
	        error code is returned.
*/
ssize_t
MimeDatabase::ReadMimeAttr(const char *type, const char *attr, void *data,
	size_t len, type_code datatype) const
{
	BNode node;
	ssize_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = OpenType(type, &node);	
	if (!err) 
		err = node.ReadAttr(attr, datatype, 0, data, len);
	return err;
}


// WriteMimeAttr
/*! \brief Writes \c len bytes of the given data to the given attribute
	       for the given MIME type.
	
	If no entry for the given type exists in the database, it is created.
	       
	\param type The MIME type
	\param attr The attribute name
	\param data Pointer to the data to write
	\param len The number of bytes to write
	\param datatype The data type of the given data
*/
status_t
MimeDatabase::WriteMimeAttr(const char *type, const char *attr, const void *data,
	size_t len, type_code datatype)
{
	BNode node;
	status_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = OpenOrCreateType(type, &node);	
	if (!err) {
		ssize_t bytes = node.WriteAttr(attr, datatype, 0, data, len);
		if (bytes < B_OK)
			err = bytes;
		else
			err = (bytes != len ? B_FILE_ERROR : B_OK);
	}
	return err;
}

// OpenType
/*! \brief Opens a BNode on the given type, failing if the type has no
           corresponding file in the database.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
MimeDatabase::OpenType(const char *type, BNode *result) const
{
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = result->SetTo(TypeToFilename(type).c_str());		 
	return err;
}

// OpenOrCreateType
/*! \brief Opens a BNode on the given type, creating a node of the
	       appropriate flavor if necessary.
	
	All MIME types are converted to lowercase for use in the filesystem.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
MimeDatabase::OpenOrCreateType(const char *type, BNode *result)
{
	std::string filename;
	std::string typeLower = to_lower(type);
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) {
		filename = TypeToFilename(type);
		err = result->SetTo(filename.c_str());		 
	}
	if (err == B_ENTRY_NOT_FOUND) {
		// Figure out what type of node we need to create
		// + Supertype == directory
		// + Non-supertype == file
		int32 pos = typeLower.find_first_of('/');
		if (pos == std::string::npos) {
			// Supertype == directory				
			BDirectory parent(fDatabaseDir.c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateDirectory(typeLower.c_str(), NULL);
		} else {
			// Non-supertype == file
			std::string super(typeLower, 0, pos);
			std::string sub(typeLower, pos+1);
			BDirectory parent((fDatabaseDir + "/" + super).c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateFile(sub.c_str(), NULL);
		}
		// Now try opening again
		err = result->SetTo(filename.c_str());
	}
	return err;	
}

inline
std::string
MimeDatabase::TypeToFilename(const char *type) const
{
	return fDatabaseDir + "/" + to_lower(type);
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, const char *extraType, bool largeIcon) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddString("be:extra_type", extraType);
	if (!err)
		err = msg.AddBool("be:large_icon", largeIcon);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, const char *extraType) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddString("be:extra_type", extraType);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, bool largeIcon) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddBool("be:large_icon", largeIcon);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have subscribed to
	the MIME Monitor service
	\param BMessage A preformatted MIME monitor message to be sent to all subscribers
*/
status_t
MimeDatabase::SendMonitorUpdate(BMessage &msg) {
	DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&)\n"));
	std::set<BMessenger>::const_iterator i;
	for (i = fMonitorSet.begin(); i != fMonitorSet.end(); i++) {
		status_t err = (*i).SendMessage(&msg, (BHandler*)NULL);
		if (err)
			DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&): BMessenger::SendMessage failed, 0x%x\n", err));
	}
	DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&) done\n"));
	return B_OK;
}


} // namespace BPrivate