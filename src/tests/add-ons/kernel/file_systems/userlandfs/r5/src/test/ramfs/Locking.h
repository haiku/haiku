// Locking.h

#ifndef LOCKING_H
#define LOCKING_H

#include <AutoLocker.h>

class Volume;

// instantiations
typedef AutoLocker<Volume, AutoLockerReadLocking<Volume> >	VolumeReadLocker;
typedef AutoLocker<Volume, AutoLockerWriteLocking<Volume> >	VolumeWriteLocker;

#endif	// LOCKING_H
