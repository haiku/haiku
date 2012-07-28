/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "AttributeStream.h"

#include <Debug.h>
#include <Node.h>


// ToDo:
// lazy Rewind from Drive, only if data is available
// BMessage node
// partial feeding (part, not the whole buffer)

AttributeInfo::AttributeInfo(const AttributeInfo &cloneThis)
	:	fName(cloneThis.fName),
		fInfo(cloneThis.fInfo)

{
}


AttributeInfo::AttributeInfo(const char* name, attr_info info)
	:	fName(name),
		fInfo(info)
{
}


AttributeInfo::AttributeInfo(const char* name, uint32 type, off_t size)
	:	fName(name)
{
	fInfo.size = size;
	fInfo.type = type;
}


const char*
AttributeInfo::Name() const
{
	return fName.String();
}

uint32
AttributeInfo::Type() const
{
	return fInfo.type;
}

off_t
AttributeInfo::Size() const
{
	return fInfo.size;
}


void
AttributeInfo::SetTo(const AttributeInfo &attr)
{
	fName = attr.fName;
	fInfo = attr.fInfo;
}

void
AttributeInfo::SetTo(const char* name, attr_info info)
{
	fName = name;
	fInfo = info;
}

void
AttributeInfo::SetTo(const char* name, uint32 type, off_t size)
{
	fName = name;
	fInfo.type = type;
	fInfo.size = size;
}


AttributeStreamNode::AttributeStreamNode()
	:	fReadFrom(NULL),
		fWriteTo(NULL)
{
}


AttributeStreamNode::~AttributeStreamNode()
{
	Detach();
}

AttributeStreamNode&
AttributeStreamNode::operator<<(AttributeStreamNode &source)
{
	fReadFrom = &source;
	fReadFrom->fWriteTo = this;
	if (fReadFrom->CanFeed())
		fReadFrom->Start();

	return source;
}

void
AttributeStreamNode::Rewind()
{
	if (fReadFrom)
		fReadFrom->Rewind();
}

void
AttributeStreamFileNode::MakeEmpty()
{
	TRESPASS();
}

off_t
AttributeStreamNode::Contains(const char* name, uint32 type)
{
	if (!fReadFrom)
		return 0;

	return fReadFrom->Contains(name, type);
}


off_t
AttributeStreamNode::Read(const char* name, const char* foreignName,
	uint32 type, off_t size, void* buffer, void (*swapFunc)(void*))
{
	if (!fReadFrom)
		return 0;

	return fReadFrom->Read(name, foreignName, type, size, buffer, swapFunc);
}


off_t
AttributeStreamNode::Write(const char* name, const char* foreignName, uint32 type,
	off_t size, const void* buffer)
{
	if (!fWriteTo)
		return 0;

	return fWriteTo->Write(name, foreignName, type, size, buffer);
}


bool
AttributeStreamNode::Drive()
{
	ASSERT(CanFeed());
	if (!fReadFrom)
		return false;

	Rewind();
	return true;
}


const AttributeInfo*
AttributeStreamNode::Next()
{
	if (fReadFrom)
		return fReadFrom->Next();

	return NULL;
}


const char*
AttributeStreamNode::Get()
{
	ASSERT(fReadFrom);
	if (!fReadFrom)
		return NULL;

	return fReadFrom->Get();
}


bool
AttributeStreamNode::Fill(char* buffer) const
{
	ASSERT(fReadFrom);
	return fReadFrom->Fill(buffer);
}


bool
AttributeStreamNode::Start()
{
	if (!fWriteTo)
		// we are at the head of the stream, start drivin'
		return Drive();

	return fWriteTo->Start();
}


void
AttributeStreamNode::Detach()
{
	AttributeStreamNode* tmpFrom = fReadFrom;
	AttributeStreamNode* tmpTo = fWriteTo;
	fReadFrom = NULL;
	fWriteTo = NULL;

	if (tmpFrom)
		tmpFrom->Detach();
	if (tmpTo)
		tmpTo->Detach();
}


AttributeStreamFileNode::AttributeStreamFileNode()
	:	fNode(NULL)
{
}


AttributeStreamFileNode::AttributeStreamFileNode(BNode* node)
	:	fNode(node)
{
	ASSERT(fNode);
}


void
AttributeStreamFileNode::Rewind()
{
	_inherited::Rewind();
	fNode->RewindAttrs();
}


void
AttributeStreamFileNode::SetTo(BNode* node)
{
	fNode = node;
}


off_t
AttributeStreamFileNode::Contains(const char* name, uint32 type)
{
	ASSERT(fNode);
	attr_info info;
	if (fNode->GetAttrInfo(name, &info) != B_OK)
		return 0;

	if (info.type != type)
		return 0;

	return info.size;
}


off_t
AttributeStreamFileNode::Read(const char* name, const char* foreignName,
	uint32 type, off_t size, void* buffer, void (*swapFunc)(void*))
{
	if (name && fNode->ReadAttr(name, type, 0, buffer, (size_t)size) == size)
		return size;

	// didn't find the attribute under the native name, try the foreign name
	if (foreignName && fNode->ReadAttr(foreignName, type, 0, buffer, (size_t)size) == size) {
		// foreign attribute, swap the data
		if (swapFunc)
			(swapFunc)(buffer);
		return size;
	}
	return 0;
}


off_t
AttributeStreamFileNode::Write(const char* name, const char* foreignName,
	uint32 type, off_t size, const void* buffer)
{
	ASSERT(fNode);
	ASSERT(dynamic_cast<BNode*>(fNode));
	off_t result = fNode->WriteAttr(name, type, 0, buffer, (size_t)size);
	if (result == size && foreignName)
		// the write operation worked fine, remove the foreign attribute
		// to not let stale data hang around
		fNode->RemoveAttr(foreignName);

	return result;
}


bool
AttributeStreamFileNode::Drive()
{
	ASSERT(fNode);
	if (!_inherited::Drive())
		return false;

	const AttributeInfo* attr;
	while ((attr = fReadFrom->Next()) != 0) {
		const char* data = fReadFrom->Get();
		off_t result = fNode->WriteAttr(attr->Name(), attr->Type(), 0,
			data, (size_t)attr->Size());
		if (result < attr->Size())
			return true;
	}
	return true;
}


const char*
AttributeStreamFileNode::Get()
{
	ASSERT(fNode);
	TRESPASS();
	return NULL;
}


bool
AttributeStreamFileNode::Fill(char* buffer) const
{
	ASSERT(fNode);
	return fNode->ReadAttr(fCurrentAttr.Name(), fCurrentAttr.Type(), 0, buffer,
		(size_t)fCurrentAttr.Size()) == (ssize_t)fCurrentAttr.Size();
}


const AttributeInfo*
AttributeStreamFileNode::Next()
{
	ASSERT(fNode);
	ASSERT(!fReadFrom);
	char attrName[256];
	if (fNode->GetNextAttrName(attrName) != B_OK)
		return NULL;

	attr_info info;
	if (fNode->GetAttrInfo(attrName, &info) != B_OK)
		return NULL;

	fCurrentAttr.SetTo(attrName, info);
	return &fCurrentAttr;
}


AttributeStreamMemoryNode::AttributeStreamMemoryNode()
	:	fAttributes(5, true),
		fCurrentIndex(-1)
{
}


void
AttributeStreamMemoryNode::MakeEmpty()
{
	fAttributes.MakeEmpty();
}


void
AttributeStreamMemoryNode::Rewind()
{
	_inherited::Rewind();
	fCurrentIndex = -1;
}


int32
AttributeStreamMemoryNode::Find(const char* name, uint32 type) const
{
	int32 count = fAttributes.CountItems();
	for (int32 index = 0; index < count; index++)
		if (strcmp(fAttributes.ItemAt(index)->fAttr.Name(), name) == 0
			&& fAttributes.ItemAt(index)->fAttr.Type() == type)
			return index;

	return -1;
}


off_t
AttributeStreamMemoryNode::Contains(const char* name, uint32 type)
{
	int32 index = Find(name, type);
	if (index < 0)
		return 0;
	return fAttributes.ItemAt(index)->fAttr.Size();
}


off_t
AttributeStreamMemoryNode::Read(const char* name, const char* DEBUG_ONLY(foreignName),
	uint32 type, off_t bufferSize, void* buffer, void (*DEBUG_ONLY(swapFunc))(void*))
{
	ASSERT(!foreignName);
	ASSERT(!swapFunc);

	AttrNode* attrNode = NULL;

	int32 index = Find(name, type);
	if (index < 0) {
		if (!fReadFrom)
			return 0;
		off_t size = fReadFrom->Contains(name, type);
		if (!size)
			return 0;

		attrNode = BufferingGet(name, type, size);
		if (!attrNode)
			return 0;
	} else
		attrNode = fAttributes.ItemAt(index);

	if (attrNode->fAttr.Size() > bufferSize)
		return 0;

	memcpy(buffer, attrNode->fData, (size_t)attrNode->fAttr.Size());
	return attrNode->fAttr.Size();
}


off_t
AttributeStreamMemoryNode::Write(const char* name, const char*, uint32 type,
	off_t size, const void* buffer)
{
	char* newBuffer = new char[size];
	memcpy(newBuffer, buffer, (size_t)size);

	AttrNode* attrNode = new AttrNode(name, type, size, newBuffer);
	fAttributes.AddItem(attrNode);
	return size;
}


bool
AttributeStreamMemoryNode::Drive()
{
	if (!_inherited::Drive())
		return false;

	while (BufferingGet())
		;

	return true;
}


AttributeStreamMemoryNode::AttrNode*
AttributeStreamMemoryNode::BufferingGet(const char* name, uint32 type, off_t size)
{
	char* newBuffer = new char[size];
	if (!fReadFrom->Fill(newBuffer)) {
		delete[] newBuffer;
		return NULL;
	}

	AttrNode* attrNode = new AttrNode(name, type, size, newBuffer);
	fAttributes.AddItem(attrNode);
	return fAttributes.LastItem();
}


AttributeStreamMemoryNode::AttrNode*
AttributeStreamMemoryNode::BufferingGet()
{
	if (!fReadFrom)
		return NULL;

	const AttributeInfo* attr = fReadFrom->Next();
	if (!attr)
		return NULL;

	return BufferingGet(attr->Name(), attr->Type(), attr->Size());
}


const AttributeInfo*
AttributeStreamMemoryNode::Next()
{
	if (fReadFrom)
		// the buffer is in the middle of the stream, get
		// one buffer at a time
		BufferingGet();

	if (fCurrentIndex + 1 >= fAttributes.CountItems())
		return NULL;

	return &fAttributes.ItemAt(++fCurrentIndex)->fAttr;
}


const char*
AttributeStreamMemoryNode::Get()
{
	ASSERT(fCurrentIndex < fAttributes.CountItems());
	return fAttributes.ItemAt(fCurrentIndex)->fData;
}


bool
AttributeStreamMemoryNode::Fill(char* buffer) const
{
	ASSERT(fCurrentIndex < fAttributes.CountItems());
	memcpy(buffer, fAttributes.ItemAt(fCurrentIndex)->fData,
		(size_t)fAttributes.ItemAt(fCurrentIndex)->fAttr.Size());

	return true;
}


AttributeStreamTemplateNode::AttributeStreamTemplateNode(
	const AttributeTemplate* attrTemplates, int32 count)
	:	fAttributes(attrTemplates),
		fCurrentIndex(-1),
		fCount(count)
{
}


off_t
AttributeStreamTemplateNode::Contains(const char* name, uint32 type)
{
	int32 index = Find(name, type);
	if (index < 0)
		return 0;

	return fAttributes[index].fSize;
}


void
AttributeStreamTemplateNode::Rewind()
{
	fCurrentIndex = -1;
}


const AttributeInfo*
AttributeStreamTemplateNode::Next()
{
	if (fCurrentIndex + 1 >= fCount)
		return NULL;

	++fCurrentIndex;

	fCurrentAttr.SetTo(fAttributes[fCurrentIndex].fAttributeName,
		fAttributes[fCurrentIndex].fAttributeType,
		fAttributes[fCurrentIndex].fSize);

	return &fCurrentAttr;
}


const char*
AttributeStreamTemplateNode::Get()
{
	ASSERT(fCurrentIndex < fCount);
	return fAttributes[fCurrentIndex].fBits;
}


bool
AttributeStreamTemplateNode::Fill(char* buffer) const
{
	ASSERT(fCurrentIndex < fCount);
	memcpy(buffer, fAttributes[fCurrentIndex].fBits,
		(size_t)fAttributes[fCurrentIndex].fSize);

	return true;
}


int32
AttributeStreamTemplateNode::Find(const char* name, uint32 type) const
{
	for (int32 index = 0; index < fCount; index++) {
		if (fAttributes[index].fAttributeType == type &&
			strcmp(name, fAttributes[index].fAttributeName) == 0) {
			return index;
		}
	}

	return -1;
}


bool
AttributeStreamFilterNode::Reject(const char*, uint32, off_t)
{
	// simple pass everything filter
	return false;
}


const AttributeInfo*
AttributeStreamFilterNode::Next()
{
	if (!fReadFrom)
		return NULL;

	for (;;) {
		const AttributeInfo* attr = fReadFrom->Next();
		if (!attr)
			break;

		if (!Reject(attr->Name(), attr->Type(), attr->Size()))
			return attr;
	}
	return NULL;
}


off_t
AttributeStreamFilterNode::Contains(const char* name, uint32 type)
{
	if (!fReadFrom)
		return 0;

	off_t size = fReadFrom->Contains(name, type);

	if (!Reject(name, type, size))
		return size;

	return 0;
}


off_t
AttributeStreamFilterNode::Read(const char* name, const char* foreignName,
	uint32 type, off_t size, void* buffer, void (*swapFunc)(void*))
{
	if (!fReadFrom)
		return 0;

	if (!Reject(name, type, size))
		return fReadFrom->Read(name, foreignName, type, size, buffer, swapFunc);

	return 0;
}


off_t
AttributeStreamFilterNode::Write(const char* name, const char* foreignName,
	uint32 type, off_t size, const void* buffer)
{
	if (!fWriteTo)
		return 0;

	if (!Reject(name, type, size))
		return fWriteTo->Write(name, foreignName, type, size, buffer);

	return size;
}


NamesToAcceptAttrFilter::NamesToAcceptAttrFilter(const char** nameList)
	:	fNameList(nameList)
{
}


bool
NamesToAcceptAttrFilter::Reject(const char* name, uint32, off_t)
{
	for (int32 index = 0; ;index++) {
		if (!fNameList[index])
			break;

		if (strcmp(name, fNameList[index]) == 0) {
			//PRINT(("filter passing through %s\n", name));
			return false;
		}
	}

	//PRINT(("filter rejecting %s\n", name));
	return true;
}


SelectiveAttributeTransformer::SelectiveAttributeTransformer(
	const char* attributeName,
	bool (*transformFunc)(const char* , uint32 , off_t, void*, void*),
	void* params)
	:	fAttributeNameToTransform(attributeName),
		fTransformFunc(transformFunc),
		fTransformParams(params),
		fTransformedBuffers(10, false)
{
}


SelectiveAttributeTransformer::~SelectiveAttributeTransformer()
{
	for (int32 index = fTransformedBuffers.CountItems() - 1; index >= 0;
			index--) {
		delete [] fTransformedBuffers.ItemAt(index);
	}
}


void
SelectiveAttributeTransformer::Rewind()
{
	for (int32 index = fTransformedBuffers.CountItems() - 1; index >= 0;
			index--) {
		delete [] fTransformedBuffers.ItemAt(index);
	}

	fTransformedBuffers.MakeEmpty();
}


off_t
SelectiveAttributeTransformer::Read(const char* name, const char* foreignName,
	uint32 type, off_t size, void* buffer, void (*swapFunc)(void*))
{
	if (!fReadFrom)
		return 0;

	off_t result = fReadFrom->Read(name, foreignName, type, size, buffer,
		swapFunc);

	if (WillTransform(name, type, size, (const char*)buffer))
		ApplyTransformer(name, type, size, (char*)buffer);

	return result;
}


bool
SelectiveAttributeTransformer::WillTransform(const char* name, uint32, off_t,
	const char*) const
{
	return strcmp(name, fAttributeNameToTransform) == 0;
}


bool
SelectiveAttributeTransformer::ApplyTransformer(const char* name, uint32 type,
	off_t size, char* data)
{
	return (fTransformFunc)(name, type, size, data, fTransformParams);
}

char*
SelectiveAttributeTransformer::CopyAndApplyTransformer(const char* name,
	uint32 type, off_t size, const char* data)
{
	char* result = NULL;
	if (data) {
		result = new char[size];
		memcpy(result, data, (size_t)size);
	}

	if (!(fTransformFunc)(name, type, size, result, fTransformParams)) {
		delete [] result;
		return NULL;
	}

	return result;
}


const AttributeInfo*
SelectiveAttributeTransformer::Next()
{
	const AttributeInfo* result = fReadFrom->Next();
	if (!result)
		return NULL;

	fCurrentAttr.SetTo(*result);
	return result;
}


const char*
SelectiveAttributeTransformer::Get()
{
	if (!fReadFrom)
		return NULL;

	const char* result = fReadFrom->Get();

	if (!WillTransform(fCurrentAttr.Name(), fCurrentAttr.Type(),
			fCurrentAttr.Size(), result)) {
		return result;
	}

	char* transformedData = CopyAndApplyTransformer(fCurrentAttr.Name(),
		fCurrentAttr.Type(), fCurrentAttr.Size(), result);

	// enlist for proper disposal when our job is done
	if (transformedData) {
		fTransformedBuffers.AddItem(transformedData);
		return transformedData;
	}

	return result;
}
