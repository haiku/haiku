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

	const wave_format_ex *		AudioFormat(int stream_index);
	const bitmap_info_header *	VideoFormat(int stream_index);
	const avi_stream_header *	StreamFormat(int stream_index);
	
	bool		GetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe);
	
	BPositionIO *Source() { return fSource; }

private:
	void		InitData();
	bool		ReadIndexChunk(int stream_index);
	bool		ReadIndexInfo(int stream_index);
	
private:
	BPositionIO *	fSource;
	OpenDMLParser *	fParser;

	struct stream_data;
	
	int				fStreamCount;
	stream_data *	fStreamData;
};

