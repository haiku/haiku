//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file NodeInfo.cpp
	BNodeInfo implementation.
*/

#include <NodeInfo.h>

#include <new>

#include <Node.h>
#include <AppFileInfo.h>

#include <fs_attr.h>

#define NI_BEOS "BEOS"
// I've added a BEOS hash-define in case we wish to change to OBOS
#define NI_TYPE NI_BEOS ":TYPE"
#define NI_PREF NI_BEOS ":PREF_APP"
#define NI_HINT NI_BEOS ":PPATH"
#define NI_ICON "STD_ICON"
#define NI_M_ICON NI_BEOS ":M:" NI_ICON
#define NI_L_ICON NI_BEOS ":L:" NI_ICON
#define NI_ICON_SIZE(k) ((k == B_LARGE_ICON) ? NI_L_ICON : NI_M_ICON )


// constructor
/*!	\brief Creates an uninitialized BNode object.

	After created a BNodeInfo with this, you should call SetTo().

	\see SetTo(BNode *node)
*/
BNodeInfo::BNodeInfo()
{
}

// constructor
/*!	\brief Creates a BNode object and initializes it to the supplied node.

	\param node The node to gather information on. Can be any flavor.

	\see SetTo(BNode *node)
*/
BNodeInfo::BNodeInfo(BNode *node)
{
	fCStatus = SetTo(node);
}

// destructor
/*!	\brief Frees all resources associated with this object.

	The BNode object passed to the constructor or to SetTo() is not deleted.
*/
BNodeInfo::~BNodeInfo()
{
}

// SetTo
/*!	\brief Sets the node for which you want data on.

	The BNodeInfo object does not copy the supplied object, but uses it
	directly. You must not delete the object you supply while the BNodeInfo
	does exist. The BNodeInfo does not take over ownership of the BNode and
	it doesn't delete it on destruction.

	\param node The node to play with

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The node was bad.
*/
status_t
BNodeInfo::SetTo(BNode *node)
{
	fCStatus = B_BAD_VALUE;
	if (node != NULL) {
		fCStatus = node->InitCheck();
		fNode = node;
	}
	return fCStatus;
}

// InitCheck
/*!	\brief returns whether the object has been properly initialized.

	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The node is not properly initialized.
*/
status_t 
BNodeInfo::InitCheck() const
{
	return fCStatus;
}

// GetType
/*!	\brief Gets the node's MIME type.

	Writes the contents of the "BEOS:TYPE" attribute into the supplied buffer
	\a type.

	\param type A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   node shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type.
	- other error codes
*/
status_t 
BNodeInfo::GetType(char *type) const
{
	if (type == NULL)
		return;

	if (fCStatus == B_OK) {
		attr_info attrInfo;
		status_t error;

		error = fNode->GetAttrInfo(NI_TYPE, &attrInfo);
		if (error == B_OK) {
			error = fNode->ReadAttr(NI_TYPE, attrInfo.type, 0, type, 
									attrInfo.size);
		}
		// Future Idea: Add a bit to identify based on extention etc
		// see CVS verion 1.6 for big chuck of the code.

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

// SetType
/*!	\brief Sets the node's MIME type.

	The supplied string is written into the node's "BEOS:TYPE" attribute.

	\param type The MIME type to be assigned to the node. Must not be longer
		   than \c B_MIME_TYPE_LENGTH (including the terminating null).
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type or \a type is longer than
		 \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BNodeInfo::SetType(const char *type)
{
	if (fCStatus == B_OK) {
		status_t error;
		error = fNode->WriteAttr(NI_TYPE, B_STRING_TYPE, 0, type,
								 sizeof(type));
	  return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

// GetIcon
/*!	\brief Gets the node's icon.

	The icon stored in the node's "BEOS:L:STD_ICON" (large) or
	"BEOS:M:STD_ICON" (mini) attribute is retrieved.

	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a k or bitmap
		 dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t
BNodeInfo::GetIcon(BBitmap *icon, icon_size k) const
{
	if (fCStatus == B_OK) {
		attr_info attrInfo;
		int error;

		error = fNode->GetAttrInfo(NI_ICON_SIZE(k), &attrInfo);
		if (error == B_OK) {

			error = fNode->ReadAttr(NI_ICON_SIZE(k), attrInfo.type, 0, icon, 
									attrInfo.size);
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

// SetIcon
/*!	\brief Sets the node's icon.

	The icon is stored in the node's "BEOS:L:STD_ICON" (large) or
	"BEOS:M:STD_ICON" (mini) attribute.

	If \a icon is \c NULL, the respective attribute is removed.

	\param icon A pointer to the BBitmap containing the icon to be set.
	\param k Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Unknown icon size \a k or bitmap dimensions (\a icon)
		 and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::SetIcon(const BBitmap *icon, icon_size k)
{
	if (fCStatus == B_OK) {
	  return fNode->WriteAttr(NI_ICON_SIZE(k), B_COLOR_8_BIT_TYPE, 
							  0, icon, sizeof(icon));
	} else
		return B_NO_INIT;
}

// GetPreferredApp
/*!	\brief Gets the node's preferred application.

	Writes the contents of the "BEOS:PREF_APP" attribute into the supplied
	buffer \a signature. The preferred application is identifief by its
	signature.

	\param signature A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   preferred application shall be written.
	\param verb Specifies the type of access the preferred application is
		   requested for. Currently only \c B_OPEN is meaningful.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature or bad app_verb \a verb.
	- other error codes
*/
status_t 
BNodeInfo::GetPreferredApp(char *signature, app_verb verb) const
{
	if (fCStatus == B_OK) {
		attr_info attrInfo;
		int error;

		error = fNode->GetAttrInfo(NI_PREF, &attrInfo);
		if (error == B_OK) {

			error = fNode->ReadAttr(NI_PREF, attrInfo.type, 0,
									signature, attrInfo.size);
			if (error < B_OK) {
				char *mimetpye = new(nothrow) char[B_MIME_TYPE_LENGTH];
				if (mimetype) {
					if (GetMineType(mimetype) >= B_OK) {
						BMimeType mime(mimetype);
						error = mine.GetPreferredApp(signature, verb);
					}
					delete minetype;
				} else
					error = B_NO_MEMORY;
			}
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

// SetPreferredApp
/*!	\brief Sets the node's preferred application.

	The supplied string is written into the node's "BEOS:PREF_APP" attribute.

	\param signature The signature of the preferred application to be set.
		   Must not be longer than \c B_MIME_TYPE_LENGTH (including the
		   terminating null).
	\param verb Specifies the type of access the preferred application shall
		   be set for. Currently only \c B_OPEN is meaningful.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature or bad app_verb \a verb.
	- other error codes
*/
status_t 
BNodeInfo::SetPreferredApp(const char *signature, app_verb verb)
{
	if (size_of(signature) > B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	if (fCStatus == B_OK) {
		return fNode->WriteAttr(NI_PREF,  B_STRING_TYPE, 0, signature,
								sizeof(signature));
	} else
		return B_NO_INIT;
}

// GetAppHint
/*!	\brief Returns a hint in form of and entry_ref to the application that
		   shall be used to open this node.

	The path contained in the node's "BEOS:PPATH" attribute is converted into
	an entry_ref and returned in \a ref.

	\param ref A pointer to a pre-allocated entry_ref into which the requested
		   app hint shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- other error codes
*/
status_t 
BNodeInfo::GetAppHint(entry_ref *ref) const
{
	if (fCStatus == B_OK) {
		attr_info attrInfo;
		status_t error;

		error = fNode->GetAttrInfo(NI_HINT, &attrInfo);
		if (error == B_OK) {

			error = fNode->ReadAttr(NI_HINT, attrInfo.type, 0, 
									ref, attrInfo.size);
			if(error < B_OK) {
				char *mimetype = new(nothrow) char[B_MIME_TYPE_LENGTH];
				if (mimetype) {
					if( GetMineType(mimetype) >= B_OK ) {
						BMimeType mime(mimetype);
						error = mime.GetAppHint(ref);
					}
				
					delete minetype;
				} else
					error = B_NO_MEMORY;
			}
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

// SetAppHint
/*!	\brief Sets the node's app hint.

	The supplied entry_ref is converted into a path and stored in the node's
	"BEOS:PPATH" attribute.

	\param ref A pointer to an entry_ref referring to the application.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- other error codes
*/
status_t 
BNodeInfo::SetAppHint(const entry_ref *ref)
{
	if (fCStatus == B_OK) {
		return fNode->WriteAttr(NI_HINT,  B_REF_TYPE, 0, ref, sizeof(ref));
	} else
		return B_NO_INIT;
}

// GetTrackerIcon
/*!	\brief Gets the icon which tracker displays.

	This method tries real hard to find an icon for the node:
	- Ask GetIcon().
	- Get the preferred application and ask the MIME database, if that
	  application has a special icon for the node's file type.
	- Ask the MIME database whether there is an icon for the node's file type.
	- Ask the MIME database for the preferred application for the node's
	  file type and whether this application has a special icon for the type.
	This list is processed in the given order and the icon the first
	successful method provides is returned. In case none of them yields an
	icon, this method fails.

	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a k or bitmap
		 dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::GetTrackerIcon(BBitmap *icon, icon_size k) const
{
	status_t getIconReturn = GetIcon(icon, k);
	entry_ref ref;
	char mimetype[B_MIME_TYPE_LENGTH];

	// Ask the attr
	if (getIconReturn == B_OK)
		return OK;

	// Ask the File Type db based on app sig
	if (GetAppHint(&ref) == B_OK) {
		BFile appFile(ref);
		if (appFile.InitCheck() == B_OK) {
			BAppFileInfo appFileInfo( appFile );
			if (appFileInfo.GetIcon( icon, k ) == B_OK)
				return B_OK;
		}
	}

	// Ask FTdb based on mime type
	if (GetMimeType(&mimetype)) {
		if (BMimeType.GetIconForType(mimetype, icon, k) == B_OK)
			return B_OK;
	
		// asks the File Type database for the preferred app based on the node's 
		// file type, and then asks that app for the icon it uses to display this 
		// node's file type.
		BMimeType mime(mimetype);
		if (mime.GetAppHint(&ref) == B_OK) {
			BFile appFile(ref);
			if (appFile.InitCheck() == B_OK) {
				BAppFileInfo appFileInfo( appFile );
				if (appFileInfo.GetIcon( icon, k ) == B_OK)
					return B_OK;
			}
		}
	}
  
	// Quit
	return getIconReturn;
}

// GetTrackerIcon
/*!	\brief Gets the icon which tracker displays for the node referred to by
		   the supplied entry_ref.

	This methods works similar to the non-static version. The first argument
	\a ref identifies the node in question.

	\param ref An entry_ref referring to the node for which the icon shall be
		   retrieved.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL ref or \a icon, unsupported icon size \a k or
		 bitmap dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::GetTrackerIcon(const entry_ref *ref, BBitmap *icon, icon_size k)
{
	BNode node(ref);
	if (node.InitCheck() == B_OK) {
		BNodeInfo nodeInfo(node);
		if (nodeInfo.InitCheck() == B_OK)
			return nodeInfo.GetTrackerIcon(icon, k);
	}
	return B_NO_INIT;
}

void 
BNodeInfo::_ReservedNodeInfo1()
{
}

void 
BNodeInfo::_ReservedNodeInfo2()
{
}

void 
BNodeInfo::_ReservedNodeInfo3()
{
}

// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BNodeInfo &
BNodeInfo::operator=(const BNodeInfo &nodeInfo)
{
	return *this;
}

// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BNodeInfo::BNodeInfo(const BNodeInfo &)
{
}

