/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#ifndef __VST_HOST_H__
#define __VST_HOST_H__

#include <String.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <List.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <image.h>
#include <string.h>
#include <ctype.h>

#define VST_PARAM_TEST_COUNT 	100

//error codes
#define VST_ERR_ALREADY_LOADED 	-1
#define VST_ERR_NOT_LOADED 		-2
#define VST_ERR_NO_MAINPROC 	-3

//param types
#define VST_PARAM_SLIDER	1
#define VST_PARAM_CHECKBOX  2
#define VST_PARAM_DROPLIST  3

//channels
#define VST_INPUT_CHANNELS	1
#define VST_OUTPUT_CHANNELS	2

//vst callback opcodes
#define VST_MASTER_AUTOMATE 0x00
#define VST_MASTER_VERSION 	0x01
#define VST_MASTER_IDLE 	0x03
#define VST_MASTER_VENDOR 	0x20
#define VST_MASTER_PRODUCT	0x21

//vst actions
#define VST_OPEN			0x00
#define VST_CLOSE			0x01
#define	VST_GET_PARAM_UNIT	0x06
#define VST_GET_PARAM_STR 	0x07
#define VST_GET_PARAM_NAME	0x08
#define VST_SET_SAMPLE_RATE	0x0A
#define VST_SET_BLOCK_SIZE	0x0B
#define VST_STATE_CHANGED 	0x0C
#define VST_GET_EFFECT_NAME	0x2D
#define VST_GET_VENDOR_STR	0x2F
#define VST_GET_PRODUCT_STR 0x30
#define VST_GET_VENDOR_VER	0x31

//vst plugin structure
struct VSTEffect
{
		int 			cookie;
		int32 			(*dispatcher)(struct VSTEffect*, int32, int32, int32, void*, float);
		void 			(*process)(struct VSTEffect*, float**, float**, int32);
		void 			(*setParameter)(struct VSTEffect*, int32, float);
		float 			(*getParameter)(struct VSTEffect*, int32);
		int32 			numPrograms;
		int32 			numParams;
		int32 			numInputs;
		int32 			numOutputs;
		int32 			flags;
		void*			_notused_pointer1;
		void*			_notused_pointer2;
		char 			_notused_block1[12];
		float 			_notused_float;
		void*			_notused_pointer3;
		void*			_notused_pointer4;
		int32 			ID;
		char 			_notused_block2[4];
		void 			(*processReplacing)(struct VSTEffect*, float**, float**, int);
};

//typedefs
typedef int32			(*audioMasterCallback)(VSTEffect*, int32, int32, int32, void*, float);
typedef VSTEffect* 		(*VSTEntryProc)(audioMasterCallback audioMaster);
inline 	float 			round(float x) {return ceil(x-0.5);}

//structure for droplist parameters
struct  DropListValue {
		int				Index;
		float 			Value;
		BString 		Name;
};

class VSTPlugin;
//vst parameter class adapter
class VSTParameter {
public:
						VSTParameter(VSTPlugin* plugin, int index);
						~VSTParameter();
		float			Value(void);
		void			SetValue(float value);
		const char*		MinimumValue(void);
		const char*		MaximumValue(void);
		const char*		Unit(void);
		int				Index(void);
		int				Type(void);
		const char*		Name(void);
		int				ListCount(void);
		DropListValue*	ListItemAt(int index);
		bigtime_t		LastChangeTime(void);
private:
		BString*		ValidateValues(BString *string);
		VSTPlugin* 		fPlugin;
		VSTEffect*		fEffect;
		int				fIndex;
		int				fType;
		BString			fName;
		BString			fUnit;
		BString			fMinValue;
		BString			fMaxValue;
		BList			fDropList;
		bigtime_t		fChanged;
		float			fLastValue;
};

//vst plugin interface
class VSTPlugin {
public:	
						VSTPlugin();
						~VSTPlugin();
		int				LoadModule(const char *path);
		int 			UnLoadModule(void);
		int				SetSampleRate(float rate);
		float			SampleRate(void);
		int 			SetBlockSize(size_t size);
		const char*		Path(void);
		size_t			BlockSize(void);
		VSTEffect*		Effect(void);
		const char*		EffectName(void);
		const char*		ModuleName(void);
		const char*		Vendor(void);
		const char*		Product(void);
		int				ParametersCount(void);
		VSTParameter*	Parameter(int index);
		int				Channels(int mode);
		int				ReAllocBuffers(void);
		void			Process(float *buffer, int samples, int channels);
private:		
		VSTEntryProc 	GetMainEntry();
		VSTEffect*		fEffect;
		VSTEntryProc	VSTMainProc;
		bool			fActive;		
		image_id 		fModule;
		BPath			fPath;
		size_t			fBlockSize;
		float			fSampleRate;
		int				fInputChannels;
		int				fOutputChannels;
		BString			fModuleName;
		BString			fEffectName;
		BString			fVendorString;
		BString			fProductString;
		BList			fParameters;
		float			**inputs;
		float			**outputs;
};

#endif //__VST_HOST_H__
