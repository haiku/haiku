/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Bryce Groff, brycegroff@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include <PartitionParameterEditor.h>

#include <String.h>
#include <View.h>


BPartitionParameterEditor::BPartitionParameterEditor()
	:
	fModificationMessage(NULL)
{
}


BPartitionParameterEditor::~BPartitionParameterEditor()
{
	delete fModificationMessage;
}


/*!	\brief Sets the controls of the editor to match the parameters
		of the given \a partition.

	For \c B_CREATE_PARAMETER_EDITOR editors, this will be the parent
	partition.
*/
void
BPartitionParameterEditor::SetTo(BPartition* partition)
{
}


/*!	\brief Sets the modification message.

	This message needs to be sent whenever an internal parameter changed.
	This call takes over ownership of the provided message.

	The message may contain a string field "parameter" with the value set
	to the name of the changed parameter.
*/
void
BPartitionParameterEditor::SetModificationMessage(BMessage* message)
{
	delete fModificationMessage;
	fModificationMessage = message;
}


/*!	\brief The currently set modification message, if any.
*/
BMessage*
BPartitionParameterEditor::ModificationMessage() const
{
	return fModificationMessage;
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


/*!	\brief Called when the user finishes editing the parameters.

	To be overridden by derived classes.
	The base class version returns \c true.

	The method is supposed to check whether the parameters the user set,
	are valid, and, if so, return \c true. Otherwise return \c false.

	\return \c true, if the current parameters are valid, \c false otherwise.
*/
bool
BPartitionParameterEditor::ValidateParameters() const
{
	return true;
}


/*!	\brief Called when a parameter has changed.

	Each editor type comes with a number of predefined parameters that
	may be changed from the outside while the editor is open. You can
	either accept the changes, and update your controls correspondingly,
	or else reject the change by returning an appropriate error code.

	To be overridden by derived classes.
	The base class version returns B_OK.

	\param name The name of the changed parameter.
	\param variant The new value of the parameter.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartitionParameterEditor::ParameterChanged(const char* name,
	const BVariant& variant)
{
	return B_NOT_SUPPORTED;
}


/*!	\brief Returns the edited parameters.

	To be overridden by derived classes.
	The base class version returns an empty string.

	\param parameters A BString to be set to the edited parameters.

	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
BPartitionParameterEditor::GetParameters(BString& parameters)
{
	parameters.SetTo("");
	return B_OK;
}
