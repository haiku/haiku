/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FAKE_JSON_DATA_GENERATOR_H
#define FAKE_JSON_DATA_GENERATOR_H


#include <DataIO.h>


typedef enum FeedOutState {
	OPEN_ARRAY,
	OPEN_QUOTE, // only used for strings
	ITEM,
	CLOSE_QUOTE, // only used for strings
	SEPARATOR,
	CLOSE_ARRAY,
	END
} FeedOutState;


class FakeJsonStreamDataIO : public BDataIO {
public:
								FakeJsonStreamDataIO(int count, uint32 checksumLimit);
	virtual						~FakeJsonStreamDataIO();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	status_t			Flush();

			uint32				Checksum() const;

protected:
	virtual	void				FillBuffer() = 0;
	virtual	status_t			NextChar(char* c) = 0;

			void				_ChecksumProcessCharacter(const char c);

protected:
			FeedOutState		fFeedOutState;
			int					fItemCount;
			int					fItemUpto;

private:
			uint32				fChecksum;
			uint32				fChecksumLimit;
};


class FakeJsonStringStreamDataIO : public FakeJsonStreamDataIO {
public:
								FakeJsonStringStreamDataIO(int count, uint32 checksumLimit);
	virtual						~FakeJsonStringStreamDataIO();

protected:
	virtual	void				FillBuffer();
			status_t			NextChar(char* c);

protected:
			int					fItemBufferSize;
			int					fItemBufferUpto;
};


class FakeJsonNumberStreamDataIO : public FakeJsonStreamDataIO {
public:
								FakeJsonNumberStreamDataIO(int count, uint32 checksumLimit);
	virtual						~FakeJsonNumberStreamDataIO();

protected:
	virtual	void				FillBuffer();
			status_t			NextChar(char* c);

protected:
			int					fItemBufferSize;
			int					fItemBufferUpto;
			char				fBuffer[32];
};


#endif // FAKE_JSON_DATA_GENERATOR_H
