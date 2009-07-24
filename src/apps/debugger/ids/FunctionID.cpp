/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FunctionID.h"

#include <new>

#include <Message.h>

#include "StringUtils.h"


// #pragma mark - FunctionID


FunctionID::FunctionID(const BMessage& archive)
	:
	BArchivable(const_cast<BMessage*>(&archive))
{
	archive.FindString("FunctionID::path", &fPath);
	archive.FindString("FunctionID::functionName", &fFunctionName);
}


FunctionID::FunctionID(const BString& path, const BString& functionName)
	:
	fPath(path),
	fFunctionName(functionName)
{
}


FunctionID::~FunctionID()
{
}


status_t
FunctionID::Archive(BMessage* archive, bool deep) const
{
	status_t error = BArchivable::Archive(archive, deep);
	if (error != B_OK)
		return error;

	error = archive->AddString("FunctionID::path", fPath);
	if (error == B_OK)
		error = archive->AddString("FunctionID::functionName", fFunctionName);
	return error;
}


uint32
FunctionID::ComputeHashValue() const
{
	return StringUtils::HashValue(fPath) * 17
		+ StringUtils::HashValue(fFunctionName);
}


bool
FunctionID::IsValid() const
{
	return !fPath.Length() == 0 && !fFunctionName.Length() == 0;
}


// #pragma mark - SourceFunctionID


SourceFunctionID::SourceFunctionID(const BMessage& archive)
	:
	FunctionID(archive)
{
}


SourceFunctionID::SourceFunctionID(const BString& sourceFilePath,
	const BString& functionName)
	:
	FunctionID(sourceFilePath, functionName)
{
}


SourceFunctionID::~SourceFunctionID()
{
}


/*static*/ BArchivable*
SourceFunctionID::Instantiate(BMessage* archive)
{
	if (archive == NULL)
		return NULL;

	SourceFunctionID* object = new(std::nothrow) SourceFunctionID(*archive);
	if (object == NULL)
		return NULL;

	if (!object->IsValid()) {
		delete object;
		return NULL;
	}

	return object;
}


bool
SourceFunctionID::operator==(const ObjectID& _other) const
{
	const SourceFunctionID* other = dynamic_cast<const SourceFunctionID*>(
		&_other);
	return other != NULL && fPath == other->fPath
		&& fFunctionName == other->fFunctionName;
}


// #pragma mark - ImageFunctionID


ImageFunctionID::ImageFunctionID(const BMessage& archive)
	:
	FunctionID(archive)
{
}


ImageFunctionID::ImageFunctionID(const BString& imageName,
	const BString& functionName)
	:
	FunctionID(imageName, functionName)
{
}


ImageFunctionID::~ImageFunctionID()
{
}


/*static*/ BArchivable*
ImageFunctionID::Instantiate(BMessage* archive)
{
	if (archive == NULL)
		return NULL;

	ImageFunctionID* object = new(std::nothrow) ImageFunctionID(*archive);
	if (object == NULL)
		return NULL;

	if (!object->IsValid()) {
		delete object;
		return NULL;
	}

	return object;
}


bool
ImageFunctionID::operator==(const ObjectID& _other) const
{
	const ImageFunctionID* other = dynamic_cast<const ImageFunctionID*>(
		&_other);
	return other != NULL && fPath == other->fPath
		&& fFunctionName == other->fFunctionName;
}
