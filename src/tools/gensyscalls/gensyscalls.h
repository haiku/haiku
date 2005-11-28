#ifndef GENSYSCALLS_H
#define GENSYSCALLS_H

// Type
class Type {
public:
								Type(const char* name, int size,
									int usedSize,
									const char* alignmentTypeName);
								~Type() {}

			const char*			TypeName() const	{ return fName; }
			int					Size() const		{ return fSize; }
			int					UsedSize() const	{ return fUsedSize; }
			const char*			AlignmentTypeName() const
									{ return fAlignmentType; }

private:
			const char*			fName;
			int					fSize;
			int					fUsedSize;
			const char*			fAlignmentType;
};

// Parameter
class Parameter : public Type {
public:
								Parameter(const char* typeName,
									const char* parameterName, int size,
									int usedSize, int offset,
									const char* alignmentTypeName);
								~Parameter() {}

			const char*			ParameterName() const { return fParameterName; }
			int					Offset() const		{ return fOffset; }

private:
			const char*			fParameterName;
			int					fOffset;
};

// Syscall
class Syscall {
public:
								Syscall(const char* name,
									const char* kernelName);
								~Syscall();

			const char*			Name() const		{ return fName; }
			const char*			KernelName() const	{ return fKernelName; }
			Type*				ReturnType() const	{ return fReturnType; }

			int					CountParameters() const;
			Parameter*			ParameterAt(int index) const;
			Parameter*			LastParameter() const;

	template<typename T> void	SetReturnType(const char* name);
	template<typename T> void	AddParameter(const char* typeName,
									const char* parameterName);

			Type*				SetReturnType(const char* name, int size,
									int usedSize,
									const char* alignmentTypeName);
			Parameter*			AddParameter(const char* typeName,
									const char* parameterName, int size,
									int usedSize, int offset,
									const char* alignmentTypeName);

private:
			struct ParameterVector;

			const char*			fName;
			const char*			fKernelName;
			Type*				fReturnType;
			ParameterVector*	fParameters;
};

// SyscallVector
class SyscallVector {
public:
								SyscallVector();
								~SyscallVector();

	static	SyscallVector*		Create();

			int					CountSyscalls() const;
			Syscall*			SyscallAt(int index) const;

			Syscall*			CreateSyscall(const char* name,
									const char* kernelName);

private:
			struct _SyscallVector;

			_SyscallVector*		fSyscalls;
};

extern SyscallVector* create_syscall_vector();


#ifndef DONT_INCLUDE_ARCH_GENSYSCALLS_H

// align_to_type
template<typename T>
int
align_to_type(int size)
{
	return (size + sizeof(T) - 1) / sizeof(T) * sizeof(T);
}

#include "arch_gensyscalls.h"

// SetReturnType
template<typename T>
void
Syscall::SetReturnType(const char* name)
{
	ReturnTypeCreator<T>::Create(this, name);
}

// AddParameter
template<typename T>
void
Syscall::AddParameter(const char* typeName, const char* parameterName)
{
	ParameterCreator<T>::Create(this, typeName, parameterName);
}

#endif	// !DONT_INCLUDE_ARCH_GENSYSCALLS_H

#endif	// GENSYSCALLS_H
