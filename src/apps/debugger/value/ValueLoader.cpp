/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ValueLoader.h"

#include "Architecture.h"
#include "BitBuffer.h"
#include "CpuState.h"
#include "Register.h"
#include "TeamMemory.h"
#include "TeamTypeInformation.h"
#include "Tracing.h"
#include "TypeLookupConstraints.h"
#include "ValueLocation.h"


ValueLoader::ValueLoader(Architecture* architecture, TeamMemory* teamMemory,
	TeamTypeInformation* typeInformation, CpuState* cpuState)
	:
	fArchitecture(architecture),
	fTeamMemory(teamMemory),
	fTypeInformation(typeInformation),
	fCpuState(cpuState)
{
	fArchitecture->AcquireReference();
	fTeamMemory->AcquireReference();
	fTypeInformation->AcquireReference();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


ValueLoader::ValueLoader(const ValueLoader& other)
	:
	fArchitecture(other.fArchitecture),
	fTeamMemory(other.fTeamMemory),
	fTypeInformation(other.fTypeInformation),
	fCpuState(other.fCpuState)
{
	fArchitecture->AcquireReference();
	fTeamMemory->AcquireReference();
	fTypeInformation->AcquireReference();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


ValueLoader::~ValueLoader()
{
	fArchitecture->ReleaseReference();
	fTeamMemory->ReleaseReference();
	fTypeInformation->ReleaseReference();
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}


status_t
ValueLoader::LoadValue(ValueLocation* location, type_code valueType,
	bool shortValueIsFine, BVariant& _value)
{
	static const size_t kMaxPieceSize = 16;
	uint64 totalBitSize = 0;
	int32 count = location->CountPieces();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = location->PieceAt(i);
		switch (piece.type) {
			case VALUE_PIECE_LOCATION_INVALID:
			case VALUE_PIECE_LOCATION_UNKNOWN:
				return B_ENTRY_NOT_FOUND;
			case VALUE_PIECE_LOCATION_MEMORY:
			case VALUE_PIECE_LOCATION_REGISTER:
				break;
		}

		if (piece.size > kMaxPieceSize) {
			TRACE_LOCALS("  -> overly long piece size (%" B_PRIu64 " bytes)\n",
				piece.size);
			return B_UNSUPPORTED;
		}

		totalBitSize += piece.bitSize;
	}

	TRACE_LOCALS("  -> totalBitSize: %" B_PRIu64 "\n", totalBitSize);

	if (totalBitSize == 0) {
		TRACE_LOCALS("  -> no size\n");
		return B_ENTRY_NOT_FOUND;
	}

	if (totalBitSize > 64) {
		TRACE_LOCALS("  -> longer than 64 bits: unsupported\n");
		return B_UNSUPPORTED;
	}

	uint64 valueBitSize = BVariant::SizeOfType(valueType) * 8;
	if (!shortValueIsFine && totalBitSize < valueBitSize) {
		TRACE_LOCALS("  -> too short for value type (%" B_PRIu64 " vs. %"
			B_PRIu64 " bits)\n", totalBitSize, valueBitSize);
		return B_BAD_VALUE;
	}

	// Load the data. Since the BitBuffer class we're using only supports big
	// endian bit semantics, we convert all data to big endian before pushing
	// them to the buffer. For later conversion to BVariant we need to make sure
	// the final buffer has the size of the value type, so we pad the most
	// significant bits with zeros.
	BitBuffer valueBuffer;
	if (totalBitSize < valueBitSize)
		valueBuffer.AddZeroBits(valueBitSize - totalBitSize);

	bool bigEndian = fArchitecture->IsBigEndian();
	const Register* registers = fArchitecture->Registers();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = location->PieceAt(
			bigEndian ? i : count - i - 1);
		uint32 bytesToRead = piece.size;
		uint32 bitSize = piece.bitSize;
		uint8 bitOffset = piece.bitOffset;

		switch (piece.type) {
			case VALUE_PIECE_LOCATION_INVALID:
			case VALUE_PIECE_LOCATION_UNKNOWN:
				return B_ENTRY_NOT_FOUND;
			case VALUE_PIECE_LOCATION_MEMORY:
			{
				target_addr_t address = piece.address;

				TRACE_LOCALS("  piece %" B_PRId32 ": memory address: %#"
					B_PRIx64 ", bits: %" B_PRIu32 "\n", i, address, bitSize);

				uint8 pieceBuffer[kMaxPieceSize];
				ssize_t bytesRead = fTeamMemory->ReadMemory(address,
					pieceBuffer, bytesToRead);
				if (bytesRead < 0)
					return bytesRead;
				if ((uint32)bytesRead != bytesToRead)
					return B_BAD_ADDRESS;

				TRACE_LOCALS_ONLY(
					TRACE_LOCALS("  -> read: ");
					for (ssize_t k = 0; k < bytesRead; k++)
						TRACE_LOCALS("%02x", pieceBuffer[k]);
					TRACE_LOCALS("\n");
				)

				// convert to big endian
				if (!bigEndian) {
					for (int32 k = bytesRead / 2 - 1; k >= 0; k--) {
						std::swap(pieceBuffer[k],
							pieceBuffer[bytesRead - k - 1]);
					}
				}

				valueBuffer.AddBits(pieceBuffer, bitSize, bitOffset);
				break;
			}
			case VALUE_PIECE_LOCATION_REGISTER:
			{
				TRACE_LOCALS("  piece %" B_PRId32 ": register: %" B_PRIu32
					", bits: %" B_PRIu32 "\n", i, piece.reg, bitSize);

				if (fCpuState == NULL) {
					WARNING("ValueLoader::LoadValue(): register piece, but no "
						"CpuState\n");
					return B_UNSUPPORTED;
				}

				BVariant registerValue;
				if (!fCpuState->GetRegisterValue(registers + piece.reg,
						registerValue)) {
					return B_ENTRY_NOT_FOUND;
				}
				if (registerValue.Size() < bytesToRead)
					return B_ENTRY_NOT_FOUND;

				if (!bigEndian)
					registerValue.SwapEndianess();
				valueBuffer.AddBits(registerValue.Bytes(), bitSize, bitOffset);
				break;
			}
		}
	}

	// If we don't have enough bits in the buffer apparently adding some failed.
	if (valueBuffer.BitSize() < valueBitSize)
		return B_NO_MEMORY;

	// convert the bits into something we can work with
	BVariant value;
	status_t error = value.SetToTypedData(valueBuffer.Bytes(), valueType);
	if (error != B_OK) {
		TRACE_LOCALS("  -> failed to set typed data: %s\n", strerror(error));
		return error;
	}

	// convert to host endianess
	#if B_HOST_IS_LENDIAN
		value.SwapEndianess();
	#endif

	_value = value;
	return B_OK;
}


status_t
ValueLoader::LoadRawValue(BVariant& location, size_t bytesToRead, void* _value)
{
	ssize_t bytesRead = fTeamMemory->ReadMemory(location.ToUInt64(),
		_value, bytesToRead);
	if (bytesRead < 0)
		return bytesRead;
	if ((uint32)bytesRead != bytesToRead)
		return B_BAD_ADDRESS;
	return B_OK;
}


status_t
ValueLoader::LoadStringValue(BVariant& location, size_t maxSize, BString& _value)
{
	static const size_t kMaxStringSize = 255;

	return fTeamMemory->ReadMemoryString(location.ToUInt64(),
		std::min(maxSize, kMaxStringSize), _value);
}


status_t
ValueLoader::LookupTypeByName(const BString& name,
	const TypeLookupConstraints& constraints, Type*& _type)
{
	return fTypeInformation->LookupTypeByName(name, constraints, _type);
}
