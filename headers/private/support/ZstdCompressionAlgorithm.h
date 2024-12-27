/*
 * Copyright 2017, Jérôme Duval.
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ZSTD_COMPRESSION_ALGORITHM_H_
#define _ZSTD_COMPRESSION_ALGORITHM_H_


#include <CompressionAlgorithm.h>


// compression level
enum {
	B_ZSTD_COMPRESSION_NONE		= 0,
	B_ZSTD_COMPRESSION_FASTEST	= 1,
	B_ZSTD_COMPRESSION_BEST		= 22,
	B_ZSTD_COMPRESSION_DEFAULT	= 4,
};


class BZstdCompressionParameters : public BCompressionParameters {
public:
								BZstdCompressionParameters(
									int compressionLevel
										= B_ZSTD_COMPRESSION_DEFAULT);
	virtual						~BZstdCompressionParameters();

			int32				CompressionLevel() const;
			void				SetCompressionLevel(int32 level);

			size_t				BufferSize() const;
			void				SetBufferSize(size_t size);

private:
			int32				fCompressionLevel;
			size_t				fBufferSize;
};


class BZstdDecompressionParameters : public BDecompressionParameters {
public:
								BZstdDecompressionParameters();
	virtual						~BZstdDecompressionParameters();

			size_t				BufferSize() const;
			void				SetBufferSize(size_t size);

private:
			size_t				fBufferSize;
};


class BZstdCompressionAlgorithm : public BCompressionAlgorithm {
public:
								BZstdCompressionAlgorithm();
	virtual						~BZstdCompressionAlgorithm();

	virtual	status_t			CreateCompressingInputStream(BDataIO* input,
									const BCompressionParameters* parameters,
									BDataIO*& _stream);
	virtual	status_t			CreateCompressingOutputStream(BDataIO* output,
									const BCompressionParameters* parameters,
									BDataIO*& _stream);
	virtual	status_t			CreateDecompressingInputStream(BDataIO* input,
									const BDecompressionParameters* parameters,
									BDataIO*& _stream);
	virtual	status_t			CreateDecompressingOutputStream(BDataIO* output,
									const BDecompressionParameters* parameters,
									BDataIO*& _stream);

	virtual	status_t			CompressBuffer(const iovec& input, iovec& output,
									const BCompressionParameters* parameters = NULL,
									iovec* scratch = NULL);
	virtual	status_t			DecompressBuffer(const iovec& input, iovec& output,
									const BDecompressionParameters* parameters = NULL,
									iovec* scratch = NULL);

private:
			struct CompressionStrategy;
			struct DecompressionStrategy;

			template<typename BaseClass, typename Strategy, typename StreamType> struct Stream;
			template<typename BaseClass, typename Strategy, typename StreamType>
				friend struct Stream;

private:
	static	status_t			_TranslateZstdError(size_t error);
};


#endif	// _ZSTD_COMPRESSION_ALGORITHM_H_
