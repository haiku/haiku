#include "MessageDriverSettingsUtils.h"

#include <driver_settings.h>
#include <File.h>
#include <Message.h>
#include <String.h>

#include <cstdio>


bool
FindMessageParameter(const char *name, BMessage& message, BMessage& save,
	int32 *startIndex = NULL)
{
	// XXX: this should be removed when we can replace BMessage with something better
	BString string;
	int32 index = startIndex ? *startIndex : 0;
	for(; message.FindMessage("Parameters", index, &save) == B_OK; index++) {
		if(save.FindString("Name", &string) == B_OK && string.ICompare(name) == 0) {
			if(startIndex)
				*startIndex = index;
			return true;
		}
	}
	
	return false;
}


static
bool
AddParameter(const driver_parameter *parameter, BMessage& message)
{
	if(!parameter)
		return false;
	
	if(parameter->name)
		message.AddString("Name", parameter->name);
	else
		return false;
	
	for(int32 index = 0; index < parameter->value_count; index++)
		if(parameter->values[index])
			message.AddString("Values", parameter->values[index]);
	
	for(int32 index = 0; index < parameter->parameter_count; index++) {
		BMessage parameterMessage;
		AddParameter(&parameter->parameters[index], parameterMessage);
		message.AddMessage("Parameters", &parameterMessage);
	}
	
	return true;
}


bool
ReadMessageDriverSettings(const char *name, BMessage& message)
{
	if(!name)
		return false;
	
	void *handle = load_driver_settings(name);
	if(!handle)
		return false;
	const driver_settings *settings = get_driver_settings(handle);
	if(!settings) {
		unload_driver_settings(handle);
		return false;
	}
	
	for(int32 index = 0; index < settings->parameter_count; index++) {
		BMessage parameter;
		AddParameter(&settings->parameters[index], parameter);
		message.AddMessage("Parameters", &parameter);
	}
	
	unload_driver_settings(handle);
	
	return true;
}


static
void
EscapeWord(BString& word)
{
	word.ReplaceAll("\\", "\\\\");
	word.ReplaceAll("#", "\\#");
	word.ReplaceAll("\"", "\\\"");
	word.ReplaceAll("\'", "\\\'");
}


static
bool
WriteParameter(BFile& file, const BMessage& parameter, int32 level)
{
	const char *name;
	if(parameter.FindString("Name", &name) != B_OK || !name)
		return false;
	
	BString line, word(name);
	EscapeWord(word);
	bool needsEscaping = word.FindFirst(' ') >= 0;
	line.SetTo('\t', level);
	if(needsEscaping)
		line << '\"';
	line << word;
	if(needsEscaping)
		line << '\"';
	line << ' ';
	
	for(int32 index = 0; parameter.FindString("Values", index, &name) == B_OK; index++)
		if(name) {
			word = name;
			EscapeWord(word);
			needsEscaping = word.FindFirst(' ') >= 0;
			if(needsEscaping)
				line << '\"';
			line << word;
			if(needsEscaping)
				line << '\"';
			line << ' ';
		}
	
	type_code type;
	int32 parameterCount;
	parameter.GetInfo("Parameters", &type, &parameterCount);
	
	if(parameterCount > 0)
		line << '{';
	
	line << '\n';
	file.Write(line.String(), line.Length());
	
	if(parameterCount > 0) {
		BMessage subParameter;
		for(int32 index = 0; parameter.FindMessage("Parameters", index,
				&subParameter) == B_OK; index++)
			WriteParameter(file, subParameter, level + 1);
		
		line.SetTo('\t', level);
		line << "}\n";
		file.Write(line.String(), line.Length());
	}
	
	return true;
}


bool
WriteMessageDriverSettings(BFile& file, const BMessage& message)
{
	if(file.InitCheck() != B_OK || !file.IsWritable())
		return false;
	
	file.SetSize(0);
	file.Seek(0, SEEK_SET);
	
	BMessage parameter;
	for(int32 index = 0; message.FindMessage("Parameters", index, &parameter) == B_OK;
			index++) {
		if(index > 0)
			file.Write("\n", 1);
		WriteParameter(file, parameter, 0);
	}
	
	return true;
}
