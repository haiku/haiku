#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/ParameterWeb.h>
#include <media/TimeSource.h>

#include <support/Autolock.h>
#include <support/Debug.h>

#include "driver_io.h"

//#include "LegacyAudioProducer.h"

#if 0
LegacyAudioProducer::LegacyAudioProducer( BMediaAddOn *addon, const char *name, int32 internal_id )
	: BMediaNode( name ), BMediaEventLooper(), BBufferProducer( B_MEDIA_RAW_AUDIO ), BControllable()
{
}
#endif
