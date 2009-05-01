#ifndef _B_FORMAT_IMPL_H_
#define _B_FORMAT_IMPL_H_

#include <SupportDefs.h>

#include <LocaleBuild.h>

class BFormatParameters;

class _IMPEXP_LOCALE BFormatImpl {
	public:
		BFormatImpl();
		virtual ~BFormatImpl();

		virtual BFormatParameters *DefaultFormatParameters() = 0;
		virtual const BFormatParameters *DefaultFormatParameters()
			const = 0;
};

#endif	// _B_FORMAT_IMPL_H_
