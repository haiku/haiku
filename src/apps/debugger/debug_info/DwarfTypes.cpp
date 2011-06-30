/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "DwarfTypes.h"

#include <new>

#include "Architecture.h"
#include "ArrayIndexPath.h"
#include "Dwarf.h"
#include "DwarfFile.h"
#include "DwarfTargetInterface.h"
#include "DwarfUtils.h"
#include "Register.h"
#include "RegisterMap.h"
#include "Tracing.h"
#include "ValueLocation.h"


namespace {


// #pragma mark - HasBitStridePredicate


template<typename EntryType>
struct HasBitStridePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->BitStride()->IsValid();
	}
};


// #pragma mark - HasByteStridePredicate


template<typename EntryType>
struct HasByteStridePredicate {
	inline bool operator()(EntryType* entry) const
	{
		return entry->ByteStride()->IsValid();
	}
};


}	// unnamed namespace


type_kind
dwarf_tag_to_type_kind(int32 tag)
{
	switch (tag) {
		case DW_TAG_class_type:
		case DW_TAG_structure_type:
		case DW_TAG_union_type:
		case DW_TAG_interface_type:
			return TYPE_COMPOUND;

		case DW_TAG_base_type:
			return TYPE_PRIMITIVE;

		case DW_TAG_pointer_type:
		case DW_TAG_reference_type:
			return TYPE_ADDRESS;

		case DW_TAG_const_type:
		case DW_TAG_packed_type:
		case DW_TAG_volatile_type:
		case DW_TAG_restrict_type:
		case DW_TAG_shared_type:
			return TYPE_MODIFIED;

		case DW_TAG_typedef:
			return TYPE_TYPEDEF;

		case DW_TAG_array_type:
			return TYPE_ARRAY;

		case DW_TAG_enumeration_type:
			return TYPE_ENUMERATION;

		case DW_TAG_subrange_type:
			return TYPE_SUBRANGE;

		case DW_TAG_unspecified_type:
			return TYPE_UNSPECIFIED;

		case DW_TAG_subroutine_type:
			return TYPE_FUNCTION;

		case DW_TAG_ptr_to_member_type:
			return TYPE_POINTER_TO_MEMBER;

	}

	return TYPE_UNSPECIFIED;
}


int32
dwarf_tag_to_subtype_kind(int32 tag)
{
	switch (tag) {
		case DW_TAG_class_type:
			return COMPOUND_TYPE_CLASS;

		case DW_TAG_structure_type:
			return COMPOUND_TYPE_STRUCT;

		case DW_TAG_union_type:
			return COMPOUND_TYPE_UNION;

		case DW_TAG_interface_type:
			return COMPOUND_TYPE_INTERFACE;

		case DW_TAG_pointer_type:
			return DERIVED_TYPE_POINTER;

		case DW_TAG_reference_type:
			return DERIVED_TYPE_REFERENCE;
	}

	return -1;
}


// #pragma mark - DwarfTypeContext


DwarfTypeContext::DwarfTypeContext(Architecture* architecture, image_id imageID,
	DwarfFile* file, CompilationUnit* compilationUnit,
	DIESubprogram* subprogramEntry, target_addr_t instructionPointer,
	target_addr_t framePointer, target_addr_t relocationDelta,
	DwarfTargetInterface* targetInterface, RegisterMap* fromDwarfRegisterMap)
	:
	fArchitecture(architecture),
	fImageID(imageID),
	fFile(file),
	fCompilationUnit(compilationUnit),
	fSubprogramEntry(subprogramEntry),
	fInstructionPointer(instructionPointer),
	fFramePointer(framePointer),
	fRelocationDelta(relocationDelta),
	fTargetInterface(targetInterface),
	fFromDwarfRegisterMap(fromDwarfRegisterMap)
{
	fArchitecture->AcquireReference();
	fFile->AcquireReference();
	fTargetInterface->AcquireReference();
}


DwarfTypeContext::~DwarfTypeContext()
{
	fArchitecture->ReleaseReference();
	fFile->ReleaseReference();
}


// #pragma mark - DwarfType


DwarfType::DwarfType(DwarfTypeContext* typeContext, const BString& name,
	const DIEType* entry)
	:
	fTypeContext(typeContext),
	fName(name),
	fByteSize(0)
{
	fTypeContext->AcquireReference();

	GetTypeID(entry, fID);
}


DwarfType::~DwarfType()
{
	fTypeContext->ReleaseReference();
}


/*static*/ bool
DwarfType::GetTypeID(const DIEType* entry, BString& _id)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "dwarf:%p", entry);
	BString id = buffer;
	if (id.Length() == 0)
		return false;

	_id = id;
	return true;
}


image_id
DwarfType::ImageID() const
{
	return fTypeContext->ImageID();
}


const BString&
DwarfType::ID() const
{
	return fID;
}


const BString&
DwarfType::Name() const
{
	return fName;
}


target_size_t
DwarfType::ByteSize() const
{
	return fByteSize;
}


status_t
DwarfType::ResolveObjectDataLocation(const ValueLocation& objectLocation,
	ValueLocation*& _location)
{
	// TODO: In some source languages the object address might be a pointer
	// to a descriptor, not the actual object data.

	// If the given location looks good already, just clone it.
	int32 count = objectLocation.CountPieces();
	if (count == 0)
		return B_BAD_VALUE;

	ValuePieceLocation piece = objectLocation.PieceAt(0);
	if (count > 1 || piece.type != VALUE_PIECE_LOCATION_MEMORY
		|| piece.size != 0 || piece.bitSize != 0) {
		ValueLocation* location
			= new(std::nothrow) ValueLocation(objectLocation);
		if (location == NULL || location->CountPieces() != count) {
			delete location;
			return B_NO_MEMORY;
		}

		_location = location;
		return B_OK;
	}

	// The location contains just a single address piece with a zero size
	// -- set the type's size.
	piece.SetSize(ByteSize());
		// TODO: Use bit size and bit offset, if specified!

	ValueLocation* location = new(std::nothrow) ValueLocation(
		objectLocation.IsBigEndian());
	if (location == NULL || !location->AddPiece(piece)) {
		delete location;
		return B_NO_MEMORY;
	}

	_location = location;
	return B_OK;
}


status_t
DwarfType::ResolveObjectDataLocation(target_addr_t objectAddress,
	ValueLocation*& _location)
{
	ValuePieceLocation piece;
	piece.SetToMemory(objectAddress);
	piece.SetSize(0);
		// We set the piece size to 0 as an indicator that the size has to be
		// set.
		// TODO: We could set the byte size from type, but that may not be
		// accurate. We may want to add bit offset and size to Type.

	ValueLocation location(fTypeContext->GetArchitecture()->IsBigEndian());
	if (!location.AddPiece(piece))
		return B_NO_MEMORY;

	return ResolveObjectDataLocation(location, _location);
}


status_t
DwarfType::ResolveLocation(DwarfTypeContext* typeContext,
	const LocationDescription* description, target_addr_t objectAddress,
	bool hasObjectAddress, ValueLocation& _location)
{
	status_t error = typeContext->File()->ResolveLocation(
		typeContext->GetCompilationUnit(), typeContext->SubprogramEntry(),
		description, typeContext->TargetInterface(),
		typeContext->InstructionPointer(), objectAddress, hasObjectAddress,
		typeContext->FramePointer(), typeContext->RelocationDelta(),
		_location);
	if (error != B_OK)
		return error;

	// translate the DWARF register indices and the bit offset/size semantics
	const Register* registers = typeContext->GetArchitecture()->Registers();
	bool bigEndian = typeContext->GetArchitecture()->IsBigEndian();
	int32 count = _location.CountPieces();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = _location.PieceAt(i);
		if (piece.type == VALUE_PIECE_LOCATION_REGISTER) {
			int32 reg = typeContext->FromDwarfRegisterMap()->MapRegisterIndex(
				piece.reg);
			if (reg >= 0) {
				piece.reg = reg;
				// The bit offset for registers is to the least
				// significant bit, while we want the offset to the most
				// significant bit.
				if (registers[reg].BitSize() > piece.bitSize) {
					piece.bitOffset = registers[reg].BitSize() - piece.bitSize
						- piece.bitOffset;
				}
			} else
				piece.SetToUnknown();
		} else if (piece.type == VALUE_PIECE_LOCATION_MEMORY) {
			// Whether the bit offset is to the least or most significant bit
			// is target architecture and source language specific.
			// TODO: Check whether this is correct!
			// TODO: Source language!
			if (!bigEndian && piece.size * 8 > piece.bitSize) {
				piece.bitOffset = piece.size * 8 - piece.bitSize
					- piece.bitOffset;
			}
		}

		piece.Normalize(bigEndian);
		_location.SetPieceAt(i, piece);
	}

	// If we only have one piece and that doesn't have a size, try to retrieve
	// the size of the type.
	if (count == 1) {
		ValuePieceLocation piece = _location.PieceAt(0);
		if (piece.IsValid() && piece.size == 0 && piece.bitSize == 0) {
			piece.SetSize(ByteSize());
				// TODO: Use bit size and bit offset, if specified!
			_location.SetPieceAt(0, piece);

			TRACE_LOCALS("  set single piece size to %llu\n", ByteSize());
		}
	}

	return B_OK;
}


// #pragma mark - DwarfInheritance


DwarfInheritance::DwarfInheritance(DIEInheritance* entry, DwarfType* type)
	:
	fEntry(entry),
	fType(type)
{
	fType->AcquireReference();
}


DwarfInheritance::~DwarfInheritance()
{
	fType->ReleaseReference();
}


Type*
DwarfInheritance::GetType() const
{
	return fType;
}


// #pragma mark - DwarfDataMember


DwarfDataMember::DwarfDataMember(DIEMember* entry, const BString& name,
	DwarfType* type)
	:
	fEntry(entry),
	fName(name),
	fType(type)
{
	fType->AcquireReference();
}


DwarfDataMember::~DwarfDataMember()
{
	fType->ReleaseReference();
}

const char*
DwarfDataMember::Name() const
{
	return fName.Length() > 0 ? fName.String() : NULL;
}


Type*
DwarfDataMember::GetType() const
{
	return fType;
}


// #pragma mark - DwarfEnumeratorValue


DwarfEnumeratorValue::DwarfEnumeratorValue(DIEEnumerator* entry,
	const BString& name, const BVariant& value)
	:
	fEntry(entry),
	fName(name),
	fValue(value)
{
}


DwarfEnumeratorValue::~DwarfEnumeratorValue()
{
}

const char*
DwarfEnumeratorValue::Name() const
{
	return fName.Length() > 0 ? fName.String() : NULL;
}


BVariant
DwarfEnumeratorValue::Value() const
{
	return fValue;
}


// #pragma mark - DwarfArrayDimension


DwarfArrayDimension::DwarfArrayDimension(DwarfType* type)
	:
	fType(type)
{
	fType->AcquireReference();
}


DwarfArrayDimension::~DwarfArrayDimension()
{
	fType->ReleaseReference();
}


Type*
DwarfArrayDimension::GetType() const
{
	return fType;
}


// #pragma mark - DwarfFunctionParameter


DwarfFunctionParameter::DwarfFunctionParameter(DIEFormalParameter* entry,
	const BString& name, DwarfType* type)
	:
	fEntry(entry),
	fName(name),
	fType(type)
{
	fType->AcquireReference();
}


DwarfFunctionParameter::~DwarfFunctionParameter()
{
	fType->ReleaseReference();
}


const char*
DwarfFunctionParameter::Name() const
{
	return fName.Length() > 0 ? fName.String() : NULL;
}


Type*
DwarfFunctionParameter::GetType() const
{
	return fType;
}


// #pragma mark - DwarfPrimitiveType


DwarfPrimitiveType::DwarfPrimitiveType(DwarfTypeContext* typeContext,
	const BString& name, DIEBaseType* entry, uint32 typeConstant)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fTypeConstant(typeConstant)
{
}


DIEType*
DwarfPrimitiveType::GetDIEType() const
{
	return fEntry;
}


uint32
DwarfPrimitiveType::TypeConstant() const
{
	return fTypeConstant;
}


// #pragma mark - DwarfCompoundType


DwarfCompoundType::DwarfCompoundType(DwarfTypeContext* typeContext,
	const BString& name, DIECompoundType* entry)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry)
{
}


DwarfCompoundType::~DwarfCompoundType()
{
	for (int32 i = 0;
			DwarfInheritance* inheritance = fInheritances.ItemAt(i); i++) {
		inheritance->ReleaseReference();
	}
	for (int32 i = 0; DwarfDataMember* member = fDataMembers.ItemAt(i); i++)
		member->ReleaseReference();
}


int32
DwarfCompoundType::CountBaseTypes() const
{
	return fInheritances.CountItems();
}


BaseType*
DwarfCompoundType::BaseTypeAt(int32 index) const
{
	return fInheritances.ItemAt(index);
}


int32
DwarfCompoundType::CountDataMembers() const
{
	return fDataMembers.CountItems();
}


DataMember*
DwarfCompoundType::DataMemberAt(int32 index) const
{
	return fDataMembers.ItemAt(index);
}


status_t
DwarfCompoundType::ResolveBaseTypeLocation(BaseType* _baseType,
	const ValueLocation& parentLocation, ValueLocation*& _location)
{
	DwarfInheritance* baseType = dynamic_cast<DwarfInheritance*>(_baseType);
	if (baseType == NULL)
		return B_BAD_VALUE;

	return _ResolveDataMemberLocation(baseType->GetDwarfType(),
		baseType->Entry()->Location(), parentLocation, _location);
}


status_t
DwarfCompoundType::ResolveDataMemberLocation(DataMember* _member,
	const ValueLocation& parentLocation, ValueLocation*& _location)
{
	DwarfDataMember* member = dynamic_cast<DwarfDataMember*>(_member);
	if (member == NULL)
		return B_BAD_VALUE;
	DwarfTypeContext* typeContext = TypeContext();

	ValueLocation* location;
	status_t error = _ResolveDataMemberLocation(member->GetDwarfType(),
		member->Entry()->Location(), parentLocation, location);
	if (error != B_OK)
		return error;

	// If the member isn't a bit field, we're done.
	DIEMember* memberEntry = member->Entry();
	if (!memberEntry->ByteSize()->IsValid()
		&& !memberEntry->BitOffset()->IsValid()
		&& !memberEntry->BitSize()->IsValid()) {
		_location = location;
		return B_OK;
	}

	BReference<ValueLocation> locationReference(location);

	// get the byte size
	target_addr_t byteSize;
	if (memberEntry->ByteSize()->IsValid()) {
		BVariant value;
		error = typeContext->File()->EvaluateDynamicValue(
			typeContext->GetCompilationUnit(), typeContext->SubprogramEntry(),
			memberEntry->ByteSize(), typeContext->TargetInterface(),
			typeContext->InstructionPointer(), typeContext->FramePointer(),
			value);
		if (error != B_OK)
			return error;
		byteSize = value.ToUInt64();
	} else
		byteSize = ByteSize();

	// get the bit offset
	uint64 bitOffset = 0;
	if (memberEntry->BitOffset()->IsValid()) {
		BVariant value;
		error = typeContext->File()->EvaluateDynamicValue(
			typeContext->GetCompilationUnit(), typeContext->SubprogramEntry(),
			memberEntry->BitOffset(), typeContext->TargetInterface(),
			typeContext->InstructionPointer(), typeContext->FramePointer(),
			value);
		if (error != B_OK)
			return error;
		bitOffset = value.ToUInt64();
	}

	// get the bit size
	uint64 bitSize = byteSize * 8;
	if (memberEntry->BitSize()->IsValid()) {
		BVariant value;
		error = typeContext->File()->EvaluateDynamicValue(
			typeContext->GetCompilationUnit(), typeContext->SubprogramEntry(),
			memberEntry->BitSize(), typeContext->TargetInterface(),
			typeContext->InstructionPointer(), typeContext->FramePointer(),
			value);
		if (error != B_OK)
			return error;
		bitSize = value.ToUInt64();
	}

	TRACE_LOCALS("bit field: byte size: %llu, bit offset/size: %llu/%llu\n",
		byteSize, bitOffset, bitSize);

	if (bitOffset + bitSize > byteSize * 8)
		return B_BAD_VALUE;

	// create the bit field value location
	ValueLocation* bitFieldLocation = new(std::nothrow) ValueLocation;
	if (bitFieldLocation == NULL)
		return B_NO_MEMORY;
	BReference<ValueLocation> bitFieldLocationReference(bitFieldLocation, true);

	if (!bitFieldLocation->SetTo(*location, bitOffset, bitSize))
		return B_NO_MEMORY;

	_location = bitFieldLocationReference.Detach();
	return B_OK;
}


DIEType*
DwarfCompoundType::GetDIEType() const
{
	return fEntry;
}


bool
DwarfCompoundType::AddInheritance(DwarfInheritance* inheritance)
{
	if (!fInheritances.AddItem(inheritance))
		return false;

	inheritance->AcquireReference();
	return true;
}


bool
DwarfCompoundType::AddDataMember(DwarfDataMember* member)
{
	if (!fDataMembers.AddItem(member))
		return false;

	member->AcquireReference();
	return true;
}


status_t
DwarfCompoundType::_ResolveDataMemberLocation(DwarfType* memberType,
	const MemberLocation* memberLocation,
	const ValueLocation& parentLocation, ValueLocation*& _location)
{
	// create the value location object for the member
	ValueLocation* location = new(std::nothrow) ValueLocation(
		parentLocation.IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	BReference<ValueLocation> locationReference(location, true);

	switch (memberLocation->attributeClass) {
		case ATTRIBUTE_CLASS_CONSTANT:
		{
			if (!location->SetTo(parentLocation, memberLocation->constant * 8,
					memberType->ByteSize() * 8)) {
				return B_NO_MEMORY;
			}

			break;
		}
		case ATTRIBUTE_CLASS_BLOCK:
		case ATTRIBUTE_CLASS_LOCLISTPTR:
		{
			// The attribute is a location description. Since we need to push
			// the parent object value onto the stack, we require the parent
			// location to be a memory location.
			if (parentLocation.CountPieces() != 1)
				return B_BAD_VALUE;
			ValuePieceLocation piece = parentLocation.PieceAt(0);
			if (piece.type != VALUE_PIECE_LOCATION_MEMORY)
				return B_BAD_VALUE;

			// convert member location to location description
			LocationDescription locationDescription;
			if (memberLocation->attributeClass == ATTRIBUTE_CLASS_BLOCK) {
				locationDescription.SetToExpression(
					memberLocation->expression.data,
					memberLocation->expression.length);
			} else {
				locationDescription.SetToLocationList(
					memberLocation->listOffset);
			}

			// evaluate the location description
			status_t error = memberType->ResolveLocation(TypeContext(),
				&locationDescription, piece.address, true, *location);
			if (error != B_OK)
				return error;

			break;
		}
		default:
		{
			// for unions the member location can be omitted -- all members
			// start at the beginning of the parent object
			if (fEntry->Tag() != DW_TAG_union_type)
				return B_BAD_VALUE;

			// since all members start at the same location, set up
			// the location by hand since we don't want the size difference
			// between the overall union and the member being
			// factored into the assigned address.
			ValuePieceLocation piece = parentLocation.PieceAt(0);
			piece.SetSize(memberType->ByteSize());
			if (!location->AddPiece(piece))
				return B_NO_MEMORY;

			break;
		}
	}

	_location = locationReference.Detach();
	return B_OK;
}


// #pragma mark - DwarfArrayType


DwarfArrayType::DwarfArrayType(DwarfTypeContext* typeContext,
	const BString& name, DIEArrayType* entry, DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fBaseType(baseType)
{
	fBaseType->AcquireReference();
}


DwarfArrayType::~DwarfArrayType()
{
	for (int32 i = 0;
		DwarfArrayDimension* dimension = fDimensions.ItemAt(i); i++) {
		dimension->ReleaseReference();
	}

	fBaseType->ReleaseReference();
}


Type*
DwarfArrayType::BaseType() const
{
	return fBaseType;
}


int32
DwarfArrayType::CountDimensions() const
{
	return fDimensions.CountItems();
}


ArrayDimension*
DwarfArrayType::DimensionAt(int32 index) const
{
	return fDimensions.ItemAt(index);
}


status_t
DwarfArrayType::ResolveElementLocation(const ArrayIndexPath& indexPath,
	const ValueLocation& parentLocation, ValueLocation*& _location)
{
	if (indexPath.CountIndices() != CountDimensions())
		return B_BAD_VALUE;
	DwarfTypeContext* typeContext = TypeContext();

	// If the array entry has a bit stride, get it. Otherwise fall back to the
	// element type size.
	int64 bitStride;
	if (DIEArrayType* bitStrideOwnerEntry = DwarfUtils::GetDIEByPredicate(
			fEntry, HasBitStridePredicate<DIEArrayType>())) {
		BVariant value;
		status_t error = typeContext->File()->EvaluateDynamicValue(
			typeContext->GetCompilationUnit(), typeContext->SubprogramEntry(),
			bitStrideOwnerEntry->BitStride(), typeContext->TargetInterface(),
			typeContext->InstructionPointer(), typeContext->FramePointer(),
			value);
		if (error != B_OK)
			return error;
		if (!value.IsInteger())
			return B_BAD_VALUE;
		bitStride = value.ToInt64();
	} else
		bitStride = BaseType()->ByteSize() * 8;

	// Iterate backward through the dimensions and compute the total offset of
	// the element.
	int64 elementOffset = 0;
	DwarfArrayDimension* previousDimension = NULL;
	int64 previousDimensionStride = 0;
	for (int32 dimensionIndex = CountDimensions() - 1;
			dimensionIndex >= 0; dimensionIndex--) {
		DwarfArrayDimension* dimension = DwarfDimensionAt(dimensionIndex);
		int64 index = indexPath.IndexAt(dimensionIndex);

		// If the dimension has a special bit/byte stride, get it.
		int64 dimensionStride = 0;
		DwarfType* dimensionType = dimension->GetDwarfType();
		DIEArrayIndexType* dimensionTypeEntry = dimensionType != NULL
			? dynamic_cast<DIEArrayIndexType*>(dimensionType->GetDIEType())
			: NULL;
		if (dimensionTypeEntry != NULL) {
			DIEArrayIndexType* bitStrideOwnerEntry
				= DwarfUtils::GetDIEByPredicate(dimensionTypeEntry,
					HasBitStridePredicate<DIEArrayIndexType>());
			if (bitStrideOwnerEntry != NULL) {
				BVariant value;
				status_t error = typeContext->File()->EvaluateDynamicValue(
					typeContext->GetCompilationUnit(),
					typeContext->SubprogramEntry(),
					bitStrideOwnerEntry->BitStride(),
					typeContext->TargetInterface(),
					typeContext->InstructionPointer(),
					typeContext->FramePointer(), value);
				if (error != B_OK)
					return error;
				if (!value.IsInteger())
					return B_BAD_VALUE;
				dimensionStride = value.ToInt64();
			} else {
				DIEArrayIndexType* byteStrideOwnerEntry
					= DwarfUtils::GetDIEByPredicate(dimensionTypeEntry,
						HasByteStridePredicate<DIEArrayIndexType>());
				if (byteStrideOwnerEntry != NULL) {
					BVariant value;
					status_t error = typeContext->File()->EvaluateDynamicValue(
						typeContext->GetCompilationUnit(),
						typeContext->SubprogramEntry(),
						byteStrideOwnerEntry->ByteStride(),
						typeContext->TargetInterface(),
						typeContext->InstructionPointer(),
						typeContext->FramePointer(), value);
					if (error != B_OK)
						return error;
					if (!value.IsInteger())
						return B_BAD_VALUE;
					dimensionStride = value.ToInt64() * 8;
				}
			}
		}

		// If we don't have a stride for the dimension yet, use the stride of
		// the previous dimension multiplied by the size of the dimension.
		if (dimensionStride == 0) {
			if (previousDimension != NULL) {
				dimensionStride = previousDimensionStride
					* previousDimension->CountElements();
			} else {
				// the last dimension -- use the element bit stride
				dimensionStride = bitStride;
			}
		}

		// If the dimension stride is still 0 (that can happen, if the dimension
		// doesn't have a stride and the previous dimension's element count is
		// not known), we can only resolve the first element.
		if (dimensionStride == 0 && index != 0) {
			WARNING("No dimension bit stride for dimension %ld and element "
				"index is not 0.\n", dimensionIndex);
			return B_BAD_VALUE;
		}

		elementOffset += dimensionStride * index;

		previousDimension = dimension;
		previousDimensionStride = dimensionStride;
	}

	TRACE_LOCALS("total element bit offset: %lld\n", elementOffset);

	// create the value location object for the element
	ValueLocation* location = new(std::nothrow) ValueLocation(
		parentLocation.IsBigEndian());
	if (location == NULL)
		return B_NO_MEMORY;
	BReference<ValueLocation> locationReference(location, true);

	// If we have a single memory piece location for the array, we compute the
	// element's location by hand -- not uncommonly the array size isn't known.
	if (parentLocation.CountPieces() == 1) {
		ValuePieceLocation piece = parentLocation.PieceAt(0);
		if (piece.type == VALUE_PIECE_LOCATION_MEMORY) {
			int64 byteOffset = elementOffset >= 0
				? elementOffset / 8 : (elementOffset - 7) / 8;
			piece.SetToMemory(piece.address + byteOffset);
			piece.SetSize(BaseType()->ByteSize());
			// TODO: Support bit offsets correctly!
			// TODO: Support bit fields (primitive types) correctly!

			if (!location->AddPiece(piece))
				return B_NO_MEMORY;

			_location = locationReference.Detach();
			return B_OK;
		}
	}

	// We can't deal with negative element offsets at this point. It doesn't
	// make a lot of sense anyway, if the array location consists of multiple
	// pieces or lives in a register.
	if (elementOffset < 0) {
		WARNING("Negative element offset unsupported for multiple location "
			"pieces or register pieces.\n");
		return B_UNSUPPORTED;
	}

	if (!location->SetTo(parentLocation, elementOffset,
			BaseType()->ByteSize() * 8)) {
		return B_NO_MEMORY;
	}

	_location = locationReference.Detach();
	return B_OK;
}



DIEType*
DwarfArrayType::GetDIEType() const
{
	return fEntry;
}


bool
DwarfArrayType::AddDimension(DwarfArrayDimension* dimension)
{
	if (!fDimensions.AddItem(dimension))
		return false;

	dimension->AcquireReference();
	return true;
}


// #pragma mark - DwarfModifiedType


DwarfModifiedType::DwarfModifiedType(DwarfTypeContext* typeContext,
	const BString& name, DIEModifiedType* entry, uint32 modifiers,
	DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fModifiers(modifiers),
	fBaseType(baseType)
{
	fBaseType->AcquireReference();
}


DwarfModifiedType::~DwarfModifiedType()
{
	fBaseType->ReleaseReference();
}


uint32
DwarfModifiedType::Modifiers() const
{
	return fModifiers;
}


Type*
DwarfModifiedType::BaseType() const
{
	return fBaseType;
}


DIEType*
DwarfModifiedType::GetDIEType() const
{
	return fEntry;
}


// #pragma mark - DwarfTypedefType


DwarfTypedefType::DwarfTypedefType(DwarfTypeContext* typeContext,
	const BString& name, DIETypedef* entry, DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fBaseType(baseType)
{
	fBaseType->AcquireReference();
}


DwarfTypedefType::~DwarfTypedefType()
{
	fBaseType->ReleaseReference();
}


Type*
DwarfTypedefType::BaseType() const
{
	return fBaseType;
}


DIEType*
DwarfTypedefType::GetDIEType() const
{
	return fEntry;
}


// #pragma mark - DwarfAddressType


DwarfAddressType::DwarfAddressType(DwarfTypeContext* typeContext,
	const BString& name, DIEAddressingType* entry,
	address_type_kind addressKind, DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fAddressKind(addressKind),
	fBaseType(baseType)
{
	fBaseType->AcquireReference();
}


DwarfAddressType::~DwarfAddressType()
{
	fBaseType->ReleaseReference();
}


address_type_kind
DwarfAddressType::AddressKind() const
{
	return fAddressKind;
}


Type*
DwarfAddressType::BaseType() const
{
	return fBaseType;
}


DIEType*
DwarfAddressType::GetDIEType() const
{
	return fEntry;
}


// #pragma mark - DwarfEnumerationType


DwarfEnumerationType::DwarfEnumerationType(DwarfTypeContext* typeContext,
	const BString& name, DIEEnumerationType* entry, DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fBaseType(baseType)
{
	if (fBaseType != NULL)
		fBaseType->AcquireReference();
}


DwarfEnumerationType::~DwarfEnumerationType()
{
	for (int32 i = 0; DwarfEnumeratorValue* value = fValues.ItemAt(i); i++)
		value->ReleaseReference();

	if (fBaseType != NULL)
		fBaseType->ReleaseReference();
}


Type*
DwarfEnumerationType::BaseType() const
{
	return fBaseType;
}


int32
DwarfEnumerationType::CountValues() const
{
	return fValues.CountItems();
}


EnumeratorValue*
DwarfEnumerationType::ValueAt(int32 index) const
{
	return fValues.ItemAt(index);
}


DIEType*
DwarfEnumerationType::GetDIEType() const
{
	return fEntry;
}


bool
DwarfEnumerationType::AddValue(DwarfEnumeratorValue* value)
{
	if (!fValues.AddItem(value))
		return false;

	value->AcquireReference();
	return true;
}


// #pragma mark - DwarfSubrangeType


DwarfSubrangeType::DwarfSubrangeType(DwarfTypeContext* typeContext,
	const BString& name, DIESubrangeType* entry, Type* baseType,
	const BVariant& lowerBound, const BVariant& upperBound)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fBaseType(baseType),
	fLowerBound(lowerBound),
	fUpperBound(upperBound)
{
	fBaseType->AcquireReference();
}


DwarfSubrangeType::~DwarfSubrangeType()
{
	fBaseType->ReleaseReference();
}


Type*
DwarfSubrangeType::BaseType() const
{
	return fBaseType;
}


DIEType*
DwarfSubrangeType::GetDIEType() const
{
	return fEntry;
}


BVariant
DwarfSubrangeType::LowerBound() const
{
	return fLowerBound;
}


BVariant
DwarfSubrangeType::UpperBound() const
{
	return fUpperBound;
}


// #pragma mark - DwarfUnspecifiedType


DwarfUnspecifiedType::DwarfUnspecifiedType(DwarfTypeContext* typeContext,
	const BString& name, DIEUnspecifiedType* entry)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry)
{
}


DwarfUnspecifiedType::~DwarfUnspecifiedType()
{
}


DIEType*
DwarfUnspecifiedType::GetDIEType() const
{
	return fEntry;
}


// #pragma mark - DwarfFunctionType


DwarfFunctionType::DwarfFunctionType(DwarfTypeContext* typeContext,
	const BString& name, DIESubroutineType* entry, DwarfType* returnType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fReturnType(returnType),
	fHasVariableArguments(false)
{
	if (fReturnType != NULL)
		fReturnType->AcquireReference();
}


DwarfFunctionType::~DwarfFunctionType()
{
	for (int32 i = 0;
		DwarfFunctionParameter* parameter = fParameters.ItemAt(i); i++) {
		parameter->ReleaseReference();
	}

	if (fReturnType != NULL)
		fReturnType->ReleaseReference();
}


Type*
DwarfFunctionType::ReturnType() const
{
	return fReturnType;
}


int32
DwarfFunctionType::CountParameters() const
{
	return fParameters.CountItems();
}


FunctionParameter*
DwarfFunctionType::ParameterAt(int32 index) const
{
	return fParameters.ItemAt(index);
}


bool
DwarfFunctionType::HasVariableArguments() const
{
	return fHasVariableArguments;
}


void
DwarfFunctionType::SetHasVariableArguments(bool hasVarArgs)
{
	fHasVariableArguments = hasVarArgs;
}


DIEType*
DwarfFunctionType::GetDIEType() const
{
	return fEntry;
}


bool
DwarfFunctionType::AddParameter(DwarfFunctionParameter* parameter)
{
	if (!fParameters.AddItem(parameter))
		return false;

	parameter->AcquireReference();
	return true;
}


// #pragma mark - DwarfPointerToMemberType


DwarfPointerToMemberType::DwarfPointerToMemberType(
	DwarfTypeContext* typeContext, const BString& name,
	DIEPointerToMemberType* entry, DwarfCompoundType* containingType,
	DwarfType* baseType)
	:
	DwarfType(typeContext, name, entry),
	fEntry(entry),
	fContainingType(containingType),
	fBaseType(baseType)
{
	fContainingType->AcquireReference();
	fBaseType->AcquireReference();
}


DwarfPointerToMemberType::~DwarfPointerToMemberType()
{
	fContainingType->ReleaseReference();
	fBaseType->ReleaseReference();
}


CompoundType*
DwarfPointerToMemberType::ContainingType() const
{
	return fContainingType;
}


Type*
DwarfPointerToMemberType::BaseType() const
{
	return fBaseType;
}


DIEType*
DwarfPointerToMemberType::GetDIEType() const
{
	return fEntry;
}
