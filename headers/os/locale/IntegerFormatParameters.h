#ifndef _B_INTEGER_FORMAT_PARAMETERS_H_
#define _B_INTEGER_FORMAT_PARAMETERS_H_

#include <NumberFormatParameters.h>

class BIntegerFormatParameters : public BNumberFormatParameters {
	public:
		BIntegerFormatParameters(const BIntegerFormatParameters *parent = NULL);
		BIntegerFormatParameters(const BIntegerFormatParameters &other);
		~BIntegerFormatParameters();

		void SetParentIntegerParameters(const BIntegerFormatParameters *parent);
		const BIntegerFormatParameters *ParentIntegerParameters() const;

		BIntegerFormatParameters &operator=(
			const BIntegerFormatParameters &other);

	private:
		const BIntegerFormatParameters	*fParent;
};

#endif	// _B_INTEGER_FORMAT_PARAMETERS_H_
