#ifndef _DRIVER_IO_H
#define _DRIVER_IO_H

#include <unistd.h>
#include <fcntl.h>

#include "driver/sound.h"

#define DRIVER_GET_PARAMS(x...)							ioctl( fd, SOUND_GET_PARAMS, x )
#define DRIVER_SET_PARAMS(x...)							ioctl( fd, SOUND_SET_PARAMS, x )
#define DRIVER_SET_PLAYBACK_COMPLETION_SEM(x...)		ioctl( fd, SOUND_SET_PLAYBACK_COMPLETION_SEM, x )
#define DRIVER_SET_CAPTURE_COMPLETION_SEM(x...)			ioctl( fd, SOUND_SET_CAPTURE_COMPLETION_SEM, x )
#define DRIVER_UNSAFE_WRITE(x...)						ioctl( fd, SOUND_UNSAFE_WRITE, x )
#define DRIVER_UNSAFE_READ(x...)						ioctl( fd, SOUND_UNSAFE_READ, x )
#define DRIVER_LOCK_FOR_DMA(x...)						ioctl( fd, SOUND_LOCK_FOR_DMA, x )
#define DRIVER_SET_CAPTURE_PREFERRED_BUF_SIZE(x...)		ioctl( fd, SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE, x )
#define DRIVER_SET_PLAYBACK_PREFERRED_BUF_SIZE(x...)	ioctl( fd, SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE, x )
#define DRIVER_GET_CAPTURE_PREFERRED_BUF_SIZE(x...)		ioctl( fd, SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE, x )
#define DRIVER_GET_PLAYBACK_PREFERRED_BUF_SIZE(x...)	ioctl( fd, SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE, x )

#endif
