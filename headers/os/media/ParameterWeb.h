/*******************************************************************************
/
/	File:			ParameterWeb.h
/
/   Description:  A BParameterWeb is a description of media controls within a BControllable
/	Media Kit Node.
/	BParameter, BParameterGroup, BContinuousParameter, BDiscreteParameter and 
/	BNullParameter are "data classes" used to describe the relation within a 
/	BParameterWeb. These are NOT direct visible classes like BControls; just data 
/	containers which applications can use to decide what kind of views to create.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_CONTROL_WEB_H)
#define _CONTROL_WEB_H

#include <MediaDefs.h>
#include <Flattenable.h>
#include <MediaNode.h>

#if !defined(_PR3_COMPATIBLE_)
enum {
    B_MEDIA_PARAMETER_TYPE      = 'BMCT',
    B_MEDIA_PARAMETER_WEB_TYPE  = 'BMCW',
    B_MEDIA_PARAMETER_GROUP_TYPE= 'BMCG'
};
#endif


//	It is highly unfortunate that a linker bug forces these symbols out
//	from the BParameter class. Hope they don't collide with anything else.

/* These are control KINDs */
	/* kind used when you don't know or care */
extern _IMPEXP_MEDIA const char * const B_GENERIC;
	/* kinds used for sliders */
extern _IMPEXP_MEDIA const char * const B_MASTER_GAIN;	/* Main Volume */
extern _IMPEXP_MEDIA const char * const B_GAIN;
extern _IMPEXP_MEDIA const char * const B_BALANCE;
extern _IMPEXP_MEDIA const char * const B_FREQUENCY;	/* like a radio tuner */
extern _IMPEXP_MEDIA const char * const B_LEVEL;		/* like for effects */
extern _IMPEXP_MEDIA const char * const B_SHUTTLE_SPEED;	/* Play, SloMo, Scan 1.0 == regular */
extern _IMPEXP_MEDIA const char * const B_CROSSFADE;		/* 0 == first input, +100 == second input */
extern _IMPEXP_MEDIA const char * const B_EQUALIZATION;		/* depth (dB) */

	/* kinds used for compressors */
extern _IMPEXP_MEDIA const char * const B_COMPRESSION;	/* 0% == no compression, 99% == 100:1 compression */
extern _IMPEXP_MEDIA const char * const B_QUALITY;		/* 0% == full compression, 100% == no compression */
extern _IMPEXP_MEDIA const char * const B_BITRATE;		/* in bits/second */
extern _IMPEXP_MEDIA const char * const B_GOP_SIZE;			/* Group Of Pictures. a k a "Keyframe every N frames" */
	/* kinds used for selectors */
extern _IMPEXP_MEDIA const char * const B_MUTE;		/* 0 == thru, 1 == mute */
extern _IMPEXP_MEDIA const char * const B_ENABLE;		/* 0 == disable, 1 == enable */
extern _IMPEXP_MEDIA const char * const B_INPUT_MUX;	/* "value" 1-N == input selected */
extern _IMPEXP_MEDIA const char * const B_OUTPUT_MUX;	/* "value" 1-N == output selected */
extern _IMPEXP_MEDIA const char * const B_TUNER_CHANNEL;		/* like cable TV */
extern _IMPEXP_MEDIA const char * const B_TRACK;		/* like a CD player; "value" should be 1-N */
extern _IMPEXP_MEDIA const char * const B_RECSTATE;	/* like mutitrack tape deck, 0 == silent, 1 == play, 2 == record */
extern _IMPEXP_MEDIA const char * const B_SHUTTLE_MODE;	/* -1 == backwards, 0 == stop, 1 == play, 2 == pause/cue */
extern _IMPEXP_MEDIA const char * const B_RESOLUTION;
extern _IMPEXP_MEDIA const char * const B_COLOR_SPACE;		/* "value" should be color_space */
extern _IMPEXP_MEDIA const char * const B_FRAME_RATE;
extern _IMPEXP_MEDIA const char * const B_VIDEO_FORMAT;	/* 1 == NTSC-M, 2 == NTSC-J, 3 == PAL-BDGHI, 4 == PAL-M, 5 == PAL-N, 6 == SECAM, 7 == MPEG-1, 8 == MPEG-2 */
	/* kinds used for junctions */
	//	the prefix of "WEB" is to avoid collission with an enum in Defs.h
extern _IMPEXP_MEDIA const char * const B_WEB_PHYSICAL_INPUT;		/* a jack on the back of the card */
extern _IMPEXP_MEDIA const char * const B_WEB_PHYSICAL_OUTPUT;
extern _IMPEXP_MEDIA const char * const B_WEB_ADC_CONVERTER;		/* from analog to digital signals */
extern _IMPEXP_MEDIA const char * const B_WEB_DAC_CONVERTER;		/* from digital to analog signals */
extern _IMPEXP_MEDIA const char * const B_WEB_LOGICAL_INPUT;		/* an "input" that may not be physical */
extern _IMPEXP_MEDIA const char * const B_WEB_LOGICAL_OUTPUT;
extern _IMPEXP_MEDIA const char * const B_WEB_LOGICAL_BUS;			/* a logical connection point that is neither input nor output; auxilliary bus */
extern _IMPEXP_MEDIA const char * const B_WEB_BUFFER_INPUT;		/* an input that corresponds to a media_input */
extern _IMPEXP_MEDIA const char * const B_WEB_BUFFER_OUTPUT;

	// a simple transport control is a discrete parameter with five values (states):
	// rewinding, stopped, playing, paused, and fast-forwarding
extern _IMPEXP_MEDIA const char * const B_SIMPLE_TRANSPORT;

class BList;
class BParameterGroup;
class BParameter;
class BNullParameter;
class BContinuousParameter;
class BDiscreteParameter;


/*	Set these flags on parameters and groups to control how a Theme will	*/
/*	render the Web. Hidden means, generally, "don't show". Advanced means,	*/
/*	generally, that you can show it or not depending on your whim.		*/
enum media_parameter_flags {
	B_HIDDEN_PARAMETER = 0x1,
	B_ADVANCED_PARAMETER = 0x2
};


class BParameterWeb :
	public BFlattenable
{
public:
		BParameterWeb();
		~BParameterWeb();

		media_node Node();

		BParameterGroup * MakeGroup(
				const char * name);

		int32 CountGroups();
		BParameterGroup * GroupAt(
				int32 index);
		int32 CountParameters();
		BParameter * ParameterAt(
				int32 index);

virtual	bool		IsFixedSize() const;
virtual	type_code	TypeCode() const;
virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	bool		AllowsTypeCode(type_code code) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:

	friend class BParameterGroup;
	friend class BControllable;

		BParameterWeb(
				const BParameterWeb & clone);
		BParameterWeb & operator=(
				const BParameterWeb & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_ControlWeb_0(void *);
virtual		status_t _Reserved_ControlWeb_1(void *);
virtual		status_t _Reserved_ControlWeb_2(void *);
virtual		status_t _Reserved_ControlWeb_3(void *);
virtual		status_t _Reserved_ControlWeb_4(void *);
virtual		status_t _Reserved_ControlWeb_5(void *);
virtual		status_t _Reserved_ControlWeb_6(void *);
virtual		status_t _Reserved_ControlWeb_7(void *);

		BList * mGroups;
		media_node mNode;
		uint32 _reserved_control_web_[8];

		BList * mOldRefs;
		BList * mNewRefs;

		void AddRefFix(
				void * oldItem,
				void * newItem);
};


class BParameterGroup :
	public BFlattenable
{
private:

		BParameterGroup(
				BParameterWeb * web,
				const char * name);
virtual	~BParameterGroup();

public:

		BParameterWeb * Web() const;
		const char * Name() const;

		void SetFlags(uint32 flags);
		uint32 Flags() const;

		BNullParameter * MakeNullParameter(
				int32 id,
				media_type m_type,
				const char * name,
				const char * kind);
		BContinuousParameter * MakeContinuousParameter(
				int32 id,
				media_type m_type,
				const char * name,
				const char * kind,
				const char * unit,
				float minimum,
				float maximum,
				float stepping);
		BDiscreteParameter * MakeDiscreteParameter(
				int32 id,
				media_type m_type,
				const char * name,
				const char * kind);
		BParameterGroup * MakeGroup(
				const char * name);

		int32 CountParameters();
		BParameter * ParameterAt(
				int32 index);
		int32 CountGroups();
		BParameterGroup * GroupAt(
				int32 index);

virtual	bool		IsFixedSize() const;
virtual	type_code	TypeCode() const;
virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	bool		AllowsTypeCode(type_code code) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:

		BParameterGroup();	/* private unimplemented */
		BParameterGroup(
				const BParameterGroup & clone);
		BParameterGroup & operator=(
				const BParameterGroup & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_ControlGroup_0(void *);
virtual		status_t _Reserved_ControlGroup_1(void *);
virtual		status_t _Reserved_ControlGroup_2(void *);
virtual		status_t _Reserved_ControlGroup_3(void *);
virtual		status_t _Reserved_ControlGroup_4(void *);
virtual		status_t _Reserved_ControlGroup_5(void *);
virtual		status_t _Reserved_ControlGroup_6(void *);
virtual		status_t _Reserved_ControlGroup_7(void *);

	friend class BParameterWeb;

		BParameterWeb * mWeb;
		BList * mControls;
		BList * mGroups;
		char * mName;
		uint32 mFlags;
		uint32 _reserved_control_group_[7];

		BParameter * MakeControl(
				int32 type);
};


/* After you create a BParameter, hook it up by calling AddInput() and/or AddOutput() */
/* (which will call the reciprocal in the target) and optionally call SetChannelCount() and SetMediaType() */
class BParameter :
	public BFlattenable
{
public:

		/* This is a parameter TYPE */
		enum media_parameter_type
		{
			B_NULL_PARAMETER,
			B_DISCRETE_PARAMETER,
			B_CONTINUOUS_PARAMETER
		};

		media_parameter_type Type() const;
		BParameterWeb * Web() const;
		BParameterGroup * Group() const;
		const char * Name() const;
		const char * Kind() const;
		const char * Unit() const;
		int32 ID() const;

		void SetFlags(uint32 flags);
		uint32 Flags() const;

virtual	type_code ValueType() = 0;
		/* These functions are typically used by client apps; they will result in */
		/* your BControllable getting called to read/write values. */
		status_t GetValue(
				void * buffer,
				size_t * ioSize,
				bigtime_t * when);
		status_t SetValue(
				const void * buffer,
				size_t size,
				bigtime_t when);
		int32 CountChannels();		/* Number of ValueType() values; default is 1 */
		void SetChannelCount(		/* One value could still control e g a stereo pair */
				int32 channel_count);

		media_type MediaType();	/* Optional (default is B_MEDIA_NO_TYPE) */
		void SetMediaType(media_type m_type);

		int32 CountInputs();
		BParameter * InputAt(
				int32 index);
		void AddInput(
				BParameter * input);
		int32 CountOutputs();
		BParameter * OutputAt(
				int32 index);
		void AddOutput(
				BParameter * output);

virtual	bool		IsFixedSize() const;
virtual	type_code	TypeCode() const;
virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	bool		AllowsTypeCode(type_code code) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:
	friend class BNullParameter;
	friend class BContinuousParameter;
	friend class BDiscreteParameter;
	friend class BParameterGroup;
	friend class BParameterWeb;

		bool SwapOnUnflatten() { return mSwapDetected; }

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_Control_0(void *);
virtual		status_t _Reserved_Control_1(void *);
virtual		status_t _Reserved_Control_2(void *);
virtual		status_t _Reserved_Control_3(void *);
virtual		status_t _Reserved_Control_4(void *);
virtual		status_t _Reserved_Control_5(void *);
virtual		status_t _Reserved_Control_6(void *);
virtual		status_t _Reserved_Control_7(void *);


		BParameter(
				int32 id,
				media_type m_type,
				media_parameter_type type,
				BParameterWeb * web,
				const char * name,
				const char * kind,
				const char * unit);
		~BParameter();

		int32 mID;
		media_parameter_type mType;
		BParameterWeb * mWeb;
		BParameterGroup * mGroup;
		char * mName;
		char * mKind;
		char * mUnit;
		BList * mInputs;
		BList * mOutputs;
		bool mSwapDetected;
		media_type mMediaType;
		int32 mChannels;
		uint32 mFlags;
		uint32 _reserved_control_[7];

virtual	void FixRefs(
				BList & old,
				BList & updated);
};


class BContinuousParameter :
	public BParameter
{
public:

virtual	type_code ValueType();

		float MinValue();
		float MaxValue();
		float ValueStep();

		/* The "response" specifies what value to display to the user. */
		/* Thus, if response is B_POLYNOMIAL with factor 2, an actual */
		/* control value of 10 would be displayed to the user as 100, and */
		/* if response was B_EXPONENTIAL and factor was 2, an actual */
		/* value of 3 would display 8 (two to the third). The ValueStep() */
		/* is given in actual control values, before the transformation for */
		/* display is done. Thus, with min 0, max 4 and value step 1, and an */
		/* exponential response with factor 10, you will get the displayed */
		/* values 1, 10, 100, 1000 and 10000 equally spaced across a slider */
		/* (or whatever UI the app puts to the parameter). */
		/* The "offset" is added to the value after transformation, before display. */
		/* If "resp" is negative, the resulting value/display relation is turned upside down. */
		enum response {
			B_UNKNOWN = 0,
			B_LINEAR = 1,	/* factor is direct multiplier >= 0 */
			B_POLYNOMIAL,	/* factor should be power; typically "2" for squared or "-1" for inverse */
			B_EXPONENTIAL,	/* factor should be base, typically 2 or 10 */
			B_LOGARITHMIC	/* factor should be base, typically 2 or 10 */
		};
		void SetResponse(
				int resp,
				float factor,
				float offset);
		void GetResponse(
				int * resp,
				float * factor,
				float * offset);

virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_ContinuousParameter_0(void *);
virtual		status_t _Reserved_ContinuousParameter_1(void *);
virtual		status_t _Reserved_ContinuousParameter_2(void *);
virtual		status_t _Reserved_ContinuousParameter_3(void *);
virtual		status_t _Reserved_ContinuousParameter_4(void *);
virtual		status_t _Reserved_ContinuousParameter_5(void *);
virtual		status_t _Reserved_ContinuousParameter_6(void *);
virtual		status_t _Reserved_ContinuousParameter_7(void *);
	friend class BParameterGroup;

		BContinuousParameter(
				int32 id,
				media_type m_type,
				BParameterWeb * web,
				const char * name,
				const char * kind,
				const char * unit,
				float minimum,
				float maximum,
				float stepping);
		~BContinuousParameter();

		float mMinimum;
		float mMaximum;
		float mStepping;
		response mResponse;
		float mFactor; 
		float mOffset;
		uint32 _reserved_control_slider_[8];

};


class BDiscreteParameter :
	public BParameter
{
public:

virtual	type_code ValueType();

		int32 CountItems();
		const char * ItemNameAt(
				int32 index);
		int32 ItemValueAt(
				int32 index);
		status_t AddItem(
				int32 value,
				const char * name);
		status_t MakeItemsFromInputs();
		status_t MakeItemsFromOutputs();
		void MakeEmpty();

virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:
		/* Mmmh, stuffing! */
virtual		status_t _Reserved_DiscreteParameter_0(void *);
virtual		status_t _Reserved_DiscreteParameter_1(void *);
virtual		status_t _Reserved_DiscreteParameter_2(void *);
virtual		status_t _Reserved_DiscreteParameter_3(void *);
virtual		status_t _Reserved_DiscreteParameter_4(void *);
virtual		status_t _Reserved_DiscreteParameter_5(void *);
virtual		status_t _Reserved_DiscreteParameter_6(void *);
virtual		status_t _Reserved_DiscreteParameter_7(void *);

	friend class BParameterGroup;

		BList * mSelections;
		BList * mValues;
		uint32 _reserved_control_selector_[8];

		BDiscreteParameter(
				int32 id,
				media_type m_type,
				BParameterWeb * web,
				const char * name,
				const char * kind);
		~BDiscreteParameter();
};


class BNullParameter :
	public BParameter
{
public:

virtual	type_code ValueType();

virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

private:
		/* Mmmh, stuffing! */
virtual		status_t _Reserved_NullParameter_0(void *);
virtual		status_t _Reserved_NullParameter_1(void *);
virtual		status_t _Reserved_NullParameter_2(void *);
virtual		status_t _Reserved_NullParameter_3(void *);
virtual		status_t _Reserved_NullParameter_4(void *);
virtual		status_t _Reserved_NullParameter_5(void *);
virtual		status_t _Reserved_NullParameter_6(void *);
virtual		status_t _Reserved_NullParameter_7(void *);

	friend class BParameterGroup;

		uint32 _reserved_control_junction_[8];

		BNullParameter(
				int32 id,
				media_type m_type,
				BParameterWeb * web,
				const char * name,
				const char * kind);
		~BNullParameter();

};


#endif /* _CONTROL_WEB_H */
