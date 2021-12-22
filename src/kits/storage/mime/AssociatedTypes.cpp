//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file AssociatedTypes.cpp
	AssociatedTypes class implementation
*/

#include <mime/AssociatedTypes.h>

#include <stdio.h>

#include <new>

#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <mime/database_support.h>
#include <mime/DatabaseDirectory.h>
#include <mime/DatabaseLocation.h>
#include <mime/MimeSniffer.h>
#include <MimeType.h>
#include <Path.h>
#include <String.h>
#include <storage_support.h>


#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!
	\class AssociatedTypes
	\brief Information about file extensions and their associated types
*/

// Constructor
//! Constructs a new AssociatedTypes object
AssociatedTypes::AssociatedTypes(DatabaseLocation* databaseLocation,
	MimeSniffer* mimeSniffer)
	:
	fDatabaseLocation(databaseLocation),
	fMimeSniffer(mimeSniffer),
	fHaveDoneFullBuild(false)
{
}

// Destructor
//! Destroys the AssociatedTypes object
AssociatedTypes::~AssociatedTypes()
{
}

// GetAssociatedTypes
/*! \brief Returns a list of mime types associated with the given file
	extension in the pre-allocated \c BMessage pointed to by \c types.

	See \c BMimeType::GetAssociatedTypes() for more information.
*/
status_t
AssociatedTypes::GetAssociatedTypes(const char *extension, BMessage *types)
{
	status_t err = extension && types ? B_OK : B_BAD_VALUE;
	std::string extStr;

	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild) {
		err = BuildAssociatedTypesTable();
	}
	// Format the extension
	if (!err) {
		extStr = PrepExtension(extension);
		err = extStr.length() > 0 ? B_OK : B_BAD_VALUE;
	}
	// Build the message
	if (!err) {
		// Clear the message, as we're just going to add to it
		types->MakeEmpty();

		// Add the types associated with this extension
		std::set<std::string> &assTypes = fAssociatedTypes[extStr];
		std::set<std::string>::const_iterator i;
		for (i = assTypes.begin(); i != assTypes.end() && !err; i++) {
			err = types->AddString(kTypesField, i->c_str());
		}
	}
	return err;
}

// GuessMimeType
/*! \brief Guesses a MIME type for the given filename based on its extension

	\param filename The filename of interest
	\param result Pointer to a pre-allocated \c BString object into which
	              the result is stored. If the function returns a value other
	              than \c B_OK, \a result will not be modified.
	\return
	- \c B_OK: success
	- \c other error code: failure
*/
status_t
AssociatedTypes::GuessMimeType(const char *filename, BString *result)
{
	status_t err = filename && result ? B_OK : B_BAD_VALUE;
	if (!err && !fHaveDoneFullBuild)
		err = BuildAssociatedTypesTable();

	// if we have a mime sniffer, let's give it a shot first
	if (!err && fMimeSniffer != NULL) {
		BMimeType mimeType;
		float priority = fMimeSniffer->GuessMimeType(filename, &mimeType);
		if (priority >= 0) {
			*result = mimeType.Type();
			return B_OK;
		}
	}

	if (!err) {
		// Extract the extension from the file
		const char *rawExtension = strrchr(filename, '.');

		// If there was an extension, grab it and look up its associated
		// type(s). Otherwise, the best guess we can offer is
		// "application/octect-stream"
		if (rawExtension && rawExtension[1] != '\0') {
			std::string extension = PrepExtension(rawExtension + 1);

			/*! \todo I'm just grabbing the first item in the set here. Should we perhaps
				do something different?
			*/
			std::set<std::string> &types = fAssociatedTypes[extension];
			std::set<std::string>::const_iterator i = types.begin();
			if (i != types.end())
				result->SetTo(i->c_str());
			else
				err = kMimeGuessFailureError;
		} else {
			err = kMimeGuessFailureError;
		}
	}
	return err;
}

// GuessMimeType
/*! \brief Guesses a MIME type for the given \c entry_ref based on its filename extension

	\param filename The entry_ref of interest
	\param result Pointer to a pre-allocated \c BString object into which
	              the result is stored. If the function returns a value other
	              than \c B_OK, \a result will not be modified.
	\return
	- \c B_OK: success
	- \c other error code: failure
*/
status_t
AssociatedTypes::GuessMimeType(const entry_ref *ref, BString *result)
{
	// Convert the entry_ref to a filename and then do the check
	if (!ref)
		return B_BAD_VALUE;
	BPath path;
	status_t err = path.SetTo(ref);
	if (!err)
		err = GuessMimeType(path.Path(), result);
	return err;
}

// SetFileExtensions
/*! \brief Sets the list of file extensions for the given type and
	updates the associated types mappings.

	All listed extensions will including the given mime type in
	their list of associated types following this call.

	All extensions previously but no longer associated with this
	mime type will no longer list this mime type as an associated
	type.

	\param app The mime type whose associated file extensions you are setting
	\param types Pointer to a \c BMessage containing an array of associated
	             file extensions in its \c Mime::kExtensionsField field.
*/
status_t
AssociatedTypes::SetFileExtensions(const char *type, const BMessage *extensions)
{
	status_t err = type && extensions ? B_OK : B_BAD_VALUE;
	if (!fHaveDoneFullBuild)
		return err;

	std::set<std::string> oldExtensions;
	std::set<std::string> &newExtensions = fFileExtensions[type];
	// Make a copy of the previous extensions
	if (!err) {
		oldExtensions = newExtensions;

		// Read through the list of new extensions, creating the new
		// file extensions list and adding the type as an associated type
		// for each extension
		newExtensions.clear();
		const char *extension;
		for (int32 i = 0;
			   extensions->FindString(kTypesField, i, &extension) == B_OK;
			     i++)
		{
			newExtensions.insert(extension);
			AddAssociatedType(extension, type);
		}

		// Remove any extensions that are still associated from the list
		// of previously associated extensions
		for (std::set<std::string>::const_iterator i = newExtensions.begin();
			   i != newExtensions.end();
			     i++)
		{
			oldExtensions.erase(*i);
		}

		// Now remove the type as an associated type for any of its previously
		// but no longer associated extensions
		for (std::set<std::string>::const_iterator i = oldExtensions.begin();
			   i != oldExtensions.end();
			     i++)
		{
			RemoveAssociatedType(i->c_str(), type);
		}
	}
	return err;
}

// DeleteFileExtensions
/*! \brief Clears the given types's file extensions list and removes the
	types from each of said extensions' associated types list.
	\param app The mime type whose file extensions you are clearing
*/
status_t
AssociatedTypes::DeleteFileExtensions(const char *type)
{
	BMessage extensions;
	return SetFileExtensions(type, &extensions);
}

// PrintToStream
//! Dumps the associated types mapping to standard output
void
AssociatedTypes::PrintToStream() const
{
	printf("\n");
	printf("-----------------\n");
	printf("Associated Types:\n");
	printf("-----------------\n");

	for (std::map<std::string, std::set<std::string> >::const_iterator i = fAssociatedTypes.begin();
		   i != fAssociatedTypes.end();
		     i++)
	{
		printf("%s: ", i->first.c_str());
		fflush(stdout);
		bool first = true;
		for (std::set<std::string>::const_iterator type = i->second.begin();
			   type != i->second.end();
			     type++)
		{
			if (first)
				first = false;
			else
				printf(", ");
			printf("%s", type->c_str());
			fflush(stdout);
		}
		printf("\n");
	}
}

// AddAssociatedType
/*! \brief Adds the given mime type to the set of associated types
	for the given extension.

	\param extension The file extension
	\param type The associated mime type
	\return
	- B_OK: success, even if the type was already in the associated types list
	- "error code": failure
*/
status_t
AssociatedTypes::AddAssociatedType(const char *extension, const char *type)
{
	status_t err = extension && type ? B_OK : B_BAD_VALUE;
	std::string extStr;
	if (!err) {
		extStr = PrepExtension(extension);
		err = extStr.length() > 0 ? B_OK : B_BAD_VALUE;
	}
	if (!err)
		fAssociatedTypes[extStr].insert(type);
	return err;
}

// RemoveAssociatedType
/*! \brief Removes the given mime type from the set of associated types
	for the given extension.

	\param extension The file extension
	\param type The associated mime type
	\return
	- B_OK: success, even if the type was not found in the associated types list
	- "error code": failure
*/
status_t
AssociatedTypes::RemoveAssociatedType(const char *extension, const char *type)
{
	status_t err = extension && type ? B_OK : B_BAD_VALUE;
	std::string extStr;
	if (!err) {
		extStr = PrepExtension(extension);
		err = extStr.length() > 0 ? B_OK : B_BAD_VALUE;
	}
	if (!err)
		fAssociatedTypes[extension].erase(type);
	return err;
}

// BuildAssociatedTypesTable
/*! \brief Crawls the mime database and builds a list of associated types
	for every associated file extension.
*/
status_t
AssociatedTypes::BuildAssociatedTypesTable()
{
	fFileExtensions.clear();
	fAssociatedTypes.clear();

	DatabaseDirectory root;
	status_t err = root.Init(fDatabaseLocation);
	if (!err) {
		root.Rewind();
		while (true) {
			BEntry entry;
			err = root.GetNextEntry(&entry);
			if (err) {
				// If we've come to the end of list, it's not an error
				if (err == B_ENTRY_NOT_FOUND)
					err = B_OK;
				break;
			} else {
				// Check that this entry is both a directory and a valid MIME string
				char supertype[B_PATH_NAME_LENGTH];
				if (entry.IsDirectory()
				      && entry.GetName(supertype) == B_OK
				         && BMimeType::IsValid(supertype))
				{
					// Make sure the supertype string is all lowercase
					BPrivate::Storage::to_lower(supertype);

					// First, iterate through this supertype directory and process
					// all of its subtypes
					DatabaseDirectory dir;
					if (dir.Init(fDatabaseLocation, supertype) == B_OK) {
						dir.Rewind();
						while (true) {
							BEntry subEntry;
							err = dir.GetNextEntry(&subEntry);
							if (err) {
								// If we've come to the end of list, it's not an error
								if (err == B_ENTRY_NOT_FOUND)
									err = B_OK;
								break;
							} else {
								// Get the subtype's name
								char subtype[B_PATH_NAME_LENGTH];
								if (subEntry.GetName(subtype) == B_OK) {
									BPrivate::Storage::to_lower(subtype);

									char fulltype[B_PATH_NAME_LENGTH];
									snprintf(fulltype, B_PATH_NAME_LENGTH, "%s/%s",
										supertype, subtype);

									// Process the subtype
									ProcessType(fulltype);
								}
							}
						}
					} else {
						DBG(OUT("Mime::AssociatedTypes::BuildAssociatedTypesTable(): "
						          "Failed opening supertype directory '%s'\n",
						            supertype));
					}

					// Second, process the supertype
					ProcessType(supertype);
				}
			}
		}
	} else {
		DBG(OUT("Mime::AssociatedTypes::BuildAssociatedTypesTable(): "
		          "Failed opening mime database directory\n"));
	}
	if (!err) {
		fHaveDoneFullBuild = true;
//		PrintToStream();
	} else {
		DBG(OUT("Mime::AssociatedTypes::BuildAssociatedTypesTable() failed, "
			"error code == 0x%" B_PRIx32 "\n", err));
	}
	return err;

}

// ProcessType
/*! \brief Handles a portion of the initial associated types table construction for
	the given mime type.

	\note To be called by BuildAssociatedTypesTable() *ONLY*. :-)

	\param type The mime type of interest. The mime string is expected to be valid
	            and lowercase. Both "supertype" and "supertype/subtype" mime types
	            are allowed.
*/
status_t
AssociatedTypes::ProcessType(const char *type)
{
	status_t err = type ? B_OK : B_BAD_VALUE;
	if (!err) {
		// Read in the list of file extension types
		BMessage msg;
		if (fDatabaseLocation->ReadMessageAttribute(type, kFileExtensionsAttr,
				msg) == B_OK) {
			// Iterate through the file extesions, adding them to the list of
			// file extensions for the mime type and adding the mime type
			// to the list of associated types for each file extension
			const char *extension;
			std::set<std::string> &fileExtensions = fFileExtensions[type];
			for (int i = 0; msg.FindString(kExtensionsField, i, &extension) == B_OK; i++) {
				std::string extStr = PrepExtension(extension);
				if (extStr.length() > 0) {
					fileExtensions.insert(extStr);
					AddAssociatedType(extStr.c_str(), type);
				}
			}
		}
	}
	return err;
}

// PrepExtension
/*! \brief Strips any leading '.' chars from the given extension and
	forces all chars to lowercase.
*/
std::string
AssociatedTypes::PrepExtension(const char *extension) const
{
	if (extension) {
		uint i = 0;
		while (extension[i] == '.')
			i++;
		return BPrivate::Storage::to_lower(&(extension[i]));
	} else {
		return "";	// This shouldn't ever happen, but if it does, an
		            // empty string is considered an invalid extension
	}
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

