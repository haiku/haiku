// DriverSettings.h

#ifndef USERLAND_FS_DRIVER_SETTINGS_H
#define USERLAND_FS_DRIVER_SETTINGS_H

struct driver_parameter;
struct driver_settings;

namespace UserlandFSUtil {

class DriverParameter;
class DriverParameterContainer;

// DriverParameterIterator
class DriverParameterIterator {
public:
								DriverParameterIterator();
								DriverParameterIterator(
									const DriverParameterIterator& other);
								~DriverParameterIterator();

			bool				HasNext() const;
			bool				GetNext(DriverParameter* parameter);

			DriverParameterIterator& operator=(
									const DriverParameterIterator& other);

private:
			friend class DriverParameterContainer;
			class Delegate;

								DriverParameterIterator(Delegate* delegate);
					void		_SetTo(Delegate* delegate, bool addReference);

		Delegate*				fDelegate;
};

// DriverParameterContainer
class DriverParameterContainer {
public:
								DriverParameterContainer();
	virtual						~DriverParameterContainer();

			int32				CountParameters() const;
			const driver_parameter*	GetParameters() const;
			bool				GetParameterAt(int32 index,
									DriverParameter* parameter) const;
			bool				FindParameter(const char* name,
									DriverParameter* parameter) const;

			DriverParameterIterator	GetParameterIterator() const;
			DriverParameterIterator	GetParameterIterator(
									const char* name) const;

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
	virtual	const driver_parameter*
								GetParametersAndCount(int32* count) const = 0;

private:
			class Iterator;
			class NameIterator;
};

// DriverSettings
class DriverSettings : public DriverParameterContainer {
public:
								DriverSettings();
	virtual						~DriverSettings();

			status_t			Load(const char* driverName);
			void				Unset();

protected:
	virtual	const driver_parameter*
								GetParametersAndCount(int32* count) const;

private:
			void*				fSettingsHandle;
			const driver_settings*	fSettings;
};

// DriverParameter
class DriverParameter : public DriverParameterContainer {
public:
								DriverParameter();
	virtual						~DriverParameter();

			void				SetTo(const driver_parameter* parameter);

			const char*			GetName() const;
			int32				CountValues() const;
			const char* const*	GetValues() const;
			const char*			ValueAt(int32 index,
									const char* noValue = NULL) const;
			bool				BoolValueAt(int32 index,
									bool noValue = false) const;
			int32				Int32ValueAt(int32 index,
									int32 noValue = 0) const;
			int64				Int64ValueAt(int32 index,
									int64 noValue = 0) const;
			
protected:
	virtual	const driver_parameter*
								GetParametersAndCount(int32* count) const;

private:
			const driver_parameter*	fParameter;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::DriverParameterIterator;
using UserlandFSUtil::DriverParameterContainer;
using UserlandFSUtil::DriverSettings;
using UserlandFSUtil::DriverParameter;

#endif	// USERLAND_FS_DRIVER_SETTINGS_H
