#include "ReaderPlugin.h"
#include "libOpenDML/OpenDMLFile.h"

class aviReader : public Reader
{
public:
				aviReader();
				~aviReader();
	
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
	OpenDMLFile	*fFile;
};


class aviReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();
