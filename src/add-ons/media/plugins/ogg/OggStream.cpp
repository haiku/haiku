#include "OggStream.h"
#include "OggSpeexStream.h"
#include "OggTheoraStream.h"
#include "OggTobiasStream.h"
#include "OggVorbisStream.h"
#include <Autolock.h>
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
	fCurrentFrame = 0;
	fCurrentTime = 0;
	this->fSerialno = serialno;
	ogg_sync_init(&fSync);
	ogg_stream_init(&fSeekStreamState,serialno);
	fCurrentPage = 0;
	fPacketOnCurrentPage = 0;
	fCurrentPacket = 0;
	ogg_stream_init(&fEndStreamState,serialno);
	fEndPage = 0;
	fPacketOnEndPage = 0;
	fEndPacket = 0;
}


OggStream::~OggStream()
{
	// free internal header packet storage
	std::vector<ogg_packet>::iterator iter = fHeaderPackets.begin();
	while (iter != fHeaderPackets.end()) {
		delete iter->packet;
		iter++;
	}
	// free internal stream state storage
	ogg_sync_clear(&fSync);
	ogg_stream_clear(&fSeekStreamState);
	ogg_stream_clear(&fEndStreamState);
}


long
OggStream::GetSerial() const
{
	return fSerialno;
}


status_t
OggStream::AddPage(off_t position, ogg_page * page)
{
	TRACE("OggStream::AddPage %llu\n",position);
	if (position >= 0) {
		fPagePositions.push_back(position);
	}
	BAutolock autolock(fSyncLock);
	char * buffer;
	buffer = ogg_sync_buffer(&fSync,page->header_len);
	memcpy(buffer,page->header,page->header_len);
	ogg_sync_wrote(&fSync,page->header_len);
	buffer = ogg_sync_buffer(&fSync,page->body_len);
	memcpy(buffer,page->body,page->body_len);
	ogg_sync_wrote(&fSync,page->body_len);
	return B_OK;
}


status_t
OggStream::GetStreamInfo(int64 *frameCount, bigtime_t *duration,
              media_format *format)
{
	TRACE("OggStream::GetStreamInfo\n");
	status_t result = B_OK;
	ogg_packet packet;
	if (fHeaderPackets.size() < 1) {
		result = GetPacket(&packet);
		if (result != B_OK) {
			TRACE("OggStream::GetStreamInfo failed to get header packet\n");
			return result;
		}
		SaveHeaderPacket(packet);
	}
	packet = fHeaderPackets[0];
	if (!packet.b_o_s) {
		TRACE("OggStream::GetStreamInfo failed : not beginning of stream\n");
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
	if (result == B_OK) {
		result = formats.GetFormatFor(description, format);
	}
	if (result != B_OK) {
		// ignore the error, allow the user to use ReadChunk interface
	}

	// fill out format from header packet
	format->user_data_type = B_CODEC_TYPE_INFO;
	strncpy((char*)format->user_data, (char*)(&packet), 4);

	format->SetMetaData((void*)&fHeaderPackets,sizeof(fHeaderPackets));
	*duration = 140000000;
	*frameCount = 60000;
	return B_OK;
}


status_t
OggStream::Seek(uint32 seekTo, int64 *frame, bigtime_t *time)
{
	TRACE("OggStream::Seek to %lld : %lld\n", *frame, *time);
	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		if (*frame > fOggFrameInfos.size() - 1) {
			// seek to end
			fCurrentPage = fEndPage;
			fPacketOnCurrentPage = fPacketOnEndPage;
			fCurrentPacket = fEndPacket;
			return B_OK;
		}
		*frame = max_c(0, *frame); // clip to zero
		// input the page into a temporary seek stream
		ogg_stream_state seekStreamState;
		uint pageno = fOggFrameInfos[*frame].GetPage();
		off_t position = fPagePositions[pageno];
		ogg_stream_init(&seekStreamState, fSerialno);
		status_t result = fReaderInterface->GetPageAt(position, &seekStreamState);
		if (result != B_OK) {
			TRACE("OggStream::GetPageAt %lld failed\n", position);
			return result; // pageno/fPagePosition corrupted?
		}
		// discard earlier packets from this page
		uint packetno = fOggFrameInfos[*frame].GetPacketOnPage();
		while (packetno-- > 0) {
			ogg_packet packet;
			// don't check for errors, we may have a packet ending on page
			ogg_stream_packetout(&seekStreamState, &packet);
		}
		// clear out the former seek stream state
		// this will delete its internal storage
		ogg_stream_clear(&fSeekStreamState);
		// initialize it with the temporary seek stream
		// this will transfer the internal storage to it
		fSeekStreamState = seekStreamState;
		// we notably do not clear our temporary stream
		// instead we just let it go out of scope
		fCurrentPage = fOggFrameInfos[*frame].GetNextPage();
		fPacketOnCurrentPage = fOggFrameInfos[*frame].GetNextPacketOnPage();
		fCurrentPacket = fOggFrameInfos[*frame].GetNextPacket();
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		*frame = *time/50000;
		return Seek(B_MEDIA_SEEK_TO_FRAME,frame,time);
	}
	return B_OK;
}


status_t
OggStream::GetNextChunk(void **chunkBuffer, int32 *chunkSize,
             media_header *mediaHeader)
{
	static ogg_packet packet;
	if (fCurrentPacket - fHeaderPackets.size() == fOggFrameInfos.size()) {
		OggFrameInfo info(fEndPage,fPacketOnEndPage,fEndPacket);
		fOggFrameInfos.push_back(info);
	}
	status_t result = GetPacket(&packet);
	if (fCurrentPacket - fHeaderPackets.size() == fOggFrameInfos.size()) {
		fOggFrameInfos[fOggFrameInfos.size()-1].SetNext(fEndPage,fPacketOnEndPage,fEndPacket);
	}
	if (result != B_OK) {
		TRACE("OggStream::GetNextChunk failed: GetPacket = %s\n", strerror(result));
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
	if (fCurrentPacket >= fEndPacket) {
		// at the end, pull the packet
		uint8 pageno = fEndPage;
		while (ogg_stream_packetpeek(&fEndStreamState, NULL) != 1) {
			BAutolock autolock(fSyncLock);
			int result;
			ogg_page page;
			while ((result = ogg_sync_pageout(&fSync,&page)) == 0) {
				status_t result = fReaderInterface->GetNextPage();
				if (result != B_OK) {
					TRACE("OggStream::GetPacket: GetNextPage = %s\n", strerror(result));
					return result;
				}
			}
			if (result == -1) {
				TRACE("OggStream::GetPacket: ogg_sync_pageout: not synced??\n");
				return B_ERROR;
			}
			if (ogg_stream_pagein(&fEndStreamState,&page) != 0) {
				TRACE("OggStream::GetPacket: ogg_stream_pagein: failed??\n");
				return B_ERROR;
			}
			fEndPage++;
		}
		if (ogg_stream_packetout(&fEndStreamState, packet) != 1) {
			TRACE("OggStream::GetPacket: ogg_stream_packetout failed at the end\n");
			return B_ERROR;
		}
		fEndPacket++;
		if (pageno != fEndPage) {
			fPacketOnEndPage = 0;
		} else {
			fPacketOnEndPage++;
		}
		fCurrentPacket = fEndPacket;
		fPacketOnCurrentPage = fPacketOnEndPage;
		fCurrentPage = fEndPage;
	} else {
		// in the middle, get packet at position
		uint8 page = fCurrentPage;
		while (ogg_stream_packetpeek(&fSeekStreamState, NULL) != 1) {
			off_t position = fPagePositions[fCurrentPage++];
			status_t result = fReaderInterface->GetPageAt(position, &fSeekStreamState);
			if (result != B_OK) {
				TRACE("OggStream::GetPacket: GetPageAt = %s\n", strerror(result));
				return result;
			}
		}
		if (ogg_stream_packetout(&fSeekStreamState, packet) != 1) {
			TRACE("OggStream::GetPacket: ogg_stream_packetout failed in the middle\n");
			return B_ERROR;
		}
		fCurrentPacket++;
		if (page != fCurrentPage) {
			fPacketOnCurrentPage = 0;
		} else {
			fPacketOnCurrentPage++;
		}
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

