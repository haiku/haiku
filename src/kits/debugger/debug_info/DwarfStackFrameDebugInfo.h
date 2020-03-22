/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_STACK_FRAME_DEBUG_INFO_H
#define DWARF_STACK_FRAME_DEBUG_INFO_H


#include <image.h>
#include <String.h>

#include "StackFrameDebugInfo.h"


class CompilationUnit;
class CpuState;
class DIEFormalParameter;
class DIESubprogram;
class DIEType;
class DIEVariable;
class DwarfFile;
class DwarfTargetInterface;
class DwarfTypeContext;
class DwarfTypeFactory;
class FunctionID;
class GlobalTypeCache;
class GlobalTypeLookup;
struct LocationDescription;
class ObjectID;
class RegisterMap;
class Variable;


class DwarfStackFrameDebugInfo : public StackFrameDebugInfo {
public:
								DwarfStackFrameDebugInfo(
									Architecture* architecture,
									image_id imageID, DwarfFile* file,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									GlobalTypeLookup* typeLookup,
									GlobalTypeCache* typeCache,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									target_addr_t relocationDelta,
									DwarfTargetInterface* targetInterface,
									RegisterMap* fromDwarfRegisterMap);
								~DwarfStackFrameDebugInfo();

			status_t			Init();

			status_t			CreateParameter(FunctionID* functionID,
									DIEFormalParameter* parameterEntry,
									Variable*& _parameter);
									// returns reference
			status_t			CreateLocalVariable(FunctionID* functionID,
									DIEVariable* variableEntry,
									Variable*& _variable);
									// returns reference
			status_t			CreateReturnValue(FunctionID* functionID,
									DIEType* returnType,
									ValueLocation* location,
									CpuState* state,
									Variable*& _variable);
									// returns reference

private:
			struct DwarfFunctionParameterID;
			struct DwarfLocalVariableID;
			struct DwarfReturnValueID;

private:
			status_t			_CreateVariable(ObjectID* id,
									const BString& name, DIEType* typeEntry,
									LocationDescription* locationDescription,
									Variable*& _variable);

	template<typename EntryType>
	static	DIEType*			_GetDIEType(EntryType* entry);

private:
			DwarfTypeContext*	fTypeContext;
			GlobalTypeLookup*	fTypeLookup;
			GlobalTypeCache*	fTypeCache;
			DwarfTypeFactory*	fTypeFactory;
};


#endif	// DWARF_STACK_FRAME_DEBUG_INFO_H
