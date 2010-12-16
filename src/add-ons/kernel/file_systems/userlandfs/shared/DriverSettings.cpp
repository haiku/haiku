// DriverSettings.cpp

#include <new>
#include <stdlib.h>
#include <string.h>

#include <driver_settings.h>

#include <Referenceable.h>

#include "DriverSettings.h"
#include "String.h"

// The parameter values that shall be evaluated to true.
static const char* kTrueValueStrings[]
	= { "1", "true", "yes", "on", "enable", "enabled" };
static const int32 kTrueValueStringCount
	= sizeof(kTrueValueStrings) / sizeof(const char*);


// #pragma mark -
// #pragma mark ----- DriverParameterIterator -----

// Delegate
class DriverParameterIterator::Delegate : public BReferenceable {
public:
								Delegate() : BReferenceable() {}
	virtual						~Delegate() {}

	virtual	Delegate*			Clone() const = 0;

	virtual	bool				HasNext() const = 0;
	virtual	bool				GetNext(DriverParameter* parameter) = 0;
};

// constructor
DriverParameterIterator::DriverParameterIterator()
	: fDelegate(NULL)
{
}

// constructor
DriverParameterIterator::DriverParameterIterator(Delegate* delegate)
	: fDelegate(delegate)
{
}

// copy constructor
DriverParameterIterator::DriverParameterIterator(
	const DriverParameterIterator& other)
	: fDelegate(NULL)
{
	_SetTo(other.fDelegate, true);
}

// destructor
DriverParameterIterator::~DriverParameterIterator()
{
	_SetTo(NULL, false);
}

// HasNext
bool
DriverParameterIterator::HasNext() const
{
	return (fDelegate ? fDelegate->HasNext() : false);
}

// GetNext
bool
DriverParameterIterator::GetNext(DriverParameter* parameter)
{
	if (!fDelegate)
		return false;
	if (fDelegate->CountReferences() > 1) {
		Delegate* clone = fDelegate->Clone();
		if (!clone)
			return false;
		_SetTo(clone, false);
	}
	return fDelegate->GetNext(parameter);
}

// =
DriverParameterIterator&
DriverParameterIterator::operator=(const DriverParameterIterator& other)
{
	_SetTo(other.fDelegate, true);
	return *this;
}

// _SetTo
void
DriverParameterIterator::_SetTo(Delegate* delegate, bool addReference)
{
	if (fDelegate)
		fDelegate->ReleaseReference();
	fDelegate = delegate;
	if (fDelegate && addReference)
		fDelegate->AcquireReference();
}


// #pragma mark -
// #pragma mark ----- DriverParameterContainer -----

// Iterator
class DriverParameterContainer::Iterator
	: public DriverParameterIterator::Delegate {
public:
	Iterator(const driver_parameter* parameters, int32 count)
		: Delegate(),
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
		return (fParameters && fCount > 0);
	}

	virtual bool GetNext(DriverParameter* parameter)
	{
		if (fParameters && fCount > 0) {
			if (parameter)
				parameter->SetTo(fParameters);
			fParameters++;
			fCount--;
			return true;
		}
		return false;
	}

private:
	const driver_parameter*	fParameters;
	int32					fCount;
};

// NameIterator
class DriverParameterContainer::NameIterator
	: public DriverParameterIterator::Delegate {
public:
	NameIterator(const driver_parameter* parameters, int32 count,
		const char* name)
		: Delegate(),
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
		return new(std::nothrow) NameIterator(fParameters, fCount,
			fName.GetString());
	}

	virtual bool HasNext() const
	{
		return (fParameters && fCount > 0);
	}

	virtual bool GetNext(DriverParameter* parameter)
	{
		if (fParameters && fCount > 0) {
			if (parameter)
				parameter->SetTo(fParameters);
			_FindNext(true);
			return true;
		}
		return false;
	}

private:
	void _FindNext(bool skipCurrent)
	{
		if (!fParameters || fCount < 1)
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
	String					fName;
};

// constructor
DriverParameterContainer::DriverParameterContainer()
{
}

// destructor
DriverParameterContainer::~DriverParameterContainer()
{
}

// CountParameters
int32
DriverParameterContainer::CountParameters() const
{
	int32 count;
	return (GetParametersAndCount(&count) ? count : 0);

}

// GetParameters
const driver_parameter*
DriverParameterContainer::GetParameters() const
{
	int32 count;
	return GetParametersAndCount(&count);
}

// GetParameterAt
bool
DriverParameterContainer::GetParameterAt(int32 index,
	DriverParameter* parameter) const
{
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(&count)) {
		if (index >= 0 && index < count) {
			if (parameter)
				parameter->SetTo(parameters + index);
			return true;
		}
	}
	return false;
}

// FindParameter
bool
DriverParameterContainer::FindParameter(const char* name,
	DriverParameter* parameter) const
{
	if (!name)
		return false;
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(&count)) {
		for (int32 i = 0; i < count; i++) {
			if (strcmp(name, parameters[i].name) == 0) {
				if (parameter)
					parameter->SetTo(parameters + i);
				return true;
			}
		}
	}
	return false;
}

// GetParameterIterator
DriverParameterIterator
DriverParameterContainer::GetParameterIterator() const
{
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(&count)) {
		if (Iterator* iterator = new(std::nothrow) Iterator(parameters, count))
			return DriverParameterIterator(iterator);
	}
	return DriverParameterIterator();
}

// GetParameterIterator
DriverParameterIterator
DriverParameterContainer::GetParameterIterator(const char* name) const
{
	int32 count;
	if (const driver_parameter* parameters = GetParametersAndCount(&count)) {
		NameIterator* iterator = new(std::nothrow) NameIterator(parameters, count,
			name);
		if (iterator)
			return DriverParameterIterator(iterator);
	}
	return DriverParameterIterator();
}

// GetParameterValue
const char*
DriverParameterContainer::GetParameterValue(const char* name,
	const char* unknownValue, const char* noValue) const
{
	DriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.ValueAt(0, noValue);
}

// GetBoolParameterValue
bool
DriverParameterContainer::GetBoolParameterValue(const char* name,
	bool unknownValue, bool noValue) const
{
	DriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.BoolValueAt(0, noValue);
}

// GetInt32ParameterValue
int32
DriverParameterContainer::GetInt32ParameterValue(const char* name,
	int32 unknownValue, int32 noValue) const
{
	DriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.Int32ValueAt(0, noValue);
}

// GetInt64ParameterValue
int64
DriverParameterContainer::GetInt64ParameterValue(const char* name,
	int64 unknownValue, int64 noValue) const
{
	DriverParameter parameter;
	if (!FindParameter(name, &parameter))
		return unknownValue;
	return parameter.Int64ValueAt(0, noValue);
}


// #pragma mark -
// #pragma mark ----- DriverSettings -----

// constructor
DriverSettings::DriverSettings()
	: DriverParameterContainer(),
	  fSettingsHandle(NULL),
	  fSettings(NULL)
{
}

// destructor
DriverSettings::~DriverSettings()
{
	Unset();
}

// Load
status_t
DriverSettings::Load(const char* driverName)
{
	Unset();
	fSettingsHandle = load_driver_settings(driverName);
	if (!fSettingsHandle)
		return B_ENTRY_NOT_FOUND;
	fSettings = get_driver_settings(fSettingsHandle);
	if (!fSettings) {
		Unset();
		return B_ERROR;
	}
	return B_OK;
}

// Unset
void
DriverSettings::Unset()
{
	if (fSettingsHandle)
		unload_driver_settings(fSettingsHandle);
	fSettingsHandle = NULL;
	fSettings = NULL;
}

// GetParametersAndCount
const driver_parameter*
DriverSettings::GetParametersAndCount(int32* count) const
{
	if (!fSettings)
		return NULL;
	*count = fSettings->parameter_count;
	return fSettings->parameters;
}


// #pragma mark -
// #pragma mark ----- DriverParameter -----

// constructor
DriverParameter::DriverParameter()
	: DriverParameterContainer(),
	  fParameter(NULL)
{
}

// destructor
DriverParameter::~DriverParameter()
{
}

// SetTo
void
DriverParameter::SetTo(const driver_parameter* parameter)
{
	fParameter = parameter;
}

// GetName
const char*
DriverParameter::GetName() const
{
	return (fParameter ? fParameter->name : NULL);
}

// CountValues
int32
DriverParameter::CountValues() const
{
	return (fParameter ? fParameter->value_count : 0);
}

// GetValues
const char* const*
DriverParameter::GetValues() const
{
	return (fParameter ? fParameter->values : 0);
}

// ValueAt
const char*
DriverParameter::ValueAt(int32 index, const char* noValue) const
{
	if (!fParameter || index < 0 || index >= fParameter->value_count)
		return noValue;
	return fParameter->values[index];
}

// BoolValueAt
bool
DriverParameter::BoolValueAt(int32 index, bool noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (!value)
		return noValue;
	for (int32 i = 0; i < kTrueValueStringCount; i++) {
		if (strcmp(value, kTrueValueStrings[i]) == 0)
			return true;
	}
	return false;
}

// Int32ValueAt
int32
DriverParameter::Int32ValueAt(int32 index, int32 noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (!value)
		return noValue;
	return atol(value);
}

// Int64ValueAt
int64
DriverParameter::Int64ValueAt(int32 index, int64 noValue) const
{
	const char* value = ValueAt(index, NULL);
	if (!value)
		return noValue;
	return strtoll(value, NULL, 10);
}

// GetParametersAndCount
const driver_parameter*
DriverParameter::GetParametersAndCount(int32* count) const
{
	if (!fParameter)
		return NULL;
	*count = fParameter->parameter_count;
	return fParameter->parameters;
}

