#ifndef _CHANNEL_MIXER_
#define _CHANNEL_MIXER_

/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: ChannelMixer.h
 *  DESCR: Converts mono into stereo  
 *         stereo into mono (this one isn't very good)
 *         (multiple channel support should be added in the future)
 ***********************************************************************/

namespace MediaKitPrivate {

class ChannelMixer
{
public:
	static status_t 
	mix(void *dest, int dest_chan_count, 
		const void *source, int source_chan_count,
		int framecount, uint32 format);

	static status_t
	mix_2_to_1(void *dest, const void *source, int framecount, uint32 format);

	static status_t
	mix_1_to_2(void *dest, const void *source, int framecount, uint32 format);
	
};

} //namespace MediaKitPrivate

#endif
