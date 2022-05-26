//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <DiskScannerAddOn.h>
#include <String.h>

// BDiskScannerPartitionAddOn

// constructor
/*!	\brief Creates a partition disk scanner add-on.
*/
BDiskScannerPartitionAddOn::BDiskScannerPartitionAddOn()
{
}


// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskScannerPartitionAddOn::~BDiskScannerPartitionAddOn()
{
}

/*!	\fn virtual const char *BDiskScannerPartitionAddOn::ShortName() = 0;
	\brief Returns a short name for the add-on.

	To be implemented by derived classes.

	The returned string identifies the respective partition kernel module is
	passed to partition_session().

	\return A short name for the add-on.
*/

/*!	\fn virtual const char *BDiskScannerPartitionAddOn::LongName() = 0;
	\brief Returns a user-readable long name for the add-on.

	To be implemented by derived classes.

	\return A long name for the add-on.
*/

/*!	\fn virtual BDiskScannerPartitionAddOn::BDiskScannerParameterEditor *
		CreateEditor(const BSession *session, const char *parameters) = 0;
	\brief Creates and returns an editor for editing partitioning parameters
		   for a specified session.

	To be implemented by derived classes.

	\param session The session to be partitioned.
	\param parameters Parameters retrieved from the partition module. Should
		   initially be presented to the user.
	\return The newly created editor. \c NULL, if an error occurred.
*/

// FBC
void BDiskScannerPartitionAddOn::_ReservedDiskScannerPartitionAddOn1() {}
void BDiskScannerPartitionAddOn::_ReservedDiskScannerPartitionAddOn2() {}
void BDiskScannerPartitionAddOn::_ReservedDiskScannerPartitionAddOn3() {}
void BDiskScannerPartitionAddOn::_ReservedDiskScannerPartitionAddOn4() {}
void BDiskScannerPartitionAddOn::_ReservedDiskScannerPartitionAddOn5() {}


// BDiskScannerFSAddOn

// constructor
/*!	\brief Creates a FS disk scanner add-on.
*/
BDiskScannerFSAddOn::BDiskScannerFSAddOn()
{
}


// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskScannerFSAddOn::~BDiskScannerFSAddOn()
{
}

/*!	\fn virtual const char *BDiskScannerFSAddOn::ShortName() = 0;
	\brief Returns a short name for the add-on.

	To be implemented by derived classes.

	The returned name identifies the file system (the kernel add-on) and is
	passed to initialize_volume().

	\return A short name for the add-on.
*/

/*!	\fn virtual const char *BDiskScannerFSAddOn::LongName() = 0;
	\brief Returns a user-readable long name for the add-on.

	To be implemented by derived classes.

	\return A long name for the add-on.
*/

/*!	\fn virtual BDiskScannerFSAddOn::BDiskScannerParameterEditor *CreateEditor(
		const BPartition *partition, const char *parameters) = 0;
	\brief Creates and returns an editor for editing initialization parameters
		   for a specified partition.

	To be implemented by derived classes.

	\param partition The partition to be initialized.
	\param parameters Parameters retrieved from the kernel FS add-on. Should
		   initially be presented to the user.
	\return The newly created editor. \c NULL, if the FS doesn't need any
			further parameters.
*/

// FBC
void BDiskScannerFSAddOn::_ReservedDiskScannerFSAddOn1() {}
void BDiskScannerFSAddOn::_ReservedDiskScannerFSAddOn2() {}
void BDiskScannerFSAddOn::_ReservedDiskScannerFSAddOn3() {}
void BDiskScannerFSAddOn::_ReservedDiskScannerFSAddOn4() {}
void BDiskScannerFSAddOn::_ReservedDiskScannerFSAddOn5() {}


// BDiskScannerParameterEditor

// constructor
/*!	\brief Creates a disk scanner parameter editor.
*/
BDiskScannerParameterEditor::BDiskScannerParameterEditor()
{
}


// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskScannerParameterEditor::~BDiskScannerParameterEditor()
{
}


/*!	\brief Returns a view containing the controls needed for editing the
		   parameters.

	To be overridden by derived classes.
	The base class version returns \c NULL.

	The returned BView is added to a window occasionally and removed, when
	editing is done. The view belongs to the editor and needs to be deleted
	by it. Subsequent calls to this method may return the same view, or each
	time delete the old one and return a new one.

	\return A view containing the controls needed for editing the parameters.
			\c NULL can be returned, if no parameters are needed.
*/
BView *
BDiskScannerParameterEditor::View()
{
	return NULL;
}


// EditingDone
/*!	\brief Called when the user finishes editing the parameters.

	To be overridden by derived classes.
	The base class version returns \c true.

	The method is supposed to check whether the parameters the user set,
	are valid, and, if so, return \c true. Otherwise an BAlert shall be
	shown, explaining the problem to the user and \c false being returned
	-- then the parameter dialog will not be closed.

	\return \c true, if the current parameters are valid, \c false otherwise.
*/
bool
BDiskScannerParameterEditor::EditingDone()
{
	return true;
}


/*!	\brief Returns the edited parameters.

	To be overridden by derived classes.
	The base class version returns an empty string.

	\param parameters A BString to be set to the edited parameters.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BDiskScannerParameterEditor::GetParameters(BString *parameters)
{
	status_t error = (parameters ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		parameters->SetTo("");
	return error;
}

// FBC
void BDiskScannerParameterEditor::_ReservedDiskScannerParameterEditor1() {}
void BDiskScannerParameterEditor::_ReservedDiskScannerParameterEditor2() {}
void BDiskScannerParameterEditor::_ReservedDiskScannerParameterEditor3() {}
void BDiskScannerParameterEditor::_ReservedDiskScannerParameterEditor4() {}
void BDiskScannerParameterEditor::_ReservedDiskScannerParameterEditor5() {}


/*!	\fn BDiskScannerPartitionAddOn *create_ds_partition_add_on();
	\brief To be provided by partition add-ons to create an add-on object.
	\return A newly created BDiskScannerPartitionAddOn for this add-on.
*/

/*!	\fn BDiskScannerFSAddOn *create_ds_fs_add_on();
	\brief To be provided by FS add-ons to create an add-on object.
	\return A newly created BDiskScannerFSAddOn for this add-on.
*/

