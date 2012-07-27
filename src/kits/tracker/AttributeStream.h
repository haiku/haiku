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

//	attribute streams allow copying/filtering/transformation of attributes
//	between file and/or memory nodes
//
//	for example one can use constructs of nodes like:
//
// 	destinationNode << transformer << buffer << filter << sourceNode
//
//	transformer may for instance perform endian-swapping or offsetting of a B_RECT attribute
//	filter may withold certain attributes
//	buffer is a memory allocated snapshot of attributes, may be repeatedly streamed into
//	other files, buffers
//
//	In addition to the whacky (but usefull) << syntax, calls like Read, Write are also
//	available
#ifndef __ATTRIBUTE_STREAM__
#define __ATTRIBUTE_STREAM__


#include <Node.h>
#include <Rect.h>
#include <String.h>
#include <TypeConstants.h>

#include <fs_attr.h>

#include "ObjectList.h"


namespace BPrivate {

struct AttributeTemplate {
	// used for read-only attribute source
	const char* fAttributeName;
	uint32 fAttributeType;
	off_t fSize;
	const char* fBits;
};


class AttributeInfo {
	// utility class for internal attribute description
public:
	AttributeInfo()
		{}
	AttributeInfo(const AttributeInfo &);
	AttributeInfo(const char*, attr_info);
	AttributeInfo(const char*, uint32, off_t);
	
	void SetTo(const AttributeInfo &);
	void SetTo(const char*, attr_info);
	void SetTo(const char*, uint32, off_t);
	const char* Name() const;
	uint32 Type() const;
	off_t Size() const;

private:
	BString fName;
	attr_info fInfo;
};


class AttributeStreamNode {
public:
	AttributeStreamNode();
	virtual ~AttributeStreamNode();

	AttributeStreamNode &operator<<(AttributeStreamNode &source);
		// workhorse call
		// to the outside makes this node a part of the stream, passing on
		// any data it has, gets, transforms, doesn't filter out
		//
		// under the hood sets up streaming into the next node; hooking
		// up source and destination, forces the stream head to start streaming

	virtual void Rewind();
		// get ready to start all over again
	virtual void MakeEmpty() {}
		// remove any attributes the node may have

	virtual off_t Contains(const char*, uint32);
		// returns size of attribute if found

	virtual off_t Read(const char* name, const char* foreignName, uint32 type, off_t size,
		void* buffer, void (*swapFunc)(void*) = 0);
		// read from this node
	virtual off_t Write(const char* name, const char* foreignName, uint32 type, off_t size,
		const void* buffer);
		// write to this node

	// work calls
	virtual bool Drive();
		// node at the head of the stream makes the entire stream
		// feed it
	virtual const AttributeInfo* Next();
		// give me the next attribute in the stream
	virtual const char* Get();
		// give me the data of the attribute in the stream that was just returned
		// by Next
		// assumes there is a buffering node somewhere on the way to
		// the source, from which the resulting buffer is borrowed
	virtual bool Fill(char* buffer) const;
		// fill the buffer with data of the attribute in the stream that was just returned
		// by next
		// <buffer> is big enough to hold the entire attribute data

	virtual bool CanFeed() const { return false; }
		// return true if can work as a source for the entire stream

private:
	bool Start();
		// utility call, used to start up the stream by finding the ultimate
		// target of the stream and calling Drive on it

	void Detach();

protected:
	AttributeStreamNode* fReadFrom;
	AttributeStreamNode* fWriteTo;
};


class AttributeStreamFileNode : public AttributeStreamNode {
	// handles reading and writing attributes to and from the
	// stream
public:
	AttributeStreamFileNode();
	AttributeStreamFileNode(BNode*);

	virtual void MakeEmpty();
	virtual void Rewind();
	virtual off_t Contains(const char* name, uint32 type);
	virtual off_t Read(const char* name, const char* foreignName, uint32 type,
		off_t size, void* buffer, void (*swapFunc)(void*) = 0);
	virtual off_t Write(const char* name, const char* foreignName, uint32 type,
		off_t size, const void* buffer);

	void SetTo(BNode*);

	BNode* Node()
		{ return fNode; }

protected:
	virtual bool CanFeed() const { return true; }

	virtual bool Drive();
		// give me all the attributes, I'll write them into myself
	virtual const AttributeInfo* Next();
		// return the info for the next attribute I can read for you
	virtual const char* Get();
	virtual bool Fill(char* buffer) const;

private:
	AttributeInfo fCurrentAttr;
	BNode* fNode;
	
	typedef AttributeStreamNode _inherited;
};


class AttributeStreamMemoryNode : public AttributeStreamNode {
	// in memory attribute buffer; can be both target of writing and source
	// of reading at the same time
public:
	AttributeStreamMemoryNode();

	virtual void MakeEmpty();
	virtual off_t Contains(const char* name, uint32 type);
	virtual off_t Read(const char* name, const char* foreignName, uint32 type, off_t size,
		void* buffer, void (*swapFunc)(void*) = 0);
	virtual off_t Write(const char* name, const char* foreignName, uint32 type, off_t size,
		const void* buffer);

protected:
	virtual bool CanFeed() const { return true; }
	virtual void Rewind();
	virtual bool Drive();
	virtual const AttributeInfo* Next();
	virtual const char* Get();
	virtual bool Fill(char* buffer) const;

	class AttrNode {
	public:
		AttrNode(const char* name, uint32 type, off_t size, char* data)
			:	fAttr(name, type, size),
				fData(data)
			{
			}

		~AttrNode()
			{
				delete [] fData;
			}
			
		AttributeInfo fAttr;
		char* fData;
	};

		// utility calls
	virtual AttrNode* BufferingGet();
	virtual AttrNode* BufferingGet(const char* name, uint32 type, off_t size);
	int32 Find(const char* name, uint32 type) const;

private:
	BObjectList<AttrNode> fAttributes;
	int32 fCurrentIndex;

	typedef AttributeStreamNode _inherited;
};


class AttributeStreamTemplateNode : public AttributeStreamNode {
	// in read-only memory attribute source
	// can only be used as a source for Next and Get
public:
	AttributeStreamTemplateNode(const AttributeTemplate*, int32 count);

	virtual off_t Contains(const char* name, uint32 type);

protected:
	virtual bool CanFeed() const { return true; }
	virtual void Rewind();
	virtual const AttributeInfo* Next();
	virtual const char* Get();
	virtual bool Fill(char* buffer) const;

	int32 Find(const char* name, uint32 type) const;

private:
	AttributeInfo fCurrentAttr;
	const AttributeTemplate* fAttributes;
	int32 fCurrentIndex;
	int32 fCount;

	typedef AttributeStreamNode _inherited;
};


class AttributeStreamFilterNode : public AttributeStreamNode {
	// filter node may not pass thru specified attributes
public:
	AttributeStreamFilterNode()
		{}
	virtual off_t Contains(const char* name, uint32 type);
	virtual off_t Read(const char* name, const char* foreignName, uint32 type, off_t size,
		void* buffer, void (*swapFunc)(void*) = 0);
	virtual off_t Write(const char* name, const char* foreignName, uint32 type, off_t size,
		const void* buffer);

protected:
	virtual bool Reject(const char* name, uint32 type, off_t size);
		// override to implement filtering
	virtual const AttributeInfo* Next();

private:
	typedef AttributeStreamNode _inherited;
};


class NamesToAcceptAttrFilter : public AttributeStreamFilterNode {
	// filter node that only passes thru attributes that match
	// a list of names
public:
	NamesToAcceptAttrFilter(const char**);

protected:
	virtual bool Reject(const char* name, uint32 type, off_t size);

private:
	const char** fNameList;
};


class SelectiveAttributeTransformer : public AttributeStreamNode {
	// node applies a transformation on specified attributes
public:
	SelectiveAttributeTransformer(const char* attributeName, bool (*)(const char*,
		uint32 , off_t , void*, void*), void* params);
	virtual ~SelectiveAttributeTransformer();

	virtual off_t Read(const char* name, const char* foreignName, uint32 type, off_t size,
		void* buffer, void (*swapFunc)(void*) = 0);

	virtual void Rewind();

protected:
	virtual bool WillTransform(const char* name, uint32 type, off_t size, const char* data) const;
		// override to implement filtering; should only return true if transformation will
		// occur
	virtual char* CopyAndApplyTransformer(const char* name, uint32 type, off_t size, const char* data);
		// makes a copy of data
	virtual bool ApplyTransformer(const char* name, uint32 type, off_t size, char* data);
		// transforms in place
	virtual const AttributeInfo* Next();
	virtual const char* Get();

private:
	AttributeInfo fCurrentAttr;
	const char* fAttributeNameToTransform;
	bool (*fTransformFunc)(const char*, uint32 , off_t , void*, void*);
	void* fTransformParams;

	BObjectList<char> fTransformedBuffers;

	typedef AttributeStreamNode _inherited;
};


template <class Type>
class AttributeStreamConstValue : public AttributeStreamNode {
public:
	AttributeStreamConstValue(const char* name, uint32 attributeType, Type value);

protected:
	virtual bool CanFeed() const { return true; }
	virtual void Rewind() { fRewound = true; }
	virtual const AttributeInfo* Next();
	virtual const char* Get();
	virtual bool Fill(char* buffer) const;

	int32 Find(const char* name, uint32 type) const;

private:
	AttributeInfo fAttr;
	Type fValue;
	bool fRewound;

	typedef AttributeStreamNode _inherited;
};


template<class Type>
AttributeStreamConstValue<Type>::AttributeStreamConstValue(const char* name,
	uint32 attributeType, Type value)
	:	fAttr(name, attributeType, sizeof(Type)),
		fValue(value),
		fRewound(true)
{
}


template<class Type>
const AttributeInfo*
AttributeStreamConstValue<Type>::Next()
{
	if (!fRewound)
		return NULL;

	fRewound = false;
	return &fAttr;
}


template<class Type>
const char*
AttributeStreamConstValue<Type>::Get()
{
	return (const char*)&fValue;
}


template<class Type>
bool
AttributeStreamConstValue<Type>::Fill(char* buffer) const
{
	memcpy(buffer, &fValue, sizeof(Type));
	return true;
}


template<class Type>
int32
AttributeStreamConstValue<Type>::Find(const char* name, uint32 type) const
{
	if (strcmp(fAttr.Name(), name) == 0 && type == fAttr.Type())
		return 0;

	return -1;
}


class AttributeStreamBoolValue : public AttributeStreamConstValue<bool> {
public:
	AttributeStreamBoolValue(const char* name, bool value)
		:	AttributeStreamConstValue<bool>(name, B_BOOL_TYPE, value)
		{}
};


class AttributeStreamInt32Value : public AttributeStreamConstValue<int32> {
public:
	AttributeStreamInt32Value(const char* name, int32 value)
		:	AttributeStreamConstValue<int32>(name, B_INT32_TYPE, value)
		{}
};


class AttributeStreamInt64Value : public AttributeStreamConstValue<int64> {
public:
	AttributeStreamInt64Value(const char* name, int64 value)
		:	AttributeStreamConstValue<int64>(name, B_INT64_TYPE, value)
		{}
};


class AttributeStreamRectValue : public AttributeStreamConstValue<BRect> {
public:
	AttributeStreamRectValue(const char* name, BRect value)
		:	AttributeStreamConstValue<BRect>(name, B_RECT_TYPE, value)
		{}
};


class AttributeStreamFloatValue : public AttributeStreamConstValue<float> {
public:
	AttributeStreamFloatValue(const char* name, float value)
		:	AttributeStreamConstValue<float>(name, B_FLOAT_TYPE, value)
		{}
};

} // namespace BPrivate

using namespace BPrivate;

#endif
