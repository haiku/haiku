/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ZLIB_COMPRESSION_ALGORITHM_H_
#define _ZLIB_COMPRESSION_ALGORITHM_H_


#include <CompressionAlgorithm.h>


// compression level
enum {
	B_ZLIB_COMPRESSION_NONE		= 0,
	B_ZLIB_COMPRESSION_FASTEST	= 1,
	B_ZLIB_COMPRESSION_BEST		= 9,
	B_ZLIB_COMPRESSION_DEFAULT	= -1,
};


class BZlibCompressionParameters : public BCompressionParameters {
public:
								BZlibCompressionParameters(
									int compressionLevel
										= B_ZLIB_COMPRESSION_DEFAULT);
	virtual						~BZlibCompressionParameters();

			int32				CompressionLevel() const;
			void				SetCompressionLevel(int32 level);

			size_t				BufferSize() const;
			void				SetBufferSize(size_t size);

			// TODO: Support setting the gzip header.
			bool				IsGzipFormat() const;
			void				SetGzipFormat(bool gzipFormat);

private:
			int32				fCompressionLevel;
			size_t				fBufferSize;
			bool				fGzipFormat;
};


class BZlibDecompressionParameters : public BDecompressionParameters {
public:
								BZlibDecompressionParameters();
	virtual						~BZlibDecompressionParameters();

			size_t				BufferSize() const;
			void				SetBufferSize(size_t size);

private:
			size_t				fBufferSize;
};


class BZlibCompressionAlgorithm : public BCompressionAlgorithm {
public:
								BZlibCompressionAlgorithm();
	virtual						~BZlibCompressionAlgorithm();

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

	virtual	status_t			CompressBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize, size_t& _compressedSize,
									const BCompressionParameters* parameters
										= NULL);
	virtual	status_t			DecompressBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize,
									size_t& _uncompressedSize,
									const BDecompressionParameters* parameters
										= NULL);

private:
			struct CompressionStrategy;
			struct DecompressionStrategy;

			template<typename BaseClass, typename Strategy> struct Stream;
			template<typename BaseClass, typename Strategy>
				friend struct Stream;

private:
	static	status_t			_TranslateZlibError(int error);
};


#endif	// _ZLIB_COMPRESSION_ALGORITHM_H_
