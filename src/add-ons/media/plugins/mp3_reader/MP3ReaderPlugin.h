#ifndef _MP3_READER_PLUGIN_H
#define _MP3_READER_PLUGIN_H

#include "ReaderPlugin.h"

namespace BPrivate { namespace media {

class mp3Reader : public Reader
{
public:
				mp3Reader();
				~mp3Reader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);
	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);

	status_t	Seek(void *cookie,
					 uint32 seekTo,
					 int64 *frame, bigtime_t *time);

	status_t	GetNextChunk(void *cookie,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);
									 
	BPositionIO *Source() { return fSeekableSource; }

private:
	bool		ParseFile();
	bool 		IsMp3File();
	
	// checks if the buffer contains a valid mp3 stream, length should be
	bool		IsValidStream(uint8 *buffer, int size);
	
	int			GetFrameLength(void *header);
	int			GetXingVbrLength(uint8 *header);
	int			GetFraunhoferVbrLength(uint8 *header);
	int			GetLameVbrLength(uint8 *header);
	int			GetId3v2Length(uint8 *header);
	int			GetInfoCbrLength(uint8 *header);
	
	bool		FindData();

	void		ParseXingVbrHeader(int64 pos);
	void		ParseFraunhoferVbrHeader(int64 pos);
	
	int64		XingSeekPoint(float percent);
	
private:
	BPositionIO *	fSeekableSource;
	int64			fFileSize;
	
	int64			fDataStart;
	int64			fDataSize;
	
	struct xing_vbr_info;
	struct fhg_vbr_info;
	
	xing_vbr_info *	fXingVbrInfo;
	fhg_vbr_info *	fFhgVbrInfo;
};


class mp3ReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
