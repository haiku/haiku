#ifndef MEDIADECODER_H
#define MEDIADECODER_H


#include <MediaFormats.h>


namespace BCodecKit {
	class BDecoder;
}


class BMediaDecoder {
	public:
		BMediaDecoder();
		BMediaDecoder(const media_format *in_format,
		              const void *info = NULL, size_t info_size = 0);
		BMediaDecoder(const media_codec_info *mci);
		virtual ~BMediaDecoder();
		status_t InitCheck() const;

		status_t SetTo(const media_format *in_format,
		               const void *info = NULL, size_t info_size = 0);
		status_t SetTo(const media_codec_info *mci);
		status_t SetInputFormat(const media_format *in_format,
		                        const void *in_info = NULL, size_t in_size = 0);
		status_t SetOutputFormat(media_format *output_format);
			// Set output format to closest acceptable format, returns the
			// format used.
		status_t Decode(void *out_buffer, int64 *out_frameCount,
		                media_header *out_mh, media_decode_info *info);
		status_t GetDecoderInfo(media_codec_info *out_info) const;

	protected:
		virtual status_t GetNextChunk(const void **chunkData, size_t *chunkLen,
		                              media_header *mh) = 0;

	private: 

		//	unimplemented
		BMediaDecoder(const BMediaDecoder &);
		BMediaDecoder & operator=(const BMediaDecoder &);

		status_t AttachToDecoder();

		BCodecKit::BDecoder*	fDecoder;
		status_t				fInitStatus;

		/* fbc data and virtuals */

		uint32 _reserved_BMediaDecoder_[33];

		virtual	status_t _Reserved_BMediaDecoder_0(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_1(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_2(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_3(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_4(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_5(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_6(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_7(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_8(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_9(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_10(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_11(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_12(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_13(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_14(int32 arg, ...);
		virtual	status_t _Reserved_BMediaDecoder_15(int32 arg, ...);
};

class BMediaBufferDecoder : public BMediaDecoder {
	public:
		BMediaBufferDecoder();
		BMediaBufferDecoder(const media_format *in_format,
		                    const void *info = NULL, size_t info_size = 0);
		BMediaBufferDecoder(const media_codec_info *mci);
		status_t DecodeBuffer(const void *input_buffer, size_t input_size,
		                      void *out_buffer, int64 *out_frameCount,
		                      media_header *out_mh,
		                      media_decode_info *info = NULL);
	protected:
		virtual status_t GetNextChunk(const void **chunkData, size_t *chunkLen,
		                              media_header *mh);
		const void *fBuffer;
		int32 fBufferSize;
};

#endif

