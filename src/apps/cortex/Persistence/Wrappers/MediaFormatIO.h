/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// MediaFormatIO.h
// * PURPOSE
//   Wrapper class for media_format, providing XML
//   serialization support using the Cortex::XML package.
//
// * TO DO +++++
//   - import & export user data?
//   - import & export metadata?
//
// * HISTORY
//   e.moon		1jul99		Begun.

#ifndef __MediaFormatIO_H__
#define __MediaFormatIO_H__

#include "XML.h"
#include <MediaDefs.h>
#include <String.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaFormatIO :
	public		IPersistent {

public:				// *** ctor/dtor
	virtual ~MediaFormatIO();

	MediaFormatIO();
	MediaFormatIO(const media_format& format);

public:				// *** accessors
	// returns B_OK if the object contains a valid format,
	// or B_ERROR if not.
	status_t getFormat(media_format& outFormat) const;
	
public:				// *** static setup method
	// call this method to install hooks for the tags needed by
	// MediaFormatIO into the given document type
	static void AddTo(XML::DocumentType* pDocType);
	
public:				// *** top-level tags

	// these tags map directly to MediaFormatIO
	static const char* const s_multi_audio_tag;
	static const char* const s_raw_audio_tag;
	static const char* const s_raw_video_tag;
	static const char* const s_multistream_tag;
	static const char* const s_encoded_audio_tag;
	static const char* const s_encoded_video_tag;

public:				// *** nested tags

	static const char* const s_video_display_info_tag;
	
	static const char* const s_multistream_flags_tag;
	static const char* const s_multistream_vid_info_tag;
	static const char* const s_multistream_avi_info_tag;

	static const char* const s_multi_audio_info_tag;
	
	static const char* const s_media_type_tag;

public:				// *** IPersistent

//	void xmlExport(
//		ostream& 					stream,
//		ExportContext&		context) const;
	
	// EXPORT:
	
	void xmlExportBegin(
		ExportContext& context) const;
		
	void xmlExportAttributes(
		ExportContext& context) const;
	
	void xmlExportContent(
		ExportContext& context) const;
	
	void xmlExportEnd(
		ExportContext& context) const;

	// IMPORT

	void xmlImportBegin(
		ImportContext&		context);
	
	void xmlImportAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context);

	void xmlImportContent(
		const char*					data,
		uint32						length,
		ImportContext&		context);
		
	void xmlImportChild(
		IPersistent*			child,
		ImportContext&		context);
		
	void xmlImportComplete(
		ImportContext&		context);
		
	void xmlImportChildBegin(
		const char*					name,
		ImportContext&		context);

	void xmlImportChildAttribute(
		const char*					key,
		const char*					value,
		ImportContext&		context);

	void xmlImportChildContent(
		const char*					data,
		uint32						length,
		ImportContext&		context);
		
	void xmlImportChildComplete(
		const char*					name,
		ImportContext&		context);

private:				// *** state
	bool						m_complete;
	media_format		m_format;
	
	BString					m_mediaType;
};

__END_CORTEX_NAMESPACE
#endif /*__MediaFormatIO_H__*/
