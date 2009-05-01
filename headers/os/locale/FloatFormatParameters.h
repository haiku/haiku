#ifndef _B_FLOAT_FORMAT_PARAMETERS_H_
#define _B_FLOAT_FORMAT_PARAMETERS_H_

#include <NumberFormatParameters.h>

enum float_format_type {
	B_FIXED_POINT_FLOAT_FORMAT,
	B_SCIENTIFIC_FLOAT_FORMAT,
	B_AUTO_FLOAT_FORMAT,			// picks one of the above depending of the
									// number to be formatted
};

class BFloatFormatParameters : public BNumberFormatParameters {
	public:
		BFloatFormatParameters(const BFloatFormatParameters *parent = NULL);
		BFloatFormatParameters(const BFloatFormatParameters &other);
		~BFloatFormatParameters();

		void SetMinimalFractionDigits(size_t minFractionDigits);
		size_t MinimalFractionDigits() const;

		void SetMaximalFractionDigits(size_t maxFractionDigits);
		size_t MaximalFractionDigits() const;

		void SetUseUpperCase(bool useCapitals);
		bool UseUpperCase() const;

		void SetFloatFormatType(float_format_type type);
		float_format_type FloatFormatType() const;

		void SetAlwaysUseFractionSeparator(bool alwaysUseFractionSeparator);
		bool AlwaysUseFractionSeparator() const;

		void SetKeepTrailingFractionZeros(bool keepTrailingFractionZeros);
		bool KeepTrailingFractionZeros() const;

		void SetParentFloatParameters(const BFloatFormatParameters *parent);
		const BFloatFormatParameters *ParentFloatParameters() const;

		BFloatFormatParameters &operator=(
			const BFloatFormatParameters &other);

	private:
		const BFloatFormatParameters	*fParent;
		size_t							fMinimalFractionDigits;
		size_t							fMaximalFractionDigits;
		bool							fUseUpperCase;
		float_format_type				fFloatFormatType;
		bool							fAlwaysUseFractionSeparator;
		bool							fKeepTrailingFractionZeros;
		uint32							fFlags;
};

#endif	// _B_FLOAT_FORMAT_PARAMETERS_H_
