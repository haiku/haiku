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

#ifndef __NumericValControl_H__
#define __NumericValControl_H__

#include "ValControl.h"

class BContinuousParameter;
#include <MediaDefs.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ValControlDigitSegment;

class NumericValControl : /*extends*/ public ValControl {
	typedef ValControl _inherited;

public:													// ctor/dtor/accessors

	// +++++ need control over number of segments +++++

	// parameter-linked ctor
	NumericValControl(
		BRect												frame,
		const char*									name,
		BContinuousParameter*				param,
		uint16											wholeDigits,
		uint16											fractionalDigits=0,
		align_mode									alignMode=ALIGN_FLUSH_RIGHT,
		align_flags									alignFlags=ALIGN_NONE);
	
	// 'plain' ctor
	NumericValControl(
		BRect												frame,
		const char*									name,
		BMessage*										message,
		uint16											wholeDigits,
		uint16											fractionalDigits=0,
		bool												negativeVisible=true,
		align_mode									alignMode=ALIGN_FLUSH_RIGHT,
		align_flags									alignFlags=ALIGN_NONE);
		
	~NumericValControl();
	
	// bound parameter, or 0 if none
	BContinuousParameter* param() const;

	// value constraints (by default, the min/max allowed by the ctor
	// settings: wholeDigits, fractionalDigits, and negativeVisible)
	
	void getConstraints(
		double*											outMinValue,
		double*											outMaxValue);

	// +++++ the current value is not yet re-constrained by this call
	
	status_t setConstraints(
		double											minValue,
		double											maxValue);

	// fetches the current value (calculated on the spot from each
	// segment.) 
	double value() const;
	
	// set the displayed value (and, if setParam is true, the
	// linked parameter.)  The value will be constrained if necessary.
	void setValue(
		double											value,
		bool												setParam=false);
	
public:													// segment interface
// 18sep99: old segment interface
//	virtual void offsetValue(double dfDelta);

	// 18sep99: new segment interface.  'offset' is given
	// in the segment's units.
	
	virtual void offsetSegmentValue(
		ValControlDigitSegment*			segment,
		int64												offset);
	
public:													// hooks
	virtual void mediaParameterChanged();

	// writes the stored value to the bound parameter
	virtual void updateParameter(
		double											value);

	virtual void initSegments();
	virtual void initConstraintsFromParam();

public:													// ValControl impl.
	void setValue(
		const void*									data,
		size_t											size); //nyi

	void getValue(
		void*												data,
		size_t*											ioSize); //nyi

	// * string value access

	virtual status_t setValueFrom(
		const char*									text);
	virtual status_t getString(
		BString&										buffer);


public:													// BHandler impl.
	virtual void MessageReceived(
		BMessage*										message);

protected:											// internal operations
	virtual void setDefaultConstraints(
		bool												negativeVisible);

private:												// guts

	// calculates the current value as an int64
	int64 _value_fixed() const; //nyi

	// sets the value of each segment based on an int64 value;
	// does not constrain the value
	void _set_value_fixed(
		int64												fixed); //nyi
	
protected:											// members

//	double								m_dfValue; removed 18sep99

	uint16												m_wholeDigits;
	uint16												m_fractionalDigits;
	
	// constraints
	int64													m_minFixed;
	int64													m_maxFixed;
	
	// bound parameter
	BContinuousParameter*					m_param;
};

__END_CORTEX_NAMESPACE
#endif /* __NumericValControl_H__ */
