#include "ReaderPlugin.h"
#include "aiff.h"

class aiffReader : public Reader
{
public:
				aiffReader();
				~aiffReader();
	
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
private:
	uint32		DecodeFrameRate(void *_80bit_float);
									 
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
	int				fValidBitsPerSample;
	int				fBytesPerSample;
	int				fBytesPerFrame;
	uint32			fFormatCode;
	
	void *			fBuffer;
	int32 			fBufferSize;
};


class aiffReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();
