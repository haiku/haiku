#ifndef _B_FLOAT_FORMAT_H_
#define _B_FLOAT_FORMAT_H_

#include <NumberFormat.h>
#include <FloatFormatParameters.h>

class BString;
class BFloatFormatImpl;

class BFloatFormat : public BNumberFormat, public BFloatFormatParameters {
	public:
		BFloatFormat(const BFloatFormat &other);
		~BFloatFormat();

		// formatting

		// no-frills version: Simply appends the formatted number to the
		// string buffer. Can fail only with B_NO_MEMORY or B_BAD_VALUE.
		status_t Format(double number, BString *buffer) const;

		// Appends the formatted number to the string buffer. Additionally
		// one can get the positions of certain fields in the formatted
		// number by supplying format_field_position structures with the
		// field_type set respectively. Passing true for allFieldPositions
		// will make the method fill in a format_field_position structure for
		// each field it writes -- the field_type values will be ignored and
		// overwritten.
		// In fieldCount, in case it is non-null, the number of fields
		// written is returned.
		// B_BUFFER_OVERFLOW is returned, if allFieldPositions is true and
		// the positions buffer is too small (fieldCount will be set
		// nevertheless, so that the caller can adjust the buffer size to
		// make them all fit).
		status_t Format(double number, BString *buffer,
						format_field_position *positions,
						int32 positionCount = 1,
						int32 *fieldCount = NULL,
						bool allFieldPositions = false) const;

		// TODO: Format() versions for (char* buffer, size_t bufferSize)
		// instead of BString*. And, of course, versions for the other
		// number types (float).
 
		// parsing
		// TODO: ...

		BFloatFormat &operator=(const BFloatFormat &other);

		BFloatFormat(BFloatFormatImpl *impl);		// conceptually private
};


#endif	// _B_FLOAT_FORMAT_H_
