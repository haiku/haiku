/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <TypeConstants.h>

#ifdef _KERNEL_MODE
#	include <debug_heap.h>
#endif

#include "demangle.h"


// C++ ABI: http://www.codesourcery.com/public/cxx-abi/abi.html


//#define TRACE_GCC3_DEMANGLER
#ifdef TRACE_GCC3_DEMANGLER
#	define TRACE(x...) PRINT(x)
#	define DEBUG_SCOPE(name)	DebugScope debug(name, fInput.String())
#else
#	define TRACE(x...) ;
#	define DEBUG_SCOPE(name)	do {} while (false)
#endif

#ifdef _KERNEL_MODE
#	define PRINT(format...)		kprintf(format)
#	define VPRINT(format, args)	PRINT("%s", format)
									// no vkprintf()
#	define NEW(constructor) new(kdebug_alloc) constructor
#	define DELETE(object)	DebugAlloc::destroy(object)
#else
#	define PRINT(format...)		printf(format)
#	define VPRINT(format, args)	vprintf(format, args)
#	define NEW(constructor) new(std::nothrow) constructor
#	define DELETE(object)	delete object
#endif


typedef long number_type;

enum {
	ERROR_OK = 0,
	ERROR_NOT_MANGLED,
	ERROR_UNSUPPORTED,
	ERROR_INVALID,
	ERROR_BUFFER_TOO_SMALL,
	ERROR_NO_MEMORY,
	ERROR_INTERNAL,
	ERROR_INVALID_PARAMETER_INDEX
};

// object classification
enum object_type {
	OBJECT_TYPE_UNKNOWN,
	OBJECT_TYPE_DATA,
	OBJECT_TYPE_FUNCTION,
	OBJECT_TYPE_METHOD_CLASS,
	OBJECT_TYPE_METHOD_OBJECT,
	OBJECT_TYPE_METHOD_UNKNOWN
};

// prefix classification
enum prefix_type {
	PREFIX_NONE,
	PREFIX_NAMESPACE,
	PREFIX_CLASS,
	PREFIX_UNKNOWN
};

// type classification
enum type_type {
	TYPE_ELLIPSIS,
	TYPE_VOID,
	TYPE_WCHAR_T,
	TYPE_BOOL,
	TYPE_CHAR,
	TYPE_SIGNED_CHAR,
	TYPE_UNSIGNED_CHAR,
	TYPE_SHORT,
	TYPE_UNSIGNED_SHORT,
	TYPE_INT,
	TYPE_UNSIGNED_INT,
	TYPE_LONG,
	TYPE_UNSIGNED_LONG,
	TYPE_LONG_LONG,
	TYPE_UNSIGNED_LONG_LONG,
	TYPE_INT128,
	TYPE_UNSIGNED_INT128,
	TYPE_FLOAT,
	TYPE_DOUBLE,
	TYPE_LONG_DOUBLE,
	TYPE_FLOAT128,
	TYPE_DFLOAT16,
	TYPE_DFLOAT32,
	TYPE_DFLOAT64,
	TYPE_DFLOAT128,
	TYPE_CHAR16_T,
	TYPE_CHAR32_T,

	TYPE_UNKNOWN,
	TYPE_CONST_CHAR_POINTER,
	TYPE_POINTER,
	TYPE_REFERENCE
};

const char* const kTypeNames[] = {
	"...",
	"void",
	"wchar_t",
	"bool",
	"char",
	"signed char",
	"unsigned char",
	"short",
	"unsigned short",
	"int",
	"unsigned int",
	"long",
	"unsigned long",
	"long long",
	"unsigned long long",
	"__int128",
	"unsigned __int128",
	"float",
	"double",
	"long double",
	"__float128",
	"__dfloat16",	// TODO: Official names for the __dfloat*!
	"__dfloat32",
	"__dfloat64",
	"__dfloat64",
	"char16_t",
	"char32_t",

	"?",
	"char const*",
	"void*",
	"void&"
};


// CV qualifier flags
enum {
	CV_QUALIFIER_RESTRICT	= 0x1,
	CV_QUALIFIER_VOLATILE	= 0x2,
	CV_QUALIFIER_CONST		= 0x4
};

enum type_modifier {
	TYPE_QUALIFIER_POINTER = 0,
	TYPE_QUALIFIER_REFERENCE,
	TYPE_QUALIFIER_RVALUE_REFERENCE,
	TYPE_QUALIFIER_COMPLEX,
	TYPE_QUALIFIER_IMAGINARY
};

static const char* const kTypeModifierSuffixes[] = {
	"*",
	"&",
	"&&",
	" complex",
	" imaginary"
};

struct operator_info {
	const char*	mangled_name;
	const char*	name;
	int			argument_count;
	int			flags;
};

// operator flags
enum {
	OPERATOR_TYPE_PARAM		= 0x01,
	OPERATOR_IS_MEMBER		= 0x02
};


static const operator_info kOperatorInfos[] = {
	{ "nw", "new", -1, OPERATOR_IS_MEMBER },
	{ "na", "new[]", -1, OPERATOR_IS_MEMBER },
	{ "dl", "delete", -1, OPERATOR_IS_MEMBER },
	{ "da", "delete[]", -1, OPERATOR_IS_MEMBER },
	{ "ps", "+", 1, 0 },		// unary
	{ "ng", "-", 1, 0 },		// unary
	{ "ad", "&", 1, 0 },		// unary
	{ "de", "*", 1, 0 },		// unary
	{ "co", "~", 1, 0 },
	{ "pl", "+", 2, 0 },
	{ "mi", "-", 2, 0 },
	{ "ml", "*", 2, 0 },
	{ "dv", "/", 2, 0 },
	{ "rm", "%", 2, 0 },
	{ "an", "&", 2, 0 },
	{ "or", "|", 2, 0 },
	{ "eo", "^", 2, 0 },
	{ "aS", "=", 2, 0 },
	{ "pL", "+=", 2, 0 },
	{ "mI", "-=", 2, 0 },
	{ "mL", "*=", 2, 0 },
	{ "dV", "/=", 2, 0 },
	{ "rM", "%=", 2, 0 },
	{ "aN", "&=", 2, 0 },
	{ "oR", "|=", 2, 0 },
	{ "eO", "^=", 2, 0 },
	{ "ls", "<<", 2, 0 },
	{ "rs", ">>", 2, 0 },
	{ "lS", "<<=", 2, 0 },
	{ "rS", ">>=", 2, 0 },
	{ "eq", "==", 2, 0 },
	{ "ne", "!=", 2, 0 },
	{ "lt", "<", 2, 0 },
	{ "gt", ">", 2, 0 },
	{ "le", "<=", 2, 0 },
	{ "ge", ">=", 2, 0 },
	{ "nt", "!", 1, 0 },
	{ "aa", "&&", 2, 0 },
	{ "oo", "||", 2, 0 },
	{ "pp", "++", 1, 0 },
	{ "mm", "--", 1, 0 },
	{ "cm", ",", -1, 0 },
	{ "pm", "->*", 2, 0 },
	{ "pt", "->", 2, 0 },
	{ "cl", "()", -1, 0 },
	{ "ix", "[]", -1, 0 },
	{ "qu", "?", 3, 0 },
	{ "st", "sizeof", 1, OPERATOR_TYPE_PARAM },		// type
	{ "sz", "sizeof", 1, 0 },						// expression
	{ "at", "alignof", 1, OPERATOR_TYPE_PARAM },	// type
	{ "az", "alignof", 1, 0 },						// expression
	{}
};


#ifdef TRACE_GCC3_DEMANGLER

struct DebugScope {
	DebugScope(const char* functionName, const char* remainingString = NULL)
		:
		fParent(sGlobalScope),
		fFunctionName(functionName),
		fLevel(fParent != NULL ? fParent->fLevel + 1 : 0)
	{
		sGlobalScope = this;
		if (remainingString != NULL) {
			PRINT("%*s%s(): \"%s\"\n", fLevel * 2, "", fFunctionName,
				remainingString);
		} else
			PRINT("%*s%s()\n", fLevel * 2, "", fFunctionName);
	}

	~DebugScope()
	{
		sGlobalScope = fParent;
		PRINT("%*s%s() done\n", fLevel * 2, "", fFunctionName);
	}

	static void Print(const char* format,...)
	{
		int level = sGlobalScope != NULL ? sGlobalScope->fLevel : 0;

		va_list args;
		va_start(args, format);
		PRINT("%*s", (level + 1) * 2, "");
		VPRINT(format, args);
		va_end(args);
	}

private:
	DebugScope*	fParent;
	const char*	fFunctionName;
	int			fLevel;

	static DebugScope* sGlobalScope;
};

DebugScope* DebugScope::sGlobalScope = NULL;

#endif	// TRACE_GCC3_DEMANGLER


class Input {
public:
	Input()
		:
		fString(NULL),
		fLength(0)
	{
	}

	void SetTo(const char* string, size_t length)
	{
		fString = string;
		fLength = length;
	}

	const char* String() const
	{
		return fString;
	}

	int CharsRemaining() const
	{
		return fLength;
	}

	void Skip(size_t count)
	{
		if (count > fLength) {
			PRINT("Input::Skip(): fOffset > fLength\n");
			return;
		}

		fString += count;
		fLength -= count;
	}

	bool HasPrefix(char prefix) const
	{
		return fLength > 0 && fString[0] == prefix;
	}

	bool HasPrefix(const char* prefix) const
	{
		size_t prefixLen = strlen(prefix);
		return prefixLen <= fLength
			&& strncmp(fString, prefix, strlen(prefix)) == 0;
	}

	bool SkipPrefix(char prefix)
	{
		if (!HasPrefix(prefix))
			return false;

		fString++;
		fLength--;
		return true;
	}

	bool SkipPrefix(const char* prefix)
	{
		size_t prefixLen = strlen(prefix);
		if (prefixLen <= fLength && strncmp(fString, prefix, prefixLen) != 0)
			return false;

		fString += prefixLen;
		fLength -= prefixLen;
		return true;
	}

	char operator[](size_t index) const
	{
		if (index >= fLength) {
			PRINT("Input::operator[](): fOffset + index >= fLength\n");
			return '\0';
		}

		return fString[index];
	}

private:
	const char*	fString;
	size_t		fLength;
};


class NameBuffer {
public:
	NameBuffer(char* buffer, size_t size)
		:
		fBuffer(buffer),
		fSize(size),
		fLength(0),
		fOverflow(false)
	{
	}

	bool IsEmpty() const
	{
		return fLength == 0;
	}

	char LastChar() const
	{
		return fLength > 0 ? fBuffer[fLength - 1] : '\0';
	}

	bool HadOverflow() const
	{
		return fOverflow;
	}

	char* Terminate()
	{
		fBuffer[fLength] = '\0';
		return fBuffer;
	}

	bool Append(const char* string, size_t length)
	{
		if (fLength + length >= fSize) {
			fOverflow = true;
			return false;
		}

		memcpy(fBuffer + fLength, string, length);
		fLength += length;
		return true;
	}

	bool Append(const char* string)
	{
		return Append(string, strlen(string));
	}

private:
	char*	fBuffer;
	size_t	fSize;
	size_t	fLength;
	bool	fOverflow;
};


struct TypeInfo {
	type_type	type;
	int			cvQualifiers;

	TypeInfo()
		:
		type(TYPE_UNKNOWN),
		cvQualifiers(0)
	{
	}

	TypeInfo(type_type type)
		:
		type(type),
		cvQualifiers(0)
	{
	}

	TypeInfo(const TypeInfo& other, int cvQualifiers = 0)
		:
		type(other.type),
		cvQualifiers(other.cvQualifiers | cvQualifiers)
	{
	}

	TypeInfo& operator=(const TypeInfo& other)
	{
		type = other.type;
		cvQualifiers = other.cvQualifiers;
		return *this;
	}
};


struct DemanglingParameters {
	bool	objectNameOnly;

	DemanglingParameters(bool objectNameOnly)
		:
		objectNameOnly(objectNameOnly)
	{
	}
};


struct DemanglingInfo : DemanglingParameters {
	object_type	objectType;

	DemanglingInfo(bool objectNameOnly)
		:
		DemanglingParameters(objectNameOnly),
		objectType(OBJECT_TYPE_UNKNOWN)
	{
	}
};


struct ParameterInfo {
	TypeInfo	type;

	ParameterInfo()
	{
	}
};


class Node;

struct NameDecorationInfo {
	const Node*	firstDecorator;
	const Node*	closestCVDecoratorList;

	NameDecorationInfo(const Node* decorator)
		:
		firstDecorator(decorator),
		closestCVDecoratorList(NULL)
	{
	}
};

struct CVQualifierInfo {
	const Node*	firstCVQualifier;
	const Node*	firstNonCVQualifier;

	CVQualifierInfo()
		:
		firstCVQualifier(NULL),
		firstNonCVQualifier(NULL)
	{
	}
};


class Node {
public:
	Node()
		:
		fNextAllocated(NULL),
		fParent(NULL),
		fNext(NULL),
		fNextReferenceable(NULL),
		fReferenceable(true)
	{
	}

	virtual ~Node()
	{
	}

	Node* NextAllocated() const			{ return fNextAllocated; }
	void SetNextAllocated(Node* node)	{ fNextAllocated = node; }

	Node* Parent() const				{ return fParent; }
	virtual void SetParent(Node* node)	{ fParent = node; }

	Node* Next() const			{ return fNext; }
	void SetNext(Node* node)	{ fNext = node; }

	bool IsReferenceable() const		{ return fReferenceable; }
	void SetReferenceable(bool flag)	{ fReferenceable = flag; }

	Node* NextReferenceable() const			{ return fNextReferenceable; }
	void SetNextReferenceable(Node* node)	{ fNextReferenceable = node; }

	virtual bool GetName(NameBuffer& buffer) const = 0;

	virtual bool GetDecoratedName(NameBuffer& buffer,
		NameDecorationInfo& decorationInfo) const
	{
		if (!GetName(buffer))
			return false;

		return decorationInfo.firstDecorator == NULL
			|| decorationInfo.firstDecorator->AddDecoration(buffer, NULL);
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		return true;
	}

	virtual void GetCVQualifierInfo(CVQualifierInfo& info) const
	{
		info.firstNonCVQualifier = this;
	}

	virtual Node* GetUnqualifiedNode(Node* beforeNode)
	{
		return this;
	}

	virtual bool IsTemplatized() const
	{
		return false;
	}

	virtual Node* TemplateParameterAt(int index) const
	{
		return NULL;
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return false;
	}

	virtual bool IsTypeName(const char* name, size_t length) const
	{
		return false;
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_UNKNOWN;
	}

	virtual prefix_type PrefixType() const
	{
		return PREFIX_NONE;
	}

	virtual TypeInfo Type() const
	{
		return TypeInfo();
	}

private:
	Node*	fNextAllocated;
	Node*	fParent;
	Node*	fNext;
	Node*	fNextReferenceable;
	bool	fReferenceable;
};


class NamedTypeNode : public Node {
public:
	NamedTypeNode(Node* name)
		:
		fName(name)
	{
		if (fName != NULL)
			fName->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return fName == NULL || fName->GetName(buffer);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return fName != NULL && fName->IsNoReturnValueFunction();
	}

	virtual TypeInfo Type() const
	{
		return fName != NULL ? fName->Type() : TypeInfo();
	}

protected:
	Node*	fName;
};


class SubstitutionNode : public Node {
public:
	SubstitutionNode(Node* node)
		:
		fNode(node)
	{
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return fNode->GetName(buffer);
	}

	virtual bool GetDecoratedName(NameBuffer& buffer,
		NameDecorationInfo& decorationInfo) const
	{
		return fNode->GetDecoratedName(buffer, decorationInfo);
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		return fNode->AddDecoration(buffer, stopDecorator);
	}

	virtual void GetCVQualifierInfo(CVQualifierInfo& info) const
	{
		fNode->GetCVQualifierInfo(info);
	}

	virtual bool IsTemplatized() const
	{
		return fNode->IsTemplatized();
	}

	virtual Node* TemplateParameterAt(int index) const
	{
		return fNode->TemplateParameterAt(index);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return fNode->IsNoReturnValueFunction();
	}

	virtual bool IsTypeName(const char* name, size_t length) const
	{
		return fNode->IsTypeName(name, length);
	}

	virtual object_type ObjectType() const
	{
		return fNode->ObjectType();
	}

	virtual prefix_type PrefixType() const
	{
		return fNode->PrefixType();
	}

	virtual TypeInfo Type() const
	{
		return fNode->Type();
	}

private:
	Node*	fNode;
};


class ArrayNode : public NamedTypeNode {
public:
	ArrayNode(Node* type, int dimension)
		:
		NamedTypeNode(type),
		fDimensionExpression(NULL),
		fDimensionNumber(dimension)
	{
	}

	ArrayNode(Node* type, Node* dimension)
		:
		NamedTypeNode(type),
		fDimensionExpression(dimension),
		fDimensionNumber(0)
	{
		fDimensionExpression->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		if (!fName->GetName(buffer))
			return false;

		buffer.Append("[", 1);

		if (fDimensionExpression != NULL) {
			if (!fDimensionExpression->GetName(buffer))
				return false;
		} else {
			char stringBuffer[16];
			snprintf(stringBuffer, sizeof(stringBuffer), "%d",
				fDimensionNumber);
			buffer.Append(stringBuffer);
		}

		return buffer.Append("]", 1);
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

	virtual TypeInfo Type() const
	{
// TODO: Check!
		return TypeInfo(TYPE_POINTER);
	}


private:
	Node*	fDimensionExpression;
	int		fDimensionNumber;
};


class ObjectNode : public NamedTypeNode {
public:
	ObjectNode(Node* name)
		:
		NamedTypeNode(name)
	{
	}

	virtual bool GetObjectName(NameBuffer& buffer,
		const DemanglingParameters& parameters)
	{
		if (parameters.objectNameOnly)
			return fName != NULL ? fName->GetName(buffer) : true;

		return GetName(buffer);
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

	virtual Node* ParameterAt(uint32 index) const
	{
		return NULL;
	}
};


class SimpleNameNode : public Node {
public:
	SimpleNameNode(const char* name)
		:
		fName(name),
		fLength(strlen(name))
	{
	}

	SimpleNameNode(const char* name, size_t length)
		:
		fName(name),
		fLength(length)
	{
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return buffer.Append(fName, fLength);
	}

protected:
	const char*	fName;
	size_t		fLength;
};


class SimpleTypeNode : public SimpleNameNode {
public:
	SimpleTypeNode(const char* name)
		:
		SimpleNameNode(name),
		fType(TYPE_UNKNOWN)
	{
	}

	SimpleTypeNode(type_type type)
		:
		SimpleNameNode(kTypeNames[type]),
		fType(type)
	{
	}

	virtual bool IsTypeName(const char* name, size_t length) const
	{
		return fLength == length && strcmp(fName, name) == 0;
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

	virtual TypeInfo Type() const
	{
		return TypeInfo(fType);
	}

private:
	type_type	fType;
};


class TypedNumberLiteralNode : public Node {
public:
	TypedNumberLiteralNode(Node* type, const char* number, size_t length)
		:
		fType(type),
		fNumber(number),
		fLength(length)
	{
		fType->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		// If the type is bool and the number is 0 or 1, we use "false" or
		// "true" respectively.
		if (fType->IsTypeName("bool", 4) && fLength == 1
			&& (fNumber[0] == '0' || fNumber[0] == '1')) {
			return buffer.Append(fNumber[0] == '0' ? "false" : "true");
		}

		// Add the type in parentheses. The GNU demangler omits "int", so do we.
		if (!fType->IsTypeName("int", 3)) {
			buffer.Append("(");
			if (!fType->GetName(buffer))
				return false;
			buffer.Append(")");
		}

		// add the number -- replace a leading 'n' by '-', if necessary
		if (fLength > 0 && fNumber[0] == 'n') {
			buffer.Append("-");
			return buffer.Append(fNumber + 1, fLength - 1);
		}

		return buffer.Append(fNumber, fLength);
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

private:
	Node*		fType;
	const char*	fNumber;
	size_t		fLength;
};


class XtructorNode : public Node {
public:
	XtructorNode(bool constructor, char type)
		:
		fConstructor(constructor),
		fType(type)
	{
	}

	virtual void SetParent(Node* node)
	{
		fUnqualifiedNode = node->GetUnqualifiedNode(this);
		Node::SetParent(node);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		if (fUnqualifiedNode == NULL)
			return false;

		if (!fConstructor)
			buffer.Append("~");

		return fUnqualifiedNode->GetName(buffer);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return true;
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_METHOD_CLASS;
	}

private:
	bool		fConstructor;
	char		fType;
	Node*		fUnqualifiedNode;
};


class SpecialNameNode : public Node {
public:
	SpecialNameNode(const char* name, Node* child)
		:
		fName(name),
		fChild(child)
	{
		fChild->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return buffer.Append(fName) && fChild->GetName(buffer);
	}

protected:
	const char*	fName;
	Node*		fChild;
};


class DecoratingNode : public Node {
public:
	DecoratingNode(Node* child)
		:
		fChildNode(child)
	{
		fChildNode->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		NameDecorationInfo decorationInfo(this);
		return fChildNode->GetDecoratedName(buffer, decorationInfo);
	}

	virtual bool GetDecoratedName(NameBuffer& buffer,
		NameDecorationInfo& decorationInfo) const
	{
		decorationInfo.closestCVDecoratorList = NULL;
		return fChildNode->GetDecoratedName(buffer, decorationInfo);
	}

protected:
	Node*	fChildNode;
};


class CVQualifiersNode : public DecoratingNode {
public:
	CVQualifiersNode(int qualifiers, Node* child)
		:
		DecoratingNode(child),
		fCVQualifiers(qualifiers)
	{
	}

	virtual bool GetDecoratedName(NameBuffer& buffer,
		NameDecorationInfo& decorationInfo) const
	{
		if (decorationInfo.closestCVDecoratorList == NULL)
			decorationInfo.closestCVDecoratorList = this;
		return fChildNode->GetDecoratedName(buffer, decorationInfo);
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		if (this == stopDecorator)
			return true;

		if (!fChildNode->AddDecoration(buffer, stopDecorator))
			return false;

		if ((fCVQualifiers & CV_QUALIFIER_RESTRICT) != 0)
			buffer.Append(" restrict");
		if ((fCVQualifiers & CV_QUALIFIER_VOLATILE) != 0)
			buffer.Append(" volatile");
		if ((fCVQualifiers & CV_QUALIFIER_CONST) != 0)
			buffer.Append(" const");

		return true;
	}

	virtual void GetCVQualifierInfo(CVQualifierInfo& info) const
	{
		if (info.firstCVQualifier == NULL)
			info.firstCVQualifier = this;
		fChildNode->GetCVQualifierInfo(info);
	}

	virtual bool IsTemplatized() const
	{
		return fChildNode->IsTemplatized();
	}

	virtual Node* TemplateParameterAt(int index) const
	{
		return fChildNode->TemplateParameterAt(index);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return fChildNode->IsNoReturnValueFunction();
	}

	virtual object_type ObjectType() const
	{
		return fChildNode->ObjectType();
	}

	virtual prefix_type PrefixType() const
	{
		return fChildNode->PrefixType();
	}

	virtual TypeInfo Type() const
	{
		return TypeInfo(fChildNode->Type(), fCVQualifiers);
	}

private:
	int		fCVQualifiers;
};


class TypeModifierNode : public DecoratingNode {
public:
	TypeModifierNode(type_modifier modifier, Node* child)
		:
		DecoratingNode(child),
		fModifier(modifier)
	{
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		if (this == stopDecorator)
			return true;

		return fChildNode->AddDecoration(buffer, stopDecorator)
			&& buffer.Append(kTypeModifierSuffixes[fModifier]);
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

	virtual TypeInfo Type() const
	{
		TypeInfo type = fChildNode->Type();
		if (type.type == TYPE_CHAR
			&& (type.cvQualifiers & CV_QUALIFIER_CONST) != 0) {
			return TypeInfo(TYPE_CONST_CHAR_POINTER);
		}

		switch (fModifier) {
			case TYPE_QUALIFIER_POINTER:
				return TypeInfo(TYPE_POINTER);
			case TYPE_QUALIFIER_REFERENCE:
				return TypeInfo(TYPE_REFERENCE);
			default:
				return TypeInfo();
		}
	}

private:
	type_modifier	fModifier;
};


class VendorTypeModifierNode : public DecoratingNode {
public:
	VendorTypeModifierNode(Node* name, Node* child)
		:
		DecoratingNode(child),
		fName(name)
	{
		fName->SetParent(this);
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		if (this == stopDecorator)
			return true;

		return fChildNode->AddDecoration(buffer, stopDecorator)
			&& buffer.Append(" ")
			&& fName->GetName(buffer);
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

private:
	Node*	fName;
};


class OperatorNode : public Node {
public:
	OperatorNode(const operator_info* info)
		:
		fInfo(info)
	{
		SetReferenceable(false);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return buffer.Append(
				isalpha(fInfo->name[0]) ?  "operator " : "operator")
			&& buffer.Append(fInfo->name);
	}

	virtual object_type ObjectType() const
	{
		return (fInfo->flags & OPERATOR_IS_MEMBER) != 0
			? OBJECT_TYPE_METHOD_CLASS : OBJECT_TYPE_UNKNOWN;
	}

private:
	const operator_info*	fInfo;
};


class VendorOperatorNode : public Node {
public:
	VendorOperatorNode(Node* name)
		:
		fName(name)
	{
		fName->SetParent(this);
		SetReferenceable(false);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return buffer.Append("operator ")
			&& fName->GetName(buffer);
	}

private:
	Node*	fName;
};


class CastOperatorNode : public Node {
public:
	CastOperatorNode(Node* child)
		:
		fChildNode(child)
	{
		fChildNode->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		return buffer.Append("operator ") && fChildNode->GetName(buffer);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return true;
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_METHOD_OBJECT;
	}

private:
	Node*	fChildNode;
};


class PrefixedNode : public Node {
public:
	PrefixedNode(Node* prefix, Node* node)
		:
		fPrefixNode(prefix),
		fNode(node)
	{
		fPrefixNode->SetParent(this);
		fNode->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		if (!fPrefixNode->GetName(buffer))
			return false;

		buffer.Append("::");
		return fNode->GetName(buffer);
	}

	virtual Node* GetUnqualifiedNode(Node* beforeNode)
	{
		return beforeNode == fNode
			? fPrefixNode->GetUnqualifiedNode(beforeNode)
			: fNode->GetUnqualifiedNode(beforeNode);
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return fNode->IsNoReturnValueFunction();
	}

	virtual object_type ObjectType() const
	{
		return fNode->ObjectType();
	}

	virtual prefix_type PrefixType() const
	{
		return PREFIX_UNKNOWN;
	}

private:
	Node*	fPrefixNode;
	Node*	fNode;
};


typedef PrefixedNode DependentNameNode;


class TemplateNode : public Node {
public:
	TemplateNode(Node* base)
		:
		fBase(base),
		fFirstArgument(NULL),
		fLastArgument(NULL)
	{
		fBase->SetParent(this);
	}

	void AddArgument(Node* child)
	{
		child->SetParent(this);

		if (fLastArgument != NULL) {
			fLastArgument->SetNext(child);
			fLastArgument = child;
		} else {
			fFirstArgument = child;
			fLastArgument = child;
		}
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		if (!fBase->GetName(buffer))
			return false;

		buffer.Append("<");

		Node* child = fFirstArgument;
		while (child != NULL) {
			if (child != fFirstArgument)
				buffer.Append(", ");

			if (!child->GetName(buffer))
				return false;

			child = child->Next();
		}

		// add a space between consecutive '>'
		if (buffer.LastChar() == '>')
			buffer.Append(" ");

		return buffer.Append(">");
	}

	virtual Node* GetUnqualifiedNode(Node* beforeNode)
	{
		return fBase != beforeNode
			? fBase->GetUnqualifiedNode(beforeNode) : this;
	}

	virtual bool IsTemplatized() const
	{
		return true;
	}

	virtual Node* TemplateParameterAt(int index) const
	{
		Node* child = fFirstArgument;
		while (child != NULL) {
			if (index == 0)
				return child;
			index--;
			child = child->Next();
		}

		return NULL;
	}

	virtual bool IsNoReturnValueFunction() const
	{
		return fBase->IsNoReturnValueFunction();
	}

	virtual object_type ObjectType() const
	{
		return fBase->ObjectType();
	}

	virtual prefix_type PrefixType() const
	{
		return fBase->PrefixType();
	}

protected:
	Node*	fBase;
	Node*	fFirstArgument;
	Node*	fLastArgument;
};


class MultiSubExpressionsNode : public Node {
public:
	MultiSubExpressionsNode()
		:
		fFirstSubExpression(NULL),
		fLastSubExpression(NULL)
	{
	}

	void AddSubExpression(Node* child)
	{
		child->SetParent(this);

		if (fLastSubExpression != NULL) {
			fLastSubExpression->SetNext(child);
			fLastSubExpression = child;
		} else {
			fFirstSubExpression = child;
			fLastSubExpression = child;
		}
	}

protected:
	Node*		fFirstSubExpression;
	Node*		fLastSubExpression;
};


class CallNode : public MultiSubExpressionsNode {
public:
	CallNode()
	{
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		// TODO: Use the real syntax!
		buffer.Append("call(");

		Node* child = fFirstSubExpression;
		while (child != NULL) {
			if (child != fFirstSubExpression)
				buffer.Append(", ");

			if (!child->GetName(buffer))
				return false;

			child = child->Next();
		}

		buffer.Append(")");

		return true;
	}
};


class OperatorExpressionNode : public MultiSubExpressionsNode {
public:
	OperatorExpressionNode(const operator_info* info)
		:
		fInfo(info)
	{
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		bool isIdentifier = isalpha(fInfo->name[0]) || fInfo->name[0] == '_';

		if (fInfo->argument_count == 1 || isIdentifier
			|| fInfo->argument_count > 3
			|| (fInfo->argument_count == 3 && strcmp(fInfo->name, "?") != 0)) {
			// prefix operator
			buffer.Append(fInfo->name);

			if (isIdentifier)
				buffer.Append("(");

			Node* child = fFirstSubExpression;
			while (child != NULL) {
				if (child != fFirstSubExpression)
					buffer.Append(", ");

				if (!child->GetName(buffer))
					return false;

				child = child->Next();
			}

			if (isIdentifier)
				buffer.Append(")");

			return true;
		}

		Node* arg1 = fFirstSubExpression;
		Node* arg2 = arg1->Next();

		buffer.Append("(");

		if (fInfo->argument_count == 2) {
			// binary infix operator
			if (!arg1->GetName(buffer))
				return false;

			buffer.Append(" ");
			buffer.Append(fInfo->name);
			buffer.Append(" ");

			if (!arg2->GetName(buffer))
				return false;

			return buffer.Append(")");
		}

		Node* arg3 = arg2->Next();

		if (fInfo->argument_count == 2) {
			// trinary operator "... ? ... : ..."
			if (!arg1->GetName(buffer))
				return false;

			buffer.Append(" ? ");

			if (!arg2->GetName(buffer))
				return false;

			buffer.Append(" : ");

			if (!arg3->GetName(buffer))
				return false;

			return buffer.Append(")");
		}

		return false;
	}

private:
	const operator_info*	fInfo;
};


class ConversionExpressionNode : public MultiSubExpressionsNode {
public:
	ConversionExpressionNode(Node* type)
		:
		fType(type)
	{
		fType->SetParent(this);
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		buffer.Append("(");

		if (!fType->GetName(buffer))
			return false;

		buffer.Append(")(");

		Node* child = fFirstSubExpression;
		while (child != NULL) {
			if (child != fFirstSubExpression)
				buffer.Append(", ");

			if (!child->GetName(buffer))
				return false;

			child = child->Next();
		}

		return buffer.Append(")");
	}

private:
	Node*	fType;
};


class PointerToMemberNode : public DecoratingNode {
public:
	PointerToMemberNode(Node* classType, Node* memberType)
		:
		DecoratingNode(memberType),
		fClassType(classType)
	{
		fClassType->SetParent(this);
	}

	virtual bool AddDecoration(NameBuffer& buffer,
		const Node* stopDecorator) const
	{
		if (this == stopDecorator)
			return true;

		if (!fChildNode->AddDecoration(buffer, stopDecorator))
			return false;

		// In most cases we need a space before the name. In some it is
		// superfluous, though.
		if (!buffer.IsEmpty() && buffer.LastChar() != '(')
			buffer.Append(" ");

		if (!fClassType->GetName(buffer))
			return false;

		return buffer.Append("::*");
	}

	virtual object_type ObjectType() const
	{
		return OBJECT_TYPE_DATA;
	}

	virtual TypeInfo Type() const
	{
		// TODO: Method pointers aren't ordinary pointers. Though we might not
		// be able to determine the difference.
		return TypeInfo(TYPE_POINTER);
	}


private:
	Node*	fClassType;
};


class FunctionNode : public ObjectNode {
public:
	FunctionNode(Node* nameNode, bool hasReturnType, bool isExternC)
		:
		ObjectNode(nameNode),
		fFirstTypeNode(NULL),
		fLastTypeNode(NULL),
		fHasReturnType(hasReturnType),
		fIsExternC(isExternC)
	{
	}

	void AddType(Node* child)
	{
		child->SetParent(this);

		if (fLastTypeNode != NULL) {
			fLastTypeNode->SetNext(child);
			fLastTypeNode = child;
		} else {
			fFirstTypeNode = child;
			fLastTypeNode = child;
		}
	}

	virtual bool GetName(NameBuffer& buffer) const
	{
		NameDecorationInfo decorationInfo(NULL);
		return GetDecoratedName(buffer, decorationInfo);
	}

	virtual bool GetDecoratedName(NameBuffer& buffer,
		NameDecorationInfo& decorationInfo) const
	{
		// write 'extern "C"'
//		if (fIsExternC)
//			buffer.Append("extern \"C\"");

		// write the return type
		Node* child = fFirstTypeNode;
		if (_HasReturnType() && child != NULL) {
			if (!child->GetName(buffer))
				return false;
			child = child->Next();

			buffer.Append(" ", 1);
		}

		// write the function name
		if (fName == NULL)
			buffer.Append("(", 1);

		CVQualifierInfo info;
		if (fName != NULL) {
			// skip CV qualifiers on our name -- we'll add them later
			fName->GetCVQualifierInfo(info);
			if (info.firstNonCVQualifier != NULL
				&& !info.firstNonCVQualifier->GetName(buffer)) {
				return false;
			}
		}

		// add non-CV qualifier decorations
		if (decorationInfo.firstDecorator != NULL) {
			if (!decorationInfo.firstDecorator->AddDecoration(buffer,
				decorationInfo.closestCVDecoratorList)) {
				return false;
			}
		}

		if (fName == NULL)
			buffer.Append(")", 1);

		// add the parameter types
		buffer.Append("(");

		// don't add a single "void" parameter
		if (child != NULL && child->Next() == NULL
			&& child->IsTypeName("void", 4)) {
			child = NULL;
		}

		Node* firstParam = child;
		while (child != NULL) {
			if (child != firstParam)
				buffer.Append(", ");

			if (!child->GetName(buffer))
				return false;

			child = child->Next();
		}

		buffer.Append(")");

		// add CV qualifiers on our name
		if (info.firstCVQualifier != NULL) {
			if (!info.firstCVQualifier->AddDecoration(buffer,
					info.firstNonCVQualifier)) {
				return false;
			}
		}

		// add CV qualifiers on us
		if (decorationInfo.closestCVDecoratorList != NULL)
			decorationInfo.closestCVDecoratorList->AddDecoration(buffer, NULL);

		return true;
	}

	virtual object_type ObjectType() const
	{
		// no name, no fun
		if (fName == NULL)
			return OBJECT_TYPE_FUNCTION;

		// check our name's prefix
		switch (fName->PrefixType()) {
			case PREFIX_NONE:
			case PREFIX_NAMESPACE:
				return OBJECT_TYPE_FUNCTION;
			case PREFIX_CLASS:
			case PREFIX_UNKNOWN:
				break;
		}

		// Our name has a prefix, but we don't know, whether it is a class or
		// namespace. Let's ask our name what it thinks it is.
		object_type type = fName->ObjectType();
		switch (type) {
			case OBJECT_TYPE_FUNCTION:
			case OBJECT_TYPE_METHOD_CLASS:
			case OBJECT_TYPE_METHOD_OBJECT:
			case OBJECT_TYPE_METHOD_UNKNOWN:
				// That's as good as it gets.
				return type;
			case OBJECT_TYPE_UNKNOWN:
			case OBJECT_TYPE_DATA:
			default:
				// Obviously our name doesn't have a clue.
				return OBJECT_TYPE_METHOD_UNKNOWN;
		}
	}

	virtual Node* ParameterAt(uint32 index) const
	{
		// skip return type
		Node* child = fFirstTypeNode;
		if (_HasReturnType() && child != NULL)
			child = child->Next();

		// ignore a single "void" parameter
		if (child != NULL && child->Next() == NULL
			&& child->IsTypeName("void", 4)) {
			return NULL;
		}

		// get the type at the index
		while (child != NULL && index > 0) {
			child = child->Next();
			index--;
		}

		return child;
	}

private:
	bool _HasReturnType() const
	{
		return fHasReturnType
			|| fName == NULL
			|| (fName->IsTemplatized() && !fName->IsNoReturnValueFunction());
	}

private:
	Node*		fFirstTypeNode;
	Node*		fLastTypeNode;
	bool		fHasReturnType;
	bool		fIsExternC;
};


// #pragma mark - Demangler


class Demangler {
public:
								Demangler();

			int					Demangle(const char* mangledName, char* buffer,
									size_t size,
									DemanglingInfo& demanglingInfo);
			int					GetParameterInfo(const char* mangledName,
									uint32 index, char* buffer, size_t size,
									ParameterInfo& info);

	// actually private, but public to make gcc 2 happy
	inline	bool				_SetError(int error);
	inline	void				_AddAllocatedNode(Node* node);

private:
	template<typename NodeType> struct NodeCreator;

	inline	bool				_SkipExpected(char c);
	inline	bool				_SkipExpected(const char* string);

			void				_Init();
			void				_Cleanup();

			int					_Demangle(const char* mangledName, char* buffer,
									size_t size,
									DemanglingInfo& demanglingInfo);
			int					_GetParameterInfo(const char* mangledName,
									uint32 index, char* buffer, size_t size,
									ParameterInfo& info);

			int					_Parse(const char* mangledName,
									const char*& versionSuffix,
									ObjectNode*& _node);

			bool				_ParseEncoding(ObjectNode*& _node);
			bool				_ParseSpecialName(Node*& _node);
			bool				_ParseCallOffset(bool& nonVirtual,
									number_type& offset1, number_type& offset2);
			bool				_ParseName(Node*& _node);
			bool				_ParseNestedName(Node*& _node);
			bool				_ParseNestedNameInternal(Node*& _node);
			bool				_ParseLocalName(Node*& _node);
			bool				_ParseUnqualifiedName(Node*& _node);
			bool				_ParseSourceName(Node*& _node);
			bool				_ParseOperatorName(Node*& _node);
			bool				_ParseType(Node*& _node);
			bool				_ParseTypeInternal(Node*& _node);
			void				_ParseCVQualifiers(int& qualifiers);
			bool				_ParseTypeWithModifier(type_modifier modifier,
									int toSkip, Node*& _node);
			bool				_TryParseBuiltinType(Node*& _node);
			bool				_ParseFunctionType(FunctionNode*& _node);;
			bool				_ParseArrayType(Node*& _node);
			bool				_ParsePointerToMemberType(Node*& _node);
			bool				_ParseTemplateParam(Node*& _node);
			bool				_ParseSubstitution(Node*& _node);
			bool				_ParseSubstitutionInternal(Node*& _node);
			bool				_ParseBareFunctionType(FunctionNode* node);
			bool				_ParseTemplateArgs(Node* node, Node*& _node);
			bool				_ParseTemplateArg(Node*& _node);
			bool				_ParseExpression(Node*& _node);
			bool				_ParseExpressionPrimary(Node*& _node);
			bool				_ParseNumber(number_type& number);

			bool				_CreateNodeAndSkip(const char* name,
									size_t length, int toSkip, Node*& _node);
			bool				_CreateNodeAndSkip(const char* name, int toSkip,
									Node*& _node);
			bool				_CreateTypeNodeAndSkip(type_type type,
									int toSkip, Node*& _node);
			bool				_CreateTypeNodeAndSkip(const char* name,
									const char* prefix,
									const char* templateArgs, int toSkip,
									Node*& _node);

			void				_RegisterReferenceableNode(Node* node);
			bool				_CreateSubstitutionNode(int index,
									Node*& _node);

private:
			Input				fInput;
			int					fError;
			Node*				fAllocatedNodes;
			Node*				fFirstReferenceableNode;
			Node*				fLastReferenceableNode;
			Node*				fTemplatizedNode;
};


template<typename NodeType>
struct Demangler::NodeCreator {
	NodeCreator(Demangler* demangler)
		:
		fDemangler(demangler)
	{
	}

	template<typename ReturnType>
	inline bool operator()(ReturnType*& _node) const
	{
		_node = NEW(NodeType);
		if (_node == NULL)
			return fDemangler->_SetError(ERROR_NO_MEMORY);

		fDemangler->_AddAllocatedNode(_node);
		return true;
	}

	template<typename ParameterType1, typename ReturnType>
	inline bool operator()(ParameterType1 arg1, ReturnType*& _node) const
	{
		_node = NEW(NodeType(arg1));
		if (_node == NULL)
			return fDemangler->_SetError(ERROR_NO_MEMORY);

		fDemangler->_AddAllocatedNode(_node);
		return true;
	}

	template<typename ParameterType1, typename ParameterType2,
		typename ReturnType>
	inline bool operator()(ParameterType1 arg1, ParameterType2 arg2,
		ReturnType*& _node) const
	{
		_node = NEW(NodeType(arg1, arg2));
		if (_node == NULL)
			return fDemangler->_SetError(ERROR_NO_MEMORY);

		fDemangler->_AddAllocatedNode(_node);
		return true;
	}

	template<typename ParameterType1, typename ParameterType2,
		typename ParameterType3, typename ReturnType>
	inline bool operator()(ParameterType1 arg1, ParameterType2 arg2,
		ParameterType3 arg3, ReturnType*& _node) const
	{
		_node = NEW(NodeType(arg1, arg2, arg3));
		if (_node == NULL)
			return fDemangler->_SetError(ERROR_NO_MEMORY);

		fDemangler->_AddAllocatedNode(_node);
		return true;
	}

private:
		Demangler*	fDemangler;
};


inline bool
Demangler::_SetError(int error)
{
	if (fError == ERROR_OK) {
		fError = error;
#ifdef TRACE_GCC3_DEMANGLER
		DebugScope::Print("_SetError(): %d, remaining input: \"%s\"\n",
			error, fInput.String());
#endif
	}
	return false;
}


inline void
Demangler::_AddAllocatedNode(Node* node)
{
	node->SetNextAllocated(fAllocatedNodes);
	fAllocatedNodes = node;
}


inline bool
Demangler::_SkipExpected(char c)
{
	return fInput.SkipPrefix(c) || _SetError(ERROR_INVALID);
}


inline bool
Demangler::_SkipExpected(const char* string)
{
	return fInput.SkipPrefix(string) || _SetError(ERROR_INVALID);
}


Demangler::Demangler()
	:
	fInput(),
	fAllocatedNodes(NULL)
{
}


int
Demangler::Demangle(const char* mangledName, char* buffer, size_t size,
	DemanglingInfo& demanglingInfo)
{
	DEBUG_SCOPE("Demangle");

	_Init();

	int result = _Demangle(mangledName, buffer, size, demanglingInfo);

	_Cleanup();

	return result;
}


int
Demangler::GetParameterInfo(const char* mangledName, uint32 index, char* buffer,
	size_t size, ParameterInfo& info)
{
	DEBUG_SCOPE("GetParameterInfo");

	_Init();

	int result = _GetParameterInfo(mangledName, index, buffer, size, info);

	_Cleanup();

	return result;
}


void
Demangler::_Init()
{
	fError = ERROR_OK;

	fFirstReferenceableNode = NULL;
	fLastReferenceableNode = NULL;
	fAllocatedNodes = NULL;
	fTemplatizedNode = NULL;
}


void
Demangler::_Cleanup()
{
	while (fAllocatedNodes != NULL) {
		Node* node = fAllocatedNodes;
		fAllocatedNodes = node->NextAllocated();
		DELETE(node);
	}
}


int
Demangler::_Demangle(const char* mangledName, char* buffer, size_t size,
	DemanglingInfo& demanglingInfo)
{
	// parse the name
	const char* versionSuffix;
	ObjectNode* node;
	int error = _Parse(mangledName, versionSuffix, node);
	if (error != ERROR_OK)
		return error;

	NameBuffer nameBuffer(buffer, size);
	bool success = node->GetObjectName(nameBuffer, demanglingInfo);

	// If versioned, append the unmodified version string
	if (success && versionSuffix != NULL)
		nameBuffer.Append(versionSuffix);

	if (nameBuffer.HadOverflow())
		return ERROR_BUFFER_TOO_SMALL;

	if (!success)
		return ERROR_INTERNAL;

	demanglingInfo.objectType = node->ObjectType();

	nameBuffer.Terminate();
	return ERROR_OK;
}


int
Demangler::_GetParameterInfo(const char* mangledName, uint32 index,
	char* buffer, size_t size, ParameterInfo& info)
{
	// parse the name
	const char* versionSuffix;
	ObjectNode* node;
	int error = _Parse(mangledName, versionSuffix, node);
	if (error != ERROR_OK)
		return error;

	// get the parameter node
	Node* parameter = node->ParameterAt(index);
	if (parameter == NULL)
		return ERROR_INVALID_PARAMETER_INDEX;

	// get the parameter name
	NameBuffer nameBuffer(buffer, size);
	bool success = parameter->GetName(nameBuffer);

	if (nameBuffer.HadOverflow())
		return ERROR_BUFFER_TOO_SMALL;

	if (!success)
		return ERROR_INTERNAL;

	nameBuffer.Terminate();

	// get the type
	info.type = parameter->Type();

	return ERROR_OK;
}


int
Demangler::_Parse(const char* mangledName, const char*& versionSuffix,
	ObjectNode*& _node)
{
	// To support versioned symbols, we ignore the version suffix when
	// demangling.
	versionSuffix = strchr(mangledName, '@');
	fInput.SetTo(mangledName,
		versionSuffix != NULL
			? versionSuffix - mangledName : strlen(mangledName));

	// <mangled-name> ::= _Z <encoding>

	if (!fInput.SkipPrefix("_Z"))
		return ERROR_NOT_MANGLED;

	if (!_ParseEncoding(_node))
		return fError;

	if (fInput.CharsRemaining() != 0) {
		// bogus at end of input
		return ERROR_INVALID;
	}

	return ERROR_OK;
}


bool
Demangler::_ParseEncoding(ObjectNode*& _node)
{
	DEBUG_SCOPE("_ParseEncoding");

	// <encoding> ::= <function name> <bare-function-type>
	//	          ::= <data name>
	//	          ::= <special-name>

	// NOTE: This is not in the specs: Local entities seem to be prefixed
	// by an 'L'.
	fInput.SkipPrefix('L');

	// parse <special-name>, if it is one
	Node* name;
	if (fInput.HasPrefix('T') || fInput.HasPrefix("GV")) {
		return _ParseSpecialName(name)
			&& NodeCreator<ObjectNode>(this)(name, _node);
	}

	// either <data name> or <function name>
	if (!_ParseName(name))
		return false;

	if (fInput.CharsRemaining() == 0 || fInput.HasPrefix('E')) {
		// <data name>
		return NodeCreator<ObjectNode>(this)(name, _node);
	}

	// <function name> -- parse remaining <bare-function-type>
	FunctionNode* functionNode;
	if (!NodeCreator<FunctionNode>(this)(name, false, false, functionNode))
		return false;
	_node = functionNode;

	// If our name is templatized, we push it onto the templatized node
	// stack while parsing the function parameters.
	Node* previousTemplatizedNode = fTemplatizedNode;
	if (name->IsTemplatized())
		fTemplatizedNode = name;

	if (!_ParseBareFunctionType(functionNode))
		return false;

	fTemplatizedNode = previousTemplatizedNode;

	return true;
}


bool
Demangler::_ParseSpecialName(Node*& _node)
{
	DEBUG_SCOPE("_ParseSpecialName");

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	// <special-name> ::= GV <object name>	# Guard variable for one-time
	//                                      # initialization
	//                    # No <type>
	if (!fInput.SkipPrefix('T')) {
		Node* name;
		return _SkipExpected("GV")
			&& _ParseName(name)
			&& NodeCreator<SpecialNameNode>(this)("guard variable for ",
				name, _node);
	}

	// <special-name> ::= TV <type>	# virtual table
	//                ::= TT <type>	# VTT structure (construction vtable
	//                              # index)
	//                ::= TI <type>	# typeinfo structure
	//                ::= TS <type>	# typeinfo name (null-terminated byte
	//                              # string)
	const char* prefix = NULL;
	switch (fInput[0]) {
		case 'V':
			prefix = "vtable for ";
			break;
		case 'T':
			prefix = "VTT for ";
			break;
		case 'I':
			prefix = "typeinfo for ";
			break;
		case 'S':
			prefix = "typeinfo name for ";
			break;
	}

	if (prefix != NULL) {
		fInput.Skip(1);
		Node* type;
		return _ParseType(type)
			&& NodeCreator<SpecialNameNode>(this)(prefix, type, _node);
	}

	// <special-name> ::= Tc <call-offset> <call-offset> <base encoding>
	//                    # base is the nominal target function of thunk
	//                    # first call-offset is 'this' adjustment
	//                    # second call-offset is result adjustment
	if (fInput.SkipPrefix('c')) {
		bool nonVirtual;
		number_type offset1;
		number_type offset2;
		ObjectNode* name;
		return _ParseCallOffset(nonVirtual, offset1, offset2)
			&& _ParseCallOffset(nonVirtual, offset1, offset2)
			&& _ParseEncoding(name)
			&& NodeCreator<SpecialNameNode>(this)(
				"covariant return thunk to ", name, _node);
	}

	// <special-name> ::= T <call-offset> <base encoding>
	//                    # base is the nominal target function of thunk
	bool nonVirtual;
	number_type offset1;
	number_type offset2;
	ObjectNode* name;
	return _ParseCallOffset(nonVirtual, offset1, offset2)
		&& _ParseEncoding(name)
		&& NodeCreator<SpecialNameNode>(this)(
			nonVirtual ? "non-virtual thunk to " : "virtual thunk to ",
			name, _node);
}


bool
Demangler::_ParseCallOffset(bool& nonVirtual, number_type& offset1,
	number_type& offset2)
{
	// <call-offset> ::= h <nv-offset> _
	//               ::= v <v-offset> _
	// <nv-offset> ::= <offset number>
	//                 # non-virtual base override
	// <v-offset>  ::= <offset number> _ <virtual offset number>
	//                 # virtual base override, with vcall offset

	// non-virtual
	if (fInput.SkipPrefix('h')) {
		nonVirtual = true;
		return _ParseNumber(offset1) && _SkipExpected('_');
	}

	// virtual
	nonVirtual = false;
	return _SkipExpected('v')
		&& _ParseNumber(offset1)
		&& _SkipExpected('_')
		&& _ParseNumber(offset2)
		&& _SkipExpected('_');
}

bool
Demangler::_ParseName(Node*& _node)
{
	DEBUG_SCOPE("_ParseName");

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	// <name> ::= <nested-name>
	//        ::= <unscoped-name>
	//        ::= <unscoped-template-name> <template-args>
	//        ::= <local-name>	# See Scope Encoding below
	//
	// <unscoped-name> ::= <unqualified-name>
	//                 ::= St <unqualified-name>   # ::std::
	//
	// <unscoped-template-name> ::= <unscoped-name>
	//                          ::= <substitution>

	switch (fInput[0]) {
		case 'N':
			// <nested-name>
			return _ParseNestedName(_node);
		case 'Z':
			// <local-name>
			return _ParseLocalName(_node);
		case 'S':
		{
			// <substitution>
			if (!fInput.HasPrefix("St")) {
				if (!_ParseSubstitution(_node))
					return false;
				break;
			}

			// std:: namespace
			fInput.Skip(2);

			Node* prefix;
			if (!NodeCreator<SimpleNameNode>(this)("std", prefix))
				return false;

			// <unqualified-name>
			Node* node;
			if (!_ParseUnqualifiedName(node)
				|| !NodeCreator<PrefixedNode>(this)(prefix, node, _node)) {
				return false;
			}

			break;
		}
		default:
			// <unqualified-name>
			if (!_ParseUnqualifiedName(_node))
				return false;
			break;
	}

	// We get here for the names that might be an <unscoped-template-name>.
	// Check whether <template-args> are following.
	if (!fInput.HasPrefix('I'))
		return true;

	// <unscoped-template-name> is referenceable
	_RegisterReferenceableNode(_node);

	return _ParseTemplateArgs(_node, _node);
}


bool
Demangler::_ParseNestedName(Node*& _node)
{
	DEBUG_SCOPE("_ParseNestedName");

	// <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
	//               ::= N [<CV-qualifiers>] <template-prefix>
	//                   <template-args> E
	//
	// <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const
	//
	// <prefix> ::= <prefix> <unqualified-name>
	//          ::= <template-prefix> <template-args>
	//          ::= <template-param>
	//          ::= # empty
	//          ::= <substitution>
	//
	// <template-prefix> ::= <prefix> <template unqualified-name>
	//                   ::= <template-param>
	//                   ::= <substitution>

	if (!_SkipExpected('N'))
		return false;

	// parse CV qualifiers
	int qualifiers;
	_ParseCVQualifiers(qualifiers);

	// parse the main part
	if (!_ParseNestedNameInternal(_node))
		return false;

	// create a CV qualifiers wrapper node, if necessary
	if (qualifiers != 0) {
		return NodeCreator<CVQualifiersNode>(this)(qualifiers, _node,
			_node);
	}

	return true;
}


bool
Demangler::_ParseNestedNameInternal(Node*& _node)
{
	DEBUG_SCOPE("_ParseNestedNameMain");

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	// the initial prefix might be a template param or a substitution
	Node* initialPrefixNode = NULL;
	Node* prefixNode = NULL;
	switch (fInput[0]) {
		case 'T':	// <template-param>
			if (!_ParseTemplateParam(initialPrefixNode))
				return false;

			// a <prefix> or <template-prefix> and as such referenceable
			_RegisterReferenceableNode(initialPrefixNode);
			break;

		case 'S':	// <substitution>
			if (!_ParseSubstitution(initialPrefixNode))
				return false;
			break;
	}

	while (true) {
		bool canTerminate = false;
		Node* node;

		if (initialPrefixNode != NULL) {
			node = initialPrefixNode;
			initialPrefixNode = NULL;
		} else {
			if (!_ParseUnqualifiedName(node))
				return false;
			canTerminate = true;
		}

		// join prefix and the new node
		if (prefixNode != NULL) {
			if (!NodeCreator<PrefixedNode>(this)(prefixNode, node, node))
				return false;
		}

		// template arguments?
		if (fInput.HasPrefix('I')) {
			// <template-prefix> is referenceable
			_RegisterReferenceableNode(node);

			// parse the template arguments
			if (!_ParseTemplateArgs(node, node))
				return false;
			canTerminate = true;
		}

		if (fInput.CharsRemaining() == 0)
			return _SetError(ERROR_INVALID);

		// end of nested name?
		if (fInput.SkipPrefix('E')) {
			// If it doesn't have template args, it must end in an
			// unqualified name.
			if (!canTerminate)
				return _SetError(ERROR_INVALID);

			_node = node;
			return true;
		}

		// The fun continues, so this is a <prefix> or <template-prefix>
		// and as such referenceable.
		prefixNode = node;
		_RegisterReferenceableNode(node);
	}
}


bool
Demangler::_ParseLocalName(Node*& _node)
{
	DEBUG_SCOPE("_ParseLocalName");

	// <local-name> := Z <function encoding> E <entity name>
	//                 [<discriminator>]
	//              := Z <function encoding> E s [<discriminator>]
	// <discriminator> := _ <non-negative number>

	// parse the function name
	ObjectNode* functionName;
	if (!_SkipExpected('Z')
		|| !_ParseEncoding(functionName)
		|| !_SkipExpected('E')) {
		return false;
	}

	Node* entityName;
	if (fInput.SkipPrefix('s')) {
		// string literal
		if (!NodeCreator<SimpleNameNode>(this)("string literal",
				entityName)) {
			return false;
		}
	} else {
		// local type or object
		if (!_ParseName(entityName))
			return false;
	}

	// parse discriminator
	number_type discriminator = 0;
	if (fInput.SkipPrefix('_')) {
		if (!_ParseNumber(discriminator))
			return false;
		if (discriminator < 0)
			return _SetError(ERROR_INVALID);
		discriminator++;
	}

	return NodeCreator<PrefixedNode>(this)(functionName, entityName, _node);
}


bool
Demangler::_ParseUnqualifiedName(Node*& _node)
{
	DEBUG_SCOPE("_ParseUnqualifiedName");

	// <unqualified-name> ::= <operator-name>
	//                    ::= <ctor-dtor-name>
	//                    ::= <source-name>
	//
	// <source-name> ::= <positive length number> <identifier>
	// <number> ::= [n] <non-negative decimal integer>
	// <identifier> ::= <unqualified source code identifier>
	//
	// <ctor-dtor-name> ::= C1	# complete object constructor
	//                  ::= C2	# base object constructor
	//                  ::= C3	# complete object allocating constructor
	//                  ::= D0	# deleting destructor
	//                  ::= D1	# complete object destructor
	//                  ::= D2	# base object destructor

	// we need at least 2 chars
	if (fInput.CharsRemaining() < 2)
		return _SetError(ERROR_INVALID);

	if (isdigit(fInput[0]) || (fInput[0] == 'n' && isdigit(fInput[1]))) {
		// <source-name>
		return _ParseSourceName(_node);
	}

	if (fInput[0] == 'C') {
		// <ctor-dtor-name> -- constructors
		switch (fInput[1]) {
			case '1':
			case '2':
			case '3':
				if (!NodeCreator<XtructorNode>(this)(true, fInput[1] - '1',
						_node)) {
					return false;
				}

				fInput.Skip(2);
				return true;
			default:
				return _SetError(ERROR_INVALID);
		}
	}

	if (fInput[0] == 'D') {
		// <ctor-dtor-name> -- destructors
		switch (fInput[1]) {
			case '0':
			case '1':
			case '2':
				if (!NodeCreator<XtructorNode>(this)(false, fInput[1] - '0',
						_node)) {
					return false;
				}

				fInput.Skip(2);
				return true;
			default:
				return _SetError(ERROR_INVALID);
		}
	}

	// must be an <operator-name>
	return _ParseOperatorName(_node);
}


bool
Demangler::_ParseSourceName(Node*& _node)
{
	DEBUG_SCOPE("_ParseSourceName");

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	number_type number;
	if (!_ParseNumber(number))
		return false;

	if (number <= 0 || number > fInput.CharsRemaining())
		return _SetError(ERROR_INVALID);

	return _CreateNodeAndSkip(fInput.String(), number, number, _node);
}


bool
Demangler::_ParseOperatorName(Node*& _node)
{
	DEBUG_SCOPE("_ParseOperatorName");

	if (fInput.CharsRemaining() < 2)
		return _SetError(ERROR_INVALID);

	const operator_info* info = NULL;
	for (int i = 0; kOperatorInfos[i].name != NULL; i++) {
		if (fInput.SkipPrefix(kOperatorInfos[i].mangled_name)) {
			info = &kOperatorInfos[i];
			break;
		}
	}

	if (info != NULL)
		return NodeCreator<OperatorNode>(this)(info, _node);

	// <operator-name> ::= cv <type>	# (cast)
	if (fInput.SkipPrefix("cv")) {
		Node* typeNode;
		if (!_ParseType(typeNode))
			return false;

		return NodeCreator<CastOperatorNode>(this)(typeNode, _node);
	}

	//  <operator-name> ::= v <digit> <source-name>	# vendor extended
	//                                                operator
	if (fInput.SkipPrefix('v')) {
		if (fInput.CharsRemaining() == 0 || !isdigit(fInput[0]))
			return _SetError(ERROR_INVALID);
		fInput.Skip(1);

		Node* name;
		return _ParseSourceName(name)
			&& NodeCreator<VendorOperatorNode>(this)(name, _node);
	}

	return _SetError(ERROR_INVALID);
}


bool
Demangler::_ParseType(Node*& _node)
{
	DEBUG_SCOPE("_ParseType");

	if (!_ParseTypeInternal(_node))
		return false;

	_RegisterReferenceableNode(_node);
	return true;
}


bool
Demangler::_ParseTypeInternal(Node*& _node)
{
	DEBUG_SCOPE("_ParseTypeInternal");

	// <type> ::= <builtin-type>
	//        ::= <function-type>
	//        ::= <class-enum-type>
	//        ::= <array-type>
	//        ::= <pointer-to-member-type>
	//        ::= <template-param>
	//        ::= <template-template-param> <template-args>
	//        ::= <substitution> # See Compression below
	//
	// <template-template-param> ::= <template-param>
	//                           ::= <substitution>
	//
	// <type> ::= <CV-qualifiers> <type>
	//        ::= P <type>	# pointer-to
	//        ::= R <type>	# reference-to
	//        ::= O <type>	# rvalue reference-to (C++0x)
	//        ::= C <type>	# complex pair (C 2000)
	//        ::= G <type>	# imaginary (C 2000)
	//        ::= U <source-name> <type>	# vendor extended type qualifier
	//
	// <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const
	//
	// <type>  ::= Dp <type>          # pack expansion of (C++0x)
	//         ::= Dt <expression> E  # decltype of an id-expression or
	//                                # class member access (C++0x)
	//         ::= DT <expression> E  # decltype of an expression (C++0x)

	if (_TryParseBuiltinType(_node)) {
		_node->SetReferenceable(false);
		return true;
	}
	if (fError != ERROR_OK)
		return false;

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	switch (fInput[0]) {
		// function type
		case 'F':
		{
			FunctionNode* functionNode;
			if (!_ParseFunctionType(functionNode))
				return false;
			_node = functionNode;
			return true;
		}

		// array type
		case 'A':
			return _ParseArrayType(_node);

		// pointer to member type
		case 'M':
			return _ParsePointerToMemberType(_node);

		// template param
		case 'T':
			if (!_ParseTemplateParam(_node))
				return false;
			break;

		// CV qualifiers
		case 'r':
		case 'V':
		case 'K':
		{
			// parse CV qualifiers
			int qualifiers;
			_ParseCVQualifiers(qualifiers);

			// parse the type
			if (!_ParseType(_node))
				return false;

			// create the wrapper node
			return NodeCreator<CVQualifiersNode>(this)(qualifiers, _node,
				_node);
		}

		// pointer, reference, etc.
		case 'P':
			return _ParseTypeWithModifier(TYPE_QUALIFIER_POINTER, 1, _node);
		case 'R':
			return _ParseTypeWithModifier(TYPE_QUALIFIER_REFERENCE, 1,
				_node);
		case 'O':
			return _ParseTypeWithModifier(
				TYPE_QUALIFIER_RVALUE_REFERENCE, 1, _node);
		case 'C':
			return _ParseTypeWithModifier(TYPE_QUALIFIER_COMPLEX, 1, _node);
		case 'G':
			return _ParseTypeWithModifier(TYPE_QUALIFIER_IMAGINARY, 1,
				_node);

		// pack and decltype
		case 'D':
#if 0
			if (fInput.CharsRemaining() < 2)
				return _SetError(ERROR_INVALID);

			switch(fInput[1]) {
				case 'p':
					fInput.Skip(2);
					return _ParseType(_node);
				case 't':
				case 'T':
				{
					fInput.Skip(2);
					Node* nameNode;
					if (!_ParseExpression(nameNode))
						return false;
					if (!fInput.SkipPrefix('E'))
						return ERROR_INVALID;
				}
			}
#endif
			// NOTE: Unsupported by the GNU demangler.
			return _SetError(ERROR_UNSUPPORTED);

		// vendor extended type qualifier
		case 'U':
			fInput.Skip(1);
			Node* name;
			Node* type;
			return _ParseSourceName(name) && _ParseType(type)
				&& NodeCreator<VendorTypeModifierNode>(this)(name, type,
					_node);

		// substitution
		case 'S':
			if (!fInput.HasPrefix("St")) {
				if (!_ParseSubstitution(_node))
					return false;
				break;
			}

			// "St" -- the "std" namespace. The grammar is ambiguous here,
			// since we could parse that as <substitution> or as
			// <class-enum-type>. We assume the latter and fall through.

		default:
		{
			// <class-enum-type> ::= <name>
			Node* nameNode;
			return _ParseName(nameNode)
				&& NodeCreator<NamedTypeNode>(this)(nameNode, _node);
		}
	}

	// We get here for the types that might be a <template-template-param>.
	// Check whether <template-args> are following.
	if (!fInput.HasPrefix('I'))
		return true;

	// <template-template-param> is referenceable
	_RegisterReferenceableNode(_node);

	return _ParseTemplateArgs(_node, _node);
}


void
Demangler::_ParseCVQualifiers(int& qualifiers)
{
	qualifiers = 0;

	if (fInput.SkipPrefix('r'))
		qualifiers |= CV_QUALIFIER_RESTRICT;
	if (fInput.SkipPrefix('V'))
		qualifiers |= CV_QUALIFIER_VOLATILE;
	if (fInput.SkipPrefix('K'))
		qualifiers |= CV_QUALIFIER_CONST;
}


bool
Demangler::_ParseTypeWithModifier(type_modifier modifier, int toSkip,
	Node*& _node)
{
	if (toSkip > 0)
		fInput.Skip(toSkip);

	Node* node;
	if (!_ParseType(node))
		return false;

	return NodeCreator<TypeModifierNode>(this)(modifier, node, _node);
}


bool
Demangler::_TryParseBuiltinType(Node*& _node)
{
	DEBUG_SCOPE("_TryParseBuiltinType");

	// <builtin-type> ::= v	# void
	//                ::= w	# wchar_t
	//                ::= b	# bool
	//                ::= c	# char
	//                ::= a	# signed char
	//                ::= h	# unsigned char
	//                ::= s	# short
	//                ::= t	# unsigned short
	//                ::= i	# int
	//                ::= j	# unsigned int
	//                ::= l	# long
	//                ::= m	# unsigned long
	//                ::= x	# long long, __int64
	//                ::= y	# unsigned long long, __int64
	//                ::= n	# __int128
	//                ::= o	# unsigned __int128
	//                ::= f	# float
	//                ::= d	# double
	//                ::= e	# long double, __float80
	//                ::= g	# __float128
	//                ::= z	# ellipsis
	//                ::= Dd # IEEE 754r decimal floating point (64 bits)
	//                ::= De # IEEE 754r decimal floating point (128 bits)
	//                ::= Df # IEEE 754r decimal floating point (32 bits)
	//                ::= Dh # IEEE 754r half-precision floating point
	//                       # (16 bits)
	//                ::= Di # char32_t
	//                ::= Ds # char16_t
	//                ::= u <source-name>	# vendor extended type

	if (fInput.CharsRemaining() == 0)
		return false;

	switch (fInput[0]) {
		case 'v':
			return _CreateTypeNodeAndSkip(TYPE_VOID, 1, _node);
		case 'w':
			return _CreateTypeNodeAndSkip(TYPE_WCHAR_T, 1, _node);
		case 'b':
			return _CreateTypeNodeAndSkip(TYPE_BOOL, 1, _node);
		case 'c':
			return _CreateTypeNodeAndSkip(TYPE_CHAR, 1, _node);
		case 'a':
			return _CreateTypeNodeAndSkip(TYPE_SIGNED_CHAR, 1, _node);
		case 'h':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_CHAR, 1, _node);
		case 's':
			return _CreateTypeNodeAndSkip(TYPE_SHORT, 1, _node);
		case 't':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_SHORT, 1,
				_node);
		case 'i':
			return _CreateTypeNodeAndSkip(TYPE_INT, 1, _node);
		case 'j':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_INT, 1, _node);
		case 'l':
			return _CreateTypeNodeAndSkip(TYPE_LONG, 1, _node);
		case 'm':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_LONG, 1, _node);
		case 'x':
			return _CreateTypeNodeAndSkip(TYPE_LONG_LONG, 1, _node);
		case 'y':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_LONG_LONG, 1, _node);
		case 'n':
			return _CreateTypeNodeAndSkip(TYPE_INT128, 1, _node);
		case 'o':
			return _CreateTypeNodeAndSkip(TYPE_UNSIGNED_INT128, 1, _node);
		case 'f':
			return _CreateTypeNodeAndSkip(TYPE_FLOAT, 1, _node);
		case 'd':
			return _CreateTypeNodeAndSkip(TYPE_DOUBLE, 1, _node);
		case 'e':
			return _CreateTypeNodeAndSkip(TYPE_LONG_DOUBLE, 1, _node);
		case 'g':
			return _CreateTypeNodeAndSkip(TYPE_FLOAT128, 1, _node);
		case 'z':
			return _CreateTypeNodeAndSkip(TYPE_ELLIPSIS, 1, _node);

		case 'D':
			if (fInput.CharsRemaining() < 2)
				return false;

			// TODO: Official names for the __dfloat*!
			switch (fInput[1]) {
				case 'd':
					return _CreateTypeNodeAndSkip(TYPE_DFLOAT64, 2, _node);
				case 'e':
					return _CreateTypeNodeAndSkip(TYPE_DFLOAT128, 2, _node);
				case 'f':
					return _CreateTypeNodeAndSkip(TYPE_DFLOAT32, 2, _node);
				case 'h':
					return _CreateTypeNodeAndSkip(TYPE_DFLOAT16, 2, _node);
				case 'i':
					return _CreateTypeNodeAndSkip(TYPE_CHAR16_T, 2, _node);
				case 's':
					return _CreateTypeNodeAndSkip(TYPE_CHAR32_T, 2, _node);
				default:
					return false;
			}

		case 'u':
		{
			fInput.Skip(1);
			Node* nameNode;
			return _ParseSourceName(nameNode)
				&& NodeCreator<NamedTypeNode>(this)(nameNode, _node);
		}

		default:
			return false;
	}
}


bool
Demangler::_ParseFunctionType(FunctionNode*& _node)
{
	DEBUG_SCOPE("_ParseFunctionType");

	// <function-type> ::= F [Y] <bare-function-type> E

	if (!_SkipExpected('F'))
		return false;

	// is 'extern "C"'?
	bool isExternC = fInput.SkipPrefix('Y');

	// create function and parse function type
	if (!NodeCreator<FunctionNode>(this)((Node*)NULL, true, isExternC,
			_node)
		|| !_ParseBareFunctionType(_node)) {
		return false;
	}

	// skip terminating 'E'
	return _SkipExpected('E');
}


bool
Demangler::_ParseArrayType(Node*& _node)
{
	DEBUG_SCOPE("_ParseArrayType");

	// <array-type> ::= A <positive dimension number> _ <element type>
	//              ::= A [<dimension expression>] _ <element type>

	if (fInput.CharsRemaining() < 2 || !fInput.SkipPrefix('A'))
		return _SetError(ERROR_INVALID);

	number_type dimensionNumber;
	Node* dimensionExpression = NULL;

	// If it looks like a number, it must be the first production, otherwise
	// the second one.
	if (isdigit(fInput[0])
		|| (fInput[0] == 'n' && fInput.CharsRemaining() >= 2
			&& isdigit(fInput[1]))) {
		if (!_ParseNumber(dimensionNumber))
			return false;
	} else {
		if (!_ParseExpression(dimensionExpression))
			return false;
	}

	// parse the type
	Node* type;
	if (!_SkipExpected('_') || !_ParseType(type))
		return false;

	// create the array node
	return dimensionExpression != NULL
		? NodeCreator<ArrayNode>(this)(type, dimensionExpression, _node)
		: NodeCreator<ArrayNode>(this)(type, dimensionNumber, _node);
}


bool
Demangler::_ParsePointerToMemberType(Node*& _node)
{
	DEBUG_SCOPE("_ParsePointerToMemberType");

	// <pointer-to-member-type> ::= M <class type> <member type>
	Node* classType;
	Node* memberType;
	return _SkipExpected('M')
		&& _ParseType(classType)
		&& _ParseType(memberType)
		&& NodeCreator<PointerToMemberNode>(this)(classType, memberType,
			_node);
}


bool
Demangler::_ParseTemplateParam(Node*& _node)
{
	DEBUG_SCOPE("_ParseTemplateParam");

	// <template-param> ::= T_	# first template parameter
	//                  ::= T <parameter-2 non-negative number> _

	if (!_SkipExpected('T'))
		return false;
	if (fTemplatizedNode == NULL)
		return _SetError(ERROR_INVALID);

	// get the index;
	number_type index = 0;
	if (!fInput.HasPrefix('_')) {
		if (!_ParseNumber(index))
			return false;

		if (index < 0)
			return _SetError(ERROR_INVALID);
		index++;
	}

	if (!_SkipExpected('_'))
		return false;

	// get the parameter
	Node* parameter = fTemplatizedNode->TemplateParameterAt(index);
	if (parameter == NULL)
		return _SetError(ERROR_INVALID);

	// create a substitution node
	return NodeCreator<SubstitutionNode>(this)(parameter, _node);
}


bool
Demangler::_ParseSubstitution(Node*& _node)
{
	DEBUG_SCOPE("_ParseSubstitution");

	if (!_ParseSubstitutionInternal(_node))
		return false;

	// substitutions are never referenceable
	_node->SetReferenceable(false);

	return true;
}


bool
Demangler::_ParseSubstitutionInternal(Node*& _node)
{
	DEBUG_SCOPE("_ParseSubstitutionInternal");

	// <substitution> ::= S <seq-id> _
	//                ::= S_
	//
	// <substitution> ::= St # ::std::
	// <substitution> ::= Sa # ::std::allocator
	// <substitution> ::= Sb # ::std::basic_string
	// <substitution> ::= Ss # ::std::basic_string < char,
	//                         ::std::char_traits<char>,
	//                         ::std::allocator<char> >
	// <substitution> ::= Si # ::std::basic_istream<char,
	//                             std::char_traits<char> >
	// <substitution> ::= So # ::std::basic_ostream<char,
	//                             std::char_traits<char> >
	// <substitution> ::= Sd # ::std::basic_iostream<char,
	//                             std::char_traits<char> >

	if (fInput.CharsRemaining() < 2 || !fInput.SkipPrefix('S'))
		return _SetError(ERROR_INVALID);

	switch (fInput[0]) {
		case 't':
			return _CreateNodeAndSkip("std", 1, _node);
		case 'a':
			return _CreateTypeNodeAndSkip("allocator", "std", NULL, 1,
				_node);
		case 'b':
			return _CreateTypeNodeAndSkip("basic_string", "std", NULL, 1,
				_node);
		case 's':
			return _CreateTypeNodeAndSkip("basic_string", "std",
				"char, std::char_traits<char>, std::allocator<char>", 1,
				_node);
		case 'i':
			return _CreateTypeNodeAndSkip("basic_istream", "std",
				"char, std::char_traits<char>", 1, _node);
		case 'o':
			return _CreateTypeNodeAndSkip("basic_ostream", "std",
				"char, std::char_traits<char>", 1, _node);
		case 'd':
			return _CreateTypeNodeAndSkip("basic_iostream", "std",
				"char, std::char_traits<char>", 1, _node);
		case '_':
			fInput.Skip(1);
			return _CreateSubstitutionNode(0, _node);
	}

	// parse <seq-id>
	int seqID = 0;
	int count = fInput.CharsRemaining();
	int i = 0;
	for (; i < count && fInput[i] != '_'; i++) {
		char c = fInput[i];
		if (isdigit(c))
			seqID = seqID * 36 + (c - '0');
		else if (c >= 'A' && c <= 'Z')
			seqID = seqID * 36 + 10 + (c - 'A');
		else
			return _SetError(ERROR_INVALID);
	}

	if (i == count)
		return _SetError(ERROR_INVALID);

	// skip digits and '_'
	fInput.Skip(i + 1);

	return _CreateSubstitutionNode(seqID + 1, _node);
}


bool
Demangler::_ParseBareFunctionType(FunctionNode* node)
{
	DEBUG_SCOPE("_ParseBareFunctionType");

	// <bare-function-type> ::= <signature type>+
	//     # types are possible return type, then parameter types

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	do {
		Node* typeNode;
		if (!_ParseType(typeNode))
			return false;

		node->AddType(typeNode);
	} while (fInput.CharsRemaining() > 0 && fInput[0] != 'E');
		// 'E' delimits <function-type>

	return true;
}


bool
Demangler::_ParseTemplateArgs(Node* node, Node*& _node)
{
	DEBUG_SCOPE("_ParseTemplateArgs");

	// <template-args> ::= I <template-arg>+ E

	if (!_SkipExpected('I'))
		return false;

	// we need at least one <template-arg>
	if (fInput.CharsRemaining() == 0 || fInput[0] == 'E')
		return _SetError(ERROR_INVALID);

	// create the node
	TemplateNode* templateNode;
	if (!NodeCreator<TemplateNode>(this)(node, templateNode))
		return false;
	_node = templateNode;

	// parse the args
	while (fInput.CharsRemaining() > 0 && fInput[0] != 'E') {
		Node* arg;
		if (!_ParseTemplateArg(arg))
			return false;
		templateNode->AddArgument(arg);
	}

	// skip the trailing 'E'
	return _SkipExpected('E');
}


bool
Demangler::_ParseTemplateArg(Node*& _node)
{
	DEBUG_SCOPE("_ParseTemplateArg");

	// <template-arg> ::= <type>			   # type or template
	//                ::= X <expression> E	   # expression
	//                ::= <expr-primary>       # simple expressions
	//                ::= I <template-arg>* E  # argument pack
	//                ::= sp <expression>      # pack expansion of (C++0x)

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	switch (fInput[0]) {
		case 'X':	// X <expression> E
			fInput.Skip(1);
			return _ParseExpression(_node) && _SkipExpected('E');

		case 'L':	// <expr-primary>
			return _ParseExpressionPrimary(_node);

		case 'I':	// I <template-arg>* E
		{
#if 0
			fInput.Skip(1);

			while (fInput.CharsRemaining() > 0 && fInput[0] != 'E') {
				Node* arg;
				if (!_ParseTemplateArg(arg))
					return false;
			}

			if (!fInput.SkipPrefix('E'))
				return _SetError(ERROR_INVALID);
			return true;
#endif
			// NOTE: Unsupported by the GNU demangler.
			return _SetError(ERROR_UNSUPPORTED);
		}

		case 's':
			if (fInput.SkipPrefix("sp")) {
				// sp <expression>
#if 0
				return _ParseExpression(_node);
#endif
				// NOTE: Unsupported by the GNU demangler.
				return _SetError(ERROR_UNSUPPORTED);
			}

			// fall through...

		default:	// <type>
			return _ParseType(_node);
	}
}


bool
Demangler::_ParseExpression(Node*& _node)
{
	DEBUG_SCOPE("_ParseExpression");

	// <expression> ::= <unary operator-name> <expression>
	//              ::= <binary operator-name> <expression> <expression>
	//              ::= <trinary operator-name> <expression> <expression>
	//                  <expression>
	//              ::= cl <expression>* E          # call
	//              ::= cv <type> expression        # conversion with one
	//                                                argument
	//              ::= cv <type> _ <expression>* E # conversion with a
	//                                                different number of
	//                                                arguments
	//              ::= st <type>		            # sizeof (a type)
	//              ::= at <type>                   # alignof (a type)
	//              ::= <template-param>
	//              ::= <function-param>
	//              ::= sr <type> <unqualified-name>
	//                    # dependent name
	//              ::= sr <type> <unqualified-name> <template-args>
	//                    # dependent template-id
	//              ::= sZ <template-param>
	//                    # size of a parameter pack
	//              ::= <expr-primary>
	//
	// <expr-primary> ::= L <type> <value number> E  # integer literal
	//                ::= L <type <value float> E    # floating literal
	//                ::= L <mangled-name> E         # external name

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	switch (fInput[0]) {
		case 'L':
			return _ParseExpressionPrimary(_node);
		case 'T':
			return _ParseTemplateParam(_node);
		// NOTE: <function-param> is not defined in the specs!
	}

	// must be an operator
	if (fInput.CharsRemaining() < 2)
		return _SetError(ERROR_INVALID);

	// some operators need special handling

	if (fInput.SkipPrefix("cl")) {
		// cl <expression>* E          # call
		CallNode* callNode;
		if (!NodeCreator<CallNode>(this)(callNode))
			return false;

		while (fInput.CharsRemaining() > 0 && fInput[0] != 'E') {
			Node* subExpression;
			if (!_ParseExpression(subExpression))
				return false;
			callNode->AddSubExpression(subExpression);
		}

		_node = callNode;
		return _SkipExpected('E');
	}

	if (fInput.SkipPrefix("cv")) {
		// cv <type> expression        # conversion with one argument
		// cv <type> _ <expression>* E # conversion with a different number
		//                               of arguments

		// parse the type
		Node* type;
		if (!_ParseType(type))
			return false;

		// create a conversion expression node
		ConversionExpressionNode* expression;
		if (!NodeCreator<ConversionExpressionNode>(this)(type, expression))
			return false;
		_node = expression;

		if (fInput.SkipPrefix('_')) {
			// multi argument conversion
			while (fInput.CharsRemaining() > 0 && fInput[0] != 'E') {
				Node* subExpression;
				if (!_ParseExpression(subExpression))
					return false;
				expression->AddSubExpression(subExpression);
			}

			return _SkipExpected('E');
		}

		// single argument conversion
		Node* subExpression;
		if (!_ParseExpression(subExpression))
			return false;
		expression->AddSubExpression(subExpression);

		return true;
	}

	if (fInput.SkipPrefix("sr")) {
		// sr <type> <unqualified-name>
		// sr <type> <unqualified-name> <template-args>

		// parse type and unqualified name and create the node
		Node* type;
		Node* name;
		if (!_ParseType(type) || !_ParseUnqualifiedName(name)
			|| !NodeCreator<DependentNameNode>(this)(type, name, _node)) {
			return false;
		}

		// If there are template arguments left, add them.
		if (!fInput.HasPrefix('I'))
			return true;

		return _ParseTemplateArgs(_node, _node);
	}

	if (fInput.SkipPrefix("sZ")) {
		// sZ <template-param>

		// NOTE: Unsupported by the GNU demangler.
		return _SetError(ERROR_UNSUPPORTED);
	}

	// no special operator, so have a look for the others

	const operator_info* info = NULL;
	for (int i = 0; kOperatorInfos[i].name != NULL; i++) {
		if (fInput.SkipPrefix(kOperatorInfos[i].mangled_name)) {
			info = &kOperatorInfos[i];
			break;
		}
	}

	// We can only deal with operators with a fixed argument count at this
	// point.
	if (info == NULL || info->argument_count < 0)
		return _SetError(ERROR_INVALID);

	// create an operator node
	OperatorExpressionNode* operatorNode;
	if (!NodeCreator<OperatorExpressionNode>(this)(info, operatorNode))
		return false;

	// parse the arguments
	int i = 0;

	// the first one might be a type
	if ((info->flags & OPERATOR_TYPE_PARAM) != 0) {
		Node* type;
		if (!_ParseType(type))
			return false;

		operatorNode->AddSubExpression(type);
		i++;
	}

	// the others are expressions
	for (; i < info->argument_count; i++) {
		Node* subExpression;
		if (!_ParseExpression(subExpression))
			return false;
		operatorNode->AddSubExpression(subExpression);
	}

	_node = operatorNode;
	return true;
}


bool
Demangler::_ParseExpressionPrimary(Node*& _node)
{
	DEBUG_SCOPE("_ParseExpressionPrimary");

	// <expr-primary> ::= L <type> <value number> E  # integer literal
	//                ::= L <type <value float> E    # floating literal
	//                ::= L <mangled-name> E         # external name

	if (!_SkipExpected('L'))
		return false;

	if (fInput.SkipPrefix("_Z")) {
		ObjectNode* node;
		if (!_ParseEncoding(node))
			return false;
		_node = node;
	} else {
		// number or float literal
		Node* type;
		if (!_ParseType(type))
			return false;

		// GNU's demangler doesn't really seem to parse the integer/float,
		// but only replaces a leading 'n' by '-'. Good enough for us, too.

		// determine the length
		int maxLength = fInput.CharsRemaining();
		int length = 0;
		while (length < maxLength && fInput[length] != 'E')
			length++;

		if (length == 0)
			return _SetError(ERROR_INVALID);

		if (!NodeCreator<TypedNumberLiteralNode>(this)(type,
				fInput.String(), length, _node)) {
			return false;
		}

		fInput.Skip(length);
	}

	return _SkipExpected('E');
}


bool
Demangler::_ParseNumber(number_type& number)
{
	DEBUG_SCOPE("_ParseNumber");

	bool negative = fInput.SkipPrefix('n');

	if (fInput.CharsRemaining() == 0)
		return _SetError(ERROR_INVALID);

	number = 0;
	int count = fInput.CharsRemaining();
	int i = 0;
	for (; i < count && isdigit(fInput[i]); i++)
		number = number * 10 + (fInput[i] - '0');

	fInput.Skip(i);

	if (negative)
		number =-number;
	return true;
}


bool
Demangler::_CreateNodeAndSkip(const char* name, size_t length, int toSkip,
	Node*& _node)
{
	if (toSkip > 0)
		fInput.Skip(toSkip);

	return NodeCreator<SimpleNameNode>(this)(name, length, _node);
}


bool
Demangler::_CreateNodeAndSkip(const char* name, int toSkip, Node*& _node)
{
	return _CreateNodeAndSkip(name, strlen(name), toSkip, _node);
}


bool
Demangler::_CreateTypeNodeAndSkip(type_type type, int toSkip, Node*& _node)
{
	if (toSkip > 0)
		fInput.Skip(toSkip);

	return NodeCreator<SimpleTypeNode>(this)(type, _node);
}


bool
Demangler::_CreateTypeNodeAndSkip(const char* name, const char* prefix,
	const char* templateArgs, int toSkip, Node*& _node)
{
	if (toSkip > 0)
		fInput.Skip(toSkip);

	// create the name node
	if (!NodeCreator<SimpleTypeNode>(this)(name, _node))
		return false;

	// add the prefix
	if (prefix != NULL) {
		Node* prefixNode;
		if (!NodeCreator<SimpleTypeNode>(this)(prefix, prefixNode)
			|| !NodeCreator<PrefixedNode>(this)(prefixNode, _node, _node)) {
			return false;
		}
	}

	// wrap the node to add the template args
	if (templateArgs != NULL) {
		TemplateNode* templateNode;
		Node* argsNode;
		if (!NodeCreator<TemplateNode>(this)(_node, templateNode)
			|| !NodeCreator<SimpleTypeNode>(this)(templateArgs, argsNode)) {
			return false;
		}
		templateNode->AddArgument(argsNode);
		_node = templateNode;
	}

	return true;
}


void
Demangler::_RegisterReferenceableNode(Node* node)
{
	// check, if not referenceable or already registered
	if (!node->IsReferenceable() || node == fLastReferenceableNode
		|| node->NextReferenceable() != NULL) {
		return;
	}

	if (fFirstReferenceableNode == NULL) {
		fFirstReferenceableNode = node;
		fLastReferenceableNode = node;
	} else {
		fLastReferenceableNode->SetNextReferenceable(node);
		fLastReferenceableNode = node;
	}
}


bool
Demangler::_CreateSubstitutionNode(int index, Node*& _node)
{
	Node* node = fFirstReferenceableNode;
	while (node != NULL && index > 0) {
		node = node->NextReferenceable();
		index--;
	}

	if (node == NULL)
		return _SetError(ERROR_INVALID);

	// create a substitution node
	return NodeCreator<SubstitutionNode>(this)(node, _node);
}


// #pragma mark -


const char*
demangle_symbol_gcc3(const char* mangledName, char* buffer, size_t bufferSize,
	bool* _isObjectMethod)
{
	bool isObjectMethod;
	if (_isObjectMethod == NULL)
		_isObjectMethod = &isObjectMethod;

	Demangler demangler;
	DemanglingInfo info(true);
	if (demangler.Demangle(mangledName, buffer, bufferSize, info) != ERROR_OK)
		return NULL;

	// Set the object method return value. Unless we know for sure that it isn't
	// an object method, we assume that it is.
	switch (info.objectType) {
		case OBJECT_TYPE_DATA:
		case OBJECT_TYPE_FUNCTION:
		case OBJECT_TYPE_METHOD_CLASS:
			*_isObjectMethod = false;
			break;
		case OBJECT_TYPE_METHOD_OBJECT:
			*_isObjectMethod = true;
			break;
		case OBJECT_TYPE_UNKNOWN:
		case OBJECT_TYPE_METHOD_UNKNOWN:
			*_isObjectMethod = strstr(buffer, "::") != NULL;
			break;
	}

	return buffer;
}


status_t
get_next_argument_gcc3(uint32* _cookie, const char* mangledName, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength)
{
	Demangler demangler;
	ParameterInfo info;
	int result = demangler.GetParameterInfo(mangledName, *_cookie, name,
		nameSize, info);
	if (result != ERROR_OK) {
		switch (result) {
			case ERROR_NOT_MANGLED:
				return B_BAD_VALUE;
			case ERROR_UNSUPPORTED:
				return B_BAD_VALUE;
			case ERROR_INVALID:
				return B_BAD_VALUE;
			case ERROR_BUFFER_TOO_SMALL:
				return B_BUFFER_OVERFLOW;
			case ERROR_NO_MEMORY:
				return B_NO_MEMORY;
			case ERROR_INVALID_PARAMETER_INDEX:
				return B_BAD_INDEX;
			case ERROR_INTERNAL:
			default:
				return B_ERROR;
		}
	}

	// translate the type
	switch (info.type.type) {
		case TYPE_BOOL:
			*_type = B_BOOL_TYPE;
			*_argumentLength = 1;
			break;

		case TYPE_CHAR:
			*_type = B_CHAR_TYPE;
			*_argumentLength = 1;
			break;

		case TYPE_SIGNED_CHAR:
			*_type = B_INT8_TYPE;
			*_argumentLength = 1;
			break;
		case TYPE_UNSIGNED_CHAR:
			*_type = B_UINT8_TYPE;
			*_argumentLength = 1;
			break;

		case TYPE_SHORT:
			*_type = B_INT16_TYPE;
			*_argumentLength = 2;
			break;
		case TYPE_UNSIGNED_SHORT:
			*_type = B_UINT16_TYPE;
			*_argumentLength = 2;
			break;

		case TYPE_INT:
			*_type = B_INT32_TYPE;
			*_argumentLength = 4;
			break;
		case TYPE_UNSIGNED_INT:
			*_type = B_UINT32_TYPE;
			*_argumentLength = 4;
			break;

		case TYPE_LONG:
			*_type = sizeof(long) == 4 ? B_INT32_TYPE : B_INT64_TYPE;
			*_argumentLength = sizeof(long);
			break;
		case TYPE_UNSIGNED_LONG:
			*_type = sizeof(long) == 4 ? B_UINT32_TYPE : B_UINT64_TYPE;
			*_argumentLength = sizeof(long);
			break;

		case TYPE_LONG_LONG:
			*_type = B_INT64_TYPE;
			*_argumentLength = 8;
			break;
		case TYPE_UNSIGNED_LONG_LONG:
			*_type = B_INT64_TYPE;
			*_argumentLength = 8;
			break;

		case TYPE_INT128:
			*_type = 0;
			*_argumentLength = 16;
			break;
		case TYPE_UNSIGNED_INT128:
			*_type = 0;
			*_argumentLength = 16;
			break;

		case TYPE_FLOAT:
			*_type = B_FLOAT_TYPE;
			*_argumentLength = sizeof(float);
			break;
		case TYPE_DOUBLE:
			*_type = B_DOUBLE_TYPE;
			*_argumentLength = sizeof(double);
			break;

		case TYPE_LONG_DOUBLE:
			*_type = 0;
			*_argumentLength = sizeof(long double);
			break;

		case TYPE_FLOAT128:
			*_type = 0;
			*_argumentLength = 16;
			break;

		case TYPE_DFLOAT16:
			*_argumentLength = 2;
		case TYPE_DFLOAT32:
			*_argumentLength *= 2;
		case TYPE_DFLOAT64:
			*_argumentLength *= 2;
		case TYPE_DFLOAT128:
			*_argumentLength *= 2;
			*_type = 0;
			break;

		case TYPE_CHAR16_T:
			*_type = B_UINT16_TYPE;
			*_argumentLength = 2;
			break;

		case TYPE_CHAR32_T:
			*_type = B_UINT32_TYPE;
			*_argumentLength = 2;
			break;

		case TYPE_CONST_CHAR_POINTER:
			*_type = B_STRING_TYPE;
			*_argumentLength = sizeof(void*);
			break;

		case TYPE_POINTER:
			*_type = B_POINTER_TYPE;
			*_argumentLength = sizeof(void*);
			break;

		case TYPE_REFERENCE:
			*_type = B_REF_TYPE;
				// TODO: That's actually entry_ref!
			*_argumentLength = sizeof(void*);
			break;

		case TYPE_WCHAR_T:
			// TODO: Type/size might change!
			*_type = B_UINT16_TYPE;
			*_argumentLength = 2;
			break;

		case TYPE_UNKNOWN:
		case TYPE_ELLIPSIS:
		case TYPE_VOID:
		default:
			// Well, tell our caller *something*.
			*_type = 0;
			*_argumentLength = sizeof(int);
	}

	// assume sizeof(int) argument alignment
	if (*_argumentLength < sizeof(int))
		*_argumentLength = sizeof(int);

	++*_cookie;
	return B_OK;
}


#ifndef _KERNEL_MODE

const char*
demangle_name_gcc3(const char* mangledName, char* buffer, size_t bufferSize)
{

	Demangler demangler;
	DemanglingInfo info(false);
	if (demangler.Demangle(mangledName, buffer, bufferSize, info) != ERROR_OK)
		return NULL;
	return buffer;
}

#endif
