/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_FORM_H_
#define _B_HTTP_FORM_H_


#include <Path.h>
#include <String.h>

#include <map>


namespace BPrivate {

namespace Network {


enum form_type {
	B_HTTP_FORM_URL_ENCODED,
	B_HTTP_FORM_MULTIPART
};


enum form_content_type {
	B_HTTPFORM_UNKNOWN,
	B_HTTPFORM_STRING,
	B_HTTPFORM_FILE,
	B_HTTPFORM_BUFFER
};


class BHttpFormData {
private:
	// Empty constructor is only kept for compatibility with map<>::operator[],
	// but never used (see BHttpForm::operator[] which does the necessary
	// check up)
								BHttpFormData();
#if (__GNUC__ >= 6) || defined(__clang__)
	friend class std::pair<const BString, BHttpFormData>;
#else
	friend class std::map<BString, BHttpFormData>;
#endif

public:
								BHttpFormData(const BString& name,
									const BString& value);
								BHttpFormData(const BString& name,
									const BPath& file);
								BHttpFormData(const BString& name,
									const void* buffer, ssize_t size);
								BHttpFormData(const BHttpFormData& other);
								~BHttpFormData();

	// Retrieve data informations
			bool				InitCheck() const;

			const BString&		Name() const;
			const BString&		String() const;
			const BPath&		File() const;
			const void*			Buffer() const;
			ssize_t				BufferSize() const;

			bool				IsFile() const;
			const BString&		Filename() const;
			const BString&		MimeType() const;
			form_content_type	Type() const;

	// Change behavior
			status_t			MarkAsFile(const BString& filename,
									const BString& mimeType = "");
			void				UnmarkAsFile();
			status_t			CopyBuffer();

	// Overloaded operators
			BHttpFormData&		operator=(const BHttpFormData& other);

private:
			form_content_type	fDataType;
			bool				fCopiedBuffer;
			bool				fFileMark;

			BString				fName;
			BString				fStringValue;
			BPath				fPathValue;
			const void*			fBufferValue;
			ssize_t				fBufferSize;

			BString				fFilename;
			BString				fMimeType;
};


class BHttpForm {
public:
	// Nested types
	class Iterator;
	typedef std::map<BString, BHttpFormData> FormStorage;

public:
								BHttpForm();
								BHttpForm(const BHttpForm& other);
								BHttpForm(const BString& formString);
								~BHttpForm();

	// Form string parsing
			void				ParseString(const BString& formString);
			BString				RawData() const;

	// Form add
			status_t			AddString(const BString& name,
									const BString& value);
			status_t			AddInt(const BString& name, int32 value);
			status_t			AddFile(const BString& fieldName,
									const BPath& file);
			status_t			AddBuffer(const BString& fieldName,
									const void* buffer, ssize_t size);
			status_t			AddBufferCopy(const BString& fieldName,
									const void* buffer, ssize_t size);

	// Mark a field as a filename
			void				MarkAsFile(const BString& fieldName,
									const BString& filename,
									const BString& mimeType);
			void				MarkAsFile(const BString& fieldName,
									const BString& filename);
			void				UnmarkAsFile(const BString& fieldName);

	// Change form type
			void				SetFormType(form_type type);

	// Form test
			bool				HasField(const BString& name) const;

	// Form retrieve
			BString				GetMultipartHeader(const BString& fieldName) const;
			form_content_type	GetType(const BString& fieldname) const;

	// Form informations
			form_type			GetFormType() const;
			const BString&		GetMultipartBoundary() const;
			BString				GetMultipartFooter() const;
			ssize_t				ContentLength() const;

	// Form iterator
			Iterator			GetIterator();

	// Form clear
			void				Clear();

	// Overloaded operators
			BHttpFormData&		operator[](const BString& name);

private:
			void				_ExtractNameValuePair(const BString& string, int32* index);
			void				_GenerateMultipartBoundary();
			BString				_GetMultipartHeader(const BHttpFormData* element) const;
			form_content_type	_GetType(FormStorage::const_iterator it) const;
			void				_Erase(FormStorage::iterator it);

private:
	friend	class Iterator;

			FormStorage			fFields;
			form_type			fType;
			BString				fMultipartBoundary;
};


class BHttpForm::Iterator {
public:
								Iterator(const Iterator& other);

			BHttpFormData*		Next();
			bool 				HasNext() const;
			void				Remove();
			BString				MultipartHeader();

			Iterator& 			operator=(const Iterator& other);

private:
								Iterator(BHttpForm* form);
			void				_FindNext();

private:
	friend	class BHttpForm;

			BHttpForm*			fForm;
			BHttpForm::FormStorage::iterator
								fStdIterator;
			BHttpFormData*		fElement;
			BHttpFormData*		fPrevElement;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_FORM_H_
