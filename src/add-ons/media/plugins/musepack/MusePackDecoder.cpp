/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "MusePackDecoder.h"

#include <ByteOrder.h>
#include <InterfaceDefs.h>


MusePackDecoder::MusePackDecoder()
{
}


MusePackDecoder::~MusePackDecoder()
{
}


status_t 
MusePackDecoder::Setup(media_format *ioEncodedFormat, const void *infoBuffer, int32 infoSize)
{
}


status_t 
MusePackDecoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
}


status_t 
MusePackDecoder::Seek(uint32 seekTo, int64 seekFrame, int64 *frame, bigtime_t seekTime, bigtime_t *time)
{
}


status_t 
MusePackDecoder::Decode(void *buffer, int64 *frameCount, media_header *mediaHeader, media_decode_info *info)
{
}

