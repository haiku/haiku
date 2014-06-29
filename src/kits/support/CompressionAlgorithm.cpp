/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <CompressionAlgorithm.h>

#include <stdlib.h>
#include <string.h>

#include <Errors.h>


// #pragma mark - BCompressionParameters


BCompressionParameters::BCompressionParameters()
{
}


BCompressionParameters::~BCompressionParameters()
{
}


// #pragma mark - BDecompressionParameters


BDecompressionParameters::BDecompressionParameters()
{
}


BDecompressionParameters::~BDecompressionParameters()
{
}


// #pragma mark - BCompressionAlgorithm


BCompressionAlgorithm::BCompressionAlgorithm()
{
}


BCompressionAlgorithm::~BCompressionAlgorithm()
{
}


status_t
BCompressionAlgorithm::CreateCompressingInputStream(BDataIO* input,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
	return B_NOT_SUPPORTED;
}


status_t
BCompressionAlgorithm::CreateCompressingOutputStream(BDataIO* output,
	const BCompressionParameters* parameters, BDataIO*& _stream)
{
	return B_NOT_SUPPORTED;
}


status_t
BCompressionAlgorithm::CreateDecompressingInputStream(BDataIO* input,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
	return B_NOT_SUPPORTED;
}


status_t
BCompressionAlgorithm::CreateDecompressingOutputStream(BDataIO* output,
	const BDecompressionParameters* parameters, BDataIO*& _stream)
{
	return B_NOT_SUPPORTED;
}


status_t
BCompressionAlgorithm::CompressBuffer(const void* input, size_t inputSize,
	void* output, size_t outputSize, size_t& _compressedSize,
	const BCompressionParameters* parameters)
{
	return B_NOT_SUPPORTED;
}


status_t
BCompressionAlgorithm::DecompressBuffer(const void* input,
	size_t inputSize, void* output, size_t outputSize,
	size_t& _uncompressedSize, const BDecompressionParameters* parameters)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark - BAbstractStream


BCompressionAlgorithm::BAbstractStream::BAbstractStream()
	:
	BDataIO(),
	fBuffer(NULL),
	fBufferCapacity(0),
	fBufferOffset(0),
	fBufferSize(0)
{
}


BCompressionAlgorithm::BAbstractStream::~BAbstractStream()
{
	free(fBuffer);
}


status_t
BCompressionAlgorithm::BAbstractStream::Init(size_t bufferSize)
{
	fBuffer = (uint8*)malloc(bufferSize);
	fBufferCapacity = bufferSize;

	return fBuffer != NULL ? B_OK : B_NO_MEMORY;
}


// #pragma mark - BAbstractInputStream


BCompressionAlgorithm::BAbstractInputStream::BAbstractInputStream(
		BDataIO* input)
	:
	BAbstractStream(),
	fInput(input),
	fEndOfInput(false),
	fNoMorePendingData(false)
{
}


BCompressionAlgorithm::BAbstractInputStream::~BAbstractInputStream()
{
}


ssize_t
BCompressionAlgorithm::BAbstractInputStream::Read(void* buffer, size_t size)
{
	if (size == 0)
		return 0;

	size_t bytesRemaining = size;
	uint8* output = (uint8*)buffer;

	while (bytesRemaining > 0) {
		// process the data still in the input buffer
		if (fBufferSize > 0) {
			size_t bytesConsumed;
			size_t bytesProduced;
			status_t error = ProcessData(fBuffer + fBufferOffset, fBufferSize,
				output, bytesRemaining, bytesConsumed, bytesProduced);
			if (error != B_OK)
				return error;

			fBufferOffset += bytesConsumed;
			fBufferSize -= bytesConsumed;
			output += bytesProduced;
			bytesRemaining -= bytesProduced;
			continue;
		}

		// We couldn't process anything, because we don't have any or not enough
		// bytes in the input buffer.

		if (fEndOfInput)
			break;

		// Move any remaining data to the start of the buffer.
		if (fBufferSize > 0) {
			if (fBufferSize == fBufferCapacity)
				return B_ERROR;

			if (fBufferOffset > 0)
				memmove(fBuffer, fBuffer + fBufferOffset, fBufferSize);
		}

		fBufferOffset = 0;

		// read from the source
		ssize_t bytesRead = fInput->Read(fBuffer + fBufferSize,
			fBufferCapacity - fBufferSize);
		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead == 0) {
			fEndOfInput = true;
			break;
		}

		fBufferSize += bytesRead;
	}

	// If we've reached the end of the input and still have room in the output
	// buffer, we have consumed all input data and want to flush all pending
	// data, now.
	if (fEndOfInput && bytesRemaining > 0 && !fNoMorePendingData) {
		size_t bytesProduced;
		status_t error = FlushPendingData(output, bytesRemaining,
			bytesProduced);
		if (error != B_OK)
			return error;

		if (bytesProduced < bytesRemaining)
			fNoMorePendingData = true;

		output += bytesProduced;
		bytesRemaining -= bytesProduced;
	}

	return size - bytesRemaining;
}


// #pragma mark - BAbstractOutputStream


BCompressionAlgorithm::BAbstractOutputStream::BAbstractOutputStream(
		BDataIO* output)
	:
	BAbstractStream(),
	fOutput(output)
{
}


BCompressionAlgorithm::BAbstractOutputStream::~BAbstractOutputStream()
{
}


ssize_t
BCompressionAlgorithm::BAbstractOutputStream::Write(const void* buffer,
	size_t size)
{
	if (size == 0)
		return 0;

	size_t bytesRemaining = size;
	uint8* input = (uint8*)buffer;

	while (bytesRemaining > 0) {
		// try to process more data
		if (fBufferSize < fBufferCapacity) {
			size_t bytesConsumed;
			size_t bytesProduced;
			status_t error = ProcessData(input, bytesRemaining,
				fBuffer + fBufferSize, fBufferCapacity - fBufferSize,
				bytesConsumed, bytesProduced);
			if (error != B_OK)
				return error;

			input += bytesConsumed;
			bytesRemaining -= bytesConsumed;
			fBufferSize += bytesProduced;
			continue;
		}

		// We couldn't process anything, because we don't have any or not enough
		// room in the output buffer.

		if (fBufferSize == 0)
			return B_ERROR;

		// write to the target
		ssize_t bytesWritten = fOutput->Write(fBuffer, fBufferSize);
		if (bytesWritten < 0)
			return bytesWritten;
		if (bytesWritten == 0)
			break;

		// Move any remaining data to the start of the buffer.
		fBufferSize -= bytesWritten;
		if (fBufferSize > 0)
			memmove(fBuffer, fBuffer + bytesWritten, fBufferSize);
	}

	return size - bytesRemaining;
}


status_t
BCompressionAlgorithm::BAbstractOutputStream::Flush()
{
	bool noMorePendingData = false;

	for (;;) {
		// let the derived class flush all pending data
		if (fBufferSize < fBufferCapacity && !noMorePendingData) {
			size_t bytesProduced;
			status_t error = FlushPendingData(fBuffer + fBufferSize,
				fBufferCapacity - fBufferSize, bytesProduced);
			if (error != B_OK)
				return error;

			noMorePendingData = bytesProduced < fBufferCapacity - fBufferSize;

			fBufferSize += bytesProduced;
		}

		// write buffered data to output
		if (fBufferSize == 0)
			break;

		status_t error = fOutput->WriteExactly(fBuffer, fBufferSize);
		if (error != B_OK)
			return error;

		fBufferSize = 0;
	}

	return fOutput->Flush();
}
