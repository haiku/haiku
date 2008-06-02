/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder, tyler@dauwalder.net
 *		Erik Jakowatz
 *		shadow303
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */

/*!
	\file Volume.h
	BVolume implementation.
*/

#include <Volume.h>

#include <errno.h>
#include <string.h>

#include <Bitmap.h>
#include <Directory.h>
#include <Node.h>
#include <Path.h>

#include <storage_support.h>
#include <syscalls.h>


/*!
	\class BVolume
	\brief Represents a disk volume
	
	Provides an interface for querying information about a volume.

	The class is a simple wrapper for a \c dev_t and the function
	fs_stat_dev. The only exception is the method is SetName(), which
	sets the name of the volume.

	\author Vincent Dominguez
	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/

/*!	\var dev_t BVolume::fDevice
	\brief The volume's device ID.
*/

/*!	\var dev_t BVolume::fCStatus
	\brief The object's initialization status.
*/


/*!	\brief Creates an uninitialized BVolume.

	InitCheck() will return \c B_NO_INIT.
*/
BVolume::BVolume()
	:
	fDevice((dev_t)-1),
	fCStatus(B_NO_INIT)
{
}


/*!	\brief Creates a BVolume and initializes it to the volume specified
		   by the supplied device ID.

	InitCheck() should be called to check whether the initialization was
	successful.

	\param device The device ID of the volume.
*/
BVolume::BVolume(dev_t device)
	:
	fDevice((dev_t)-1),
	fCStatus(B_NO_INIT)
{
	SetTo(device);
}


/*!	\brief Creates a BVolume and makes it a clone of the supplied one.

	Afterwards the object refers to the same device the supplied object
	does. If the latter is not properly initialized, this object isn't
	either.

	\param volume The volume object to be cloned.
*/
BVolume::BVolume(const BVolume &volume)
	:
	fDevice(volume.fDevice),
	fCStatus(volume.fCStatus)
{
}


/*!	\brief Frees all resources associated with the object.

	Does nothing.
*/
BVolume::~BVolume()
{
}


/*!	\brief Returns the result of the last initialization.
	\return
	- \c B_OK: The object is properly initialized.
	- an error code otherwise
*/
status_t
BVolume::InitCheck(void) const
{	
	return fCStatus;
}


/*!	\brief Re-initializes the object to refer to the volume specified by
		   the supplied device ID.
	\param device The device ID of the volume.
	\param
	- \c B_OK: Everything went fine.
	- an error code otherwise
*/
status_t
BVolume::SetTo(dev_t device)
{
	// uninitialize
	Unset();
	// check the parameter
	status_t error = (device >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
// 		fs_info info;
// 		if (fs_stat_dev(device, &info) != 0)
// 			error = errno;
	}
	// set the new value
	if (error == B_OK)	
		fDevice = device;
	// set the init status variable
	fCStatus = error;
	return fCStatus;
}


/*!	\brief Uninitialized the BVolume.
*/
void
BVolume::Unset()
{
	fDevice = (dev_t)-1;
	fCStatus = B_NO_INIT;
}


/*!	\brief Returns the device ID of the volume the object refers to.
	\return Returns the device ID of the volume the object refers to
			or -1, if the object is not properly initialized.
*/
dev_t
BVolume::Device() const 
{
	return fDevice;
}


/*!	\brief Returns whether two BVolume objects are equal.

	Two volume objects are said to be equal, if they either are both
	uninitialized, or both are initialized and refer to the same volume.

	\param volume The object to be compared with.
	\result \c true, if this object and the supplied one are equal, \c false
			otherwise.
*/
bool
BVolume::operator==(const BVolume &volume) const
{
	return fDevice == volume.fDevice;
}


/*!	\brief Returns whether two BVolume objects are unequal.

	Two volume objects are said to be equal, if they either are both
	uninitialized, or both are initialized and refer to the same volume.

	\param volume The object to be compared with.
	\result \c true, if this object and the supplied one are unequal, \c false
			otherwise.
*/
bool
BVolume::operator!=(const BVolume &volume) const
{
	return !(*this == volume);
}


/*!	\brief Assigns another BVolume object to this one.

	This object is made an exact clone of the supplied one.

	\param volume The volume from which shall be assigned.
	\return A reference to this object.
*/
BVolume&
BVolume::operator=(const BVolume &volume)
{
	if (&volume != this) {
		this->fDevice = volume.fDevice;
		this->fCStatus = volume.fCStatus;
	}
	return *this;
}


void BVolume::_TurnUpTheVolume1() {} 
void BVolume::_TurnUpTheVolume2() {} 
void BVolume::_TurnUpTheVolume3() {} 
void BVolume::_TurnUpTheVolume4() {} 
void BVolume::_TurnUpTheVolume5() {} 
void BVolume::_TurnUpTheVolume6() {} 
void BVolume::_TurnUpTheVolume7() {} 
void BVolume::_TurnUpTheVolume8() {}
