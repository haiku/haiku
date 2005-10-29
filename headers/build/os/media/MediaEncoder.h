#ifndef MEDIAENCODER_H
#define MEDIAENCODER_H

#include <MediaDefs.h>
#include <MediaFormats.h>

namespace BPrivate {
	class Encoder;
	class _AddonManager;
}

class BMediaEncoder {
	public:
		BMediaEncoder();
		BMediaEncoder(const media_format *output_format);
		BMediaEncoder(const media_codec_info *mci);
		virtual ~BMediaEncoder();
		status_t InitCheck() const;

		status_t SetTo(const media_format *output_format);
		status_t SetTo(const media_codec_info *mci);
		status_t SetFormat(media_format *input_format,
		                   media_format *output_format,
		                   media_file_format *mfi = NULL);
		status_t Encode(const void *buffer, int64 frame_count,
		                media_encode_info *info);
		status_t GetEncodeParameters(encode_parameters *parameters) const;
		status_t SetEncodeParameters(encode_parameters *parameters);
	protected:
		virtual status_t WriteChunk(const void *chunk_data, size_t chunk_len,
		                            media_encode_info *info) = 0;
		virtual status_t AddTrackInfo(uint32 code, const char *data, size_t size);

	private:
		//	unimplemented
		BMediaEncoder(const BMediaEncoder &);
		BMediaEncoder & operator=(const BMediaEncoder &);

		static status_t write_chunk(void *classptr, const void *chunk_data,
		                            size_t chunk_len, media_encode_info *info);
		void	Init();
		void	ReleaseEncoder();

		BPrivate::_AddonManager	*fEncoderMgr;
		BPrivate::Encoder		*fEncoder;
		int32					fEncoderID;
		bool					fFormatValid;
		bool					fEncoderStarted;
		status_t				fInitStatus;

		/* fbc data and virtuals */

		uint32 _reserved_BMediaEncoder_[32];

		virtual	status_t _Reserved_BMediaEncoder_0(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_1(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_2(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_3(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_4(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_5(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_6(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_7(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_8(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_9(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_10(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_11(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_12(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_13(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_14(int32 arg, ...);
		virtual	status_t _Reserved_BMediaEncoder_15(int32 arg, ...);
};

class BMediaBufferEncoder: public BMediaEncoder {
	public:
		BMediaBufferEncoder();
		BMediaBufferEncoder(const media_format *output_format);
		BMediaBufferEncoder(const media_codec_info *mci);
		status_t EncodeToBuffer(void *output_buffer, size_t *output_size,
		                        const void *input_buffer, int64 frame_count,
		                        media_encode_info *info);
	protected:
		virtual status_t WriteChunk(const void *chunk_data, size_t chunk_len,
		                            media_encode_info *info);

		void *fBuffer;
		int32 fBufferSize;
};


#endif

