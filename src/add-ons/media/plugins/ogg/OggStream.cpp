#include "OggStream.h"
#include "OggSpeexStream.h"
#include "OggTheoraStream.h"
#include "OggTobiasStream.h"
#include "OggVorbisStream.h"
#include <stdio.h>

#define TRACE_THIS 1
#if TRACE_THIS
  #define TRACE printf
#else
  #define TRACE(a...) ((void)0)
#endif

/*
 * OggStream codec identification and instantiation
 */

/* static */ OggStream *
OggStream::makeOggStream(OggReader::GetPageInterface * interface,
                         long serialno, const ogg_packet & packet)
{
	TRACE("OggStream::makeOggStream\n");
	OggStream * stream;
	if (OggVorbisStream::IsValidHeader(packet)) {
		stream = new OggVorbisStream(serialno);
	} else if (OggTobiasStream::IsValidHeader(packet)) {
		stream = new OggTobiasStream(serialno);
	} else if (OggSpeexStream::IsValidHeader(packet)) {
		stream = new OggSpeexStream(serialno);
	} else if (OggTheoraStream::IsValidHeader(packet)) {
		stream = new OggTheoraStream(serialno);
	} else {
		stream = new OggStream(serialno);
	}
	stream->fReaderInterface = interface;
	return stream;
}


/* static */ bool
OggStream::findIdentifier(const ogg_packet & packet, const char * id, uint pos)
{
	uint length = strlen(id);
	if ((unsigned)packet.bytes < pos+length) {
		return false;
	}
	return !memcmp(&packet.packet[pos], id, length);
}


/*
 * OggStream generic functions
 */

OggStream::OggStream(long serialno)
{
	TRACE("OggStream::OggStream\n");
	this->fSerialno = serialno;
	ogg_stream_init(&fStreamState,serialno);
}


OggStream::~OggStream()
{

}


long
OggStream::GetSerial() const
{
	return fSerialno;
}


status_t
OggStream::AddPage(off_t position, ogg_page * page)
{
	TRACE("OggStream::AddPage\n");
	if (position >= 0) {
		fPagePositions.push_back(position);
	}
	ogg_stream_pagein(&fStreamState,page);
	return B_OK;
}


status_t
OggStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
              media_format *format)
{
	TRACE("OggStream::GetStreamInfo\n");
	debugger("OggStream::GetStreamInfo");
	status_t result = B_OK;
	ogg_packet packet;
	if (fHeaderPackets.size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = fHeaderPackets[0];
	if (!packet.b_o_s) {
		return B_ERROR; // first packet was not beginning of stream
	}

	// parse header packet
	uint32 four_bytes = *(uint32*)(&packet);

	// get the format for the description
	media_format_description description;
	description.family = B_MISC_FORMAT_FAMILY;
	description.u.misc.file_format = 'OggS';
	description.u.misc.codec = four_bytes;
	BMediaFormats formats;
	result = formats.InitCheck();
	if (result != B_OK) {
		return result;
	}
	if (!formats.Lock()) {
		return B_ERROR;
	}
	result = formats.GetFormatFor(description, format);
	formats.Unlock();
	if (result != B_OK) {
		return result;
	}

	// fill out format from header packet
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data, "vorb", 4);

	format->SetMetaData((void*)&fHeaderPackets,sizeof(&fHeaderPackets));
	*duration = 80000000;
	*frameCount = 60000;
	return B_OK;
}


status_t
OggStream::Seek(uint32 seekTo, int64 *frame, bigtime_t *time)
{
	debugger("OggStream::Seek");
	return B_OK;
}


status_t
OggStream::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
             media_header *mediaHeader)
{
	static ogg_packet packet;
	status_t result = GetPacket(&packet);
	if (result != B_OK) {
		return result;
	}
	*chunkBuffer = &packet;
	*chunkSize = sizeof(packet);
	return B_OK;
}


// subclass pull input function
status_t
OggStream::GetPacket(ogg_packet * packet)
{
	while (ogg_stream_packetpeek(&fStreamState,NULL) != 1) {
		fReaderInterface->GetNextPage();
	}
	if (ogg_stream_packetout(&fStreamState,packet) != 1) {
		return B_ERROR;
	}
	return B_OK;
}


// GetStreamInfo helpers
void
OggStream::SaveHeaderPacket(ogg_packet packet)
{
	uint8 * buffer = new uint8[packet.bytes];
	memcpy(buffer, packet.packet, packet.bytes);
	packet.packet = buffer;
	fHeaderPackets.push_back(packet);
}


// estimate 1 frame = 256 bytes
int64
OggStream::PositionToFrame(off_t position)
{
	return position/256;
}


off_t
OggStream::FrameToPosition(int64 frame)
{
	return frame*256;
}


// estimate 1 byte = 125 microseconds
bigtime_t
OggStream::PositionToTime(off_t position)
{
	return position*125;
}


off_t
OggStream::TimeToPosition(bigtime_t time)
{
	return time/125;
}
