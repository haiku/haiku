#include "MessageDriverSettingsUtils.h"

#include <driver_settings.h>
#include <File.h>
#include <Message.h>
#include <String.h>
#include <TypeConstants.h>

#include <cstdio>


BMessage*
FindMessageParameter(const char *name, BMessage& message, int32 *startIndex = NULL)
{
	// XXX: this is a hack and should be removed when we can replace BMessage with
	// something better
	const BMessage *parameter;
	BString string;
	ssize_t size;
	int32 index = startIndex ? *startIndex : 0;
	for(; message.FindData("parameters", B_MESSAGE_TYPE, index,
			reinterpret_cast<const void**>(&parameter), &size) == B_OK;
			index++) {
		if(parameter->FindString("name", &string) == B_OK
				&& string.ICompare(name)) {
			if(startIndex)
				*startIndex = index;
			return const_cast<BMessage*>(parameter);
		}
	}
	
	return NULL;
}


static
bool
AddParameter(const driver_parameter *parameter, BMessage& message)
{
	if(!parameter)
		return false;
	
	if(parameter->name)
		message.AddString("name", parameter->name);
	else
		return false;
	
	for(int32 index = 0; index < parameter->value_count; index++)
		if(parameter->values[index])
			message.AddString("values", parameter->values[index]);
	
	for(int32 index = 0; index < parameter->parameter_count; index++) {
		BMessage parameterMessage;
		AddParameter(&parameter->parameters[index], parameterMessage);
		message.AddMessage("parameters", &parameterMessage);
	}
	
	return true;
}


bool
ReadMessageDriverSettings(const char *name, BMessage& message)
{
	if(!name)
		return false;
	
	void *handle = load_driver_settings(name);
	const driver_settings *settings = get_driver_settings(handle);
	if(!settings) {
		unload_driver_settings(handle);
		return false;
	}
	
	for(int32 index = 0; index < settings->parameter_count; index++) {
		BMessage parameter;
		AddParameter(&settings->parameters[index], parameter);
		message.AddMessage("parameters", &parameter);
	}
	
	unload_driver_settings(handle);
	
	return true;
}


static
bool
WriteParameter(BFile& file, const BMessage& parameter, int32 level)
{
	const char *name;
	if(parameter.FindString("name", &name) != B_OK || !name)
		return false;
	
	BString line;
	line.SetTo('\t', level);
	line << name << ' ';
	
	for(int32 index = 0; parameter.FindString("values", index, &name) == B_OK; index++)
		if(name)
			line << name << ' ';
	
	type_code type;
	int32 parameterCount;
	parameter.GetInfo("parameters", &type, &parameterCount);
	
	if(parameterCount > 0)
		line << '{';
	
	line << '\n';
	file.Write(line.String(), line.Length());
	
	if(parameterCount > 0) {
		BMessage subParameter;
		for(int32 index = 0; parameter.FindMessage("parameters", index,
				&subParameter) == B_OK; index++)
			WriteParameter(file, subParameter, level + 1);
		
		line.SetTo('\t', level);
		line << '\n';
		file.Write(line.String(), line.Length());
	}
	
	return true;
}


bool
WriteMessageDriverSettings(BFile& file, const BMessage& message)
{
	if(file.InitCheck() != B_OK || !file.IsWritable())
		return false;
	
	file.Seek(0, SEEK_SET);
	
	BMessage parameter;
	for(int32 index = 0; message.FindMessage("parameters", index, &parameter) == B_OK;
			index++)
		WriteParameter(file, parameter, 0);
	
	return true;
}
