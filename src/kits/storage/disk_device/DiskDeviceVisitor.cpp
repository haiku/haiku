//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <DiskDeviceVisitor.h>

/*!	\class BDiskDeviceVisitor
	\brief Base class of visitors used for BDiskDevice and
		   BPartition iteration.

	BDiskDeviceRoster and BDiskDevice provide iteration methods,
	that work together with an instance of a derived class of
	BDiskDeviceVisitor. For each encountered BDiskDevice and
	BPartition the respective Visit() method is invoked. The return value
	of that method specifies whether the iteration shall be terminated at
	that point.
*/

// constructor
/*!	\brief Creates a new disk device visitor.
*/
BDiskDeviceVisitor::BDiskDeviceVisitor()
{
}

// destructor
/*!	\brief Free all resources associated with this object.

	Does nothing.
*/
BDiskDeviceVisitor::~BDiskDeviceVisitor()
{
}

// Visit
/*!	\brief Invoked when a BDiskDevice is visited.

	If the method returns \c true, the iteration is terminated at this point,
	on \c false continued.

	Overridden by derived classes.
	This class' version does nothing and it returns \c false.

	\param device The visited disk device.
	\return \c true, if the iteration shall be terminated at this point,
			\c false otherwise.
*/
bool
BDiskDeviceVisitor::Visit(BDiskDevice *device)
{
	return false;
}

// Visit
/*!	\brief Invoked when a BPartition is visited.

	If the method returns \c true, the iteration is terminated at this point,
	on \c false continued.

	Overridden by derived classes.
	This class' version does nothing and it returns \c false.

	\param partition The visited partition.
	\param level The level of the partition in the partition tree.
	\return \c true, if the iteration shall be terminated at this point,
			\c false otherwise.
*/
bool
BDiskDeviceVisitor::Visit(BPartition *partition, int32 level)
{
	return false;
}

