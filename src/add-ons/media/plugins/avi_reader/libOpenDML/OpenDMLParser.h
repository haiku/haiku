/*
 * Copyright (c) 2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _OPEN_DML_PARSER_H
#define _OPEN_DML_PARSER_H

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

#endif
