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

static const uint32_t	kMaxELFHeaderSize	= sizeof(Elf32_Ehdr) + 32;
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
static
void
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
	uint32_t	offset;
	uint32_t	size;
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

		// read ELF header
		Elf32_Ehdr fileHeader;
		Read(0, &fileHeader, sizeof(Elf32_Ehdr), "Failed to read ELF header.");

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
		fELFHeaderSize	= GetUInt16(fileHeader.e_ehsize);
		fSectionHeaderTableOffset = GetUInt32(fileHeader.e_shoff);
		fSectionHeaderSize	= GetUInt16(fileHeader.e_shentsize);
		fSectionHeaderCount = GetUInt16(fileHeader.e_shnum);
		bool hasSectionHeaderTable = (fSectionHeaderTableOffset != 0);

		// check the sanity of the header values
		// ELF header size
		if (fELFHeaderSize < sizeof(Elf32_Ehdr) || fELFHeaderSize > kMaxELFHeaderSize) {
			throw Exception(EIO,
				"Invalid ELF header: invalid ELF header size: %lu.",
				fELFHeaderSize);
		}

		// section header table offset and entry count/size
		uint32_t sectionHeaderTableSize = 0;
		if (hasSectionHeaderTable) {
			if (fSectionHeaderTableOffset < fELFHeaderSize
				|| fSectionHeaderTableOffset > fFileSize) {
				throw Exception(EIO, "Invalid ELF header: invalid section "
								"header table offset: %lu.",
								fSectionHeaderTableOffset);
			}
			sectionHeaderTableSize = fSectionHeaderSize * fSectionHeaderCount;
			if (fSectionHeaderSize < sizeof(Elf32_Shdr)
				|| fSectionHeaderTableOffset + sectionHeaderTableSize
						> fFileSize) {
				throw Exception(EIO, "Invalid ELF header: section header "
								"table exceeds file: %lu.",
								fSectionHeaderTableOffset
									+ sectionHeaderTableSize);
			}


			// load section header string section
			uint16_t sectionHeaderStringSectionIndex
				= GetUInt16(fileHeader.e_shstrndx);
			if (sectionHeaderStringSectionIndex != SHN_UNDEF) {
				if (sectionHeaderStringSectionIndex >= fSectionHeaderCount) {
					throw Exception(EIO, "Invalid ELF header: invalid section "
									"header string section index: %lu.",
									sectionHeaderStringSectionIndex);
				}

				// get the section info
				SectionInfo info;
				if (_ReadSectionHeader(sectionHeaderStringSectionIndex,
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

	bool FindSectionByName(const char* name, SectionInfo& foundInfo)
	{
		// can't find the section by name without section names
		if (!fSectionHeaderStrings)
			return false;

		// iterate through the section headers
		for (int32_t i = 0; i < (int32_t)fSectionHeaderCount; i++) {
			SectionInfo info;
			if (_ReadSectionHeader(i, info)) {
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

	// GetInt16
	int16_t GetInt16(int16_t value)
	{
		return (fHostEndianess ? value : _SwapUInt16(value));
	}

	// GetUInt16
	uint16_t GetUInt16(uint16_t value)
	{
		return (fHostEndianess ? value : _SwapUInt16(value));
	}

	// GetInt32
	int32_t GetInt32(int32_t value)
	{
		return (fHostEndianess ? value : _SwapUInt32(value));
	}

	// GetUInt32
	uint32_t GetUInt32(uint32_t value)
	{
		return (fHostEndianess ? value : _SwapUInt32(value));
	}

private:
	bool _ReadSectionHeader(int index, SectionInfo& info)
	{
		uint32_t shOffset = fSectionHeaderTableOffset
			+ index * fSectionHeaderSize;
		Elf32_Shdr sectionHeader;
		Read(shOffset, &sectionHeader, sizeof(Elf32_Shdr),
			"Failed to read ELF section header.");

		// get the header values
		uint32_t type		= GetUInt32(sectionHeader.sh_type);
		uint32_t offset		= GetUInt32(sectionHeader.sh_offset);
		uint32_t size		= GetUInt32(sectionHeader.sh_size);
		uint32_t nameIndex	= GetUInt32(sectionHeader.sh_name);

		// check the values
		// SHT_NULL marks the header unused,
		if (type == SHT_NULL)
			return false;

		// SHT_NOBITS sections take no space in the file
		if (type != SHT_NOBITS) {
			if (offset < fELFHeaderSize || offset > fFileSize) {
				throw Exception(EIO, "Invalid ELF section header: "
								"invalid section offset: %lu.", offset);
			}
			uint32_t sectionEnd = offset + size;
			if (sectionEnd > fFileSize) {
				throw Exception(EIO, "Invalid ELF section header: "
								"section exceeds file: %lu.", sectionEnd);
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

private:
	int			fFD;
	bool		fHostEndianess;
	off_t		fFileSize;
	uint32_t	fELFHeaderSize;
	uint32_t	fSectionHeaderTableOffset;
	uint32_t	fSectionHeaderSize;
	uint32_t	fSectionHeaderCount;
	char*		fSectionHeaderStrings;
	int			fSectionHeaderStringsLength;
};


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
