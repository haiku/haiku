#ifndef _SAMPLINGRATE_CONVERTER_
#define _SAMPLINGRATE_CONVERTER_

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: SamplingrateConverter.cpp
 *  DESCR: Converts between different sampling rates
 *         This is really bad code, as there are much better algorithms
 *         than the simple duplicating or dropping of samples.
 *         Also, the implementation isn't very nice.
 *         Feel free to send me better versions to marcus@overhagen.de
 ***********************************************************************/

namespace MediaKitPrivate {

class SamplingrateConverter
{
public:
	static status_t 
	convert(void *dest, int destframecount,
			const void *src, int srcframecount,
            uint32 format, 
            int channel_count);
};

} //namespace MediaKitPrivate

#endif

