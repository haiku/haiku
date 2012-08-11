/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <string>

#include <system_revision.h>

// We use htonl(), which is defined in <ByteOrder.h> on BeOS R5.
#ifdef HAIKU_HOST_PLATFORM_BEOS
	#include <ByteOrder.h>
#else
	#include <arpa/inet.h>
#endif


using std::string;
using std::max;
using std::min;


// #pragma mark - ELF definitions


// types
typedef uint32_t	Elf32_Addr;
typedef uint16_t	Elf32_Half;
typedef uint32_t	Elf32_Off;
typedef int32_t		Elf32_Sword;
typedef uint32_t	Elf32_Word;
typedef uint64_t	Elf64_Addr;
typedef uint64_t	Elf64_Off;
typedef uint16_t	Elf64_Half;
typedef uint32_t	Elf64_Word;
typedef int32_t		Elf64_Sword;
typedef uint64_t	Elf64_Xword;
typedef int64_t		Elf64_Sxword;

// e_ident indices
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7
#define EI_NIDENT	16

// object file header
typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half		e_type;
	Elf32_Half		e_machine;
	Elf32_Word		e_version;
	Elf32_Addr		e_entry;
	Elf32_Off		e_phoff;
	Elf32_Off		e_shoff;
	Elf32_Word		e_flags;
	Elf32_Half		e_ehsize;
	Elf32_Half		e_phentsize;
	Elf32_Half		e_phnum;
	Elf32_Half		e_shentsize;
	Elf32_Half		e_shnum;
	Elf32_Half		e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	Elf64_Half		e_type;
	Elf64_Half		e_machine;
	Elf64_Word		e_version;
	Elf64_Addr		e_entry;
	Elf64_Off		e_phoff;
	Elf64_Off		e_shoff;
	Elf64_Word		e_flags;
	Elf64_Half		e_ehsize;
	Elf64_Half		e_phentsize;
	Elf64_Half		e_phnum;
	Elf64_Half		e_shentsize;
	Elf64_Half		e_shnum;
	Elf64_Half		e_shstrndx;
} Elf64_Ehdr;

// e_ident EI_CLASS and EI_DATA values
#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2
#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2

// program header
typedef struct {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;

typedef struct {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;
	Elf64_Addr	p_vaddr;
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;
	Elf64_Xword	p_memsz;
	Elf64_Xword	p_align;
} Elf64_Phdr;

// p_type
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDIR	6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

// section header
typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
} Elf64_Shdr;

// sh_type values
#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

// special section indexes
#define SHN_UNDEF		0

static const char		kELFFileMagic[4]	= { 0x7f, 'E', 'L', 'F' };


// #pragma mark - Usage

// usage
static const char *kUsage =
"Usage: %s  <file> <revision>\n"
"\n"
"Finds the haiku revision section in ELF object file <file> and replaces the\n"
"writes the number given by <revision> into the first 32 bits of the\n"
"section.\n"
;

// command line args
static int sArgc;
static const char *const *sArgv;

// print_usage
void
print_usage(bool error)
{
	// get nice program name
	const char *programName = (sArgc > 0 ? sArgv[0] : "resattr");
	if (const char *lastSlash = strrchr(programName, '/'))
		programName = lastSlash + 1;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, programName);
}

// print_usage_and_exit
static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


// #pragma mark - Exception


class Exception {
public:
	// constructor
	Exception()
		: fError(errno),
		  fDescription()
	{
	}

	// constructor
	Exception(const char* format,...)
		: fError(errno),
		  fDescription()
	{
		va_list args;
		va_start(args, format);
		SetTo(errno, format, args);
		va_end(args);
	}

	// constructor
	Exception(int error)
		: fError(error),
		  fDescription()
	{
	}

	// constructor
	Exception(int error, const char* format,...)
		: fError(error),
		  fDescription()
	{
		va_list args;
		va_start(args, format);
		SetTo(error, format, args);
		va_end(args);
	}

	// copy constructor
	Exception(const Exception& exception)
		: fError(exception.fError),
		  fDescription(exception.fDescription)
	{
	}

	// destructor
	~Exception()
	{
	}

	// SetTo
	void SetTo(int error, const char* format, va_list arg)
	{
		char buffer[2048];
		vsprintf(buffer, format, arg);
		fError = error;
		fDescription = buffer;
	}

	// Error
	int Error() const
	{
		return fError;
	}

	// Description
	const string& Description() const
	{
		return fDescription;
	}

private:
	int		fError;
	string	fDescription;
};


// #pragma mark - ELFObject


struct SectionInfo {
	uint32_t	type;
	off_t		offset;
	size_t		size;
	const char*	name;
};


class ELFObject {
public:
	ELFObject()
		: fFD(-1),
		  fSectionHeaderStrings(NULL),
		  fSectionHeaderStringsLength(0)
	{
	}

	~ELFObject()
	{
		Unset();
	}

	void Unset()
	{
		if (fFD >= 0) {
			close(fFD);
			fFD = -1;
		}

		delete[] fSectionHeaderStrings;
		fSectionHeaderStrings = NULL;
	}

	void SetTo(const char* fileName)
	{
		Unset();

		// open the file
		fFD = open(fileName, O_RDWR);
		if (fFD < 0)
			throw Exception("Failed to open \"%s\"", fileName);

		// get the file size
		fFileSize = FileSize();
		if (fFileSize < 0)
			throw Exception("Failed to get the file size.");

		// Read identification information
		unsigned char ident[EI_NIDENT];
		Read(0, ident, sizeof(ident), "Failed to read ELF identification.");
		if (memcmp(ident, kELFFileMagic, sizeof(kELFFileMagic)) != 0)
			throw Exception("Not a valid ELF file.");
		fELFClass = ident[EI_CLASS];

		if (fELFClass == ELFCLASS64)
			_ParseELFHeader<Elf64_Ehdr, Elf64_Shdr>();
		else
			_ParseELFHeader<Elf32_Ehdr, Elf32_Shdr>();
	}

	bool FindSectionByName(const char* name, SectionInfo& foundInfo)
	{
		// can't find the section by name without section names
		if (!fSectionHeaderStrings)
			return false;

		// iterate through the section headers
		for (size_t i = 0; i < fSectionHeaderCount; i++) {
			SectionInfo info;

			bool result;
			if (fELFClass == ELFCLASS64)
				result = _ReadSectionHeader<Elf64_Shdr>(i, info);
			else
				result = _ReadSectionHeader<Elf32_Shdr>(i, info);

			if (result) {
//printf("section %3d: offset: %7d, size: %7d, name: %s\n", i, info.offset, info.size, info.name);
				if (strcmp(info.name, name) == 0) {
					foundInfo = info;
					return true;
				}
			}
		}

		return false;
	}

	void Read(off_t position, void* buffer, size_t size,
		const char *errorMessage = NULL)
	{
		if (lseek(fFD, position, SEEK_SET) < 0)
			throw Exception(errorMessage);

		ssize_t bytesRead = read(fFD, buffer, size);
		if (bytesRead < 0)
			throw Exception(errorMessage);

		if ((size_t)bytesRead != size) {
			if (errorMessage) {
				throw Exception("%s Read too few bytes (%d/%d).",
					errorMessage, (int)bytesRead, (int)size);
			} else {
				throw Exception("Read too few bytes (%ld/%lu).",
					(int)bytesRead, (int)size);
			}
		}
	}

	void Write(off_t position, const void* buffer, size_t size,
		const char *errorMessage = NULL)
	{
		if (lseek(fFD, position, SEEK_SET) < 0)
			throw Exception(errorMessage);

		ssize_t bytesWritten = write(fFD, buffer, size);
		if (bytesWritten < 0)
			throw Exception(errorMessage);

		if ((size_t)bytesWritten != size) {
			if (errorMessage) {
				throw Exception("%s Wrote too few bytes (%d/%d).",
					errorMessage, (int)bytesWritten, (int)size);
			} else {
				throw Exception("Wrote too few bytes (%ld/%lu).",
					(int)bytesWritten, (int)size);
			}
		}
	}

	off_t FileSize()
	{
		off_t currentPos = lseek(fFD, 0, SEEK_END);
		if (currentPos < 0)
			return -1;

		return lseek(fFD, currentPos, SEEK_SET);
	}

	template<typename Type>
	Type GetValue(Type& value);

private:
	template<typename EhdrType, typename ShdrType>
	void _ParseELFHeader()
	{
		// read ELF header
		EhdrType fileHeader;
		Read(0, &fileHeader, sizeof(EhdrType), "Failed to read ELF header.");

		// check data encoding (endianess)
		switch (fileHeader.e_ident[EI_DATA]) {
			case ELFDATA2LSB:
				fHostEndianess = (htonl(1) != 1);
				break;
			case ELFDATA2MSB:
				fHostEndianess = (htonl(1) == 1);
				break;
			default:
			case ELFDATANONE:
				throw Exception(EIO, "Unsupported ELF data encoding.");
				break;
		}

		// get the header values
		fELFHeaderSize	= GetValue(fileHeader.e_ehsize);
		fSectionHeaderTableOffset = GetValue(fileHeader.e_shoff);
		fSectionHeaderSize	= GetValue(fileHeader.e_shentsize);
		fSectionHeaderCount = GetValue(fileHeader.e_shnum);
		bool hasSectionHeaderTable = (fSectionHeaderTableOffset != 0);

		// check the sanity of the header values
		// ELF header size
		if (fELFHeaderSize < sizeof(EhdrType)) {
			throw Exception(EIO,
				"Invalid ELF header: invalid ELF header size: %lu.",
				fELFHeaderSize);
		}

		// section header table offset and entry count/size
		if (hasSectionHeaderTable) {
			if (fSectionHeaderTableOffset < (off_t)fELFHeaderSize
				|| fSectionHeaderTableOffset > fFileSize) {
				throw Exception(EIO, "Invalid ELF header: invalid section "
								"header table offset: %llu.",
								fSectionHeaderTableOffset);
			}
			size_t sectionHeaderTableSize
				= fSectionHeaderSize * fSectionHeaderCount;
			if (fSectionHeaderSize < (off_t)sizeof(ShdrType)
				|| fSectionHeaderTableOffset + (off_t)sectionHeaderTableSize
					> fFileSize) {
				throw Exception(EIO, "Invalid ELF header: section header "
								"table exceeds file: %llu.",
								fSectionHeaderTableOffset
									+ sectionHeaderTableSize);
			}


			// load section header string section
			uint16_t sectionHeaderStringSectionIndex
				= GetValue(fileHeader.e_shstrndx);
			if (sectionHeaderStringSectionIndex != SHN_UNDEF) {
				if (sectionHeaderStringSectionIndex >= fSectionHeaderCount) {
					throw Exception(EIO, "Invalid ELF header: invalid section "
									"header string section index: %u.",
									sectionHeaderStringSectionIndex);
				}

				// get the section info
				SectionInfo info;
				if (_ReadSectionHeader<ShdrType>(sectionHeaderStringSectionIndex,
						info)) {
					fSectionHeaderStrings = new char[info.size + 1];
					Read(info.offset, fSectionHeaderStrings, info.size,
						"Failed to read section header string section.");
					fSectionHeaderStringsLength = info.size;
					// null-terminate to be on the safe side
					fSectionHeaderStrings[info.size] = '\0';
				}
			}
		}
	}

	template<typename ShdrType>
	bool _ReadSectionHeader(int index, SectionInfo& info)
	{
		off_t shOffset = fSectionHeaderTableOffset
			+ index * fSectionHeaderSize;
		ShdrType sectionHeader;
		Read(shOffset, &sectionHeader, sizeof(ShdrType),
			"Failed to read ELF section header.");

		// get the header values
		uint32_t type		= GetValue(sectionHeader.sh_type);
		off_t offset		= GetValue(sectionHeader.sh_offset);
		size_t size			= GetValue(sectionHeader.sh_size);
		uint32_t nameIndex	= GetValue(sectionHeader.sh_name);

		// check the values
		// SHT_NULL marks the header unused,
		if (type == SHT_NULL)
			return false;

		// SHT_NOBITS sections take no space in the file
		if (type != SHT_NOBITS) {
			if (offset < (off_t)fELFHeaderSize || offset > fFileSize) {
				throw Exception(EIO, "Invalid ELF section header: "
								"invalid section offset: %llu.", offset);
			}
			off_t sectionEnd = offset + size;
			if (sectionEnd > fFileSize) {
				throw Exception(EIO, "Invalid ELF section header: "
								"section exceeds file: %llu.", sectionEnd);
			}
		}

		// get name, if we have a string section
		if (fSectionHeaderStrings) {
			if (nameIndex >= (uint32_t)fSectionHeaderStringsLength) {
				throw Exception(EIO, "Invalid ELF section header: "
								"invalid name index: %lu.", nameIndex);
			}
			info.name = fSectionHeaderStrings + nameIndex;
		} else {
			info.name = "";
		}

		info.type = type;
		info.offset = offset;
		info.size = size;


		return true;
	}

	// _SwapUInt16
	static inline uint16_t _SwapUInt16(uint16_t value)
	{
		return ((value & 0xff) << 8) | (value >> 8);
	}

	// _SwapUInt32
	static inline uint32_t _SwapUInt32(uint32_t value)
	{
		return ((uint32_t)_SwapUInt16(value & 0xffff) << 16)
			| _SwapUInt16(uint16_t(value >> 16));
	}

	// _SwapUInt64
	static inline uint64_t _SwapUInt64(uint64_t value)
	{
		return ((uint64_t)_SwapUInt32(value & 0xffffffff) << 32)
			| _SwapUInt32(uint32_t(value >> 32));
	}

private:
	int			fFD;
	uint8_t		fELFClass;
	bool		fHostEndianess;
	off_t		fFileSize;
	size_t		fELFHeaderSize;
	off_t		fSectionHeaderTableOffset;
	size_t		fSectionHeaderSize;
	size_t		fSectionHeaderCount;
	char*		fSectionHeaderStrings;
	uint32_t	fSectionHeaderStringsLength;
};


template<>
int16_t ELFObject::GetValue(int16_t& value)
{
	return (fHostEndianess ? value : _SwapUInt16(value));
}


template<>
uint16_t ELFObject::GetValue(uint16_t& value)
{
	return (fHostEndianess ? value : _SwapUInt16(value));
}


template<>
int32_t ELFObject::GetValue(int32_t& value)
{
	return (fHostEndianess ? value : _SwapUInt32(value));
}


template<>
uint32_t ELFObject::GetValue(uint32_t& value)
{
	return (fHostEndianess ? value : _SwapUInt32(value));
}


template<>
int64_t ELFObject::GetValue(int64_t& value)
{
	return (fHostEndianess ? value : _SwapUInt64(value));
}


template<>
uint64_t ELFObject::GetValue(uint64_t& value)
{
	return (fHostEndianess ? value : _SwapUInt64(value));
}


// main
int
main(int argc, const char* const* argv)
{
	sArgc = argc;
	sArgv = argv;

	if (argc < 3)
		print_usage_and_exit(true);

	// parameters
	const char* fileName = argv[1];
	const char* revisionString = argv[2];

	try {
		ELFObject elfObject;
		elfObject.SetTo(fileName);

		// find haiku revision section
		SectionInfo info;
		if (!elfObject.FindSectionByName("_haiku_revision", info)) {
			fprintf(stderr, "haiku revision section not found\n");
			exit(1);
		}

		// write revision string to section
		elfObject.Write(info.offset, revisionString,
			min((size_t)SYSTEM_REVISION_LENGTH, strlen(revisionString) + 1),
			"Failed to write revision.");

	} catch (Exception exception) {
		if (exception.Description() == "") {
			fprintf(stderr, "%s\n", strerror(exception.Error()));
		} else {
			fprintf(stderr, "%s: %s\n", exception.Description().c_str(),
				strerror(exception.Error()));
		}
		exit(1);
	}

	return 0;
}
