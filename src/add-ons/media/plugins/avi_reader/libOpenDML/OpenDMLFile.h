#include <DataIO.h>
#include "OpenDMLParser.h"

class OpenDMLFile
{
public:
				OpenDMLFile();
				~OpenDMLFile();

	static bool IsSupported(BPositionIO *source);

	status_t	SetTo(BPositionIO *source);
	
	int			StreamCount();
	
	bigtime_t	Duration();
	uint32		FrameCount();
	
	bool		IsVideo(int stream_index);
	bool		IsAudio(int stream_index);

	const avi_main_header *AviMainHeader();

	const wave_format_ex *		AudioFormat(int stream_index, size_t *size = 0);
	const bitmap_info_header *	VideoFormat(int stream_index);
	const avi_stream_header *	StreamFormat(int stream_index);
	
	bool		GetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe);
	
	BPositionIO *Source() { return fSource; }

private:
	void		InitData();

	bool		OdmlReadIndexChunk(int stream_index);
	bool		OdmlReadIndexInfo(int stream_index);
	bool		OdmlGetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe);

	bool		AviGetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe);
		
private:
	BPositionIO *	fSource;
	OpenDMLParser *	fParser;

	struct stream_data;
	
	int				fStreamCount;
	stream_data *	fStreamData;
};

