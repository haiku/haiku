/*
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include <PartitionParameterEditor.h>

#include <String.h>
#include <View.h>


BPartitionParameterEditor::BPartitionParameterEditor()
{
}


BPartitionParameterEditor::~BPartitionParameterEditor()
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
BView*
BPartitionParameterEditor::View()
{
	return NULL;
}


// FinishedEditing
/*!	\brief Called when the user finishes editing the parameters.

	To be overridden by derived classes.
	The base class version returns \c true.

	The method is supposed to check whether the parameters the user set,
	are valid, and, if so, return \c true. Otherwise return \c false.

	\return \c true, if the current parameters are valid, \c false otherwise.
*/
bool
BPartitionParameterEditor::FinishedEditing()
{
	return false;
}


/*!	\brief Returns the edited parameters.

	To be overridden by derived classes.
	The base class version returns an empty string.

	\param parameters A BString to be set to the edited parameters.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartitionParameterEditor::GetParameters(BString* parameters)
{
	status_t error = (parameters ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		parameters->SetTo("");
	return error;
}


/*!	\brief Called when type information has changed.

	To be overridden by derived classes.
	The base class version returns B_OK.

	\param type A string that is the new type.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartitionParameterEditor::PartitionTypeChanged(const char* type)
{
	return B_NOT_SUPPORTED;
}


/*!	\brief Called when name information has changed.

	To be overridden by derived classes.
	The base class version returns B_OK.

	\param name A string that is the new name.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartitionParameterEditor::PartitionNameChanged(const char* name)
{
	return B_NOT_SUPPORTED;
}

