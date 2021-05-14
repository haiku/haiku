/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <mime/SupportingApps.h>

#include <stdio.h>

#include <new>

#include <Directory.h>
#include <Message.h>
#include <MimeType.h>
#include <Path.h>
#include <String.h>

#include <mime/database_support.h>
#include <mime/DatabaseDirectory.h>
#include <mime/DatabaseLocation.h>
#include <storage_support.h>


#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!
	\class SupportingApps
	\brief Supporting apps information for the entire database
*/

// Constructor
//! Constructs a new SupportingApps object
SupportingApps::SupportingApps(DatabaseLocation* databaseLocation)
	:
	fDatabaseLocation(databaseLocation),
	fHaveDoneFullBuild(false)
{
}

// Destructor
//! Destroys the SupportingApps object
SupportingApps::~SupportingApps()
{
}

// GetSupportingApps
/*! \brief Returns a list of signatures of supporting applications for the
	given type in the pre-allocated \c BMessage pointed to by \c apps.

	See \c BMimeType::GetSupportingApps() for more information.
*/
status_t
SupportingApps::GetSupportingApps(const char *type, BMessage *apps)
{
	status_t err = type && apps ? B_OK : B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild)
		err = BuildSupportingAppsTable();

	if (!err) {
		// Clear the message, as we're just going to add to it
		apps->MakeEmpty();

		BMimeType mime(type);
		err = mime.InitCheck();
		if (!err) {
			if (mime.IsSupertypeOnly()) {
				// Add the apps that support this supertype (plus their count)
				std::set<std::string> &superApps = fSupportingApps[type];
				int32 count = 0;
				std::set<std::string>::const_iterator i;
				for (i = superApps.begin(); i != superApps.end() && !err; i++) {
					err = apps->AddString(kApplicationsField, (*i).c_str());
					count++;
				}
				if (!err)
					err = apps->AddInt32(kSupportingAppsSuperCountField, count);
			} else {
				// Add the apps that support this subtype (plus their count)
				std::set<std::string> &subApps = fSupportingApps[type];
				int32 count = 0;
				std::set<std::string>::const_iterator i;
				for (i = subApps.begin(); i != subApps.end() && !err; i++) {
					err = apps->AddString(kApplicationsField, (*i).c_str());
					count++;
				}
				if (!err)
					err = apps->AddInt32(kSupportingAppsSubCountField, count);

				// Now add any apps that support the supertype, but not the
				// subtype (plus their count).
				BMimeType superMime;
				err = mime.GetSupertype(&superMime);
				if (!err)
					err = superMime.InitCheck();
				if (!err) {
					std::set<std::string> &superApps = fSupportingApps[superMime.Type()];
					count = 0;
					for (i = superApps.begin(); i != superApps.end() && !err; i++) {
						if (subApps.find(*i) == subApps.end()) {
							err = apps->AddString(kApplicationsField, (*i).c_str());
							count++;
						}
					}
					if (!err)
						err = apps->AddInt32(kSupportingAppsSuperCountField, count);
				}
			}
		}
	}
	return err;
}

// SetSupportedTypes
/*! \brief Sets the list of supported types for the given application and
	updates the supporting apps mappings.

	All types listed as being supported types will including the given
	app signature in their list of supporting apps following this call.

	If \a fullSync is true, all types previously but no longer supported
	by this application with no longer list this application as a
	supporting app.

	If \a fullSync is false, said previously supported types will be
	saved to a "stranded types" mapping and appropriately synchronized
	the next time SetSupportedTypes() is called with a \c true \a fullSync
	parameter.

	The stranded types mapping is properly maintained even in the event
	of types being removed and then re-added to the list of supporting
	types with consecutive \c false \a fullSync parameters.

	\param app The application whose supported types you are setting
	\param types Pointer to a \c BMessage containing an array of supported
	             mime types in its \c Mime::kTypesField field.
	\param fullSync If \c true, \c app is removed as a supporting application
	                for any types for which it is no longer a supporting application
	                (including types which were removed as supporting types with
	                previous callsto SetSupportedTypes(..., false)). If \c false,
	                said mappings are not updated until the next SetSupportedTypes(..., true)
	                call.
*/
status_t
SupportingApps::SetSupportedTypes(const char *app, const BMessage *types, bool fullSync)
{
	status_t err = app && types ? B_OK : B_BAD_VALUE;
	if (!fHaveDoneFullBuild)
		return err;

	std::set<std::string> oldTypes;
	std::set<std::string> &newTypes = fSupportedTypes[app];
	std::set<std::string> &strandedTypes = fStrandedTypes[app];
	// Make a copy of the previous types if we're doing a full sync
	if (!err) {
		oldTypes = newTypes;

		// Read through the list of new supported types, creating the new
		// supported types list and adding the app as a supporting app for
		// each type.
		newTypes.clear();
		const char *type;
		for (int32 i = 0; types->FindString(kTypesField, i, &type) == B_OK;
				i++) {
			newTypes.insert(type);
			AddSupportingApp(type, app);
		}

		// Update the list of stranded types by removing any types that are newly
		// re-supported and adding any types that are newly un-supported
		for (std::set<std::string>::const_iterator i = newTypes.begin();
				i != newTypes.end(); i++) {
			strandedTypes.erase(*i);
		}
		for (std::set<std::string>::const_iterator i = oldTypes.begin();
				i != oldTypes.end(); i++) {
			if (newTypes.find(*i) == newTypes.end())
				strandedTypes.insert(*i);
		}

		// Now, if we're doing a full sync, remove the app as a supporting
		// app for any of its stranded types and then clear said list of
		// stranded types.
		if (fullSync) {
			for (std::set<std::string>::const_iterator i = strandedTypes.begin();
					i != strandedTypes.end(); i++) {
				RemoveSupportingApp((*i).c_str(), app);
			}
			strandedTypes.clear();
		}
	}
	return err;
}


/*! \brief Clears the given application's supported types list and optionally
	removes the application from each of said types' supporting apps list.
	\param app The application whose supported types you are clearing
	\param fullSync See SupportingApps::SetSupportedTypes()
*/
status_t
SupportingApps::DeleteSupportedTypes(const char *app, bool fullSync)
{
	BMessage types;
	return SetSupportedTypes(app, &types, fullSync);
}

// AddSupportingApp
/*! \brief Adds the given application signature to the set of supporting
	apps for the given type.

	\param type The full mime type
	\param app The full application signature (i.e. "application/app-subtype")
	\return
	- B_OK: success, even if the app was already in the supporting apps list
	- "error code": failure
*/
status_t
SupportingApps::AddSupportingApp(const char *type, const char *app)
{
	status_t err = type && app ? B_OK : B_BAD_VALUE;
	if (!err)
		fSupportingApps[type].insert(app);
	return err;
}

// RemoveSupportingApp
/*! \brief Removes the given application signature from the set of supporting
	apps for the given type.

	\param type The full mime type
	\param app The full application signature (i.e. "application/app-subtype")
	\return
	- B_OK: success, even if the app was not found in the supporting apps list
	- "error code": failure
*/
status_t
SupportingApps::RemoveSupportingApp(const char *type, const char *app)
{
	status_t err = type && app ? B_OK : B_BAD_VALUE;
	if (!err)
		fSupportingApps[type].erase(app);
	return err;
}

// BuildSupportingAppsTable
/*! \brief Crawls the mime database and builds a list of supporting application
	signatures for every supported type.
*/
status_t
SupportingApps::BuildSupportingAppsTable()
{
	fSupportedTypes.clear();
	fSupportingApps.clear();
	fStrandedTypes.clear();

	DatabaseDirectory dir;
	status_t status = dir.Init(fDatabaseLocation, "application");

	// Build the supporting apps table based on the mime database
	if (status == B_OK) {
		dir.Rewind();

		// Handle each application type
		while (true) {
			entry_ref ref;
			status = dir.GetNextRef(&ref);
			if (status < B_OK) {
				// If we've come to the end of list, it's not an error
				if (status == B_ENTRY_NOT_FOUND)
					status = B_OK;
				break;
			}

			// read application signature from file
			BString appSignature;
			BNode node(&ref);
			if (node.InitCheck() == B_OK && node.ReadAttrString(kTypeAttr,
					&appSignature) >= B_OK) {
				// Read in the list of supported types
				BMessage msg;
				if (fDatabaseLocation->ReadMessageAttribute(appSignature,
						kSupportedTypesAttr, msg) == B_OK) {
					// Iterate through the supported types, adding them to the list of
					// supported types for the application and adding the application's
					// signature to the list of supporting apps for each type
					BString type;
					std::set<std::string> &supportedTypes = fSupportedTypes[appSignature.String()];
					for (int i = 0; msg.FindString(kTypesField, i, &type) == B_OK; i++) {
						type.ToLower();
							// MIME types are case insensitive, so we lowercase everything
						supportedTypes.insert(type.String());
						AddSupportingApp(type.String(), appSignature.String());
					}
				}
			}
		}
	}

	if (status == B_OK)
		fHaveDoneFullBuild = true;
	else
		DBG(OUT("SupportingApps::BuildSupportingAppsTable() failed: %s\n", strerror(status)));

	return status;
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

