/*
 * Copyright 2017-2023, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include <Json.h>
#include <JsonTextWriter.h>

#include <AutoDeleter.h>
#include <DataIO.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "JsonSamples.h"


using namespace BPrivate;


static const char* kTextData = "abcdefghijklmnopqrstuvwxyz!@#$"
							   "ABCDEFGHIJKLMNOPQRSTUVWXYZ)(*&"
							   "0123456789{}[]|:;'<>,.?/~`_+-="
							   "abcdefghijklmnopqrstuvwxyz!@#$"
							   "ABCDEFGHIJKLMNOPQRSTUVWXYZ)(*&"
							   "0123456789{}[]|:;'<>,.?/~`_+-=";
static const int kTextDataLength = 180;

static const size_t kHighVolumeItemCount = 10000;
static const uint32 kChecksumLimit = 100000;

typedef enum FeedOutState {
	OPEN_ARRAY,
	OPEN_QUOTE, // only used for strings
	ITEM,
	CLOSE_QUOTE, // only used for strings
	SEPARATOR,
	CLOSE_ARRAY,
	END
} FeedOutState;


/*!	This class can be used by a text to accept JSON events and then to maintain a checksum of the
	strings and numbers that it encounters in order to get some sort of a checksum on the data
	that it is seeing. It can then compare this to the data that was emitted in order to verify
	that the JSON parser has parsed and passed-through all of the data correctly.
*/
class ChecksumJsonEventListener : public BJsonEventListener {
public:
	ChecksumJsonEventListener(int32 checksumLimit)
		:
		fChecksum(0),
		fChecksumLimit(checksumLimit),
		fError(B_OK),
		fCompleted(false)
	{
	}
	virtual			~ChecksumJsonEventListener() {}

	status_t		Error() const { return fError; }
	uint32			Checksum() const { return fChecksum; }
	void 			Complete() { fCompleted = true; }

	virtual void HandleError(status_t status, int32 line, const char* message)
	{
		fError = status;
	}

	virtual bool Handle(const BJsonEvent& event)
	{
		if (fCompleted || B_OK != fError)
			return false;

		switch (event.EventType()) {
			case B_JSON_NUMBER:
			{
				const char* content = event.Content();
				_ChecksumProcessCharacters(content, strlen(content));
				break;
			}
			case B_JSON_STRING:
			case B_JSON_OBJECT_NAME:
			{
				const char* content = event.Content();
				_ChecksumProcessCharacters(content, strlen(content));
				break;
			}
			default:
				break;
		}

		return true;
	}

private:
	void _ChecksumProcessCharacters(const char* content, size_t len)
	{
		for (size_t i = 0; i < len; i++)
			fChecksum = (fChecksum + static_cast<int32>(content[i])) % fChecksumLimit;
	}

	uint32		fChecksum;
	uint32		fChecksumLimit;
	status_t	fError;
	bool		fCompleted;
};


class FakeJsonStreamDataIO : public BDataIO {
public:
	FakeJsonStreamDataIO(int count, uint32 checksumLimit)
		:
		fFeedOutState(OPEN_ARRAY),
		fItemCount(count),
		fItemUpto(0),
		fChecksum(0),
		fChecksumLimit(checksumLimit)
	{
	}
	virtual			~FakeJsonStreamDataIO() {}

	uint32			Checksum() const { return fChecksum; }
	virtual ssize_t	Write(const void* buffer, size_t size) { return B_NOT_SUPPORTED; }
	virtual ssize_t	Read(void* buffer, size_t size)
	{
		char* buffer_c = static_cast<char*>(buffer);
		status_t result = B_OK;
		size_t i = 0;

		while (i < size && result == B_OK) {
			result = NextChar(&buffer_c[i]);
			if (result == B_OK)
				i++;
		}

		if (0 != i)
			return i;

		return result;
	}

protected:
	virtual void		FillBuffer() = 0;
	virtual status_t	NextChar(char* c) = 0;

	void _ChecksumProcessCharacter(const char c)
	{
		fChecksum = (fChecksum + static_cast<int32>(c)) % fChecksumLimit;
	}

	FeedOutState		fFeedOutState;
	int					fItemCount;
	int					fItemUpto;

private:
	uint32				fChecksum;
	uint32				fChecksumLimit;
};


class FakeJsonStringStreamDataIO : public FakeJsonStreamDataIO {
public:
	FakeJsonStringStreamDataIO(int count, uint32 checksumLimit)
		:
		FakeJsonStreamDataIO(count, checksumLimit),
		fItemBufferSize(0),
		fItemBufferUpto(0)
	{
		FillBuffer();
	}
	virtual ~FakeJsonStringStreamDataIO() {}

protected:
	virtual void FillBuffer()
	{
		fItemBufferSize = random() % kTextDataLength;
		fItemBufferUpto = 0;
	}

	virtual status_t NextChar(char* c)
	{
		switch (fFeedOutState) {
			case OPEN_ARRAY:
				c[0] = '[';
				fFeedOutState = OPEN_QUOTE;
				return B_OK;
			case OPEN_QUOTE:
				c[0] = '"';
				fFeedOutState = ITEM;
				return B_OK;
			case ITEM:
				c[0] = kTextData[fItemBufferUpto];
				_ChecksumProcessCharacter(kTextData[fItemBufferUpto]);
				fItemBufferUpto++;
				if (fItemBufferUpto >= fItemBufferSize) {
					fFeedOutState = CLOSE_QUOTE;
					FillBuffer();
				}
				return B_OK;
			case CLOSE_QUOTE:
				c[0] = '"';
				fItemUpto++;
				if (fItemUpto >= fItemCount)
					fFeedOutState = CLOSE_ARRAY;
				else
					fFeedOutState = SEPARATOR;
				return B_OK;
			case SEPARATOR:
				c[0] = ',';
				fFeedOutState = OPEN_QUOTE;
				return B_OK;
			case CLOSE_ARRAY:
				c[0] = ']';
				fFeedOutState = END;
				return B_OK;
			default:
				return -1; // end of file
		}
	}

	int	fItemBufferSize;
	int	fItemBufferUpto;
};


class FakeJsonNumberStreamDataIO : public FakeJsonStreamDataIO {
public:
	FakeJsonNumberStreamDataIO(int count, uint32 checksumLimit)
		:
		FakeJsonStreamDataIO(count, checksumLimit),
		fItemBufferSize(0),
		fItemBufferUpto(0)
	{
		memset(fBuffer, 0, 32);
		FillBuffer();
	}
	virtual ~FakeJsonNumberStreamDataIO() {}

protected:
	virtual void FillBuffer()
	{
		int32 value = static_cast<int32>(random());
		fItemBufferSize = snprintf(fBuffer, 32, "%" B_PRIu32, value);
		fItemBufferUpto = 0;
	}

	virtual status_t NextChar(char* c)
	{
		switch (fFeedOutState) {
			case OPEN_ARRAY:
				c[0] = '[';
				fFeedOutState = ITEM;
				return B_OK;
			case ITEM:
				c[0] = fBuffer[fItemBufferUpto];
				_ChecksumProcessCharacter(fBuffer[fItemBufferUpto]);
				fItemBufferUpto++;
				if (fItemBufferUpto >= fItemBufferSize) {
					fItemUpto++;
					if (fItemUpto >= fItemCount)
						fFeedOutState = CLOSE_ARRAY;
					else
						fFeedOutState = SEPARATOR;
					FillBuffer();
				}
				return B_OK;
			case SEPARATOR:
				c[0] = ',';
				fFeedOutState = ITEM;
				return B_OK;
			case CLOSE_ARRAY:
				c[0] = ']';
				fFeedOutState = END;
				return B_OK;
			default:
				return -1; // end of file
		}
	}

	int fItemBufferSize;
	int fItemBufferUpto;
	char fBuffer[32];
};


class JsonEndToEndTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JsonEndToEndTest);
	CPPUNIT_TEST(TestFalseA);
	CPPUNIT_TEST(TestTrueA);
	CPPUNIT_TEST(TestNullA);
	CPPUNIT_TEST(TestNumberA);
	CPPUNIT_TEST(TestStringA);
	CPPUNIT_TEST(TestStringB);
	CPPUNIT_TEST(TestArrayA);
	CPPUNIT_TEST(TestArrayB);
	CPPUNIT_TEST(TestObjectA);
	CPPUNIT_TEST(TestStringUnterminated);
	CPPUNIT_TEST(TestArrayUnterminated);
	CPPUNIT_TEST(TestObjectUnterminated);
	CPPUNIT_TEST(TestHighVolumeStringParsing);
	CPPUNIT_TEST(TestHighVolumeNumberParsing);
	CPPUNIT_TEST(TestHighVolumeStringSampleGenerationOnly);
	CPPUNIT_TEST(TestHighVolumeNumberSampleGenerationOnly);
	CPPUNIT_TEST_SUITE_END();

	void _TestParseAndWrite(const char* input, const char* expectedOutput)
	{
		BMemoryIO inputData(input, strlen(input));
		BMallocIO outputData;
		BPrivate::BJsonTextWriter listener(&outputData);

		BPrivate::BJson::Parse(&inputData, &listener);

		CPPUNIT_ASSERT_EQUAL(B_OK, listener.ErrorStatus());
		CPPUNIT_ASSERT_MESSAGE(
			"expected did not equal actual output",
			0 == strncmp(expectedOutput, (char*)outputData.Buffer(), outputData.BufferLength()));
	}

	/*! This method will test an element being unterminated; such an object that
		is missing the terminating "}" symbol or a string that has no closing
		quote.  This is tested here because the writer
	*/
	void _TestUnterminated(const char *input)
	{
		BDataIO* inputData = new BMemoryIO(input, strlen(input));
		ObjectDeleter<BDataIO> inputDataDeleter(inputData);
		BMallocIO* outputData = new BMallocIO();
		ObjectDeleter<BMallocIO> outputDataDeleter(outputData);
		BPrivate::BJsonTextWriter* listener
			= new BJsonTextWriter(outputData);
		ObjectDeleter<BPrivate::BJsonTextWriter> listenerDeleter(listener);

		BPrivate::BJson::Parse(inputData, listener);

		CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, listener->ErrorStatus());
	}

public:
	void TestFalseA()
	{
		_TestParseAndWrite(JSON_SAMPLE_FALSE_A_IN, JSON_SAMPLE_FALSE_A_EXPECTED_OUT);
	}

	void TestTrueA()
	{
		_TestParseAndWrite(JSON_SAMPLE_TRUE_A_IN, JSON_SAMPLE_TRUE_A_EXPECTED_OUT);
	}

	void TestNullA()
	{
		_TestParseAndWrite(JSON_SAMPLE_NULL_A_IN, JSON_SAMPLE_NULL_A_EXPECTED_OUT);
	}

	void TestNumberA()
	{
		_TestParseAndWrite(JSON_SAMPLE_NUMBER_A_IN, JSON_SAMPLE_NUMBER_A_EXPECTED_OUT);
	}

	void TestStringA()
	{
		_TestParseAndWrite(JSON_SAMPLE_STRING_A_IN, JSON_SAMPLE_STRING_A_EXPECTED_OUT);
	}

	void TestStringA2()
	{
		_TestParseAndWrite(JSON_SAMPLE_STRING_A2_IN, JSON_SAMPLE_STRING_A_EXPECTED_OUT);
	}

	void TestStringB()
	{
		_TestParseAndWrite(JSON_SAMPLE_STRING_B_IN, JSON_SAMPLE_STRING_B_EXPECTED_OUT);
	}

	void TestArrayA()
	{
		_TestParseAndWrite(JSON_SAMPLE_ARRAY_A_IN, JSON_SAMPLE_ARRAY_A_EXPECTED_OUT);
	}

	void TestArrayB()
	{
		_TestParseAndWrite(JSON_SAMPLE_ARRAY_B_IN, JSON_SAMPLE_ARRAY_B_EXPECTED_OUT);
	}

	void TestObjectA()
	{
		_TestParseAndWrite(JSON_SAMPLE_OBJECT_A_IN, JSON_SAMPLE_OBJECT_A_EXPECTED_OUT);
	}

	void TestStringUnterminated()
	{
		_TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_STRING);
	}

	void TestArrayUnterminated()
	{
		_TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_ARRAY);
	}

	void TestObjectUnterminated()
	{
		_TestUnterminated(JSON_SAMPLE_BROKEN_UNTERMINATED_OBJECT);
	}

	void TestHighVolumeStringParsing()
	{
		FakeJsonStreamDataIO* inputData = new FakeJsonStringStreamDataIO(kHighVolumeItemCount,
			kChecksumLimit);
		ChecksumJsonEventListener* listener = new ChecksumJsonEventListener(kChecksumLimit);

		BPrivate::BJson::Parse(inputData, listener);

		CPPUNIT_ASSERT_EQUAL(B_OK, listener->Error());
		CPPUNIT_ASSERT_EQUAL(inputData->Checksum(), listener->Checksum());
	}


	void TestHighVolumeNumberParsing()
	{
		FakeJsonStreamDataIO* inputData = new FakeJsonNumberStreamDataIO(kHighVolumeItemCount,
			kChecksumLimit);
		ChecksumJsonEventListener* listener = new ChecksumJsonEventListener(kChecksumLimit);

		BPrivate::BJson::Parse(inputData, listener);

		CPPUNIT_ASSERT_EQUAL(B_OK, listener->Error());
		CPPUNIT_ASSERT_EQUAL(inputData->Checksum(), listener->Checksum());
	}

	/*! Just here so it is possible to extract timings for the data generation cost.
	*/
	void TestHighVolumeStringSampleGenerationOnly()
	{
		FakeJsonStreamDataIO* inputData = new FakeJsonStringStreamDataIO(kHighVolumeItemCount,
			kChecksumLimit);
		char c;

		while (inputData->Read(&c, 1) == 1) {
			// do nothing
		}
	}

	void TestHighVolumeNumberSampleGenerationOnly()
	{
		FakeJsonStreamDataIO* inputData = new FakeJsonNumberStreamDataIO(kHighVolumeItemCount,
			kChecksumLimit);
		char c;

		while (inputData->Read(&c, 1) == 1) {
			// do nothing
		}
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(JsonEndToEndTest, getTestSuiteName());
