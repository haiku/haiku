/*
 * Copyright 2018, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CODEC_ROSTER_H
#define _CODEC_ROSTER_H

#include <Decoder.h>
#include <Encoder.h>
#include <MediaDefs.h>
#include <Reader.h>
#include <Streamer.h>
#include <Writer.h>


namespace BPrivate {
namespace media {


class BCodecRoster {
public:

	static status_t		InstantiateReader(BReader** reader,
							int32* streamCount, media_file_format* mff,
							BDataIO* source);
	static void			ReleaseReader(BReader* reader);

	static status_t		InstantiateDecoder(BDecoder** decoder,
							const media_format& format);
	static status_t		InstantiateDecoder(BDecoder** decoder,
							const media_codec_info& mci);
	static void			ReleaseDecoder(BDecoder* decoder);

	static status_t		GetDecoderInfo(BDecoder* decoder,
							media_codec_info* info);
	
	static status_t		InstantiateWriter(BWriter** writer,
							const media_file_format& mff,
							BDataIO* target);
	static void			ReleaseWriter(BWriter* writer);

	static status_t		InstantiateEncoder(BEncoder** encoder,
							const media_format& format);
	static status_t		InstantiateEncoder(BEncoder** encoder,
							const media_codec_info* codecInfo,
							uint32 flags);
	static void			ReleaseEncoder(BEncoder* encoder);

	static status_t		InstantiateStreamer(BStreamer** streamer,
							BUrl url, BDataIO** source);
	static void			ReleaseStreamer(BStreamer* streamer);

/*!	\brief	Use this to iterate through the available encoders for a given file
			format.
	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param fileFormat	A pointer to a valid media_file_format structure
						as can be obtained through get_next_file_format().
	\param inputFormat	This is the type of data given to the encoder.
	\param _outputFormat This is the type of data the encoder will output.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
 */
	static status_t		GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
							const media_format* inputFormat, media_format* _outputFormat,
							media_codec_info* _codecInfo);

/*!	\brief	Use this to iterate through the available encoders with
			restrictions to the input and output media_format while the
			encoders can specialize wildcards in the media_formats.

	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param fileFormat	A pointer to a valid media_file_format structure
						as can be obtained through get_next_file_format().
						You can pass \c NULL if you don't care.
	\param inputFormat	This is the type of data given to the encoder,
						wildcards are accepted.
	\param outputFormat	This is the type of data you want the encoder to
						output. Wildcards are accepted.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.
	\param _acceptedInputFormat This is the type of data that the encoder will
						accept as input. Wildcards in \a inFormat will be
						specialized here.
	\param _acceptedOutputFormat This is the type of data that the encoder will
						output. Wildcards in \a _outFormat will be specialized
						here.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
*/
	static status_t		GetNextEncoder(int32* cookie, const media_file_format* fileFormat,
							const media_format* inputFormat, const media_format* outputFormat,
							media_codec_info* _codecInfo, media_format* _acceptedInputFormat,
							media_format* _acceptedOutputFormat);

/*!	\brief	Iterate over the all the available encoders without media_format
			restrictions.

	\param cookie		A pointer to a preallocated cookie, which you need
						to initialize to \c 0 before calling this function
						the first time.
	\param _codecInfo	Pointer to a preallocated media_codec_info structure,
						information about the encoder is placed there.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_INDEX: There are no more encoders.
*/
	static status_t		GetNextEncoder(int32* cookie, media_codec_info* _codecInfo);
};

} // namespace media
} // namespace BPrivate

using namespace BPrivate::media;

#endif	// _CODEC_ROSTER_H
