#ifndef _OPENDML_PARSER_H
#define _OPENDML_PARSER_H

#include <DataIO.h>
#include "avi.h"

struct stream_info
{
	stream_info *		next;
	bool				is_audio;
	bool				is_video;
	bool				stream_header_valid;
	avi_stream_header	stream_header;
	wave_format_ex		*audio_format;
	size_t				audio_format_size;
	bool 				video_format_valid;
	bitmap_info_header	video_format;
	int64				odml_index_start;
	uint32				odml_index_size;
};

class OpenDMLParser
{
public:
					OpenDMLParser();
					~OpenDMLParser();
	bool	 		Parse(BPositionIO *source);
	
	int				StreamCount();

	const stream_info * StreamInfo(int index);
	
	int64			StandardIndexStart();
	uint32			StandardIndexSize();
	
	int64			MovieListStart();

	const avi_main_header * AviMainHeader();
	const odml_extended_header * OdmlExtendedHeader();

private:
	bool	 		ParseChunk_AVI(int number, uint64 start, uint32 size);
	bool	 		ParseChunk_LIST(uint64 start, uint32 size);
	bool	 		ParseChunk_idx1(uint64 start, uint32 size);
	bool	 		ParseChunk_indx(uint64 start, uint32 size);
	bool	 		ParseChunk_avih(uint64 start, uint32 size);
	bool	 		ParseChunk_strh(uint64 start, uint32 size);
	bool	 		ParseChunk_strf(uint64 start, uint32 size);
	bool	 		ParseChunk_dmlh(uint64 start, uint32 size);
	bool	 		ParseList_movi(uint64 start, uint32 size);
	bool	 		ParseList_generic(uint64 start, uint32 size);
	bool	 		ParseList_strl(uint64 start, uint32 size);

private:
	void			CreateNewStreamInfo();

	BPositionIO *	fSource;
	int64 			fSize;
	
	int64			fMovieListStart;
	
	int64			fStandardIndexStart;
	uint32			fStandardIndexSize;
	
	int				fStreamCount;
	int				fMovieChunkCount;
	
	avi_main_header fAviMainHeader;
	bool			fAviMainHeaderValid;
	
	odml_extended_header fOdmlExtendedHeader;
	bool			fOdmlExtendedHeaderValid;
	
	stream_info	*	fStreams;
	stream_info *	fCurrentStream;
};

#endif // _OPENDML_PARSER_H
