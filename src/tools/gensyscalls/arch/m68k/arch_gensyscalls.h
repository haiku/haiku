// Included by gensyscalls.

typedef int AlignmentType;
static const char* kAlignmentType = "int";
static const int kAlignment = sizeof(AlignmentType);

// ReturnTypeCreator
template<typename T>
class ReturnTypeCreator {
public:
	static void Create(Syscall* syscall, const char* name)
	{
		int size = sizeof(T);
		int usedSize = align_to_type<AlignmentType>(size);
		const char* alignmentType
			= (size != usedSize && size < kAlignment ? kAlignmentType : 0);

		syscall->SetReturnType(name, size, usedSize, alignmentType);
	}
};

template<>
class ReturnTypeCreator<void> {
public:
	static void Create(Syscall* syscall, const char* name)
	{
		syscall->SetReturnType(name, 0, 0, 0);
	}
};

// ParameterCreator
template<typename T>
class ParameterCreator {
public:
	static void Create(Syscall* syscall, const char* typeName,
		const char* parameterName)
	{
		// compute offset
		int offset = 0;
		if (Parameter* previous = syscall->LastParameter())
			offset = previous->Offset() + previous->UsedSize();

		int size = sizeof(T);
		int usedSize = align_to_type<AlignmentType>(size);
		const char* alignmentType
			= (size != usedSize && size < kAlignment ? kAlignmentType : 0);

		syscall->AddParameter(typeName, parameterName, size, usedSize, offset,
			alignmentType);
	}
};
