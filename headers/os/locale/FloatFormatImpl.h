#ifndef _B_FLOAT_FORMAT_IMPL_H_
#define _B_FLOAT_FORMAT_IMPL_H_

#include <NumberFormatImpl.h>

struct format_field_position;
class BFloatFormatParameters;
class BString;

class BFloatFormatImpl : public BNumberFormatImpl {
	public:
		BFloatFormatImpl();
		virtual ~BFloatFormatImpl();

		// formatting

		virtual status_t Format(const BFloatFormatParameters *parameters,
								double number, BString *buffer) const = 0;
		virtual status_t Format(const BFloatFormatParameters *parameters,
								double number, BString *buffer,
								format_field_position *positions,
								int32 positionCount = 1,
								int32 *fieldCount = NULL,
								bool allFieldPositions = false) const = 0;

		// TODO: ...


		virtual BNumberFormatParameters *DefaultNumberFormatParameters();
		virtual const BNumberFormatParameters *DefaultNumberFormatParameters()
			const;

		virtual BFloatFormatParameters *DefaultFloatFormatParameters() = 0;
		virtual const BFloatFormatParameters *DefaultFloatFormatParameters()
			const = 0;
};


#endif	// _B_FLOAT_FORMAT_IMPL_H_
