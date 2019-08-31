/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LOCKING_H
#define LOCKING_H

#include <util/AutoLock.h>

class Volume;

// instantiations
typedef AutoLocker<Volume, AutoLockerReadLocking<Volume> >	VolumeReadLocker;
typedef AutoLocker<Volume, AutoLockerWriteLocking<Volume> >	VolumeWriteLocker;

#endif	// LOCKING_H
