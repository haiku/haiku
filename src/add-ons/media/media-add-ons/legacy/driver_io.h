#ifndef _DRIVER_IO_H
#define _DRIVER_IO_H

#include <unistd.h>
#include <fcntl.h>
#include <Drivers.h>
#include "OldSoundDriver.h"

#define DRIVER_GET_PARAMS(x...)							ioctl( fd, SOUND_GET_PARAMS, x )
#define DRIVER_SET_PARAMS(x...)							ioctl( fd, SOUND_SET_PARAMS, x )
#define DRIVER_SET_PLAYBACK_COMPLETION_SEM(x...)		ioctl( fd, SOUND_SET_PLAYBACK_COMPLETION_SEM, x )
#define DRIVER_SET_CAPTURE_COMPLETION_SEM(x...)			ioctl( fd, SOUND_SET_CAPTURE_COMPLETION_SEM, x )
#define DRIVER_WRITE_BUFFER(x...)						ioctl( fd, SOUND_WRITE_BUFFER, x )
#define DRIVER_READ_BUFFER(x...)						ioctl( fd, SOUND_READ_BUFFER, x )
#define DRIVER_LOCK_FOR_DMA(x...)						ioctl( fd, SOUND_LOCK_FOR_DMA, x )

#endif
