// NumericValControl.h
// * PURPOSE
//   Extends ValControl to provide the basis for a variety
//   of numeric UI controls.
// * HISTORY
//   e.moon			18sep99		General cleanup; values are now stored
//												in fixed-point form only.
//   e.moon			17jan99		Begun
// +++++ 31jan99: negatives are still handled wrong...
//
// CLASS: NumericValControl
#ifndef NUMERIC_VAL_CONTROL_H
#define NUMERIC_VAL_CONTROL_H

#include "cortex_defs.h"
#include "ValControl.h"

#include <MediaDefs.h>


class BContinuousParameter;


__BEGIN_CORTEX_NAMESPACE

class ValControlDigitSegment;


class NumericValControl : public ValControl {
	public:
		typedef ValControl _inherited;

	public:

		// +++++ need control over number of segments +++++

		// parameter-linked ctor
		NumericValControl(BRect	frame, const char* name, BContinuousParameter* param,
			uint16 wholeDigits, uint16 fractionalDigits = 0, align_mode alignMode = ALIGN_FLUSH_RIGHT,
			align_flags alignFlags = ALIGN_NONE);
	
		// 'plain' ctor
		NumericValControl(BRect frame, const char* name, BMessage* message, uint16 wholeDigits,
			uint16 fractionalDigits = 0, bool negativeVisible = true,
			align_mode alignMode = ALIGN_FLUSH_RIGHT, align_flags alignFlags = ALIGN_NONE);

		~NumericValControl();

		BContinuousParameter* param() const;
			// bound parameter, or 0 if none

		void getConstraints(double* outMinValue, double* outMaxValue);
			// value constraints (by default, the min/max allowed by the ctor
			// settings: wholeDigits, fractionalDigits, and negativeVisible)

		status_t setConstraints(double minValue, double maxValue);
			// the current value is not yet re-constrained by this call

		double value() const;
			// fetches the current value (calculated on the spot from each
			// segment.) 

		void setValue(double value, bool setParam = false);
			// set the displayed value (and, if setParam is true, the
			// linked parameter.)  The value will be constrained if necessary.

	public: // segment interface
		// 18sep99: old segment interface
		//	virtual void offsetValue(double dfDelta);

		// 18sep99: new segment interface.  'offset' is given
		// in the segment's units.
	
		virtual void offsetSegmentValue(ValControlDigitSegment* segment, int64 offset);

	public:
		virtual void mediaParameterChanged();

		virtual void updateParameter(double value);
			// writes the stored value to the bound parameter

		virtual void initSegments();
		virtual void initConstraintsFromParam();

	public: // ValControl impl.
		void setValue(const void* data, size_t size); //nyi

		void getValue(void* data, size_t* ioSize); //nyi

		// string value access
		virtual status_t setValueFrom(const char* text);
		virtual status_t getString(BString& buffer);

	public:
		virtual void MessageReceived(BMessage* message);

	protected: // internal operations
		virtual void _SetDefaultConstraints(bool negativeVisible);

	private:
		int64 _ValueFixed() const; //nyi
			// calculates the current value as an int64

		void _SetValueFixed(int64 fixed); //nyi
			// sets the value of each segment based on an int64 value;
			// does not constrain the value
	
	protected:
		//	double								m_dfValue; removed 18sep99

		BContinuousParameter*	fParam;
			// bound parameter

		uint16					fWholeDigits;
		uint16					fFractionalDigits;

		// constraints
		int64					fMinFixed;
		int64					fMaxFixed;
	

};

__END_CORTEX_NAMESPACE
#endif /* NUMERIC_VAL_CONTROL_H */
