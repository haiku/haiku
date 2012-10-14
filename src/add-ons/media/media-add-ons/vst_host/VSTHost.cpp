/*
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 
 
#include <stdio.h>
#include <stdlib.h>
#include <image.h>

#include <Application.h>

#include "VSTHost.h"

static int32 VHostCallback(VSTEffect* effect, int32 opcode, int32 index,
	int32 value, void* ptr, float opt);

//Trim string
static void
TrimString(BString *string) {
	char* str = string->LockBuffer(256);
    uint32 k = 0;
    uint32 i = 0;
    for(i=0; str[i]!='\0';) {
        if (isspace(str[i])) {
            k = i;
            for(uint32 j = i; j < strlen(str) - 1; j++) {
                str[j] = str[j + 1];
            }
            str[strlen(str) - 1] = '\0';
            i = k;
        } else {
            i++;
        }
    }
    string->UnlockBuffer();
}

//VST Parameter class
VSTParameter::VSTParameter(VSTPlugin* plugin, int index)
{
	fIndex = index;
	fEffect = plugin->Effect();
	fDropList.MakeEmpty();

	char temp[256]; 
  	//get parameter name
  	temp[0] = 0;
	fEffect->dispatcher(fEffect, VST_GET_PARAM_NAME, index, 0, temp, 0);
	fName.SetTo(temp);
	TrimString(&fName);
	//get parameter label (unit)
	temp[0] = 0;
	fEffect->dispatcher(fEffect, VST_GET_PARAM_UNIT, index, 0, temp, 0);
	fUnit.SetTo(temp);
	ValidateValues(&fUnit);
	//store current value
	float val = fEffect->getParameter(fEffect, index);
	//test for minimum value
	fEffect->setParameter(fEffect, index, 0);
	temp[0] = 0;
	fEffect->dispatcher(fEffect, VST_GET_PARAM_STR, index, 0, temp, 0);
	fMinValue.SetTo(temp);
	ValidateValues(&fMinValue);
	//test for maximum value
	temp[0] = 0;
	fEffect->setParameter(fEffect, index, 1.0);
	fEffect->dispatcher(fEffect, VST_GET_PARAM_STR, index, 0, temp, 0);
	fMaxValue.SetTo(temp);	
	ValidateValues(&fMaxValue);
	//test for discrete values
	char test_disp[VST_PARAM_TEST_COUNT][256];
	float test_values[VST_PARAM_TEST_COUNT];
	float delta = 1.0 / (float)VST_PARAM_TEST_COUNT;
	int test_cnt = 0;
	for(int tst_val = 0; tst_val < VST_PARAM_TEST_COUNT; tst_val++){
		float v = (float)tst_val / (float)VST_PARAM_TEST_COUNT;
		if (tst_val >= VST_PARAM_TEST_COUNT - 1) {
			v = 1.0;
		}
		fEffect->setParameter(fEffect, index, v);
		float new_value = fEffect->getParameter(fEffect, index);
		bool valtest = false;
		for(int i = 0; i < test_cnt; i++) {
			if (fabs(test_values[i] - new_value) < delta) {
				valtest = true;
				break;
			}
		}
		if (valtest == false) {
			test_values[test_cnt] = new_value;
			fEffect->dispatcher(fEffect, VST_GET_PARAM_STR, index,
				0, test_disp[test_cnt], 0);
			test_cnt++;
		}
	}
						
	//restore value
	fEffect->setParameter(fEffect, index, val);
		
	//detect param type
	if (test_cnt == 2) {
		fType = VST_PARAM_CHECKBOX;
		
		DropListValue* min_item = new DropListValue();
		min_item->Value = 0.0;
		min_item->Index = 0;
		min_item->Name = fMinValue;
		fDropList.AddItem(min_item);
		
		DropListValue* max_item = new DropListValue();
		max_item->Value = 1.0;
		max_item->Index = 1;
		max_item->Name = fMaxValue;
		fDropList.AddItem(max_item);		
	} else if (test_cnt > 2 && test_cnt < VST_PARAM_TEST_COUNT / 2) {
		fType = VST_PARAM_DROPLIST;
		
		for(int i = 0; i < test_cnt; i++) {
			DropListValue* item = new DropListValue();
			item->Value = test_values[i];
			item->Index = i;
			item->Name = test_disp[i];
			fDropList.AddItem(item);
		}		
	} else {
		fType = VST_PARAM_SLIDER;
	}
	fChanged = 0LL;
}

VSTParameter::~VSTParameter()
{	
}

BString*
VSTParameter::ValidateValues(BString* string)
{	
	if (string->Length() == 0) {
		return string;
	}
		
	bool isNum = true;
	
	const char *ptr = string->String();
	for(; *ptr!=0; ptr++) {
		char ch = *ptr;
		if (!((ch >= '0' && ch <= '9') || ch == '.' || ch == '-')) {
			isNum = false;
			break;
		}
	}
	
	if (isNum) {
		float val = atof(string->String());

		if (val <= -pow(2, 31)) {
			string->SetTo("-∞");
		} else if (val >= pow(2, 31)) {
			string->SetTo("∞");
		} else {
			char temp[256];
			sprintf(temp, "%g", val);
			string->SetTo(temp);
		}
	} else {
		TrimString(string);
		if (*string == "oo" || *string == "inf")
			string->SetTo("∞");
		if (*string == "-oo" || *string == "-inf")
			string->SetTo("-∞");
		
	}	
	return string;
}

int
VSTParameter::ListCount(void)
{
	return fDropList.CountItems();
}

DropListValue*
VSTParameter::ListItemAt(int index)
{
	DropListValue* item = NULL;
	if (index >= 0 && index < fDropList.CountItems()) {
		item = (DropListValue*)fDropList.ItemAt(index);
	}
	return item;
}


float
VSTParameter::Value()
{	
	float value = fEffect->getParameter(fEffect, fIndex);
	if (fType == VST_PARAM_DROPLIST) {
		//scan for near value
		int	min_index = 0;
		float min_delta = 1.0;
		for(int i = 0; i < fDropList.CountItems(); i++) {
			DropListValue* item = (DropListValue*)fDropList.ItemAt(i);
			float delta	= fabs(item->Value - value);
			if (delta <= min_delta) {
				min_delta = delta;
				min_index = i;
			}
		}
		value = min_index;
	}
	fLastValue = value;
	return value;
}

void
VSTParameter::SetValue(float value)
{
	if (value == fLastValue) {
		return;
	}
	
	if (fType == VST_PARAM_DROPLIST) {
		//take value by index
		int index = (int)round(value);
		if (index >= 0 && index < fDropList.CountItems()) {
			DropListValue *item = (DropListValue*)fDropList.ItemAt(index);
			value = item->Value;
			fLastValue = index;
		} else {
			return;
		}
	} else {
		fLastValue = value;
	}
	fChanged = system_time();
	fEffect->setParameter(fEffect, fIndex, value);
}

bigtime_t
VSTParameter::LastChangeTime(void)
{
	return fChanged;
}

const char*
VSTParameter::MinimumValue(void)
{
	return fMinValue.String();
}

const char*
VSTParameter::MaximumValue(void)
{
	return fMaxValue.String();
}

const char*
VSTParameter::Unit(void)
{
	return fUnit.String();
}

int
VSTParameter::Index(void)
{
	return fIndex;
}

int
VSTParameter::Type(void)
{
	return fType;
}

const char*
VSTParameter::Name(void)
{
	return fName.String();
}

//VST Plugin class
VSTPlugin::VSTPlugin()
{
	fActive = false;
	fEffect = NULL;
	VSTMainProc = NULL;
	fInputChannels = 0;
	fOutputChannels = 0;
	fSampleRate = 44100.f;
	fBlockSize = 0;
	inputs = NULL;
	outputs = NULL;
	fParameters.MakeEmpty();
}

VSTPlugin::~VSTPlugin()
{
	fParameters.MakeEmpty();
	UnLoadModule();
}

int
VSTPlugin::LoadModule(const char *path)
{
	char effectName[256] = {0};
	char vendorString[256] = {0};
	char productString[256] = {0};

	if (fActive) {
		return VST_ERR_ALREADY_LOADED;
	}

	fPath = BPath(path);
	
	fModule = load_add_on(path);
	if (fModule <= 0) {
		return VST_ERR_NOT_LOADED;
	}

	if (get_image_symbol(fModule, "main_plugin", B_SYMBOL_TYPE_TEXT,
			(void**)&VSTMainProc) != B_OK) {
		unload_add_on(fModule);
		return VST_ERR_NO_MAINPROC;
	}		
	
	fEffect = VSTMainProc(VHostCallback);
	if (fEffect==NULL) {
		unload_add_on(fModule);
		return VST_ERR_NOT_LOADED;
	}
	
	fEffect->dispatcher(fEffect, VST_OPEN, 0, 0, 0, 0);
		
	fEffect->dispatcher(fEffect, VST_GET_EFFECT_NAME, 0, 0, effectName, 0);
	fEffectName.SetTo(effectName);
	TrimString(&fEffectName);
	
	fModuleName.SetTo("VST:");
	fModuleName.Append(fPath.Leaf());
	
	fEffect->dispatcher(fEffect, VST_GET_VENDOR_STR, 0, 0, vendorString, 0);
	fVendorString.SetTo(vendorString);
	TrimString(&fVendorString);
	
	fEffect->dispatcher(fEffect, VST_GET_PRODUCT_STR, 0, 0, productString, 0);
	fProductString.SetTo(productString);	
	TrimString(&fProductString);
	
	fInputChannels = fEffect->numInputs;
	fOutputChannels = fEffect->numOutputs;
	
	for(int i=0; i < fEffect->numParams; i++) {
		VSTParameter *param = new VSTParameter(this, i);
		fParameters.AddItem(param);
	}
	
	fEffect->dispatcher(fEffect, VST_STATE_CHANGED, 0, 1, 0, 0);
	
	ReAllocBuffers();
	
	fActive = true;
	return B_OK;
}

int
VSTPlugin::UnLoadModule(void)
{
	if (!fActive || fModule <= 0) {
		return VST_ERR_NOT_LOADED;
	}
	fEffect->dispatcher(fEffect, VST_STATE_CHANGED, 0, 0, 0, 0);
	fEffect->dispatcher(fEffect, VST_CLOSE, 0, 0, 0, 0);
	
	unload_add_on(fModule);
	
	return B_OK;
}

int
VSTPlugin::Channels(int mode)
{
	switch(mode) {
		case VST_INPUT_CHANNELS:
			return fInputChannels;
		case VST_OUTPUT_CHANNELS:
			return fOutputChannels;
		default:
			return 0;
	}	
}

int
VSTPlugin::SetSampleRate(float rate)
{
	fSampleRate = rate;
	fEffect->dispatcher(fEffect, VST_SET_SAMPLE_RATE, 0, 0, 0, rate);
	return B_OK;
}

float
VSTPlugin::SampleRate(void)
{
	return fSampleRate;
}

int
VSTPlugin::SetBlockSize(size_t size)
{
	fBlockSize = size;
	fEffect->dispatcher(fEffect, VST_SET_BLOCK_SIZE, 0, size, 0, 0);
	ReAllocBuffers();
	return B_OK;
}

const char*
VSTPlugin::Path(void)
{
	return fPath.Path();
}

int
VSTPlugin::ReAllocBuffers(void)
{
	if (inputs != NULL) {
		for(int32 i = 0; i < fInputChannels; i++)	{
			delete inputs[i];
		}		
	}
	
	if (outputs != NULL) {
		for(int32 i = 0; i < fOutputChannels; i++)	{
			delete outputs[i];
		}		
	}
	
	if (fInputChannels > 0) {
		inputs = new float*[fInputChannels];
		for(int32 i = 0; i < fInputChannels; i++)	{
			inputs[i] = new float[fBlockSize]; 
			memset(inputs[i], 0, fBlockSize * sizeof(float)); 
		}
	} 
 
	if (fOutputChannels > 0) { 
		outputs = new float*[fOutputChannels];
		for(int32_t i = 0; i < fOutputChannels; i++) {
			outputs[i] = new float[fBlockSize];
			memset (outputs[i], 0, fBlockSize * sizeof(float));
		}
	}
	return B_OK;
}

size_t
VSTPlugin::BlockSize(void)
{
	return fBlockSize;
}

int
VSTPlugin::ParametersCount(void)
{
	return fParameters.CountItems();
}

VSTParameter*
VSTPlugin::Parameter(int index)
{
	VSTParameter* param = NULL;
	
	if (index >= 0 && index < fParameters.CountItems()) {
		param = (VSTParameter*)fParameters.ItemAt(index);
	}
	
	return param;
}

VSTEffect*
VSTPlugin::Effect(void)
{
	return fEffect;
}

const char*
VSTPlugin::EffectName(void)
{
	return fEffectName.String();
}

const char*
VSTPlugin::ModuleName(void)
{
	return fModuleName.String();
}

const char*
VSTPlugin::Vendor(void)
{
	return fVendorString.String();
}

const char*
VSTPlugin::Product(void)
{
	return fProductString.String();
}


void
VSTPlugin::Process(float *buffer, int samples, int channels)
{	
	//todo: full channels remapping needed
	float* src = buffer;
	
	if (channels == fInputChannels) { //channel to channel
		for(int j = 0; j < samples; j++) {
			for(int c = 0; c < fInputChannels; c++) {
				inputs[c][j] = *src++;
			}
		}
	} else if ( channels == 1) {	//from mone to multichannel
		for(int j = 0; j < samples; j++, src++) {
			for(int c = 0; c < fInputChannels; c++) {
				inputs[c][j] = *src;
			}
		}
	}
		
	fEffect->processReplacing(fEffect, inputs, outputs, fBlockSize);
	
	float* dst = buffer;
	
	if (channels == fOutputChannels) { //channel to channel
		for(int j = 0; j < samples; j++) {
			for(int c = 0; c < fOutputChannels; c++) {				
				*dst++ = outputs[c][j];
			}	
		}
	} else if (channels == 1) {  //from multichannel to mono
		for(int j = 0; j < samples; j++, dst++) {
			float mix = 0;
			for(int c = 0; c < fOutputChannels; c++) {				
				mix += outputs[c][j];
			}	
			*dst = mix / (float)fOutputChannels;
		}		
	}
}

static int32 
VHostCallback(VSTEffect* effect, int32 opcode, int32 index, int32 value,
	void* ptr, float opt)
{
	intptr_t result = 0;
 
	switch(opcode)
	{
		case VST_MASTER_PRODUCT:
			if (ptr) {
				strcpy((char*)ptr, "VSTHost Media AddOn");
				result = 1;
			}
			break;		 
		case VST_MASTER_VERSION :
			result = 2300;
			break;
	}

	return result;
}
