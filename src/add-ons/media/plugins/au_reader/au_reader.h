#include "ReaderPlugin.h"
#include "au.h"

class auReader : public Reader
{
public:
				auReader();
				~auReader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

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
									 
	BPositionIO *Source() { return fSource; }
	
private:
	BPositionIO *	fSource;
	
	media_format	fFormat;
	int64			fDataStart;
	int64			fDataSize;

	int64			fFrameCount;
	bigtime_t		fDuration;
	
	bool			fRaw;

	int64			fPosition;
	
	int				fChannelCount;
	int32			fFrameRate;
	int				fBitsPerSample;
	int				fBitsPerFrame;
	int				fBlockAlign;
	uint32			fFormatCode;
	
	void *			fBuffer;
	int32 			fBufferSize;
};


class auReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();
