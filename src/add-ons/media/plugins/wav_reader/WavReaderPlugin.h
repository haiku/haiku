#include "ReaderPlugin.h"
#include "wav.h"

class WavReader : public Reader
{
public:
				WavReader();
				~WavReader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);
	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);

	status_t	Seek(void *cookie,
					 media_seek_type seekTo,
					 int64 *frame, bigtime_t *time);

	status_t	GetNextChunk(void *cookie,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);
									 
	BPositionIO *Source() { return fSource; }
	
private:
	BPositionIO *	fSource;
	uint64			fDataSize;
	wave_header 	fRawHeader;
};


class WavReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();
