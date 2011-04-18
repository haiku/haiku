//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered 
//  by the MIT license.
//---------------------------------------------------------------------
/*!
	\file FindDirectory.cpp
	find_directory() implementations.	
*/

#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>


// find_directory
//!	Returns a path of a directory specified by a directory_which constant.
/*!	\param which the directory_which constant specifying the directory
	\param path a BPath object to be initialized to the directory's path
	\param createIt \c true, if the directory shall be created, if it doesn't
		   already exist, \c false otherwise.
	\param volume the volume on which the directory is located
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- another error code
*/
status_t
find_directory(directory_which which, BPath* path, bool createIt,
			   BVolume* volume)
{
	if (path == NULL)
		return B_BAD_VALUE;

	dev_t device = (dev_t)-1;
	if (volume && volume->InitCheck() == B_OK)
		device = volume->Device();

	char buffer[B_PATH_NAME_LENGTH];
	status_t error = find_directory(which, device, createIt, buffer,
		B_PATH_NAME_LENGTH);
	if (error == B_OK)
		error = path->SetTo(buffer);

	return error;
}

