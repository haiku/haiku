/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FUNCTION_ID_H
#define FUNCTION_ID_H


#include <Archivable.h>
#include <String.h>

#include "ObjectID.h"


class FunctionID : public ObjectID, public BArchivable {
protected:
								FunctionID(const BMessage& archive);
								FunctionID(const BString& path,
									const BString& functionName);

public:
	virtual						~FunctionID();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

			const BString&		FunctionName() const { return fFunctionName; }

protected:
	virtual	uint32				ComputeHashValue() const;

			bool				IsValid() const;

protected:
			BString				fPath;
			BString				fFunctionName;
};


class SourceFunctionID : public FunctionID {
public:
								SourceFunctionID(const BMessage& archive);
								SourceFunctionID(const BString& sourceFilePath,
									const BString& functionName);
	virtual						~SourceFunctionID();

	static	BArchivable*		Instantiate(BMessage* archive);

			const BString&		SourceFilePath() const	{ return fPath; }

	virtual	bool				operator==(const ObjectID& other) const;
};


class ImageFunctionID : public FunctionID {
public:
								ImageFunctionID(const BMessage& archive);
								ImageFunctionID(const BString& imageName,
									const BString& functionName);
	virtual						~ImageFunctionID();

	static	BArchivable*		Instantiate(BMessage* archive);

			const BString&		ImageName() const	{ return fPath; }

	virtual	bool				operator==(const ObjectID& other) const;
};


#endif	// FUNCTION_ID_H
