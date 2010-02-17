#ifndef _B_NUMBER_FORMAT_H_
#define _B_NUMBER_FORMAT_H_

#include <Format.h>
#include <NumberFormatParameters.h>

class BNumberFormatImpl;

class BNumberFormat : public BFormat {
	protected:
		BNumberFormat(const BNumberFormat &other);
		~BNumberFormat();

		BNumberFormat &operator=(const BNumberFormat &other);

		BNumberFormat();
};


#endif	// _B_NUMBER_FORMAT_H_
