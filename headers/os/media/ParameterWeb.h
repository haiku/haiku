/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _CONTROL_WEB_H
#define _CONTROL_WEB_H


#include <Flattenable.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <TypeConstants.h>


// Parameter Kinds

extern const char* const B_GENERIC;

// slider controls
extern const char* const B_MASTER_GAIN;
extern const char* const B_GAIN;
extern const char* const B_BALANCE;
extern const char* const B_FREQUENCY;
extern const char* const B_LEVEL;
extern const char* const B_SHUTTLE_SPEED;
extern const char* const B_CROSSFADE;		// 0-100 (first - second)
extern const char* const B_EQUALIZATION;	// dB

// compression controls
extern const char* const B_COMPRESSION;
extern const char* const B_QUALITY;
extern const char* const B_BITRATE;			// bits/s
extern const char* const B_GOP_SIZE;

// selector controls
extern const char* const B_MUTE;
extern const char* const B_ENABLE;
extern const char* const B_INPUT_MUX;
extern const char* const B_OUTPUT_MUX;
extern const char* const B_TUNER_CHANNEL;
extern const char* const B_TRACK;
extern const char* const B_RECSTATE;
extern const char* const B_SHUTTLE_MODE;
extern const char* const B_RESOLUTION;
extern const char* const B_COLOR_SPACE;
extern const char* const B_FRAME_RATE;
extern const char* const B_VIDEO_FORMAT;
	// 1 = NTSC-M, 2 = NTSC-J, 3 = PAL-BDGHI, 4 = PAL-M, 5 = PAL-N, 6 = SECAM,
	// 7 = MPEG-1, 8 = MPEG-2

// junction controls
extern const char* const B_WEB_PHYSICAL_INPUT;
extern const char* const B_WEB_PHYSICAL_OUTPUT;
extern const char* const B_WEB_ADC_CONVERTER;
extern const char* const B_WEB_DAC_CONVERTER;
extern const char* const B_WEB_LOGICAL_INPUT;
extern const char* const B_WEB_LOGICAL_OUTPUT;
extern const char* const B_WEB_LOGICAL_BUS;
extern const char* const B_WEB_BUFFER_INPUT;
extern const char* const B_WEB_BUFFER_OUTPUT;

// transport control
extern const char* const B_SIMPLE_TRANSPORT;
	// 0-4: rewind, stop, play, pause, fast-forward

class BContinuousParameter;
class BDiscreteParameter;
class BList;
class BNullParameter;
class BParameter;
class BParameterGroup;
class BTextParameter;

// Parameter flags for influencing Media Themes
enum media_parameter_flags {
	B_HIDDEN_PARAMETER		= 1,
	B_ADVANCED_PARAMETER	= 2
};

class BParameterWeb : public BFlattenable {
public:
								BParameterWeb();
								~BParameterWeb();

			media_node			Node();

			BParameterGroup*	MakeGroup(const char* name);
			int32				CountGroups();
			BParameterGroup*	GroupAt(int32 index);

			int32				CountParameters();
			BParameter*			ParameterAt(int32 index);

	// BFlattenable implementation
	virtual	bool				IsFixedSize() const;
	virtual type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterGroup;
	friend class BControllable;

								BParameterWeb(const BParameterWeb& other);
			BParameterWeb&		operator=(const BParameterWeb& other);

	// reserved
	virtual	status_t			_Reserved_ControlWeb_0(void*);
	virtual	status_t			_Reserved_ControlWeb_1(void*);
	virtual	status_t			_Reserved_ControlWeb_2(void*);
	virtual	status_t			_Reserved_ControlWeb_3(void*);
	virtual	status_t			_Reserved_ControlWeb_4(void*);
	virtual	status_t			_Reserved_ControlWeb_5(void*);
	virtual	status_t			_Reserved_ControlWeb_6(void*);
	virtual	status_t			_Reserved_ControlWeb_7(void*);

			void				AddRefFix(void* oldItem, void* newItem);

private:
			BList*				fGroups;
			media_node			fNode;
			uint32				_reserved[8];
			BList*				fOldRefs;
			BList*				fNewRefs;
};


class BParameterGroup : public BFlattenable {
private:
								BParameterGroup(BParameterWeb* web,
									const char* name);
	virtual						~BParameterGroup();

public:
			BParameterWeb*		Web() const;
			const char*			Name() const;

			void				SetFlags(uint32 flags);
			uint32				Flags() const;

			BNullParameter*		MakeNullParameter(int32 id, media_type type,
									const char* name, const char* kind);
			BContinuousParameter* MakeContinuousParameter(int32 id,
									media_type type, const char* name,
									const char* kind, const char* unit,
									float min, float max, float step);
			BDiscreteParameter*	MakeDiscreteParameter(int32 id, media_type type,
									const char* name, const char* kind);
			BTextParameter*		MakeTextParameter(int32 id, media_type type,
									const char* name, const char* kind,
									size_t maxBytes);

			BParameterGroup*	MakeGroup(const char* name);

			int32				CountParameters();
			BParameter*			ParameterAt(int32 index);

			int32				CountGroups();
			BParameterGroup*	GroupAt(int32 index);

	// BFlattenable implementation
	virtual	bool				IsFixedSize() const;
	virtual type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterWeb;

								BParameterGroup();
								BParameterGroup(const BParameterGroup& other);
			BParameterGroup&	operator=(const BParameterGroup& other);

			BParameter*			MakeControl(int32 type);

	// reserved
	virtual	status_t			_Reserved_ControlGroup_0(void*);
	virtual	status_t			_Reserved_ControlGroup_1(void*);
	virtual	status_t			_Reserved_ControlGroup_2(void*);
	virtual	status_t			_Reserved_ControlGroup_3(void*);
	virtual	status_t			_Reserved_ControlGroup_4(void*);
	virtual	status_t			_Reserved_ControlGroup_5(void*);
	virtual	status_t			_Reserved_ControlGroup_6(void*);
	virtual	status_t			_Reserved_ControlGroup_7(void*);

private:
			BParameterWeb*		fWeb;
			BList*				fControls;
			BList*				fGroups;
			char*				fName;
			uint32				fFlags;
			uint32				_reserved[7];
};


class BParameter : public BFlattenable {
public:
	enum media_parameter_type {
		B_NULL_PARAMETER,
		B_DISCRETE_PARAMETER,
		B_CONTINUOUS_PARAMETER,
		B_TEXT_PARAMETER
	};

			media_parameter_type Type() const;
			BParameterWeb*		Web() const;
			BParameterGroup*	Group() const;
			const char*			Name() const;
			const char*			Kind() const;
			const char*			Unit() const;
			int32				ID() const;
			
			void				SetFlags(uint32 flags);
			uint32				Flags() const;
	
	virtual	type_code			ValueType() = 0;
	
			status_t			GetValue(void* buffer, size_t* _size,
									bigtime_t* _when);
			status_t			SetValue(const void* buffer, size_t size,
									bigtime_t when);

			int32				CountChannels();
			void				SetChannelCount(int32 count);

			media_type			MediaType();
			void				SetMediaType(media_type type);

			int32				CountInputs();
			BParameter*			InputAt(int32 index);
			void				AddInput(BParameter* input);

			int32				CountOutputs();
			BParameter*			OutputAt(int32 index);
			void				AddOutput(BParameter* output);

	// BFlattenable implementation
	virtual	bool				IsFixedSize() const;
	virtual type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BNullParameter;
	friend class BContinuousParameter;
	friend class BDiscreteParameter;
	friend class BTextParameter;
	friend class BParameterGroup;
	friend class BParameterWeb;

								BParameter(int32 id, media_type mediaType,
									media_parameter_type type,
									BParameterWeb* web, const char* name,
									const char* kind, const char* unit);
								~BParameter();

	// reserved
	virtual	status_t			_Reserved_Control_0(void*);
	virtual	status_t			_Reserved_Control_1(void*);
	virtual	status_t			_Reserved_Control_2(void*);
	virtual	status_t			_Reserved_Control_3(void*);
	virtual	status_t			_Reserved_Control_4(void*);
	virtual	status_t			_Reserved_Control_5(void*);
	virtual	status_t			_Reserved_Control_6(void*);
	virtual	status_t			_Reserved_Control_7(void*);

			bool				SwapOnUnflatten() { return fSwapDetected; }
	virtual	void				FixRefs(BList& old, BList& updated);

private:
			int32				fID;
			media_parameter_type fType;
			BParameterWeb*		fWeb;
			BParameterGroup*	fGroup;
			char*				fName;
			char*				fKind;
			char*				fUnit;
			BList*				fInputs;
			BList*				fOutputs;
			bool				fSwapDetected;
			media_type			fMediaType;
			int32				fChannels;
			uint32				fFlags;

			uint32				_reserved[7];
};


class BContinuousParameter : public BParameter {
public:
	enum response {
		B_UNKNOWN = 0,
		B_LINEAR,
		B_POLYNOMIAL,
		B_EXPONENTIAL,
		B_LOGARITHMIC
	};

	virtual	type_code			ValueType();

			float				MinValue();
			float				MaxValue();
			float				ValueStep();

			void				SetResponse(int response, float factor,
									float offset);
			void				GetResponse(int* _response, float* factor,
									float* offset);

	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterGroup;

								BContinuousParameter(int32 id,
									media_type mediaType,
									BParameterWeb* web, const char* name,
									const char* kind, const char* unit,
									float min, float max, float step);
								~BContinuousParameter();

	// reserved
	virtual	status_t			_Reserved_ContinuousParameter_0(void*);
	virtual	status_t			_Reserved_ContinuousParameter_1(void*);
	virtual	status_t			_Reserved_ContinuousParameter_2(void*);
	virtual	status_t			_Reserved_ContinuousParameter_3(void*);
	virtual	status_t			_Reserved_ContinuousParameter_4(void*);
	virtual	status_t			_Reserved_ContinuousParameter_5(void*);
	virtual	status_t			_Reserved_ContinuousParameter_6(void*);
	virtual	status_t			_Reserved_ContinuousParameter_7(void*);

private:
			float				fMinimum;
			float				fMaximum;
			float				fStepping;
			response			fResponse;
			float				fFactor;
			float				fOffset;
			
			uint32				_reserved[8];
};


class BDiscreteParameter : public BParameter {
public:
	virtual	type_code			ValueType();

			int32				CountItems();
			const char*			ItemNameAt(int32 index);
			int32				ItemValueAt(int32 index);
			status_t			AddItem(int32 value, const char* name);

			status_t			MakeItemsFromInputs();
			status_t			MakeItemsFromOutputs();

			void				MakeEmpty();

	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterGroup;

								BDiscreteParameter(int32 id,
									media_type mediaType,
									BParameterWeb* web, const char* name,
									const char* kind);
								~BDiscreteParameter();

	// reserved
	virtual	status_t			_Reserved_DiscreteParameter_0(void*);
	virtual	status_t			_Reserved_DiscreteParameter_1(void*);
	virtual	status_t			_Reserved_DiscreteParameter_2(void*);
	virtual	status_t			_Reserved_DiscreteParameter_3(void*);
	virtual	status_t			_Reserved_DiscreteParameter_4(void*);
	virtual	status_t			_Reserved_DiscreteParameter_5(void*);
	virtual	status_t			_Reserved_DiscreteParameter_6(void*);
	virtual	status_t			_Reserved_DiscreteParameter_7(void*);

private:
			BList*				fSelections;
			BList*				fValues;
			
			uint32				_reserved[8];
};


class BTextParameter : public BParameter {
public:
	virtual	type_code			ValueType();

			size_t				MaxBytes() const;

	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterGroup;

								BTextParameter(int32 id,
									media_type mediaType,
									BParameterWeb* web, const char* name,
									const char* kind, size_t maxBytes);
								~BTextParameter();

	// reserved
	virtual	status_t			_Reserved_TextParameter_0(void*);
	virtual	status_t			_Reserved_TextParameter_1(void*);
	virtual	status_t			_Reserved_TextParameter_2(void*);
	virtual	status_t			_Reserved_TextParameter_3(void*);
	virtual	status_t			_Reserved_TextParameter_4(void*);
	virtual	status_t			_Reserved_TextParameter_5(void*);
	virtual	status_t			_Reserved_TextParameter_6(void*);
	virtual	status_t			_Reserved_TextParameter_7(void*);

private:
			uint32				fMaxBytes;
			
			uint32				_reserved[8];
};


class BNullParameter : public BParameter {
public:
	virtual	type_code			ValueType();

	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

private:
	friend class BParameterGroup;

								BNullParameter(int32 id,
									media_type mediaType,
									BParameterWeb* web, const char* name,
									const char* kind);
								~BNullParameter();

	// reserved
	virtual	status_t			_Reserved_NullParameter_0(void*);
	virtual	status_t			_Reserved_NullParameter_1(void*);
	virtual	status_t			_Reserved_NullParameter_2(void*);
	virtual	status_t			_Reserved_NullParameter_3(void*);
	virtual	status_t			_Reserved_NullParameter_4(void*);
	virtual	status_t			_Reserved_NullParameter_5(void*);
	virtual	status_t			_Reserved_NullParameter_6(void*);
	virtual	status_t			_Reserved_NullParameter_7(void*);

private:
			uint32				_reserved[8];
};


#endif	// _CONTROL_WEB_H
