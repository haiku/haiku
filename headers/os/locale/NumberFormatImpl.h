#ifndef _B_NUMBER_FORMAT_IMPL_H_
#define _B_NUMBER_FORMAT_IMPL_H_

#include <FormatImpl.h>

struct format_field_position;
class BNumberFormatParameters;

class BNumberFormatImpl : public BFormatImpl {
	public:
		BNumberFormatImpl();
		virtual ~BNumberFormatImpl();

		virtual BFormatParameters *DefaultFormatParameters();
		virtual const BFormatParameters *DefaultFormatParameters() const;

		virtual BNumberFormatParameters *DefaultNumberFormatParameters() = 0;
		virtual const BNumberFormatParameters *DefaultNumberFormatParameters()
			const = 0;
};

#endif	// _B_NUMBER_FORMAT_IMPL_H_
