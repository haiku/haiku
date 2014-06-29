/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COMPRESSION_ALGORITHM_H_
#define _COMPRESSION_ALGORITHM_H_


#include <DataIO.h>


class BCompressionParameters {
public:
								BCompressionParameters();
	virtual						~BCompressionParameters();
};


class BDecompressionParameters {
public:
								BDecompressionParameters();
	virtual						~BDecompressionParameters();
};


class BCompressionAlgorithm {
public:
								BCompressionAlgorithm();
	virtual						~BCompressionAlgorithm();

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

protected:
			class BAbstractStream;
			class BAbstractInputStream;
			class BAbstractOutputStream;
};


class BCompressionAlgorithm::BAbstractStream : public BDataIO {
public:
								BAbstractStream();
	virtual						~BAbstractStream();

			status_t			Init(size_t bufferSize);

protected:
	virtual	status_t			ProcessData(const void* input, size_t inputSize,
									void* output, size_t outputSize,
									size_t& bytesConsumed,
									size_t& bytesProduced) = 0;
									// must consume or produce at least 1 byte
									// or return an error
	virtual	status_t			FlushPendingData(void* output,
									size_t outputSize,
									size_t& bytesProduced) = 0;

protected:
			uint8*				fBuffer;
			size_t				fBufferCapacity;
			size_t				fBufferOffset;
			size_t				fBufferSize;
};


class BCompressionAlgorithm::BAbstractInputStream : public BAbstractStream {
public:
								BAbstractInputStream(BDataIO* input);
	virtual						~BAbstractInputStream();

	virtual	ssize_t				Read(void* buffer, size_t size);

private:
			BDataIO*			fInput;
			bool				fEndOfInput;
			bool				fNoMorePendingData;
};


class BCompressionAlgorithm::BAbstractOutputStream : public BAbstractStream {
public:
								BAbstractOutputStream(BDataIO* output);
	virtual						~BAbstractOutputStream();

	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	status_t			Flush();

private:
			ssize_t				_Write(const void* buffer, size_t size,
									bool flush);

private:
			BDataIO*			fOutput;
};


#endif	// _COMPRESSION_ALGORITHM_H_
