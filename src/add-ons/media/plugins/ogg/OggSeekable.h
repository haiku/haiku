#ifndef _OGG_SEEKABLE_H
#define _OGG_SEEKABLE_H

#include "OggTrack.h"
#include "OggReaderPlugin.h"
#include <vector>

namespace BPrivate { namespace media {

class OggSeekable : public OggTrack {
public:
	static OggSeekable * makeOggSeekable(BPositionIO * positionIO, BLocker * positionLock,
						                 long serialno, const ogg_packet & packet);

	// interface for OggReader
	virtual status_t	GetStreamInfo(int64 *frameCount, bigtime_t *duration,
						              media_format *format);
	virtual status_t	Seek(uint32 seekTo, int64 *frame, bigtime_t *time);
	virtual status_t	GetNextChunk(void **chunkBuffer, int32 *chunkSize,
						             media_header *mediaHeader);

protected:
				OggSeekable(long serialno);
public:
	virtual		~OggSeekable();

	// reader push input function
	status_t	AddPage(off_t position, const ogg_page & page);

	void		SetLastPagePosition(off_t position);
	off_t		GetLastPagePosition();
private:
	off_t				fLastPagePosition;

protected:
	off_t		Seek(off_t position, int32 mode);
	off_t		Position(void) const;
	status_t	GetSize(off_t * size);
	status_t	ReadPage(ogg_page * page, int read_size = 4*B_PAGE_SIZE);

	// subclass pull input function
	status_t	GetPacket(ogg_packet * packet);
	ogg_packet			fChunkPacket;

protected:
	int64				fCurrentFrame;
	bigtime_t			fCurrentTime;

	int64				fFirstGranulepos;
	float				fFrameRate;

private:
	off_t				fPosition;
	std::vector<off_t>	fPagePositions;

private:
	ogg_sync_state		fSync;
	ogg_stream_state	fStreamState;
	BPositionIO *		fPositionIO;
	BLocker				fPositionLock;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif // _OGG_SEEKABLE_H
