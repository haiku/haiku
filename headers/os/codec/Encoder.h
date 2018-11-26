/*
 * Copyright 2009-2015, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _ENCODER_PLUGIN_H
#define _ENCODER_PLUGIN_H


#include <MediaPlugin.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include <View.h>


namespace BCodecKit {

namespace BPrivate {
	class PluginManager;
}


class BChunkWriter {
public:
								BChunkWriter();
	virtual						~BChunkWriter();

protected:
	friend class BEncoder;

	virtual	status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize,
									media_encode_info* encodeInfo) = 0;
};


class BEncoder {
public:
	// Some codecs may only support certain input color spaces, or output
	// color spaces, or multiple of 16 width/height... This method is needed
	// for get_next_encoder() functionality. If _acceptedInputFormat is NULL,
	// you simply return a status indicating if proposed format is acceptable.
	// If it contains wildcards for fields that you have restrictions on,
	// return an error. In that case, the user should be using the form of
	// get_next_encoder() that allows to specify the accepted format. If
	// _acceptedInputFormat is not NULL, copy the proposedFormat into
	// _acceptedInputFormat and specialize any wildcards. You must (!) also
	// change non-wildcard fields, like the video width if you want to round to
	// the nearest multiple of 16 for example. Only if the format is completely
	// unacceptable, return an error.
	virtual	status_t			AcceptedFormat(
									const media_format* proposedInputFormat,
									media_format* _acceptedInputFormat = NULL)
										= 0;

	// The passed media_format may not contain wildcards and must be the same
	// format that was passed to get_next_encoder() (or it must be the format
	// returned in _acceptedInputFormat).
	virtual	status_t			SetUp(const media_format* inputFormat) = 0;

	virtual	status_t			AddTrackInfo(uint32 code, const void* data,
									size_t size, uint32 flags = 0);

	// Ownership of the BView and BParameterWeb remain with the Encoder.
	// A window embedding the view must remove it before it is destroyed.
	virtual	BView*				ParameterView();

	virtual	BParameterWeb*		ParameterWeb();
	virtual	status_t			GetParameterValue(int32 id, void* value,
									size_t* size) const;
	virtual	status_t			SetParameterValue(int32 id, const void* value,
									size_t size);

	virtual status_t			GetEncodeParameters(
									encode_parameters* parameters) const;
	virtual status_t			SetEncodeParameters(
									encode_parameters* parameters);

	virtual status_t			Encode(const void* buffer, int64 frameCount,
									media_encode_info* info) = 0;

			status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize,
									media_encode_info* encodeInfo);

			void				SetChunkWriter(BChunkWriter* writer);

	virtual status_t			Perform(perform_code code, void* data);

protected:
								BEncoder();
	virtual						~BEncoder();

private:
			BChunkWriter*		fChunkWriter;

			BMediaPlugin*		fMediaPlugin;

	// needed for plug-in reference count management
	friend class BCodecKit::BPrivate::PluginManager;

	virtual void				_ReservedEncoder1();
	virtual void				_ReservedEncoder2();
	virtual void				_ReservedEncoder3();
	virtual void				_ReservedEncoder4();
	virtual void				_ReservedEncoder5();
	virtual void				_ReservedEncoder6();
	virtual void				_ReservedEncoder7();
	virtual void				_ReservedEncoder8();
	virtual void				_ReservedEncoder9();
	virtual void				_ReservedEncoder10();
	virtual void				_ReservedEncoder11();
	virtual void				_ReservedEncoder12();
	virtual void				_ReservedEncoder13();
	virtual void				_ReservedEncoder14();
	virtual void				_ReservedEncoder15();
	virtual void				_ReservedEncoder16();
	virtual void				_ReservedEncoder17();
	virtual void				_ReservedEncoder18();
	virtual void				_ReservedEncoder19();
	virtual void				_ReservedEncoder20();

	// FBC padding
			uint32				fReserved[20];
};


class BEncoderPlugin : public virtual BMediaPlugin {
public:
								BEncoderPlugin();

	virtual	BEncoder*			NewEncoder(
									const media_codec_info& codecInfo) = 0;

	virtual	BEncoder*			NewEncoder(
									const media_format& format) = 0;

	virtual	status_t			RegisterNextEncoder(int32* cookie,
									media_codec_info* codecInfo,
									media_format_family* formatFamily,
									media_format* inputFormat,
									media_format* outputFormat) = 0;
};


}


#endif // _ENCODER_PLUGIN_H
