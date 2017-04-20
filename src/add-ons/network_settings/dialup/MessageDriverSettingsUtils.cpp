/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MessageDriverSettingsUtils.h"

#include <PPPDefs.h>
#include <settings_tools.h>
#include <File.h>
#include <Message.h>
#include <String.h>

#include <cstdio>


static bool AddParameters(const BMessage& message, driver_settings *to);


bool
FindMessageParameter(const char *name, const BMessage& message, BMessage *save,
	int32 *startIndex)
{
	// XXX: this should be removed when we can replace BMessage with something better
	BString string;
	int32 index = startIndex ? *startIndex : 0;
	for(; message.FindMessage(MDSU_PARAMETERS, index, save) == B_OK; index++) {
		if(save->FindString(MDSU_NAME, &string) == B_OK
				&& string.ICompare(name) == 0) {
			if(startIndex)
				*startIndex = index;
			return true;
		}
	}

	return false;
}


static
bool
AddValues(const BMessage& message, driver_parameter *parameter)
{
	const char *value;
	for(int32 index = 0; message.FindString(MDSU_VALUES, index, &value) == B_OK;
			index++)
		if(!add_driver_parameter_value(value, parameter))
			return false;

	return true;
}


inline
bool
AddParameters(const BMessage& message, driver_parameter *to)
{
	if(!to)
		return false;

	return AddParameters(message,
		reinterpret_cast<driver_settings*>(&to->parameter_count));
}


static
bool
AddParameters(const BMessage& message, driver_settings *to)
{
	const char *name;
	BMessage current;
	driver_parameter *parameter;
	for(int32 index = 0; message.FindMessage(MDSU_PARAMETERS, index,
			&current) == B_OK; index++) {
		name = current.FindString(MDSU_NAME);
		parameter = new_driver_parameter(name);
		if(!AddValues(current, parameter))
			return false;

		AddParameters(current, parameter);
		add_driver_parameter(parameter, to);
	}

	return true;
}


driver_settings*
MessageToDriverSettings(const BMessage& message)
{
	driver_settings *settings = new_driver_settings();

	if(!AddParameters(message, settings)) {
		free_driver_settings(settings);
		return NULL;
	}

	return settings;
}


static
bool
AddParameter(const driver_parameter *parameter, BMessage *message)
{
	if(!parameter || !message)
		return false;

	if(parameter->name)
		message->AddString(MDSU_NAME, parameter->name);
	else
		return false;

	for(int32 index = 0; index < parameter->value_count; index++)
		if(parameter->values[index])
			message->AddString(MDSU_VALUES, parameter->values[index]);

	for(int32 index = 0; index < parameter->parameter_count; index++) {
		BMessage parameterMessage;
		AddParameter(&parameter->parameters[index], &parameterMessage);
		message->AddMessage(MDSU_PARAMETERS, &parameterMessage);
	}

	return true;
}


bool
ReadMessageDriverSettings(const char *name, BMessage *message)
{
	if(!name || !message)
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
		AddParameter(&settings->parameters[index], &parameter);
		message->AddMessage(MDSU_PARAMETERS, &parameter);
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
	if(parameter.FindString(MDSU_NAME, &name) != B_OK || !name)
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

	for(int32 index = 0; parameter.FindString(MDSU_VALUES, index, &name) == B_OK;
			index++)
		if(name) {
			line << ' ';
			word = name;
			EscapeWord(word);
			needsEscaping = word.FindFirst(' ') >= 0;
			if(needsEscaping)
				line << '\"';
			line << word;
			if(needsEscaping)
				line << '\"';
		}

	type_code type;
	int32 parameterCount;
	parameter.GetInfo(MDSU_PARAMETERS, &type, &parameterCount);

	if(parameterCount > 0)
		line << " {";

	line << '\n';
	file.Write(line.String(), line.Length());

	if(parameterCount > 0) {
		BMessage subParameter;
		for(int32 index = 0; parameter.FindMessage(MDSU_PARAMETERS, index,
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
	for(int32 index = 0; message.FindMessage(MDSU_PARAMETERS, index, &parameter) == B_OK;
			index++) {
		if(index > 0)
			file.Write("\n", 1);
		WriteParameter(file, parameter, 0);
	}

	return true;
}
