#ifndef FileReadWriteTest_H
#define FileReadWriteTest_H

#include <TestCase.h>
#include <TestSuite.h>

#include <String.h>


class FileReadWriteTest : public CppUnit::TestCase
{

public:
								FileReadWriteTest();
	virtual 					~FileReadWriteTest();

			void				TestReadFileWithMultipleLines();
			void				TestReadFileWithoutAnyNewLines();
			void				TestReadFileWithLinesLongerThanBufferSize();
			void				TestReadFileWithEmptyLines();
			void				TestReadEmptyFile();

	static	void 				AddTests(BTestSuite& suite);

private:
			void 				TestReadGeneric(BString input, BString& output);
};

#endif // FileReadWriteTest_H
