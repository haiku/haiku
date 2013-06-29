/*
 * Copyright 2007-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _B_DRIVER_SETTINGS_H
#define _B_DRIVER_SETTINGS_H


#include <SupportDefs.h>


struct driver_parameter;
struct driver_settings;
struct entry_ref;


namespace BPrivate {


class BDriverParameter;
class BDriverParameterContainer;


class BDriverParameterIterator {
public:
								BDriverParameterIterator();
								BDriverParameterIterator(
									const BDriverParameterIterator& other);
								~BDriverParameterIterator();

			bool				HasNext() const;
			BDriverParameter	Next();

			BDriverParameterIterator& operator=(
									const BDriverParameterIterator& other);

private:
			friend class BDriverParameterContainer;
			class Delegate;

private:
								BDriverParameterIterator(Delegate* delegate);
			void				_SetTo(Delegate* delegate, bool addReference);

private:
			Delegate*			fDelegate;
};


class BDriverParameterContainer {
public:
								BDriverParameterContainer();
	virtual						~BDriverParameterContainer();

			int32				CountParameters() const;
			const driver_parameter*	Parameters() const;
			BDriverParameter	ParameterAt(int32 index) const;
			bool				FindParameter(const char* name,
									BDriverParameter* _parameter) const;
			BDriverParameter	GetParameter(const char* name) const;

			BDriverParameterIterator ParameterIterator() const;
			BDriverParameterIterator ParameterIterator(const char* name) const;

			const char*			GetParameterValue(const char* name,
									const char* unknownValue = NULL,
									const char* noValue = NULL) const;
			bool				GetBoolParameterValue(const char* name,
									bool unknownValue = false,
									bool noValue = false) const;
			int32				GetInt32ParameterValue(const char* name,
									int32 unknownValue = 0,
									int32 noValue = 0) const;
			int64				GetInt64ParameterValue(const char* name,
									int64 unknownValue = 0,
									int64 noValue = 0) const;

protected:
	virtual	const driver_parameter* GetParametersAndCount(int32& _count) const
									= 0;

private:
			class Iterator;
			class NameIterator;
};


class BDriverSettings : public BDriverParameterContainer {
public:
								BDriverSettings();
	virtual						~BDriverSettings();

			status_t			Load(const char* driverNameOrAbsolutePath);
			status_t			Load(const entry_ref& ref);
			status_t			SetToString(const char* string);
			void				Unset();

protected:
	virtual	const driver_parameter* GetParametersAndCount(int32& _count) const;

private:
			void*				fSettingsHandle;
			const driver_settings* fSettings;
};


class BDriverParameter : public BDriverParameterContainer {
public:
								BDriverParameter();
								BDriverParameter(
									const driver_parameter* parameter);
								BDriverParameter(const BDriverParameter& other);
	virtual						~BDriverParameter();

			void				SetTo(const driver_parameter* parameter);

			bool				IsValid() const;

			const char*			Name() const;
			int32				CountValues() const;
			const char* const*	Values() const;
			const char*			ValueAt(int32 index,
									const char* noValue = NULL) const;
			bool				BoolValueAt(int32 index,
									bool noValue = false) const;
			int32				Int32ValueAt(int32 index,
									int32 noValue = 0) const;
			int64				Int64ValueAt(int32 index,
									int64 noValue = 0) const;

			BDriverParameter&	operator=(const BDriverParameter& other);

protected:
	virtual	const driver_parameter* GetParametersAndCount(int32& _count) const;

private:
			const driver_parameter*	fParameter;
};


}	// namespace BPrivate


using BPrivate::BDriverParameterIterator;
using BPrivate::BDriverSettings;
using BPrivate::BDriverParameter;


#endif	// _B_DRIVER_SETTINGS_H
