/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_STRING_BUILDER_H
#define PACKAGE_INFO_STRING_BUILDER_H


#include <ctype.h>

#include <DataIO.h>
#include <package/PackageInfo.h>


namespace BPackageKit {


struct BPackageInfo::StringBuilder {
	StringBuilder()
		:
		fData(),
		fError(B_OK),
		fBasePackage()
	{
	}

	status_t Error() const
	{
		return fError;
	}

	status_t GetString(BString& _string) const
	{
		if (fError != B_OK) {
			_string = BString();
			return fError;
		}

		_string.SetTo((const char*)fData.Buffer(), fData.BufferLength());
		return (size_t)_string.Length() == fData.BufferLength()
			? B_OK : B_NO_MEMORY;
	}

	template<typename Value>
	StringBuilder& Write(const char* attribute, Value value)
	{
		if (_IsValueEmpty(value))
			return *this;

		_Write(attribute);
		_Write('\t');
		_WriteValue(value);
		_Write('\n');
		return *this;
	}

	StringBuilder& WriteFlags(const char* attribute, uint32 flags)
	{
		if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0
			&& (flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) == 0) {
			return *this;
		}

		_Write(attribute);
		_Write('\t');

		if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0)
			_Write(" approve_license");
		if ((flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) == 0)
			_Write(" system_package");

		_Write('\n');
		return *this;
	}

	StringBuilder& BeginRequires(const BString& basePackage)
	{
		fBasePackage = basePackage;
		return *this;
	}

	StringBuilder& EndRequires()
	{
		fBasePackage.Truncate(0);
		return *this;
	}

private:
	void _WriteValue(const char* value)
	{
		_WriteMaybeQuoted(value);
	}

	void _WriteValue(const BPackageVersion& value)
	{
		if (fError != B_OK)
			return;

		if (value.InitCheck() != B_OK) {
			fError = B_BAD_VALUE;
			return;
		}

		_Write(value.ToString());
	}

	void _WriteValue(const BStringList& value)
	{
		int32 count = value.CountStrings();
		if (count == 1) {
			_WriteMaybeQuoted(value.StringAt(0));
		} else {
			_Write("{\n", 2);

			int32 count = value.CountStrings();
			for (int32 i = 0; i < count; i++) {
				_Write('\t');
				_WriteMaybeQuoted(value.StringAt(i));
				_Write('\n');
			}

			_Write('}');
		}
	}

	template<typename Value>
	void _WriteValue(const BObjectList<Value>& value)
	{
		// Note: The fBasePackage solution is disgusting, but any attempt of
		// encapsulating the stringification via templates seems to result in
		// an Internal Compiler Error with gcc 2.

		_Write("{\n", 2);

		int32 count = value.CountItems();
		for (int32 i = 0; i < count; i++) {
			_Write('\t');
			_WriteListElement(value.ItemAt(i));
			_Write('\n');
		}

		_Write('}');
	}

	template<typename Value>
	void _WriteListElement(const Value* value)
	{
		_Write(value->ToString());
		if (!fBasePackage.IsEmpty()
			&& value->Name() == fBasePackage) {
			_Write(" base");
		}
	}

	void _WriteListElement(const BGlobalWritableFileInfo* value)
	{
		_WriteMaybeQuoted(value->Path());
		if (value->IsDirectory()) {
			_Write(' ');
			_Write("directory");
		}
		if (value->IsIncluded()) {
			_Write(' ');
			_Write(kWritableFileUpdateTypes[value->UpdateType()]);
		}
	}

	void _WriteListElement(const BUserSettingsFileInfo* value)
	{
		_WriteMaybeQuoted(value->Path());
		if (value->IsDirectory()) {
			_Write(" directory");
		} else if (!value->TemplatePath().IsEmpty()) {
			_Write(" template ");
			_WriteMaybeQuoted(value->TemplatePath());
		}
	}

	void _WriteListElement(const BUser* value)
	{
		_WriteMaybeQuoted(value->Name());

		if (!value->RealName().IsEmpty()) {
			_Write(" real-name ");
			_WriteMaybeQuoted(value->RealName());
		}

		if (!value->Home().IsEmpty()) {
			_Write(" home ");
			_WriteMaybeQuoted(value->Home());
		}

		if (!value->Shell().IsEmpty()) {
			_Write(" shell ");
			_WriteMaybeQuoted(value->Shell());
		}

		if (!value->Groups().IsEmpty()) {
			_Write(" groups ");
			BString groups = value->Groups().Join(" ");
			if (groups.IsEmpty()) {
				if (fError == B_OK)
					fError = B_NO_MEMORY;
				return;
			}
			_Write(groups);
		}
	}

	static inline bool _IsValueEmpty(const char* value)
	{
		return value[0] == '\0';
	}

	static inline bool _IsValueEmpty(const BPackageVersion& value)
	{
		return false;
	}

	template<typename List>
	static inline bool _IsValueEmpty(const List& value)
	{
		return value.IsEmpty();
	}

	void _WriteMaybeQuoted(const char* data)
	{
		// check whether quoting is needed
		bool needsQuoting = false;
		bool needsEscaping = false;
		for (const char* it = data; *it != '\0'; it++) {
			if (isalnum(*it) || *it == '.' || *it == '-' || *it == '_'
				|| *it == ':' || *it == '+') {
				continue;
			}

			needsQuoting = true;

			if (*it == '\t' || *it == '\n' || *it == '"' || *it == '\\') {
				needsEscaping = true;
				break;
			}
		}

		if (!needsQuoting) {
			_Write(data);
			return;
		}

		// we need quoting
		_Write('"');

		// escape the string, if necessary
		if (needsEscaping) {
			const char* start = data;
			const char* end = data;
			while (*end != '\0') {
				char replacement[2];
				switch (*end) {
					case '\t':
						replacement[1] = 't';
						break;
					case '\n':
						replacement[1] = 'n';
						break;
					case '"':
					case '\\':
						replacement[1] = *end;
						break;
					default:
						end++;
						continue;
				}

				if (start < end)
					_Write(start, end - start);

				replacement[0] = '\\';
				_Write(replacement, 2);
				start = ++end;
			}

			if (start < end)
				_Write(start, end - start);
		} else
			_Write(data);

		_Write('"');
	}

	inline void _Write(char data)
	{
		_Write(&data, 1);
	}

	inline void _Write(const char* data)
	{
		_Write(data, strlen(data));
	}

	inline void _Write(const BString& data)
	{
		_Write(data, data.Length());
	}

	void _Write(const void* data, size_t size)
	{
		if (fError == B_OK) {
			ssize_t bytesWritten = fData.Write(data, size);
			if (bytesWritten < 0)
				fError = bytesWritten;
		}
	}

private:
	BMallocIO	fData;
	status_t	fError;
	BString		fBasePackage;
};


} // namespace BPackageKit


#endif	// PACKAGE_INFO_STRING_BUILDER_H
