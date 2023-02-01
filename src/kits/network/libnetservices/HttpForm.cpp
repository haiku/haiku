/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <HttpForm.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

#include <File.h>
#include <NodeInfo.h>
#include <TypeConstants.h>
#include <Url.h>


static int32 kBoundaryRandomSize = 16;

using namespace std;
using namespace BPrivate::Network;


// #pragma mark - BHttpFormData


BHttpFormData::BHttpFormData()
	:
	fDataType(B_HTTPFORM_STRING),
	fCopiedBuffer(false),
	fFileMark(false),
	fBufferValue(NULL),
	fBufferSize(0)
{
}


BHttpFormData::BHttpFormData(const BString& name, const BString& value)
	:
	fDataType(B_HTTPFORM_STRING),
	fCopiedBuffer(false),
	fFileMark(false),
	fName(name),
	fStringValue(value),
	fBufferValue(NULL),
	fBufferSize(0)
{
}


BHttpFormData::BHttpFormData(const BString& name, const BPath& file)
	:
	fDataType(B_HTTPFORM_FILE),
	fCopiedBuffer(false),
	fFileMark(false),
	fName(name),
	fPathValue(file),
	fBufferValue(NULL),
	fBufferSize(0)
{
}


BHttpFormData::BHttpFormData(const BString& name, const void* buffer,
	ssize_t size)
	:
	fDataType(B_HTTPFORM_BUFFER),
	fCopiedBuffer(false),
	fFileMark(false),
	fName(name),
	fBufferValue(buffer),
	fBufferSize(size)
{
}


BHttpFormData::BHttpFormData(const BHttpFormData& other)
	:
	fCopiedBuffer(false),
	fFileMark(false),
	fBufferValue(NULL),
	fBufferSize(0)
{
	*this = other;
}


BHttpFormData::~BHttpFormData()
{
	if (fCopiedBuffer)
		delete[] reinterpret_cast<const char*>(fBufferValue);
}


// #pragma mark - Retrieve data informations


bool
BHttpFormData::InitCheck() const
{
	if (fDataType == B_HTTPFORM_BUFFER)
		return fBufferValue != NULL;

	return true;
}


const BString&
BHttpFormData::Name() const
{
	return fName;
}


const BString&
BHttpFormData::String() const
{
	return fStringValue;
}


const BPath&
BHttpFormData::File() const
{
	return fPathValue;
}


const void*
BHttpFormData::Buffer() const
{
	return fBufferValue;
}


ssize_t
BHttpFormData::BufferSize() const
{
	return fBufferSize;
}


bool
BHttpFormData::IsFile() const
{
	return fFileMark;
}


const BString&
BHttpFormData::Filename() const
{
	return fFilename;
}


const BString&
BHttpFormData::MimeType() const
{
	return fMimeType;
}


form_content_type
BHttpFormData::Type() const
{
	return fDataType;
}


// #pragma mark - Change behavior


status_t
BHttpFormData::MarkAsFile(const BString& filename, const BString& mimeType)
{
	if (fDataType == B_HTTPFORM_UNKNOWN || fDataType == B_HTTPFORM_FILE)
		return B_ERROR;

	fFilename = filename;
	fMimeType = mimeType;
	fFileMark = true;

	return B_OK;
}


void
BHttpFormData::UnmarkAsFile()
{
	fFilename.Truncate(0, true);
	fMimeType.Truncate(0, true);
	fFileMark = false;
}


status_t
BHttpFormData::CopyBuffer()
{
	if (fDataType != B_HTTPFORM_BUFFER)
		return B_ERROR;

	char* copiedBuffer = new(std::nothrow) char[fBufferSize];
	if (copiedBuffer == NULL)
		return B_NO_MEMORY;

	memcpy(copiedBuffer, fBufferValue, fBufferSize);
	fBufferValue = copiedBuffer;
	fCopiedBuffer = true;

	return B_OK;
}


BHttpFormData&
BHttpFormData::operator=(const BHttpFormData& other)
{
	fDataType = other.fDataType;
	fCopiedBuffer = false;
	fFileMark = other.fFileMark;
	fName = other.fName;
	fStringValue = other.fStringValue;
	fPathValue = other.fPathValue;
	fBufferValue = other.fBufferValue;
	fBufferSize = other.fBufferSize;
	fFilename = other.fFilename;
	fMimeType = other.fMimeType;

	if (other.fCopiedBuffer)
		CopyBuffer();

	return *this;
}


// #pragma mark - BHttpForm


BHttpForm::BHttpForm()
	:
	fType(B_HTTP_FORM_URL_ENCODED)
{
}


BHttpForm::BHttpForm(const BHttpForm& other)
	:
	fFields(other.fFields),
	fType(other.fType),
	fMultipartBoundary(other.fMultipartBoundary)
{
}


BHttpForm::BHttpForm(const BString& formString)
	:
	fType(B_HTTP_FORM_URL_ENCODED)
{
	ParseString(formString);
}


BHttpForm::~BHttpForm()
{
	Clear();
}


// #pragma mark - Form string parsing


void
BHttpForm::ParseString(const BString& formString)
{
	int32 index = 0;

	while (index < formString.Length())
		_ExtractNameValuePair(formString, &index);
}


BString
BHttpForm::RawData() const
{
	BString result;

	if (fType == B_HTTP_FORM_URL_ENCODED) {
		for (FormStorage::const_iterator it = fFields.begin();
			it != fFields.end(); it++) {
			const BHttpFormData* currentField = &it->second;

			switch (currentField->Type()) {
				case B_HTTPFORM_UNKNOWN:
					break;

				case B_HTTPFORM_STRING:
					result << '&' << BUrl::UrlEncode(currentField->Name())
						<< '=' << BUrl::UrlEncode(currentField->String());
					break;

				case B_HTTPFORM_FILE:
					break;

				case B_HTTPFORM_BUFFER:
					// Send the buffer only if its not marked as a file
					if (!currentField->IsFile()) {
						result << '&' << BUrl::UrlEncode(currentField->Name())
							<< '=';
						result.Append(
							reinterpret_cast<const char*>(currentField->Buffer()),
							currentField->BufferSize());
					}
					break;
			}
		}

		result.Remove(0, 1);
	} else if (fType == B_HTTP_FORM_MULTIPART) {
		//  Very slow and memory consuming method since we're caching the
		// file content, this should be preferably handled by the protocol
		for (FormStorage::const_iterator it = fFields.begin();
			it != fFields.end(); it++) {
			const BHttpFormData* currentField = &it->second;
			result << _GetMultipartHeader(currentField);

			switch (currentField->Type()) {
				case B_HTTPFORM_UNKNOWN:
					break;

				case B_HTTPFORM_STRING:
					result << currentField->String();
					break;

				case B_HTTPFORM_FILE:
				{
					BFile upFile(currentField->File().Path(), B_READ_ONLY);
					char readBuffer[1024];
					ssize_t readSize;

					readSize = upFile.Read(readBuffer, 1024);

					while (readSize > 0) {
						result.Append(readBuffer, readSize);
						readSize = upFile.Read(readBuffer, 1024);
					}
					break;
				}

				case B_HTTPFORM_BUFFER:
					result.Append(
						reinterpret_cast<const char*>(currentField->Buffer()),
						currentField->BufferSize());
					break;
			}

			result << "\r\n";
		}

		result << "--" << fMultipartBoundary << "--\r\n";
	}

	return result;
}


// #pragma mark - Form add


status_t
BHttpForm::AddString(const BString& fieldName, const BString& value)
{
	BHttpFormData formData(fieldName, value);
	if (!formData.InitCheck())
		return B_ERROR;

	fFields.insert(pair<BString, BHttpFormData>(fieldName, formData));
	return B_OK;
}


status_t
BHttpForm::AddInt(const BString& fieldName, int32 value)
{
	BString strValue;
	strValue << value;

	return AddString(fieldName, strValue);
}


status_t
BHttpForm::AddFile(const BString& fieldName, const BPath& file)
{
	BHttpFormData formData(fieldName, file);
	if (!formData.InitCheck())
		return B_ERROR;

	fFields.insert(pair<BString, BHttpFormData>(fieldName, formData));

	if (fType != B_HTTP_FORM_MULTIPART)
		SetFormType(B_HTTP_FORM_MULTIPART);
	return B_OK;
}


status_t
BHttpForm::AddBuffer(const BString& fieldName, const void* buffer,
	ssize_t size)
{
	BHttpFormData formData(fieldName, buffer, size);
	if (!formData.InitCheck())
		return B_ERROR;

	fFields.insert(pair<BString, BHttpFormData>(fieldName, formData));
	return B_OK;
}


status_t
BHttpForm::AddBufferCopy(const BString& fieldName, const void* buffer,
	ssize_t size)
{
	BHttpFormData formData(fieldName, buffer, size);
	if (!formData.InitCheck())
		return B_ERROR;

	// Copy the buffer of the inserted form data copy to
	// avoid an unneeded copy of the buffer upon insertion
	pair<FormStorage::iterator, bool> insertResult
		= fFields.insert(pair<BString, BHttpFormData>(fieldName, formData));

	return insertResult.first->second.CopyBuffer();
}


// #pragma mark - Mark a field as a filename


void
BHttpForm::MarkAsFile(const BString& fieldName, const BString& filename,
	const BString& mimeType)
{
	FormStorage::iterator it = fFields.find(fieldName);

	if (it == fFields.end())
		return;

	it->second.MarkAsFile(filename, mimeType);
	if (fType != B_HTTP_FORM_MULTIPART)
		SetFormType(B_HTTP_FORM_MULTIPART);
}


void
BHttpForm::MarkAsFile(const BString& fieldName, const BString& filename)
{
	MarkAsFile(fieldName, filename, "");
}


void
BHttpForm::UnmarkAsFile(const BString& fieldName)
{
	FormStorage::iterator it = fFields.find(fieldName);

	if (it == fFields.end())
		return;

	it->second.UnmarkAsFile();
}


// #pragma mark - Change form type


void
BHttpForm::SetFormType(form_type type)
{
	fType = type;

	if (fType == B_HTTP_FORM_MULTIPART)
		_GenerateMultipartBoundary();
}


// #pragma mark - Form test


bool
BHttpForm::HasField(const BString& name) const
{
	return (fFields.find(name) != fFields.end());
}


// #pragma mark - Form retrieve


BString
BHttpForm::GetMultipartHeader(const BString& fieldName) const
{
	FormStorage::const_iterator it = fFields.find(fieldName);

	if (it == fFields.end())
		return BString("");

	return _GetMultipartHeader(&it->second);
}


form_type
BHttpForm::GetFormType() const
{
	return fType;
}


const BString&
BHttpForm::GetMultipartBoundary() const
{
	return fMultipartBoundary;
}


BString
BHttpForm::GetMultipartFooter() const
{
	BString result = "--";
	result << fMultipartBoundary << "--\r\n";
	return result;
}


ssize_t
BHttpForm::ContentLength() const
{
	if (fType == B_HTTP_FORM_URL_ENCODED)
		return RawData().Length();

	ssize_t contentLength = 0;

	for (FormStorage::const_iterator it = fFields.begin();
		it != fFields.end(); it++) {
		const BHttpFormData* c = &it->second;
		contentLength += _GetMultipartHeader(c).Length();

		switch (c->Type()) {
			case B_HTTPFORM_UNKNOWN:
				break;

			case B_HTTPFORM_STRING:
				contentLength += c->String().Length();
				break;

			case B_HTTPFORM_FILE:
			{
				BFile upFile(c->File().Path(), B_READ_ONLY);
				upFile.Seek(0, SEEK_END);
				contentLength += upFile.Position();
				break;
			}

			case B_HTTPFORM_BUFFER:
				contentLength += c->BufferSize();
				break;
		}

		contentLength += 2;
	}

	contentLength += fMultipartBoundary.Length() + 6;

	return contentLength;
}


// #pragma mark Form iterator


BHttpForm::Iterator
BHttpForm::GetIterator()
{
	return BHttpForm::Iterator(this);
}


// #pragma mark - Form clear


void
BHttpForm::Clear()
{
	fFields.clear();
}


// #pragma mark - Overloaded operators


BHttpFormData&
BHttpForm::operator[](const BString& name)
{
	if (!HasField(name))
		AddString(name, "");

	return fFields[name];
}


void
BHttpForm::_ExtractNameValuePair(const BString& formString, int32* index)
{
	// Look for a name=value pair
	int16 firstAmpersand = formString.FindFirst("&", *index);
	int16 firstEqual = formString.FindFirst("=", *index);

	BString name;
	BString value;

	if (firstAmpersand == -1) {
		if (firstEqual != -1) {
			formString.CopyInto(name, *index, firstEqual - *index);
			formString.CopyInto(value, firstEqual + 1,
				formString.Length() - firstEqual - 1);
		} else
			formString.CopyInto(value, *index,
				formString.Length() - *index);

		*index = formString.Length() + 1;
	} else {
		if (firstEqual != -1 && firstEqual < firstAmpersand) {
			formString.CopyInto(name, *index, firstEqual - *index);
			formString.CopyInto(value, firstEqual + 1,
				firstAmpersand - firstEqual - 1);
		} else
			formString.CopyInto(value, *index, firstAmpersand - *index);

		*index = firstAmpersand + 1;
	}

	AddString(name, value);
}


void
BHttpForm::_GenerateMultipartBoundary()
{
	fMultipartBoundary = "----------------------------";

	srand(time(NULL));
		// TODO: Maybe a more robust way to seed the random number
		// generator is needed?

	for (int32 i = 0; i < kBoundaryRandomSize; i++)
		fMultipartBoundary << (char)(rand() % 10 + '0');
}


// #pragma mark - Field information access by std iterator


BString
BHttpForm::_GetMultipartHeader(const BHttpFormData* element) const
{
	BString result;
	result << "--" << fMultipartBoundary << "\r\n";
	result << "Content-Disposition: form-data; name=\"" << element->Name()
		<< '"';

	switch (element->Type()) {
		case B_HTTPFORM_UNKNOWN:
			break;

		case B_HTTPFORM_FILE:
		{
			result << "; filename=\"" << element->File().Leaf() << '"';

			BNode fileNode(element->File().Path());
			BNodeInfo fileInfo(&fileNode);

			result << "\r\nContent-Type: ";
			char tempMime[128];
			if (fileInfo.GetType(tempMime) == B_OK)
				result << tempMime;
			else
				result << "application/octet-stream";

			break;
		}

		case B_HTTPFORM_STRING:
		case B_HTTPFORM_BUFFER:
			if (element->IsFile()) {
				result << "; filename=\"" << element->Filename() << '"';

				if (element->MimeType().Length() > 0)
					result << "\r\nContent-Type: " << element->MimeType();
				else
					result << "\r\nContent-Type: text/plain";
			}
			break;
	}

	result << "\r\n\r\n";

	return result;
}


// #pragma mark - Iterator


BHttpForm::Iterator::Iterator(BHttpForm* form)
	:
	fElement(NULL)
{
	fForm = form;
	fStdIterator = form->fFields.begin();
	_FindNext();
}


BHttpForm::Iterator::Iterator(const Iterator& other)
{
	*this = other;
}


bool
BHttpForm::Iterator::HasNext() const
{
	return fStdIterator != fForm->fFields.end();
}


BHttpFormData*
BHttpForm::Iterator::Next()
{
	BHttpFormData* element = fElement;
	_FindNext();
	return element;
}


void
BHttpForm::Iterator::Remove()
{
	fForm->fFields.erase(fStdIterator);
	fElement = NULL;
}


BString
BHttpForm::Iterator::MultipartHeader()
{
	return fForm->_GetMultipartHeader(fPrevElement);
}


BHttpForm::Iterator&
BHttpForm::Iterator::operator=(const Iterator& other)
{
	fForm = other.fForm;
	fStdIterator = other.fStdIterator;
	fElement = other.fElement;
	fPrevElement = other.fPrevElement;

	return *this;
}


void
BHttpForm::Iterator::_FindNext()
{
	fPrevElement = fElement;

	if (fStdIterator != fForm->fFields.end()) {
		fElement = &fStdIterator->second;
		fStdIterator++;
	} else
		fElement = NULL;
}
