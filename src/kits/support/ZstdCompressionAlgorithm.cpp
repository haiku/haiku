/*
 * Copyright 2017, Jérôme Duval.
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ZstdCompressionAlgorithm.h>

#include <errno.h>
#include <string.h>

#include <algorithm>
#include <new>

#ifdef ZSTD_ENABLED
  #include <zstd.h>
  #include <zstd_errors.h>
#endif

#include <DataIO.h>


// build compression support only for userland
#if defined(ZSTD_ENABLED) && !defined(_KERNEL_MODE) && !defined(_BOOT_MODE)
#	define B_ZSTD_COMPRESSION_SUPPORT 1
#endif


static const size_t kMinBufferSize		= 1024;
static const size_t kMaxBufferSize		= 1024 * 1024;
static const size_t kDefaultBufferSize	= 4 * 1024;


static size_t
sanitize_buffer_size(size_t size)
{
	if (size < kMinBufferSize)
		return kMinBufferSize;
	return std::min(size, kMaxBufferSize);
}


// #pragma mark - BZstdCompressionParameters


BZstdCompressionParameters::BZstdCompressionParameters(
	int compressionLevel)
	:
	BCompressionParameters(),
	fCompressionLevel(compressionLevel),
	fBufferSize(kDefaultBufferSize)
{
}


BZstdCompressionParameters::~BZstdCompressionParameters()
{
}


int32
BZstdCompressionParameters::CompressionLevel() const
{
	return fCompressionLevel;
}


void
BZstdCompressionParameters::SetCompressionLevel(int32 level)
{
	fCompressionLevel = level;
}


size_t
BZstdCompressionParameters::BufferSize() const
{
	return fBufferSize;
}


void
BZstdCompressionParameters::SetBufferSize(size_t size)
{
	fBufferSize = sanitize_buffer_size(size);
}


// #pragma mark - BZstdDecompressionParameters


BZstdDecompressionParameters::BZstdDecompressionParameters()
	:
	BDecompressionParameters(),
	fBufferSize(kDefaultBufferSize)
{
}


BZstdDecompressionParameters::~BZstdDecompressionParameters()
{
}


size_t
BZstdDecompressionParameters::BufferSize() const
{
	return fBufferSize;
}


void
BZstdDecompressionParameters::SetBufferSize(size_t size)
{
	fBufferSize = sanitize_buffer_size(size);
}


// #pragma mark - CompressionStrategy


#ifdef B_ZSTD_COMPRESSION_SUPPORT


struct BZstdCompressionAlgorithm::CompressionStrategy {
	typedef BZstdCompressionParameters Parameters;

	static const bool kNeedsFinalFlush = true;

	static size_t Init(ZSTD_CStream **stream,
		const BZstdCompressionParameters* parameters)
	{
		int32 compressionLevel = B_ZSTD_COMPRESSION_DEFAULT;
		if (parameters != NULL) {
			compressionLevel = parameters->CompressionLevel();
		}

		*stream = ZSTD_createCStream();
		return ZSTD_initCStream(*stream, compressionLevel);
	}

	static void Uninit(ZSTD_CStream *stream)
	{
		ZSTD_freeCStream(stream);
	}

	static size_t Process(ZSTD_CStream *stream, ZSTD_inBuffer *input,
		ZSTD_outBuffer *output, bool flush)
	{
		if (flush)
			return ZSTD_flushStream(stream, output);
		else
			return ZSTD_compressStream(stream, output, input);
	}
};


#endif	// B_ZSTD_COMPRESSION_SUPPORT


// #pragma mark - DecompressionStrategy


#ifdef ZSTD_ENABLED


struct BZstdCompressionAlgorithm::DecompressionStrategy {
	typedef BZstdDecompressionParameters Parameters;

	static const bool kNeedsFinalFlush = false;

	static size_t Init(ZSTD_DStream **stream,
		const BZstdDecompressionParameters* /*parameters*/)
	{
		*stream = ZSTD_createDStream();
		return ZSTD_initDStream(*stream);
	}

	static void Uninit(ZSTD_DStream *stream)
	{
		ZSTD_freeDStream(stream);
	}

	static size_t Process(ZSTD_DStream *stream, ZSTD_inBuffer *input,
		ZSTD_outBuffer *output, bool flush)
	{
		return ZSTD_decompressStream(stream, output, input);
	}

};


// #pragma mark - Stream


template<typename BaseClass, typename Strategy, typename StreamType>
struct BZstdCompressionAlgorithm::Stream : BaseClass {
	Stream(BDataIO* io)
		:
		BaseClass(io),
		fStreamInitialized(false)
	{
	}

	~Stream()
	{
		if (fStreamInitialized) {
			if (Strategy::kNeedsFinalFlush)
				this->Flush();
			Strategy::Uninit(fStream);
		}
	}

	status_t Init(const typename Strategy::Parameters* parameters)
	{
		status_t error = this->BaseClass::Init(
			parameters != NULL ? parameters->BufferSize() : kDefaultBufferSize);
		if (error != B_OK)
			return error;

		size_t zstdError = Strategy::Init(&fStream, parameters);
		if (ZSTD_getErrorCode(zstdError) != ZSTD_error_no_error)
			return _TranslateZstdError(zstdError);

		fStreamInitialized = true;
		return B_OK;
	}

	virtual status_t ProcessData(const void* input, size_t inputSize,
		void* output, size_t outputSize, size_t& bytesConsumed,
		size_t& bytesProduced)
	{
		return _ProcessData(input, inputSize, output, outputSize,
			bytesConsumed, bytesProduced, false);
	}

	virtual status_t FlushPendingData(void* output, size_t outputSize,
		size_t& bytesProduced)
	{
		size_t bytesConsumed;
		return _ProcessData(NULL, 0, output, outputSize,
			bytesConsumed, bytesProduced, true);
	}

	template<typename BaseParameters>
	static status_t Create(BDataIO* io, BaseParameters* _parameters,
		BDataIO*& _stream)
	{
		const typename Strategy::Parameters* parameters
#ifdef _BOOT_MODE
			= static_cast<const typename Strategy::Parameters*>(_parameters);
#else
			= dynamic_cast<const typename Strategy::Parameters*>(_parameters);
#endif
		Stream* stream = new(std::nothrow) Stream(io);
		if (stream == NULL)
			return B_NO_MEMORY;

		status_t error = stream->Init(parameters);
		if (error != B_OK) {
			delete stream;
			return error;
		}

		_stream = stream;
		return B_OK;
	}

private:
	status_t _ProcessData(const void* input, size_t inputSize,
		void* output, size_t outputSize, size_t& bytesConsumed,
		size_t& bytesProduced, bool flush)
	{
		inBuffer.src = input;
		inBuffer.pos = 0;
		inBuffer.size = inputSize;
		outBuffer.dst = output;
		outBuffer.pos = 0;
		outBuffer.size = outputSize;

		size_t zstdError = Strategy::Process(fStream, &inBuffer, &outBuffer, flush);
		if (ZSTD_getErrorCode(zstdError) != ZSTD_error_no_error)
			return _TranslateZstdError(zstdError);

		bytesConsumed = inBuffer.pos;
		bytesProduced = outBuffer.pos;
		return B_OK;
	}

private:
	bool		fStreamInitialized;
	StreamType	*fStream;
	ZSTD_inBuffer inBuffer;
	ZSTD_outBuffer outBuffer;
};


#endif	// ZSTD_ENABLED


// #pragma mark - BZstdCompressionAlgorithm


BZstdCompressionAlgorithm::BZstdCompressionAlgorithm()
	:
	BCompressionAlgorithm()
{
}


BZstdCompressionAlgorithm::~BZstdCompressionAlgorithm()
{
}


status_t
BZstdCompressionAlgorithm::CreateCompressingInputStream(BDataIO* input,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef B_ZSTD_COMPRESSION_SUPPORT
	return Stream<BAbstractInputStream, CompressionStrategy, ZSTD_CStream>::Create(
		input, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZstdCompressionAlgorithm::CreateCompressingOutputStream(BDataIO* output,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef B_ZSTD_COMPRESSION_SUPPORT
	return Stream<BAbstractOutputStream, CompressionStrategy, ZSTD_CStream>::Create(
		output, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZstdCompressionAlgorithm::CreateDecompressingInputStream(BDataIO* input,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef ZSTD_ENABLED
	return Stream<BAbstractInputStream, DecompressionStrategy, ZSTD_DStream>::Create(
		input, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZstdCompressionAlgorithm::CreateDecompressingOutputStream(BDataIO* output,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef ZSTD_ENABLED
	return Stream<BAbstractOutputStream, DecompressionStrategy, ZSTD_DStream>::Create(
		output, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZstdCompressionAlgorithm::CompressBuffer(const void* input,
	size_t inputSize, void* output, size_t outputSize, size_t& _compressedSize,
	const BCompressionParameters* parameters)
{
#ifdef B_ZSTD_COMPRESSION_SUPPORT
	const BZstdCompressionParameters* zstdParameters
		= dynamic_cast<const BZstdCompressionParameters*>(parameters);
	int compressionLevel = zstdParameters != NULL
		? zstdParameters->CompressionLevel()
		: B_ZSTD_COMPRESSION_DEFAULT;

	size_t zstdError = ZSTD_compress(output, outputSize, input,
		inputSize, compressionLevel);
	if (ZSTD_isError(zstdError))
		return _TranslateZstdError(zstdError);

	_compressedSize = zstdError;
	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZstdCompressionAlgorithm::DecompressBuffer(const void* input,
	size_t inputSize, void* output, size_t outputSize,
	size_t& _uncompressedSize, const BDecompressionParameters* parameters)
{
#ifdef ZSTD_ENABLED
	size_t zstdError = ZSTD_decompress(output, outputSize, input,
		inputSize);
	if (ZSTD_isError(zstdError))
		return _TranslateZstdError(zstdError);

	_uncompressedSize = zstdError;
	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


/*static*/ status_t
BZstdCompressionAlgorithm::_TranslateZstdError(size_t error)
{
#ifdef ZSTD_ENABLED
	switch (ZSTD_getErrorCode(error)) {
		case ZSTD_error_no_error:
			return B_OK;
		case ZSTD_error_seekableIO:
			return B_BAD_VALUE;
		case ZSTD_error_corruption_detected:
		case ZSTD_error_checksum_wrong:
			return B_BAD_DATA;
		case ZSTD_error_version_unsupported:
			return B_BAD_VALUE;
		case ZSTD_error_dstSize_tooSmall:
			return B_BUFFER_OVERFLOW;
		default:
			return B_ERROR;
	}
#else
	return B_NOT_SUPPORTED;
#endif
}
