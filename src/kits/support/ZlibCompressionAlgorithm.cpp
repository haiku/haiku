/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ZlibCompressionAlgorithm.h>

#include <errno.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <zlib.h>

#include <DataIO.h>


// build compression support only for userland
#if !defined(_KERNEL_MODE) && !defined(_BOOT_MODE)
#	define B_ZLIB_COMPRESSION_SUPPORT 1
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


// #pragma mark - BZlibCompressionParameters


BZlibCompressionParameters::BZlibCompressionParameters(
	int compressionLevel)
	:
	BCompressionParameters(),
	fCompressionLevel(compressionLevel),
	fBufferSize(kDefaultBufferSize),
	fGzipFormat(false)
{
}


BZlibCompressionParameters::~BZlibCompressionParameters()
{
}


int32
BZlibCompressionParameters::CompressionLevel() const
{
	return fCompressionLevel;
}


void
BZlibCompressionParameters::SetCompressionLevel(int32 level)
{
	fCompressionLevel = level;
}


size_t
BZlibCompressionParameters::BufferSize() const
{
	return fBufferSize;
}


void
BZlibCompressionParameters::SetBufferSize(size_t size)
{
	fBufferSize = sanitize_buffer_size(size);
}


bool
BZlibCompressionParameters::IsGzipFormat() const
{
	return fGzipFormat;
}


void
BZlibCompressionParameters::SetGzipFormat(bool gzipFormat)
{
	fGzipFormat = gzipFormat;
}


// #pragma mark - BZlibDecompressionParameters


BZlibDecompressionParameters::BZlibDecompressionParameters()
	:
	BDecompressionParameters(),
	fBufferSize(kDefaultBufferSize)
{
}


BZlibDecompressionParameters::~BZlibDecompressionParameters()
{
}


size_t
BZlibDecompressionParameters::BufferSize() const
{
	return fBufferSize;
}


void
BZlibDecompressionParameters::SetBufferSize(size_t size)
{
	fBufferSize = sanitize_buffer_size(size);
}


// #pragma mark - CompressionStrategy


#ifdef B_ZLIB_COMPRESSION_SUPPORT


struct BZlibCompressionAlgorithm::CompressionStrategy {
	typedef BZlibCompressionParameters Parameters;

	static const bool kNeedsFinalFlush = true;

	static int Init(z_stream& stream,
		const BZlibCompressionParameters* parameters)
	{
		int32 compressionLevel = B_ZLIB_COMPRESSION_DEFAULT;
		bool gzipFormat = false;
		if (parameters != NULL) {
			compressionLevel = parameters->CompressionLevel();
			gzipFormat = parameters->IsGzipFormat();
		}

		return deflateInit2(&stream, compressionLevel,
			Z_DEFLATED,
			MAX_WBITS + (gzipFormat ? 16 : 0),
			MAX_MEM_LEVEL,
			Z_DEFAULT_STRATEGY);
	}

	static void Uninit(z_stream& stream)
	{
		deflateEnd(&stream);
	}

	static int Process(z_stream& stream, bool flush)
	{
		return deflate(&stream, flush ? Z_FINISH : 0);
	}
};


#endif	// B_ZLIB_COMPRESSION_SUPPORT


// #pragma mark - DecompressionStrategy


struct BZlibCompressionAlgorithm::DecompressionStrategy {
	typedef BZlibDecompressionParameters Parameters;

	static const bool kNeedsFinalFlush = false;

	static int Init(z_stream& stream,
		const BZlibDecompressionParameters* /*parameters*/)
	{
		// auto-detect zlib/gzip header
		return inflateInit2(&stream, 32 + MAX_WBITS);
	}

	static void Uninit(z_stream& stream)
	{
		inflateEnd(&stream);
	}

	static int Process(z_stream& stream, bool flush)
	{
		return inflate(&stream, flush ? Z_FINISH : 0);
	}
};


// #pragma mark - Stream


template<typename BaseClass, typename Strategy>
struct BZlibCompressionAlgorithm::Stream : BaseClass {
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

		memset(&fStream, 0, sizeof(fStream));

		int zlibError = Strategy::Init(fStream, parameters);
		if (zlibError != Z_OK)
			return _TranslateZlibError(zlibError);

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
		fStream.next_in = (Bytef*)input;
		fStream.avail_in = inputSize;
		fStream.next_out = (Bytef*)output;
		fStream.avail_out = outputSize;

		int zlibError = Strategy::Process(fStream, flush);
		if (zlibError != Z_OK) {
			if (zlibError == Z_STREAM_END) {
				if (fStream.avail_in != 0)
					return B_BAD_DATA;
			} else
				return _TranslateZlibError(zlibError);
		}

		bytesConsumed = inputSize - (size_t)fStream.avail_in;
		bytesProduced = outputSize - (size_t)fStream.avail_out;
		return B_OK;
	}

private:
	z_stream	fStream;
	bool		fStreamInitialized;
};


// #pragma mark - BZlibCompressionAlgorithm


BZlibCompressionAlgorithm::BZlibCompressionAlgorithm()
	:
	BCompressionAlgorithm()
{
}


BZlibCompressionAlgorithm::~BZlibCompressionAlgorithm()
{
}


status_t
BZlibCompressionAlgorithm::CreateCompressingInputStream(BDataIO* input,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef B_ZLIB_COMPRESSION_SUPPORT
	return Stream<BAbstractInputStream, CompressionStrategy>::Create(
		input, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZlibCompressionAlgorithm::CreateCompressingOutputStream(BDataIO* output,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
#ifdef B_ZLIB_COMPRESSION_SUPPORT
	return Stream<BAbstractOutputStream, CompressionStrategy>::Create(
		output, parameters, _stream);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZlibCompressionAlgorithm::CreateDecompressingInputStream(BDataIO* input,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
	return Stream<BAbstractInputStream, DecompressionStrategy>::Create(
		input, parameters, _stream);
}


status_t
BZlibCompressionAlgorithm::CreateDecompressingOutputStream(BDataIO* output,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
	return Stream<BAbstractOutputStream, DecompressionStrategy>::Create(
		output, parameters, _stream);
}


status_t
BZlibCompressionAlgorithm::CompressBuffer(const void* input,
	size_t inputSize, void* output, size_t outputSize, size_t& _compressedSize,
	const BCompressionParameters* parameters)
{
#ifdef B_ZLIB_COMPRESSION_SUPPORT
	const BZlibCompressionParameters* zlibParameters
		= dynamic_cast<const BZlibCompressionParameters*>(parameters);
	int compressionLevel = zlibParameters != NULL
		? zlibParameters->CompressionLevel()
		: B_ZLIB_COMPRESSION_DEFAULT;

	uLongf bytesUsed = outputSize;
	int zlibError = compress2((Bytef*)output, &bytesUsed, (const Bytef*)input,
		(uLong)inputSize, compressionLevel);
	if (zlibError != Z_OK)
		return _TranslateZlibError(zlibError);

	_compressedSize = (size_t)bytesUsed;
	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BZlibCompressionAlgorithm::DecompressBuffer(const void* input,
	size_t inputSize, void* output, size_t outputSize,
	size_t& _uncompressedSize, const BDecompressionParameters* parameters)
{
	uLongf bytesUsed = outputSize;
	int zlibError = uncompress((Bytef*)output, &bytesUsed, (const Bytef*)input,
		(uLong)inputSize);
	if (zlibError != Z_OK)
		return _TranslateZlibError(zlibError);

	_uncompressedSize = (size_t)bytesUsed;
	return B_OK;
}


/*static*/ status_t
BZlibCompressionAlgorithm::_TranslateZlibError(int error)
{
	switch (error) {
		case Z_OK:
			return B_OK;
		case Z_STREAM_END:
		case Z_NEED_DICT:
			// a special event (no error), but the caller doesn't seem to handle
			// it
			return B_ERROR;
		case Z_ERRNO:
			return errno;
		case Z_STREAM_ERROR:
			return B_BAD_VALUE;
		case Z_DATA_ERROR:
			return B_BAD_DATA;
		case Z_MEM_ERROR:
			return B_NO_MEMORY;
		case Z_BUF_ERROR:
			return B_BUFFER_OVERFLOW;
		case Z_VERSION_ERROR:
			return B_BAD_VALUE;
		default:
			return B_ERROR;
	}
}
