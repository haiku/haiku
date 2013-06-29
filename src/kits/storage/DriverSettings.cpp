/*
 * Copyright 2007-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <DriverSettings.h>

#include <stdlib.h>
#include <string.h>

#include <new>

#include <driver_settings.h>
#include <Path.h>
#include <String.h>

#include <Referenceable.h>


// The parameter values that shall be evaluated to true.
static const char* const kTrueValueStrings[]
	= { "1", "true", "yes", "on", "enable", "enabled" };
static const int32 kTrueValueStringCount
	= sizeof(kTrueValueStrings) / sizeof(const char*);


namespace BPrivate {


// #pragma mark - BDriverParameterIterator


class BDriverParameterIterator::Delegate : public BReferenceable {
public:
								Delegate() : BReferenceable() {}
	virtual						~Delegate() {}

	virtual	Delegate*			Clone() const = 0;

	virtual	bool				HasNext() const = 0;
	virtual	BDriverParameter	Next() = 0;
};


BDriverParameterIterator::BDriverParameterIterator()
	:
	fDelegate(NULL)
{
}


BDriverParameterIterator::BDriverParameterIterator(Delegate* delegate)
	:
	fDelegate(delegate)
{
}


BDriverParameterIterator::BDriverParameterIterator(
	const BDriverParameterIterator& other)
	:
	fDelegate(NULL)
{
	_SetTo(other.fDelegate, true);
}


BDriverParameterIterator::~BDriverParameterIterator()
{
	_SetTo(NULL, false);
}


bool
BDriverParameterIterator::HasNext() const
{
	return fDelegate != NULL ? fDelegate->HasNext() : false;
}


BDriverParameter
BDriverParameterIterator::Next()
{
	if (fDelegate == NULL)
		return BDriverParameter();

	if (fDelegate->CountReferences() > 1) {
		Delegate* clone = fDelegate->Clone();
		if (clone == NULL)
			return BDriverParameter();
		_SetTo(clone, false);
	}

	return fDelegate->Next();
}


BDriverParameterIterator&
BDriverParameterIterator::operator=(const BDriverParameterIterator& other)
{
	_SetTo(other.fDelegate, true);
	return *this;
}


void
BDriverParameterIterator::_SetTo(Delegate* delegate, bool addReference)
{
	if (fDelegate != NULL)
		fDelegate->ReleaseReference();
	fDelegate = delegate;
	if (fDelegate != NULL && addReference)
		fDelegate->AcquireReference();
}


// #pragma mark - BDriverParameterContainer


class BDriverParameterContainer::Iterator
	: public BDriverParameterIterator::Delegate {
public:
	Iterator(const driver_parameter* parameters, int32 count)
		:
		Delegate(),
		fParameters(parameters),
		fCount(count)
	{
	}

	virtual ~Iterator()
	{
	}

	virtual Delegate* Clone() const
	{
		return new(std::nothrow) Iterator(fParameters, fCount);
	}

	virtual bool HasNext() const
	{
		return fParameters != NULL && fCount > 0;
	}

	virtual	BDriverParameter Next()
	{
		if (fParameters == NULL || fCount <= 0)
			return BDriverParameter();

		fCount--;
		return BDriverParameter(fParameters++);
	}

private:
	const driver_parameter*	fParameters;
	int32					fCount;
};


class BDriverParameterContainer::NameIterator
	: public BDriverParameterIterator::Delegate {
public:
	NameIterator(const driver_parameter* parameters, int32 count,
		const BString& name)
		:
		Delegate(),
		fParameters(parameters),
		fCount(count),
		fName(name)
	{
		_FindNext(false);
	}

	virtual ~NameIterator()
	{
	}

	virtual Delegate* Clone() const
	{
		return new(std::nothrow) NameIterator(fParameters, fCount, fName);
	}

	virtual bool HasNext() const
	{
		return fParameters != NULL && fCount > 0;
	}

	virtual	BDriverParameter Next()
	{
		if (fParameters == NULL || fCount <= 0)
			return BDriverParameter();

		const driver_parameter* parameter = fParameters;
		_FindNext(true);
		return BDriverParameter(parameter);
	}

private:
	void _FindNext(bool skipCurrent)
	{
		if (fParameters == NULL || fCount < 1)
			return;
		if (skipCurrent) {
			fParameters++;
			fCount--;
		}
		while (fCount > 0 && fName != fParameters->name) {
			fParameters++;
			fCount--;
		}
	}

private:
	const driver_parameter*	fParameters;
	int32					fCount;
	BString					fName;
};


BDriverParameterContainer::BDriverParameterContainer()
{
}


BDriverParameterContainer::~BDriverParameterContainer()
{
}


int32
BDriverParameterContainer::CountParameters() const
{
	int32 count;
	return GetParametersAndCount(count) != NULL ? count : 0;

}


const driver_parameter*
BDriverParameterContainer::Parameters() const
{
	int32 count;
	return GetParametersAndCount(count);
}


BDriverParameter
BDriverParameterContainer::ParameterAt(int32 index) const
{
	int32 count;
	const driver_parameter* parameters = GetParametersAndCount(count);
	if (parameters == NULL || index < 0 || index >= count)
		return BDriverParameter();

	return BDriverParameter(parameters + index);
}


bool
BDriverParameterContainer::FindParameter(const char* name,
	BDriverParameter* _parameter) const
{
	if (name == NULL)
		return false;

	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(count)) {
		for (int32 i = 0; i < count; i++) {
			if (strcmp(name, parameters[i].name) == 0) {
				if (_parameter != NULL)
					_parameter->SetTo(parameters + i);
				return true;
			}
		}
	}
	return false;
}


BDriverParameter
BDriverParameterContainer::GetParameter(const char* name) const
{
	BDriverParameter parameter;
	FindParameter(name, &parameter);
	return parameter;
}


BDriverParameterIterator
BDriverParameterContainer::ParameterIterator() const
{
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(count)) {
		if (Iterator* iterator = new(std::nothrow) Iterator(parameters, count))
			return BDriverParameterIterator(iterator);
	}
	return BDriverParameterIterator();
}


BDriverParameterIterator
BDriverParameterContainer::ParameterIterator(const char* name) const
{
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(count)) {
		NameIterator* iterator
			= new(std::nothrow) NameIterator(parameters, count, name);
		if (iterator != NULL)
			return BDriverParameterIterator(iterator);
	}
	return BDriverParameterIterator();
}


const char*
BDriverParameterContainer::GetParameterValue(const char* name,
	const char* unknownValue, const char* noValue) const
{
	BDriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.ValueAt(0, noValue);
}


bool
BDriverParameterContainer::GetBoolParameterValue(const char* name,
	bool unknownValue, bool noValue) const
{
	BDriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.BoolValueAt(0, noValue);
}


int32
BDriverParameterContainer::GetInt32ParameterValue(const char* name,
	int32 unknownValue, int32 noValue) const
{
	BDriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.Int32ValueAt(0, noValue);
}


int64
BDriverParameterContainer::GetInt64ParameterValue(const char* name,
	int64 unknownValue, int64 noValue) const
{
	BDriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.Int64ValueAt(0, noValue);
}


// #pragma mark - BDriverSettings


BDriverSettings::BDriverSettings()
	:
	BDriverParameterContainer(),
	fSettingsHandle(NULL),
	fSettings(NULL)
{
}


BDriverSettings::~BDriverSettings()
{
	Unset();
}


status_t
BDriverSettings::Load(const char* driverNameOrAbsolutePath)
{
	Unset();

	fSettingsHandle = load_driver_settings(driverNameOrAbsolutePath);
	if (fSettingsHandle == NULL)
		return B_ENTRY_NOT_FOUND;

	fSettings = get_driver_settings(fSettingsHandle);
	if (fSettings == NULL) {
		Unset();
		return B_ERROR;
	}

	return B_OK;
}


status_t
BDriverSettings::Load(const entry_ref& ref)
{
	Unset();

	BPath path;
	status_t error = path.SetTo(&ref);
	return error == B_OK ? Load(path.Path()) : error;
}


status_t
BDriverSettings::SetToString(const char* string)
{
	Unset();

	fSettingsHandle = parse_driver_settings_string(string);
	if (fSettingsHandle == NULL)
		return B_BAD_DATA;

	fSettings = get_driver_settings(fSettingsHandle);
	if (fSettings == NULL) {
		Unset();
		return B_ERROR;
	}

	return B_OK;
}


void
BDriverSettings::Unset()
{
	if (fSettingsHandle != NULL)
		unload_driver_settings(fSettingsHandle);

	fSettingsHandle = NULL;
	fSettings = NULL;
}


const driver_parameter*
BDriverSettings::GetParametersAndCount(int32& _count) const
{
	if (fSettings == NULL)
		return NULL;

	_count = fSettings->parameter_count;
	return fSettings->parameters;
}


// #pragma mark - BDriverParameter


BDriverParameter::BDriverParameter()
	:
	BDriverParameterContainer(),
	fParameter(NULL)
{
}


BDriverParameter::BDriverParameter(const driver_parameter* parameter)
	:
	BDriverParameterContainer(),
	fParameter(parameter)
{
}


BDriverParameter::BDriverParameter(const BDriverParameter& other)
	:
	BDriverParameterContainer(),
	fParameter(other.fParameter)
{
}


BDriverParameter::~BDriverParameter()
{
}


void
BDriverParameter::SetTo(const driver_parameter* parameter)
{
	fParameter = parameter;
}


bool
BDriverParameter::IsValid() const
{
	return fParameter != NULL;
}


const char*
BDriverParameter::Name() const
{
	return fParameter != NULL ? fParameter->name : NULL;
}


int32
BDriverParameter::CountValues() const
{
	return fParameter != NULL ? fParameter->value_count : 0;
}


const char* const*
BDriverParameter::Values() const
{
	return fParameter != NULL ? fParameter->values : 0;
}


const char*
BDriverParameter::ValueAt(int32 index, const char* noValue) const
{
	if (fParameter == NULL || index < 0 || index >= fParameter->value_count)
		return noValue;
	return fParameter->values[index];
}


bool
BDriverParameter::BoolValueAt(int32 index, bool noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (value == NULL)
		return noValue;

	for (int32 i = 0; i < kTrueValueStringCount; i++) {
		if (strcmp(value, kTrueValueStrings[i]) == 0)
			return true;
	}
	return false;
}


int32
BDriverParameter::Int32ValueAt(int32 index, int32 noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (value == NULL)
		return noValue;
	return atol(value);
}


int64
BDriverParameter::Int64ValueAt(int32 index, int64 noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (value == NULL)
		return noValue;
	return strtoll(value, NULL, 10);
}


const driver_parameter*
BDriverParameter::GetParametersAndCount(int32& _count) const
{
	if (fParameter == NULL)
		return NULL;

	_count = fParameter->parameter_count;
	return fParameter->parameters;
}


BDriverParameter&
BDriverParameter::operator=(const BDriverParameter& other)
{
	fParameter = other.fParameter;
	return *this;
}


}	// namespace BPrivate
