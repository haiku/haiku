/*
 * Copyright (c) 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageInfo.h"

#include <Alert.h>
#include <ByteOrder.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>
#include <kernel/OS.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfo"

#define RETURN_AND_SET_STATUS(err) fStatus = err; \
	fprintf(stderr, "err at %s():%d: %x\n", __FUNCTION__, __LINE__, err); \
	return fStatus;

const uint32 kSkipOffset = 33;

// Section constants
enum {
	P_GROUPS_SECTION = 0,
	P_PATH_SECTION,
	P_USER_PATH_SECTION,
	P_LICENSE_SECTION
};


// Element constants
enum {
	P_NONE = 0,
	P_FILE,
	P_DIRECTORY,
	P_LINK,
	P_SCRIPT
};


PackageInfo::PackageInfo()
	:
	fStatus(B_NO_INIT),
	fPackageFile(0),
	fDescription(B_TRANSLATE("No package available.")),
	fProfiles(2),
	fHasImage(false)
{
}


PackageInfo::PackageInfo(const entry_ref *ref)
	:
	fStatus(B_NO_INIT),
	fPackageFile(new BFile(ref, B_READ_ONLY)),
	fDescription(B_TRANSLATE("No package selected.")),
	fProfiles(2),
	fHasImage(false)
{
	fStatus = Parse();
}


PackageInfo::~PackageInfo()
{
	pkg_profile *iter = 0;
	while (1) {
		iter = static_cast<pkg_profile *>(fProfiles.RemoveItem((long int)0));
		if (iter == NULL)
			break;

		delete iter;
	}

	PackageItem *file = 0;
	while (true) {
		file = static_cast<PackageItem *>(fFiles.RemoveItem((long int)0));
		if (file == NULL)
			break;

		delete file;
	}

	while (true) {
		file = static_cast<PackageScript *>(fScripts.RemoveItem((long int)0));
		if (file == NULL)
			break;

		delete file;
	}

	delete fPackageFile;
}


status_t
PackageInfo::Parse()
{
	// TODO: Clean up
	if (!fPackageFile || fPackageFile->InitCheck() != B_OK) {
		RETURN_AND_SET_STATUS(B_ERROR);
	}

	// Check for the presence of the first AlB tag - as the 'magic number'.
	// This also ensures that the file header section is present - which
	// is a crucial pkg section
	char buffer[16];
	fPackageFile->Read(buffer, 8);
	if (buffer[0] != 'A' || buffer[1] != 'l' || buffer[2] != 'B'
		|| buffer[3] != 0x1a) {
		RETURN_AND_SET_STATUS(B_ERROR);
	}

	fHasImage = false;

	// Parse all known parts of the given .pkg file

	uint32 i;
	int8 bytesRead;
	off_t actualSize = 0;
	fPackageFile->GetSize(&actualSize);
	uint64 fileSize = 0;

	const char padding[7] = { 0, 0, 0, 0, 0, 0, 0 };

	system_info sysinfo;
	get_system_info(&sysinfo);

	uint64 infoOffset = 0, groupsOffset = 0;
	uint64 length = 0;

	// Parse the file header
	while (true) {
		bytesRead = fPackageFile->Read(buffer, 7);
		if (bytesRead != 7) {
			RETURN_AND_SET_STATUS(B_ERROR);
		}

		if (!memcmp(buffer, "PhIn", 5)) {
		} else if (!memcmp(buffer, "FVer", 5)) {
			// Not used right now
			fPackageFile->Seek(4, SEEK_CUR);
			parser_debug("FVer\n");
		} else if (!memcmp(buffer, "AFla", 5)) {
			// Not used right now TODO: Check what this tag is for
			fPackageFile->Seek(8, SEEK_CUR);
			parser_debug("AFla\n");
		} else if (!memcmp(buffer, "FSiz", 5)) {
			fPackageFile->Read(&fileSize, 8);
			swap_data(B_UINT64_TYPE, &fileSize, sizeof(uint64),
					B_SWAP_BENDIAN_TO_HOST);
			parser_debug("FSiz %llu\n", fileSize);
		} else if (!memcmp(buffer, "COff", 5)) {
			fPackageFile->Read(&infoOffset, 8);
			swap_data(B_UINT64_TYPE, &infoOffset, sizeof(uint64),
					B_SWAP_BENDIAN_TO_HOST);
			parser_debug("COff %llu\n", infoOffset);
		} else if (!memcmp(buffer, "AOff", 5)) {
			fPackageFile->Read(&groupsOffset, 8);
			swap_data(B_UINT64_TYPE, &groupsOffset, sizeof(uint64),
					B_SWAP_BENDIAN_TO_HOST);
			parser_debug("AOff %llu\n", groupsOffset);
		} else if (!memcmp(buffer, padding, 7)) {
			// This means the end of this section - we should move to the
			// groups section.
			if (groupsOffset) {
				fPackageFile->Seek(groupsOffset, SEEK_SET);
			}
			parser_debug("End!\n");
			break;
		} else {
			RETURN_AND_SET_STATUS(B_ERROR);
		}
	}

	fPackageFile->Read(buffer, 7);
	if (memcmp(buffer, "PkgA", 5) || !groupsOffset || !infoOffset) {
		RETURN_AND_SET_STATUS(B_ERROR);
	}

	// Section header identifying constant byte sequences:
	const char groupsMarker[7] = { 0, 0, 0, 1, 0, 0, 4 };
	const char idMarker[7] = { 0, 0, 0, 2, 0, 0, 4 };
	const char pathMarker[7] = { 0, 0, 0, 3, 0, 0, 4 };
	const char upathMarker[7] = { 0, 0, 0, 4, 0, 0, 4 };
	const char licenseMarker[7] = { 0, 0, 0, 18, 0, 0, 4 };
	const char descMarker[7] = { 0, 0, 0, 5, 0, 0, 2 };
	const char helpMarker[7] = { 0, 0, 0, 10, 0, 0, 3 };

	const char splashScreenMarker[7] = { 0, 0, 0, 8, 0, 0, 3 };
	const char disclaimerMarker[7] = { 0, 0, 0, 7, 0, 0, 3 };

	const char nameMarker[7] = { 0, 0, 0, 13, 0, 0, 2 };
	const char versionMarker[7] = { 0, 0, 0, 14, 0, 0, 2 };
	const char devMarker[7] = { 0, 0, 0, 15, 0, 0, 2 };
	const char shortDescMarker[7] = { 0, 0, 0, 17, 0, 0, 2 };

	int8 section = P_GROUPS_SECTION, installDirectoryFlag = 0;

	pkg_profile group;
	BList groups(3), userPaths(3), systemPaths(10);
	bool groupStarted = false;
	parser_debug("Package Info reached!\n");
	// TODO: Maybe checking whether the needed number of bytes are read
	//	everytime would be a good idea

	// Parse the package info section
	while (true) {
		bytesRead = fPackageFile->Read(buffer, 7);
		if (bytesRead != 7) {
			parser_debug("EOF!\n");
			break;
		}

		if (!memcmp(buffer, groupsMarker, 7)) {
			section = P_GROUPS_SECTION;
			parser_debug("Got to Groups section\n");
			continue;
		} else if (!memcmp(buffer, pathMarker, 7)) {
			section = P_PATH_SECTION;
			parser_debug("Got to System Paths\n");
			continue;
		} else if (!memcmp(buffer, upathMarker, 7)) {
			section = P_USER_PATH_SECTION;
			parser_debug("Got to User Paths\n");
			continue;
		} else if (!memcmp(buffer, licenseMarker, 7)) {
			section = P_LICENSE_SECTION;
			parser_debug("Got to License\n");
			continue;
			// After this, non sectioned tags follow
		} else if (!memcmp(buffer, disclaimerMarker, 7)) {
			uint64 length;
			fPackageFile->Read(&length, 8);
			swap_data(B_UINT64_TYPE, &length, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);

			uint64 original;
			if (fPackageFile->Read(&original, 8) != 8) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}
			swap_data(B_UINT64_TYPE, &original, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);

			fPackageFile->Seek(4, SEEK_CUR);

			uint8 *compressed = new uint8[length];
			if (fPackageFile->Read(compressed, length)
					!= static_cast<int64>(length)) {
				delete compressed;
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			uint8 *disclaimer = new uint8[original + 1];
			status_t ret = inflate_data(compressed, length, disclaimer,
				original);
			disclaimer[original] = 0;
			delete compressed;
			if (ret != B_OK) {
				delete disclaimer;
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			fDisclaimer = (char *)disclaimer;
			delete disclaimer;

			continue;
		} else if (!memcmp(buffer, splashScreenMarker, 7)) {
			uint64 length;
			fPackageFile->Read(&length, 8);
			swap_data(B_UINT64_TYPE, &length, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);

			uint64 original;
			if (fPackageFile->Read(&original, 8) != 8) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}
			swap_data(B_UINT64_TYPE, &original, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);

			fPackageFile->Seek(4, SEEK_CUR);

			uint8 *compressed = new uint8[length];
			if (fPackageFile->Read(compressed, length)
					!= static_cast<int64>(length)) {
				delete compressed;
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			fImage.SetSize(original);
			status_t ret = inflate_data(compressed, length,
				static_cast<uint8 *>(const_cast<void *>(fImage.Buffer())),
				original);
			delete compressed;
			if (ret != B_OK) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}
			fHasImage = true;
			continue;
		}

		switch (section) {
			case P_PATH_SECTION:
			{
				if (!memcmp(buffer, "DPat", 5)) {
					parser_debug("DPat\n");
					continue;
				} else if (!memcmp(buffer, "FDst", 5)) {
					parser_debug("FDst - ");
					directory_which dir;
					if (fPackageFile->Read(&dir, 4) != 4) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}
					swap_data(B_UINT32_TYPE, &dir, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);
					BPath *path = new BPath();
					status_t ret = find_directory(dir, path);
					if (ret != B_OK) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}

					parser_debug("%s\n", path->Path());

					systemPaths.AddItem(path);
				} else if (!memcmp(buffer, "PaNa", 5)) {
					parser_debug("PaNa\n");
					if (fPackageFile->Read(&length, 4) != 4) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);
					// Since its a default, system path, we can ignore the path
					// name - all information needed is beside the FDst tag.
					fPackageFile->Seek(length, SEEK_CUR);
				} else if (!memcmp(buffer, padding, 7)) {
					parser_debug("Padding!\n");
					continue;
				} else {
					RETURN_AND_SET_STATUS(B_ERROR);
				}
				break;
			}

			case P_GROUPS_SECTION:
			{
				if (!memcmp(buffer, "IGrp", 5)) {
					// Creata a new group
					groupStarted = true;
					group = pkg_profile();
					parser_debug("IGrp\n");
				} else if (!memcmp(buffer, "GrpN", 5)) {
					if (!groupStarted) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}

					parser_debug("GrpN\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *name = new char[length + 1];
					fPackageFile->Read(name, length);
					name[length] = 0;
					group.name = name;
					delete name;
				} else if (!memcmp(buffer, "GrpD", 5)) {
					if (!groupStarted) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}

					parser_debug("GrpD\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *desc = new char[length + 1];
					fPackageFile->Read(desc, length);
					desc[length] = 0;
					group.description = desc;
					delete desc;
				} else if (!memcmp(buffer, "GrHt", 5)) {
					if (!groupStarted) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}

					parser_debug("GrHt\n");
					// For now, we don't need group help
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);
					fPackageFile->Seek(length, SEEK_CUR);
				} else if (!memcmp(buffer, padding, 5)) {
					if (!groupStarted) {
						parser_debug("No group - padding!\n");
						continue;
					}

					fProfiles.AddItem(new pkg_profile(group));
					parser_debug("Group added: %s %s\n", group.name.String(),
						group.description.String());

					groupStarted = false;
				} else if (!memcmp(buffer, "GrId", 5)) {
					uint32 id;
					fPackageFile->Read(&id, 4);
					swap_data(B_UINT32_TYPE, &id, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					parser_debug("GrId\n");

					if (id == 0xffffffff)
						groups.AddItem(NULL);
					else
						groups.AddItem(fProfiles.ItemAt(id));
				} else if (!memcmp(buffer, idMarker, 7)
					|| !memcmp(buffer, groupsMarker, 7)) {
					parser_debug("Marker, jumping!\n");
					continue;
				} else {
					RETURN_AND_SET_STATUS(B_ERROR);
				}
				break;
			}

			case P_LICENSE_SECTION:
			{
				if (!memcmp(buffer, "Lic?", 5)) {
					parser_debug("Lic?\n");
					// This tag informs whether a license is present in the
					// package or not. Since we don't care about licenses right
					// now, just skip this section
					fPackageFile->Seek(4, SEEK_CUR);
				} else if (!memcmp(buffer, "LicP", 5)) {
					parser_debug("LicP\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					fPackageFile->Seek(length, SEEK_CUR);
				} else if (!memcmp(buffer, padding, 7)) {
					continue;
				} else if (!memcmp(buffer, descMarker, 7)) {
					parser_debug("Description text reached\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *description = new char[length + 1];
					fPackageFile->Read(description, length);
					description[length] = 0;
					fDescription = description;

					// Truncate all leading newlines
					for (i = 0; i < length; i++) {
						if (fDescription[i] != '\n')
							break;
					}
					fDescription.Remove(0, i);

					delete description;
					parser_debug("Description text reached\n");

					// After this, there's a known size sequence of bytes, which
					// meaning is yet to be determined.

					// One is already known. The byte (or just its least
					// significant bit) at offset 21 from the description text
					// is responsible for the install folder existence
					// information. If it is 0, there is no install folder, if
					// it is 1 (or the least significant bit is set) it means
					// we should install all 0xffffffff files/directories to
					// the first directory existing in the package
					fPackageFile->Seek(21, SEEK_CUR);
					if (fPackageFile->Read(&installDirectoryFlag, 1) != 1) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}

					fPackageFile->Seek(11, SEEK_CUR);
				} else if (!memcmp(buffer, nameMarker, 7)) {
					parser_debug("Package name reached\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *name = new char[length + 1];
					fPackageFile->Read(name, length);
					name[length] = 0;
					fName = name;
					delete name;
				} else if (!memcmp(buffer, versionMarker, 7)) {
					parser_debug("Package version reached\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *version = new char[length + 1];
					fPackageFile->Read(version, length);
					version[length] = 0;
					fVersion = version;
					delete version;
				} else if (!memcmp(buffer, devMarker, 7)) {
					parser_debug("Package developer reached\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *dev = new char[length + 1];
					fPackageFile->Read(dev, length);
					dev[length] = 0;
					fDeveloper = dev;
					delete dev;
				} else if (!memcmp(buffer, shortDescMarker, 7)) {
					parser_debug("Package short description reached\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *desc = new char[length + 1];
					fPackageFile->Read(desc, length);
					desc[length] = 0;
					fShortDesc = desc;
					delete desc;
				} else if (!memcmp(buffer, helpMarker, 7)) {
					// The help text is a stored in deflated state, preceded by a 64 bit
					// compressed size, 64 bit inflated size and a 32 bit integer
					// Since there was no discussion whether we need this help text,
					// it will be skipped
					parser_debug("Help text reached\n");
					//uint64 length64;
					fPackageFile->Read(&length, 8);
					swap_data(B_UINT64_TYPE, &length, sizeof(uint64),
						B_SWAP_BENDIAN_TO_HOST);

					fPackageFile->Seek(12 + length, SEEK_CUR);
				}
				break;
			}

			case P_USER_PATH_SECTION:
			{
				if (!memcmp(buffer, "DPat", 5)) {
					parser_debug("DPat\n");
					continue;
				} else if (!memcmp(buffer, "DQue", 5)) {
					parser_debug("DQue\n");
					continue;
				} else if (!memcmp(buffer, "DQTi", 5)) {
					parser_debug("DQTi\n");
					uint32 length;
					if (fPackageFile->Read(&length, 4) != 4) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);
					char *ti = new char[length + 1];
					fPackageFile->Read(ti, length);
					ti[length] = 0;
					parser_debug("DQTi - %s\n", ti);
					delete ti;
				} else if (!memcmp(buffer, "DQSz", 5)) {
					parser_debug("DQSz\n");
					uint64 size;
					if (fPackageFile->Read(&size, 8) != 8) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}
					swap_data(B_UINT64_TYPE, &size, sizeof(uint64),
						B_SWAP_BENDIAN_TO_HOST);
					parser_debug("DQSz - %Ld\n", size);
				} else if (!memcmp(buffer, "DQMi", 5)) {
					// TODO actually check if the query finds a file with
					// size found previously
					parser_debug("DQMi\n");
					uint32 length;
					if (fPackageFile->Read(&length, 4) != 4) {
						RETURN_AND_SET_STATUS(B_ERROR);
					}
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);
					char *signature = new char[length + 1];
					fPackageFile->Read(signature, length);
					signature[length] = 0;
					parser_debug("DQMi - %s\n", signature);
					delete signature;
				} else if (!memcmp(buffer, "PaNa", 5)) {
					parser_debug("PaNa\n");
					fPackageFile->Read(&length, 4);
					swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
						B_SWAP_BENDIAN_TO_HOST);

					char *pathname = new char[length + 1];
					fPackageFile->Read(pathname, length);
					pathname[length] = 0;
					BString *path = new BString(pathname);
					if (length > 0 && pathname[length - 1] == '/')
						path->Remove(length - 1, 1);
					userPaths.AddItem(path);
					delete pathname;
				} else if (!memcmp(buffer, padding, 7)) {
					parser_debug("Padding!\n");
					continue;
				} else {
					parser_debug("Unknown user path section %s\n", buffer);
					RETURN_AND_SET_STATUS(B_ERROR);
				}
				break;
			}
		}
	}

	BString nameString, mimeString, signatureString, linkString;
	BString itemPath = "", installDirectory = "";
	uint32 directoryCount = 0;

	uint8 element = P_NONE;
	uint32 itemGroups = 0, path = 0, cust = 0, ctime = 0, mtime = 0;
	uint32 platform = 0xffffffff;
	uint64 offset = 0, size = 0, originalSize = 0, mode = 0;
	uint8 pathType = P_INSTALL_PATH;
	status_t ret;

	fPackageFile->Seek(infoOffset, SEEK_SET);

	// Parse package file data
	while (true) {
		bytesRead = fPackageFile->Read(buffer, 7);
		if (bytesRead != 7) {
			RETURN_AND_SET_STATUS(B_ERROR);
		}

#define INIT_VARS(tag, type) \
		parser_debug(tag "\n"); \
		element = type; \
		mimeString = ""; \
		nameString = ""; \
		linkString = ""; \
		signatureString = ""; \
		itemGroups = 0; \
		ctime = 0; \
		mtime = 0; \
		offset = 0; \
		cust = 0; \
		mode = 0; \
		platform = 0xffffffff; \
		size = 0; \
		originalSize = 0

		if (!memcmp(buffer, "FilI", 5)) {
			INIT_VARS("FilI", P_FILE);
		} else if (!memcmp(buffer, "FldI", 5)) {
			INIT_VARS("FldI", P_DIRECTORY);
		} else if (!memcmp(buffer, "LnkI", 5)) {
			INIT_VARS("LnkI", P_LINK);
		} else if (!memcmp(buffer, "ScrI", 5)) {
			INIT_VARS("ScrI", P_SCRIPT);
		} else if (!memcmp(buffer, "Name", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Name\n");
			fPackageFile->Read(&length, 4);
			swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);

			char *name = new char[length + 1];
			fPackageFile->Read(name, length);
			name[length] = 0;

			nameString = name;
			delete name;
		} else if (!memcmp(buffer, "Grps", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Grps\n");
			fPackageFile->Read(&itemGroups, 4);
			swap_data(B_UINT32_TYPE, &itemGroups, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "Dest", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Dest\n");
			fPackageFile->Read(&path, 4);
			swap_data(B_UINT32_TYPE, &path, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "Cust", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Cust\n");
			fPackageFile->Read(&cust, 4);
			swap_data(B_UINT32_TYPE, &cust, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "Repl", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Repl\n");
			fPackageFile->Seek(4, SEEK_CUR);
			// TODO: Should the replace philosophy depend on this flag? For now
			//	I always leave the decision to the user
		} else if (!memcmp(buffer, "Plat", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Plat\n");
			fPackageFile->Read(&platform, 4);
			swap_data(B_UINT32_TYPE, &platform, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "CTim", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("CTim\n");
			fPackageFile->Read(&ctime, 4);
			swap_data(B_UINT32_TYPE, &ctime, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "MTim", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("MTim\n");
			fPackageFile->Read(&mtime, 4);
			swap_data(B_UINT32_TYPE, &mtime, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "OffT", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("OffT\n");
			fPackageFile->Read(&offset, 8);
			swap_data(B_UINT64_TYPE, &offset, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "Mime", 5)) {
			if (element != P_FILE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			fPackageFile->Read(&length, 4);
			swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);

			char *mime = new char[length + 1];
			fPackageFile->Read(mime, length);
			mime[length] = 0;
			parser_debug("Mime: %s\n", mime);

			mimeString = mime;
			delete mime;
		} else if (!memcmp(buffer, "CmpS", 5)) {
			if (element == P_NONE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("CmpS\n");
			fPackageFile->Read(&size, 8);
			swap_data(B_UINT64_TYPE, &size, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "OrgS", 5)) {
			if (element != P_FILE && element != P_LINK && element != P_SCRIPT) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("OrgS\n");
			fPackageFile->Read(&originalSize, 8);
			swap_data(B_UINT64_TYPE, &originalSize, sizeof(uint64),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "VrsI", 5)) {
			if (element != P_FILE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("VrsI\n");
			fPackageFile->Seek(24, SEEK_CUR);
			// TODO
			// Also, check what those empty 20 bytes mean
		} else if (!memcmp(buffer, "Mode", 5)) {
			if (element != P_FILE && element != P_LINK) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("Mode\n");
			fPackageFile->Read(&mode, 4);
			swap_data(B_UINT32_TYPE, &mode, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);
		} else if (!memcmp(buffer, "FDat", 5)) {
			if (element != P_DIRECTORY) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			parser_debug("FDat\n");
		} else if (!memcmp(buffer, "ASig", 5)) {
			if (element != P_FILE) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			fPackageFile->Read(&length, 4);
			swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);

			char *signature = new char[length + 1];
			fPackageFile->Read(signature, length);
			signature[length] = 0;
			parser_debug("Signature: %s\n", signature);

			signatureString = signature;
			delete signature;
		} else if (!memcmp(buffer, "Link", 5)) {
			if (element != P_LINK) {
				RETURN_AND_SET_STATUS(B_ERROR);
			}

			fPackageFile->Read(&length, 4);
			swap_data(B_UINT32_TYPE, &length, sizeof(uint32),
				B_SWAP_BENDIAN_TO_HOST);

			char *link = new char[length + 1];
			fPackageFile->Read(link, length);
			link[length] = 0;
			parser_debug("Link: %s\n", link);

			linkString = link;
			delete link;
		} else if (!memcmp(buffer, padding, 7)) {
			PackageItem *item = NULL;

			parser_debug("Padding!\n");
			if (platform != 0xffffffff
				&& static_cast<platform_types>(platform)
					!= sysinfo.platform_type) {
				// If the file/directory/item's platform is different than the
				// target platform (or different than the 'any' constant),
				// ignore this file
			} else if (element == P_FILE) {
				if (itemGroups && offset && size) {
					BString dest = "";
					uint8 localType = pathType;

					if (path == 0xfffffffe)
						dest << itemPath << "/" << nameString.String();
					else if (path == 0xffffffff) {
						localType = P_INSTALL_PATH;
						dest = installDirectory;
						dest << nameString;
					} else {
						if (cust) {
							BString *def = static_cast<BString *>(
								userPaths.ItemAt(path));
							if (!def) {
								RETURN_AND_SET_STATUS(B_ERROR);
							}
							if ((*def)[0] == '/')
								localType = P_SYSTEM_PATH;
							else
								localType = P_USER_PATH;

							dest << *def << "/" << nameString;
						} else {
							BPath *def = static_cast<BPath *>(
								systemPaths.ItemAt(path));
							if (!def) {
								RETURN_AND_SET_STATUS(B_ERROR);
							}
							localType = P_SYSTEM_PATH;

							dest << def->Path() << "/" << nameString;
						}
					}

					parser_debug("Adding file: %s!\n", dest.String());

					item = new PackageFile(fPackageFile, dest, localType, ctime,
						mtime, offset, size, originalSize, 0, mimeString,
						signatureString, mode);
				}
			} else if (element == P_DIRECTORY) {
				if (itemGroups) {
					if (installDirectoryFlag != 0) {
						if (installDirectoryFlag < 0) {
							// Normal directory
							if (path == 0xfffffffe) {
								// Install to current directory
								itemPath << "/" << nameString.String();
								directoryCount++;
							} else if (path == 0xffffffff) {
								// Install to install directory
								pathType = P_INSTALL_PATH;
								itemPath = installDirectory;
								itemPath << nameString;
								directoryCount = 1;
							} else {
								// Install to defined directory
								if (cust) {
									BString *def = static_cast<BString *>(
										userPaths.ItemAt(path));
									if (!def) {
										RETURN_AND_SET_STATUS(B_ERROR);
									}
									if ((*def)[0] == '/')
										pathType = P_SYSTEM_PATH;
									else
										pathType = P_USER_PATH;

									itemPath = *def;
								} else {
									BPath *def = static_cast<BPath *>(
										systemPaths.ItemAt(path));
									if (!def) {
										RETURN_AND_SET_STATUS(B_ERROR);
									}
									pathType = P_SYSTEM_PATH;

									itemPath = def->Path();
								}

								itemPath << "/" << nameString;
								directoryCount = 1;
							}
						} else {
							// Install directory
							if (path != 0xffffffff) {
								RETURN_AND_SET_STATUS(B_ERROR);
							}

							installDirectory = nameString;
							installDirectory << "/";
							pathType = P_INSTALL_PATH;
							itemPath = nameString;

							installDirectoryFlag = -1;
						}

						parser_debug("Adding the directory %s!\n",
							itemPath.String());

						item = new PackageDirectory(fPackageFile, itemPath,
							pathType, ctime, mtime, offset, size);
					} else
						installDirectoryFlag = -1;
				}
			} else if (element == P_LINK) {
				if (itemGroups && linkString.Length()) {
					BString dest = "";
					uint8 localType = pathType;

					if (path == 0xfffffffe)
						dest << itemPath << "/" << nameString.String();
					else if (path == 0xffffffff) {
						localType = P_INSTALL_PATH;
						dest = installDirectory;
						dest << nameString;
					} else {
						if (cust) {
							BString *def = static_cast<BString *>(
								userPaths.ItemAt(path));
							if (!def) {
								RETURN_AND_SET_STATUS(B_ERROR);
							}
							if ((*def)[0] == '/')
								localType = P_SYSTEM_PATH;
							else
								localType = P_USER_PATH;

							dest << *def << "/" << nameString;
						} else {
							BPath *def = static_cast<BPath *>(systemPaths.ItemAt(path));
							if (!def) {
								RETURN_AND_SET_STATUS(B_ERROR);
							}
							localType = P_SYSTEM_PATH;

							dest << def->Path() << "/" << nameString;
						}
					}

					parser_debug("Adding link: %s! (type %s)\n", dest.String(),
						pathType == P_SYSTEM_PATH
							? "System" : localType == P_INSTALL_PATH
							? "Install" : "User");

					item = new PackageLink(fPackageFile, dest, linkString,
						localType, ctime, mtime, mode, offset, size);
				}
			} else if (element == P_SCRIPT) {
				fScripts.AddItem(new PackageScript(fPackageFile, offset, size,
					originalSize));
			} else {
				// If the directory tree count is equal to zero, this means all
				// directory trees have been closed and a padding sequence means the
				// end of the section
				if (directoryCount == 0)
					break;
				ret = itemPath.FindLast('/');
				if (ret == B_ERROR) {
					itemPath = "";
				}
				else {
					itemPath.Truncate(ret);
				}
				directoryCount--;
			}

			if (item) {
				_AddItem(item, originalSize, itemGroups, path, cust);
			}

			element = P_NONE;
		} else if (!memcmp(buffer, "PkgA", 5)) {
			parser_debug("PkgA\n");
			break;
		} else if (!memcmp(buffer, "PtcI", 5)) {
			parser_debug("PtcI\n");
			break;
		} else {
			fprintf(stderr, "Unknown file tag %s\n", buffer);
			RETURN_AND_SET_STATUS(B_ERROR);
		}
	}

	if (static_cast<uint64>(actualSize) != fileSize) {
		// Inform the user of a possible error
		int32 selection;
		BAlert *warning = new BAlert("filesize_wrong",
			B_TRANSLATE("There seems to be a file size mismatch in the "
				"package file. The package might be corrupted or have been "
				"modified after its creation. Do you still wish to continue?"),
				B_TRANSLATE("Continue"),
				B_TRANSLATE("Abort"), NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		warning->SetShortcut(1, B_ESCAPE);
		selection = warning->Go();

		if (selection == 1) {
			RETURN_AND_SET_STATUS(B_ERROR);
		}
	}

	if (!groups.IsEmpty())
		fProfiles = groups;

	return B_OK;
}


void
PackageInfo::_AddItem(PackageItem *item, uint64 size, uint32 groups,
	uint32 path, uint32 cust)
{
	// Add the item to all groups it resides in
	uint32 i, n = fProfiles.CountItems(), mask = 1;
	pkg_profile *profile;

	fFiles.AddItem(item);

	for (i = 0;i < n;i++) {
		if (groups & mask) {
			profile = static_cast<pkg_profile *>(fProfiles.ItemAt(i));
			profile->items.AddItem(item);
			profile->space_needed += size;
			// If there is at least one non-predefined destination element
			// in the package, we give the user the ability to select the
			// installation directory.
			// If there are only predefined path files in the package, but
			// such defined by the user, the user will be able to select
			// the destination volume
			if (path == 0xffffffff)
				profile->path_type = P_INSTALL_PATH;
			else if (path < 0xfffffffe &&
					profile->path_type != P_INSTALL_PATH) {
				if (cust) {
					profile->path_type = P_USER_PATH;
				}
			}
		}
		mask = mask << 1;
	}
}

