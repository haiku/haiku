/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ValueWriter.h"

#include "Architecture.h"
#include "BitBuffer.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Register.h"
#include "TeamMemory.h"
#include "Tracing.h"
#include "ValueLocation.h"


ValueWriter::ValueWriter(Architecture* architecture,
	DebuggerInterface* interface, CpuState* cpuState, thread_id targetThread)
	:
	fArchitecture(architecture),
	fDebuggerInterface(interface),
	fCpuState(cpuState),
	fTargetThread(targetThread)
{
	fArchitecture->AcquireReference();
	fDebuggerInterface->AcquireReference();
	if (fCpuState != NULL)
		fCpuState->AcquireReference();
}


ValueWriter::~ValueWriter()
{
	fArchitecture->ReleaseReference();
	fDebuggerInterface->ReleaseReference();
	if (fCpuState != NULL)
		fCpuState->ReleaseReference();
}


status_t
ValueWriter::WriteValue(ValueLocation* location, BVariant& value)
{
	if (!location->IsWritable())
		return B_BAD_VALUE;

	int32 count = location->CountPieces();
	if (fCpuState == NULL) {
		for (int32 i = 0; i < count; i++) {
			const ValuePieceLocation piece = location->PieceAt(i);
			if (piece.type == VALUE_PIECE_LOCATION_REGISTER) {
				TRACE_LOCALS("  -> asked to write value with register piece, "
					"but no CPU state to write to.\n");
				return B_UNSUPPORTED;
			}
		}
	}

	bool cpuStateWriteNeeded = false;
	size_t byteOffset = 0;
	bool bigEndian = fArchitecture->IsBigEndian();
	const Register* registers = fArchitecture->Registers();
	for (int32 i = 0; i < count; i++) {
		ValuePieceLocation piece = location->PieceAt(
			bigEndian ? i : count - i - 1);
		uint32 bytesToWrite = piece.size;

		uint8* targetData = (uint8*)value.Bytes() + byteOffset;

		switch (piece.type) {
			case VALUE_PIECE_LOCATION_MEMORY:
			{
				target_addr_t address = piece.address;

				TRACE_LOCALS("  piece %" B_PRId32 ": memory address: %#"
					B_PRIx64 ", bits: %" B_PRIu32 "\n", i, address,
					bytesToWrite * 8);

				ssize_t bytesWritten = fDebuggerInterface->WriteMemory(address,
					targetData, bytesToWrite);

				if (bytesWritten < 0)
					return bytesWritten;
				if ((uint32)bytesWritten != bytesToWrite)
					return B_BAD_ADDRESS;

				break;
			}
			case VALUE_PIECE_LOCATION_REGISTER:
			{
				TRACE_LOCALS("  piece %" B_PRId32 ": register: %" B_PRIu32
					", bits: %" B_PRIu64 "\n", i, piece.reg, piece.bitSize);

				const Register* target = registers + piece.reg;
				BVariant pieceValue;
				switch (bytesToWrite) {
					case 1:
						pieceValue.SetTo(*(uint8*)targetData);
						break;
					case 2:
						pieceValue.SetTo(*(uint16*)targetData);
						break;
					case 4:
						pieceValue.SetTo(*(uint32*)targetData);
						break;
					case 8:
						pieceValue.SetTo(*(uint64*)targetData);
						break;
					default:
						TRACE_LOCALS("Asked to write unsupported piece size %"
							B_PRId32 " to register\n", bytesToWrite);
						return B_UNSUPPORTED;
				}

				if (!fCpuState->SetRegisterValue(target, pieceValue))
					return B_NO_MEMORY;

				cpuStateWriteNeeded = true;
				break;
			}
			default:
				return B_UNSUPPORTED;
		}

		byteOffset += bytesToWrite;
	}

	if (cpuStateWriteNeeded)
		return fDebuggerInterface->SetCpuState(fTargetThread, fCpuState);

	return B_OK;
}
