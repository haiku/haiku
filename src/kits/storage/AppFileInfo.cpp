// BAppFileInfo.cpp

#include <AppFileInfo.h>

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// constructor
/*!	\brief Creates an uninitialized BAppFileInfo object.
*/
BAppFileInfo::BAppFileInfo()
			: fResources(NULL),
			  fWhere(B_USE_BOTH_LOCATIONS)
{
}

// constructor
/*!	\brief Creates an BAppFileInfo object and initializes it to the supplied
		   file.

	The caller retains ownership of the supplied BFile object. It must not
	be deleted during the life time of the BAppFileInfo. It is not deleted
	when the BAppFileInfo is destroyed.

	\param file The file the object shall be initialized to.
*/
BAppFileInfo::BAppFileInfo(BFile *file)
			: fResources(NULL),
			  fWhere(B_USE_BOTH_LOCATIONS)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.

	The BFile the object is set to is not deleted.
*/
BAppFileInfo::~BAppFileInfo()
{
}

// SetTo
/*!	\brief Initializes the BAppFileInfo to the supplied file.

	The caller retains ownership of the supplied BFile object. It must not
	be deleted during the life time of the BAppFileInfo. It is not deleted
	when the BAppFileInfo is destroyed.

	\param file The file the object shall be initialized to.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a file or \a file is not properly initialized.
*/
status_t
BAppFileInfo::SetTo(BFile *file)
{
	return NOT_IMPLEMENTED;
}

// GetType
/*!	\brief Gets the file's MIME type.

	\param type A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type or the type string stored in the
	  attribute/resources is longer than \c B_MIME_TYPE_LENGTH.
	- \c B_BAD_TYPE: The attribute/resources the type string is stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No type is set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetType(char *type) const
{
	return NOT_IMPLEMENTED;
}

// SetType
/*!	\brief Sets the file's MIME type.

	If \a type is \c NULL the file's MIME type is unset.

	\param type The MIME type to be assigned to the file. Must not be longer
		   than \c B_MIME_TYPE_LENGTH (including the terminating null).
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a type is longer than \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BAppFileInfo::SetType(const char *type)
{
	return NOT_IMPLEMENTED;
}

// GetSignature
/*!	\brief Gets the file's application signature.

	\param signature A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the application
		   signature of the file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature or the signature stored in the
	  attribute/resources is longer than \c B_MIME_TYPE_LENGTH.
	- \c B_BAD_TYPE: The attribute/resources the signature is stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No signature is set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetSignature(char *signature) const
{
	return NOT_IMPLEMENTED;
}

// SetSignature
/*!	\brief Sets the file's application signature.

	If \a signature is \c NULL the file's application signature is unset.

	\param signature The application signature to be assigned to the file.
		   Must not be longer than \c B_MIME_TYPE_LENGTH (including the
		   terminating null). May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a signature is longer than \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BAppFileInfo::SetSignature(const char *signature)
{
	return NOT_IMPLEMENTED;
}

// GetAppFlags
/*!	\brief Gets the file's application flags.

	\param flags A pointer to a pre-allocated uint32 into which the application
		   flags of the file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a flags.
	- \c B_BAD_TYPE: The attribute/resources the flags are stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No application flags are set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetAppFlags(uint32 *flags) const
{
	return NOT_IMPLEMENTED;
}

// SetAppFlags
/*!	\brief Sets the file's application flags.
	\param flags The application flags to be assigned to the file.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetAppFlags(uint32 flags)
{
	return NOT_IMPLEMENTED;
}

// GetSupportedTypes
/*!	\brief Gets the MIME types supported by the application.

	The supported MIME types are added to a field "types" of type
	\c B_STRING_TYPE in \a types.

	\param types A pointer to a pre-allocated BMessage into which the
		   MIME types supported by the appplication shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a types.
	- \c B_BAD_TYPE: The attribute/resources the supported types are stored in
	  have the wrong type.
	- \c B_ENTRY_NOT_FOUND: No supported types are set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetSupportedTypes(BMessage *types) const
{
	return NOT_IMPLEMENTED;
}

// SetSupportedTypes
/*!	\brief Sets the MIME types supported by the application.

	If \a types is \c NULL the application's supported types are unset.

	The supported MIME types must be stored in a field "types" of type
	\c B_STRING_TYPE in \a types.

	\param types The supported types to be assigned to the file.
		   May be \c NULL.
	\param syncAll ???
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes

	\todo What does \a syncAll mean?
*/
status_t
BAppFileInfo::SetSupportedTypes(const BMessage *types, bool syncAll)
{
	return NOT_IMPLEMENTED;
}

// SetSupportedTypes
/*!	\brief Sets the MIME types supported by the application.

	If \a types is \c NULL the application's supported types are unset.

	The supported MIME types must be stored in a field "types" of type
	\c B_STRING_TYPE in \a types.

	\param types The supported types to be assigned to the file.
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetSupportedTypes(const BMessage *types)
{
	return NOT_IMPLEMENTED;
}

// IsSupportedType
/*!	\brief Returns whether the application supports the supplied MIME type.

	If the application supports the wildcard type "application/octet-stream"
	any this method returns \c true for any MIME type.

	\param type The MIME type in question.
	\return \c true, if \a type is a valid MIME type and it is supported by
			the application, \c false otherwise.
*/
bool
BAppFileInfo::IsSupportedType(const char *type) const
{
	return false;	// not implemented
}

// Supports
/*!	\brief Returns whether the application supports the supplied MIME type
		   explicitly.

	Unlike IsSupportedType(), this method returns \c true, only if the type
	is explicitly supported, regardless of whether it supports
	"application/octet-stream".

	\param type The MIME type in question.
	\return \c true, if \a type is a valid MIME type and it is explicitly
			supported by the application, \c false otherwise.
*/
bool
BAppFileInfo::Supports(BMimeType *type) const
{
	return false;	// not implemented
}

// GetIcon
/*!	\brief Gets the file's icon.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a which or bitmap
		 dimensions (\a icon) and icon size (\a which) do not match.
	- other error codes
*/
status_t
BAppFileInfo::GetIcon(BBitmap *icon, icon_size which) const
{
	return NOT_IMPLEMENTED;
}

// SetIcon
/*!	\brief Sets the file's icon.

	If \a icon is \c NULL the file's icon is unset.

	\param icon A pointer to the BBitmap containing the icon to be set.
		   May be \c NULL.
	\param which Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Unknown icon size \a which or bitmap dimensions (\a icon)
		 and icon size (\a which) do not match.
	- other error codes
*/
status_t
BAppFileInfo::SetIcon(const BBitmap *icon, icon_size which)
{
	return NOT_IMPLEMENTED;
}

// GetVersionInfo
/*!	\brief Gets the file's version info.
	\param info A pointer to a pre-allocated version_info structure into which
		   the version info should be written.
	\param kind Specifies the kind of the version info to be retrieved:
		   \c B_APP_VERSION_KIND for the application's version info and
		   \c B_SYSTEM_VERSION_KIND for the suite's info the application
		   belongs to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a info.
	- other error codes
*/
status_t
BAppFileInfo::GetVersionInfo(version_info *info, version_kind kind) const
{
	return NOT_IMPLEMENTED;
}

// SetVersionInfo
/*!	\brief Sets the file's version info.

	If \a info is \c NULL the file's version info is unset.

	\param info The version info to be set. May be \c NULL.
	\param kind Specifies kind of version info to be set:
		   \c B_APP_VERSION_KIND for the application's version info and
		   \c B_SYSTEM_VERSION_KIND for the suite's info the application
		   belongs to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetVersionInfo(const version_info *info, version_kind kind)
{
	return NOT_IMPLEMENTED;
}

// GetIconForType
/*!	\brief Gets the icon the application provides for a given MIME type.
	\param type The MIME type in question.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type or \a icon, unsupported icon size
		 \a which or bitmap dimensions (\a icon) and icon size (\a which) do
		 not match.
	- other error codes
*/
status_t
BAppFileInfo::GetIconForType(const char *type, BBitmap *icon,
							 icon_size which) const
{
	return NOT_IMPLEMENTED;
}

// SetIconForType
/*!	\brief Sets the icon the application provides for a given MIME type.

	If \a icon is \c NULL the icon is unset.

	\param type The MIME type in question.
	\param icon A pointer to the BBitmap containing the icon to be set.
		   May be \c NULL.
	\param which Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Unknown icon size \a which or bitmap dimensions (\a icon)
		 and icon size (\a which) do not match.
	- other error codes
*/
status_t
BAppFileInfo::SetIconForType(const char *type, const BBitmap *icon,
							 icon_size which)
{
	return NOT_IMPLEMENTED;
}

// SetInfoLocation
/*!	\brief Specifies the location where the meta data shall be stored.

	The options for \a location are:
	- \c B_USE_ATTRIBUTES: Store the data in the attributes.
	- \c B_USE_RESOURCES: Store the data in the resources.
	- \c B_USE_BOTH_LOCATIONS: Store the data in attributes and resources.

	\param location The location where the meta data shall be stored.
*/
void
BAppFileInfo::SetInfoLocation(info_location location)
{
}

// IsUsingAttributes
/*!	\brief Returns whether the object stores the meta data (also) in the
		   file's attributes.
	\return \c true, if the meta data are (also) stored in the file's
			attributes, \c false otherwise.
*/
bool
BAppFileInfo::IsUsingAttributes() const
{
	return false;	// not implemented
}

// IsUsingResources
/*!	\brief Returns whether the object stores the meta data (also) in the
		   file's resources.
	\return \c true, if the meta data are (also) stored in the file's
			resources, \c false otherwise.
*/
bool
BAppFileInfo::IsUsingResources() const
{
	return false;	// not implemented
}

// FBC
void BAppFileInfo::_ReservedAppFileInfo1() {}
void BAppFileInfo::_ReservedAppFileInfo2() {}
void BAppFileInfo::_ReservedAppFileInfo3() {}

// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BAppFileInfo &
BAppFileInfo::operator=(const BAppFileInfo &)
{
	return *this;
}

// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BAppFileInfo::BAppFileInfo(const BAppFileInfo &)
{
}

