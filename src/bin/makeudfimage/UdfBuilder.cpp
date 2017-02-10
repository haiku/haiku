//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file UdfBuilder.cpp

	Main UDF image building class implementation.
*/

#include "UdfBuilder.h"

#include <Directory.h>
#include <Entry.h>
#include <Node.h>
#include <OS.h>
#include <Path.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

#include "DString.h"
#include "ExtentStream.h"
#include "MemoryChunk.h"
#include "UdfDebug.h"
#include "Utils.h"

using Udf::bool_to_string;
using Udf::check_size_error;

//! Application identifier entity_id
static const Udf::entity_id kApplicationId(0, "*OpenBeOS makeudfimage");

static const Udf::logical_block_address kNullLogicalBlock(0, 0);
static const Udf::extent_address kNullExtent(0, 0);
static const Udf::long_address kNullAddress(0, 0, 0, 0);

/*! \brief Returns the number of the block in which the byte offset specified
	by \a pos resides in the data space specified by the extents in \a dataSpace.
	
	Used to figure out the value for Udf::file_id_descriptor::tag::location fields.
	
	\param block Output parameter into which the block number of interest is stored.
*/
static
status_t
block_for_offset(off_t pos, std::list<Udf::long_address> &dataSpace, uint32 blockSize,
                 uint32 &block)
{
	status_t error = pos >= 0 ? B_OK : B_BAD_VALUE;
	if (!error) {
		off_t streamPos = 0;
		for (std::list<Udf::long_address>::const_iterator i = dataSpace.begin();
		       i != dataSpace.end();
		         i++)
		{
			if (streamPos <= pos && pos < streamPos+i->length()) {
				// Found it
				off_t difference = pos - streamPos;
				block = i->block() + difference / blockSize;
				return B_OK;
			} else {
				streamPos += i->length();
			}
		}
		// Didn't find it, so pos is past the end of the data space
		error = B_ERROR;
	}
	return error;
}

/*! \brief Creates a new UdfBuilder object.

	\param udfRevision The UDF revision to write, formatted as in the UDF
	       domain id suffix, i.e. UDF 2.50 is represented by 0x0250.
*/
UdfBuilder::UdfBuilder(const char *outputFile, uint32 blockSize, bool doUdf,
                       const char *udfVolumeName, uint16 udfRevision, bool doIso, 
                       const char *isoVolumeName, const char *rootDirectory,
                       const ProgressListener &listener, bool truncate)
	: fInitStatus(B_NO_INIT)
	, fOutputFile(outputFile, B_READ_WRITE | B_CREATE_FILE | (truncate ? B_ERASE_FILE : 0))
	, fOutputFilename(outputFile)
	, fBlockSize(blockSize)
	, fBlockShift(0)
	, fDoUdf(doUdf)
	, fUdfVolumeName(udfVolumeName)
	, fUdfRevision(udfRevision)
	, fUdfDescriptorVersion(udfRevision <= 0x0150 ? 2 : 3)
	, fUdfDomainId(udfRevision == 0x0150 ? Udf::kDomainId150 : Udf::kDomainId201)
	, fDoIso(doIso)
	, fIsoVolumeName(isoVolumeName)
	, fRootDirectory(rootDirectory ? rootDirectory : "")
	, fRootDirectoryName(rootDirectory)
	, fListener(listener)
	, fAllocator(blockSize)
	, fPartitionAllocator(0, 257, fAllocator)
	, fStatistics()
	, fBuildTime(0)		// set at start of Build()
	, fBuildTimeStamp()	// ditto
	, fNextUniqueId(16)	// Starts at 16 thanks to MacOS... See UDF-2.50 3.2.1
	, f32BitIdsNoLongerUnique(false) // Set to true once fNextUniqueId requires > 32bits
{
	DEBUG_INIT_ETC("UdfBuilder", ("blockSize: %ld, doUdf: %s, doIso: %s",
	               blockSize, bool_to_string(doUdf), bool_to_string(doIso)));

	// Check the output file
	status_t error = _OutputFile().InitCheck();
	if (error) {
		_PrintError("Error opening output file: 0x%lx, `%s'", error,
		            strerror(error));
	}
	// Check the allocator
	if (!error) {
		error = _Allocator().InitCheck();
		if (error) {
			_PrintError("Error creating block allocator: 0x%lx, `%s'", error,
			            strerror(error));
		}
	}
	// Check the block size
	if (!error) {
		error = Udf::get_block_shift(_BlockSize(), fBlockShift);
		if (!error)
			error = _BlockSize() >= 512 ? B_OK : B_BAD_VALUE;
		if (error)
			_PrintError("Invalid block size: %ld", blockSize);
	}
	// Check that at least one type of filesystem has
	// been requested
	if (!error) {
		error = _DoUdf() || _DoIso() ? B_OK : B_BAD_VALUE;
		if (error)
			_PrintError("No filesystems requested.");
	}			
	// Check the volume names
	if (!error) {
		if (_UdfVolumeName().Utf8Length() == 0)
			_UdfVolumeName().SetTo("(Unnamed UDF Volume)");
		if (_IsoVolumeName().Utf8Length() == 0)
			_IsoVolumeName().SetTo("UNNAMED_ISO");
		if (_DoUdf()) {
			error = _UdfVolumeName().Cs0Length() <= 128 ? B_OK : B_ERROR;
			if (error) {
				_PrintError("Udf volume name too long (%ld bytes, max "
				            "length is 128 bytes.",
				            _UdfVolumeName().Cs0Length());
			}
		}
		if (!error && _DoIso()) {		
			error = _IsoVolumeName().Utf8Length() <= 32 ? B_OK : B_ERROR;
			// ToDo: Should also check for illegal characters
			if (error) {
				_PrintError("Iso volume name too long (%ld bytes, max "
				            "length is 32 bytes.",
				            _IsoVolumeName().Cs0Length());
			}
		}
	}
	// Check the udf revision
	if (!error) {
		error = _UdfRevision() == 0x0150 || _UdfRevision() == 0x0201
		        ? B_OK : B_ERROR;
		if (error) {
			_PrintError("Invalid UDF revision 0x%04x", _UdfRevision());
		}
	}
	// Check the root directory
	if (!error) {
		error = _RootDirectory().InitCheck();
		if (error) {
			_PrintError("Error initializing root directory entry: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}
	
	if (!error) {
		fInitStatus = B_OK;
	}
}

status_t
UdfBuilder::InitCheck() const
{
	return fInitStatus;
}

/*! \brief Builds the disc image.
*/
status_t
UdfBuilder::Build()
{
	DEBUG_INIT("UdfBuilder");
	status_t error = InitCheck();
	if (error)
		RETURN(error);
		
	// Note the time at which we're starting
	fStatistics.Reset();
	_SetBuildTime(_Stats().StartTime());

	// Udf variables
	uint16 partitionNumber = 0;
	Udf::anchor_volume_descriptor anchor256;
	Udf::anchor_volume_descriptor anchorN;
	Udf::extent_address primaryVdsExtent;
	Udf::extent_address reserveVdsExtent;
	Udf::primary_volume_descriptor primary;
	Udf::partition_descriptor partition;
	Udf::unallocated_space_descriptor freespace;
	Udf::logical_volume_descriptor logical;
	Udf::implementation_use_descriptor implementationUse;
	Udf::long_address filesetAddress;
	Udf::extent_address filesetExtent;
	Udf::extent_address integrityExtent;
	Udf::file_set_descriptor fileset;
	node_data rootNode;

	// Iso variables
//	Udf::extent_address rootDirentExtent;	
	
		
	_OutputFile().Seek(0, SEEK_SET);
	fListener.OnStart(fRootDirectoryName.c_str(), fOutputFilename.c_str(),
	                  _UdfVolumeName().Utf8(), _UdfRevision());

	_PrintUpdate(VERBOSITY_LOW, "Initializing volume");

	// Reserve the first 32KB and zero them out.
	if (!error) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing reserved area");
		const int reservedAreaSize = 32 * 1024;
		Udf::extent_address extent(0, reservedAreaSize);
		_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for reserved area");
		error = _Allocator().GetExtent(extent);
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             extent.location(), extent.length());
			ssize_t bytes = _OutputFile().Zero(reservedAreaSize);
			error = check_size_error(bytes, reservedAreaSize);
		}
		// Error check
		if (error) {				 
			_PrintError("Error creating reserved area: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}	

	const int vrsBlockSize = 2048;
	
	// Write the iso portion of the vrs
	if (!error && _DoIso()) {
		_PrintUpdate(VERBOSITY_MEDIUM, "iso: Writing primary volume descriptor");
		
		// Error check
		if (error) {				 
			_PrintError("Error writing iso vrs: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}
			
	// Write the udf portion of the vrs
	if (!error && _DoUdf()) {
		Udf::extent_address extent;
		// Bea
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing bea descriptor");
		_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for bea descriptor");
		Udf::volume_structure_descriptor_header bea(0, Udf::kVSDID_BEA, 1);
		error = _Allocator().GetNextExtent(vrsBlockSize, true, extent);
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             extent.location(), extent.length());
			ssize_t bytes = _OutputFile().Write(&bea, sizeof(bea));
			error = check_size_error(bytes, sizeof(bea));
			if (!error) {
				bytes = _OutputFile().Zero(vrsBlockSize-sizeof(bea));
				error = check_size_error(bytes, vrsBlockSize-sizeof(bea));
			}
		}				
		// Nsr
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing nsr descriptor");
		Udf::volume_structure_descriptor_header nsr(0, _UdfRevision() <= 0x0150
		                                            ? Udf::kVSDID_ECMA167_2
		                                            : Udf::kVSDID_ECMA167_3,
		                                            1);
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for nsr descriptor");
			_Allocator().GetNextExtent(vrsBlockSize, true, extent);
		}
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             extent.location(), extent.length());
			ssize_t bytes = _OutputFile().Write(&nsr, sizeof(nsr));
			error = check_size_error(bytes, sizeof(nsr));
			if (!error) {
				bytes = _OutputFile().Zero(vrsBlockSize-sizeof(nsr));
				error = check_size_error(bytes, vrsBlockSize-sizeof(nsr));
			}
		}				
		// Tea
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing tea descriptor");
		Udf::volume_structure_descriptor_header tea(0, Udf::kVSDID_TEA, 1);
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for tea descriptor");
			error = _Allocator().GetNextExtent(vrsBlockSize, true, extent);
		}
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             extent.location(), extent.length());
			ssize_t bytes = _OutputFile().Write(&tea, sizeof(tea));
			error = check_size_error(bytes, sizeof(tea));
			if (!error) {
				bytes = _OutputFile().Zero(vrsBlockSize-sizeof(tea));
				error = check_size_error(bytes, vrsBlockSize-sizeof(tea));
			}
		}				
		// Error check
		if (error) {				 
			_PrintError("Error writing udf vrs: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}
	
	// Write the udf anchor256 and volume descriptor sequences
	if (!error && _DoUdf()) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing anchor256");
		// reserve anchor256
		_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for anchor256");
		error = _Allocator().GetBlock(256);
		if (!error) 
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             256, _BlockSize());
		// reserve primary vds (min length = 16 blocks, which is plenty for us)
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for primary vds");
			error = _Allocator().GetNextExtent(off_t(16) << _BlockShift(), true, primaryVdsExtent);
			if (!error) {
				_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
				             primaryVdsExtent.location(), primaryVdsExtent.length());
				ssize_t bytes = _OutputFile().ZeroAt(off_t(primaryVdsExtent.location()) << _BlockShift(),
				                                   primaryVdsExtent.length());
				error = check_size_error(bytes, primaryVdsExtent.length());
			}
		}		
		// reserve reserve vds. try to grab the 16 blocks preceding block 256. if
		// that fails, just grab any 16. most commercial discs just put the reserve
		// vds immediately following the primary vds, which seems a bit stupid to me,
		// now that I think about it... 
		if (!error) {
			_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for reserve vds");
			reserveVdsExtent.set_location(256-16);
			reserveVdsExtent.set_length(off_t(16) << _BlockShift());
			error = _Allocator().GetExtent(reserveVdsExtent);
			if (error)
				error = _Allocator().GetNextExtent(off_t(16) << _BlockShift(), true, reserveVdsExtent);
			if (!error) {
				_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
				             reserveVdsExtent.location(), reserveVdsExtent.length());
				ssize_t bytes = _OutputFile().ZeroAt(off_t(reserveVdsExtent.location()) << _BlockShift(),
				                                   reserveVdsExtent.length());
				error = check_size_error(bytes, reserveVdsExtent.length());
			}
		}
		// write anchor_256
		if (!error) {
			anchor256.main_vds() = primaryVdsExtent;
			anchor256.reserve_vds() = reserveVdsExtent;
			Udf::descriptor_tag &tag = anchor256.tag();
			tag.set_id(Udf::TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER);
			tag.set_version(_UdfDescriptorVersion());
			tag.set_serial_number(0);
			tag.set_location(256);
			tag.set_checksums(anchor256);
			_OutputFile().Seek(off_t(256) << _BlockShift(), SEEK_SET);
			ssize_t bytes = _OutputFile().Write(&anchor256, sizeof(anchor256));
			error = check_size_error(bytes, sizeof(anchor256));
			if (!error && bytes < ssize_t(_BlockSize())) {
				bytes = _OutputFile().Zero(_BlockSize()-sizeof(anchor256));
				error = check_size_error(bytes, _BlockSize()-sizeof(anchor256));
			}
		}
		uint32 vdsNumber = 0;
		// write primary_vd
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing primary volume descriptor");
			// build primary_vd
			primary.set_vds_number(vdsNumber);
			primary.set_primary_volume_descriptor_number(0);
			uint32 nameLength = _UdfVolumeName().Cs0Length();
			if (nameLength > primary.volume_identifier().size()-1) {
				_PrintWarning("udf: Truncating volume name as stored in primary "
				              "volume descriptor to 31 byte limit. This shouldn't matter, "
				              "as the complete name is %d bytes long, which is short enough "
				              "to fit completely in the logical volume descriptor.",
				              nameLength);
			}
			Udf::DString volumeIdField(_UdfVolumeName(),
			                           primary.volume_identifier().size());
			memcpy(primary.volume_identifier().data, volumeIdField.String(),
			       primary.volume_identifier().size());
			primary.set_volume_sequence_number(1);
			primary.set_max_volume_sequence_number(1);
			primary.set_interchange_level(2);
			primary.set_max_interchange_level(3);
			primary.set_character_set_list(1);
			primary.set_max_character_set_list(1);
			// first 16 chars of volume set id must be unique. first 8 must be
			// a hex representation of a timestamp
			char timestamp[9];
			sprintf(timestamp, "%08lX", _BuildTime());
			std::string volumeSetId(timestamp);
			volumeSetId = volumeSetId + "--------" + "(unnamed volume set)";
			Udf::DString volumeSetIdField(volumeSetId.c_str(),
			                              primary.volume_set_identifier().size());
			memcpy(primary.volume_set_identifier().data, volumeSetIdField.String(),
			       primary.volume_set_identifier().size());
			primary.descriptor_character_set() = Udf::kCs0CharacterSet;
			primary.explanatory_character_set() = Udf::kCs0CharacterSet;
			primary.volume_abstract() = kNullExtent;
			primary.volume_copyright_notice() = kNullExtent;
			primary.application_id() = kApplicationId;
			primary.recording_date_and_time() = _BuildTimeStamp();
			primary.implementation_id() = Udf::kImplementationId;
			memset(primary.implementation_use().data, 0,
			       primary.implementation_use().size());
			primary.set_predecessor_volume_descriptor_sequence_location(0);
			primary.set_flags(0);	// ToDo: maybe 1 is more appropriate?	       
			memset(primary.reserved().data, 0, primary.reserved().size());
			primary.tag().set_id(Udf::TAGID_PRIMARY_VOLUME_DESCRIPTOR);
			primary.tag().set_version(_UdfDescriptorVersion());
			primary.tag().set_serial_number(0);
				// note that the checksums haven't been set yet, since the
				// location is dependent on which sequence (primary or reserve)
				// the descriptor is currently being written to. Thus we have to
				// recalculate the checksums for each sequence.
			DUMP(primary);
			// write primary_vd to primary vds
			primary.tag().set_location(primaryVdsExtent.location()+vdsNumber);
			primary.tag().set_checksums(primary);
			ssize_t bytes = _OutputFile().WriteAt(off_t(primary.tag().location()) << _BlockShift(),
			                              &primary, sizeof(primary));
			error = check_size_error(bytes, sizeof(primary));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(primary.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write primary_vd to reserve vds				                             
			if (!error) {
				primary.tag().set_location(reserveVdsExtent.location()+vdsNumber);
				primary.tag().set_checksums(primary);
				ssize_t bytes = _OutputFile().WriteAt(off_t(primary.tag().location()) << _BlockShift(),
	        			                              &primary, sizeof(primary));
				error = check_size_error(bytes, sizeof(primary));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt(off_t((primary.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}

		// write partition descriptor
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing partition descriptor");
			// build partition descriptor
	 		vdsNumber++;
			partition.set_vds_number(vdsNumber);
			partition.set_partition_flags(1);
			partition.set_partition_number(partitionNumber);
			partition.partition_contents() = _UdfRevision() <= 0x0150
			                                 ? Udf::kPartitionContentsId1xx
			                                 : Udf::kPartitionContentsId2xx;
			memset(partition.partition_contents_use().data, 0,
			       partition.partition_contents_use().size());
			partition.set_access_type(Udf::ACCESS_READ_ONLY);
			partition.set_start(_Allocator().Tail());
			partition.set_length(0);
				// Can't set the length till we've built most of rest of the image,
				// so we'll set it to 0 now and fix it once we know how big
				// the partition really is.
			partition.implementation_id() = Udf::kImplementationId;
			memset(partition.implementation_use().data, 0,
			       partition.implementation_use().size());
			memset(partition.reserved().data, 0,
			       partition.reserved().size());
			partition.tag().set_id(Udf::TAGID_PARTITION_DESCRIPTOR);
			partition.tag().set_version(_UdfDescriptorVersion());
			partition.tag().set_serial_number(0);
				// note that the checksums haven't been set yet, since the
				// location is dependent on which sequence (primary or reserve)
				// the descriptor is currently being written to. Thus we have to
				// recalculate the checksums for each sequence.
			DUMP(partition);
			// write partition descriptor to primary vds
			partition.tag().set_location(primaryVdsExtent.location()+vdsNumber);
			partition.tag().set_checksums(partition);
			ssize_t bytes = _OutputFile().WriteAt(off_t(partition.tag().location()) << _BlockShift(),
			                              &partition, sizeof(partition));
			error = check_size_error(bytes, sizeof(partition));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(partition.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write partition descriptor to reserve vds				                             
			if (!error) {
				partition.tag().set_location(reserveVdsExtent.location()+vdsNumber);
				partition.tag().set_checksums(partition);
				ssize_t bytes = _OutputFile().WriteAt(off_t(partition.tag().location()) << _BlockShift(),
	        			                              &partition, sizeof(partition));
				error = check_size_error(bytes, sizeof(partition));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((off_t(partition.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}

		// write unallocated space descriptor
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing unallocated space descriptor");
			// build freespace descriptor
	 		vdsNumber++;
			freespace.set_vds_number(vdsNumber);
			freespace.set_allocation_descriptor_count(0);
			freespace.tag().set_id(Udf::TAGID_UNALLOCATED_SPACE_DESCRIPTOR);
			freespace.tag().set_version(_UdfDescriptorVersion());
			freespace.tag().set_serial_number(0);
				// note that the checksums haven't been set yet, since the
				// location is dependent on which sequence (primary or reserve)
				// the descriptor is currently being written to. Thus we have to
				// recalculate the checksums for each sequence.
			DUMP(freespace);
			// write freespace descriptor to primary vds
			freespace.tag().set_location(primaryVdsExtent.location()+vdsNumber);
			freespace.tag().set_checksums(freespace);
			ssize_t bytes = _OutputFile().WriteAt(off_t(freespace.tag().location()) << _BlockShift(),
			                              &freespace, sizeof(freespace));
			error = check_size_error(bytes, sizeof(freespace));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(freespace.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write freespace descriptor to reserve vds				                             
			if (!error) {
				freespace.tag().set_location(reserveVdsExtent.location()+vdsNumber);
				freespace.tag().set_checksums(freespace);
				ssize_t bytes = _OutputFile().WriteAt(off_t(freespace.tag().location()) << _BlockShift(),
	        			                              &freespace, sizeof(freespace));
				error = check_size_error(bytes, sizeof(freespace));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((off_t(freespace.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}

		// write logical_vd
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing logical volume descriptor");
			// build logical_vd
	 		vdsNumber++;
	 		logical.set_vds_number(vdsNumber);
	 		logical.character_set() = Udf::kCs0CharacterSet;
	 		error = (_UdfVolumeName().Cs0Length() <=
	 		        logical.logical_volume_identifier().size())
	 		          ? B_OK : B_ERROR;
	 			// We check the length in the constructor, so this should never
	 			// trigger an error, but just to be safe...
	 		if (!error) { 
				Udf::DString volumeIdField(_UdfVolumeName(),
				                           logical.logical_volume_identifier().size());
				memcpy(logical.logical_volume_identifier().data, volumeIdField.String(),
				       logical.logical_volume_identifier().size());
				logical.set_logical_block_size(_BlockSize());
				logical.domain_id() = _UdfDomainId();
				memset(logical.logical_volume_contents_use().data, 0,
				       logical.logical_volume_contents_use().size());			
				// Allocate a block for the file set descriptor
				_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for file set descriptor");
				error = _PartitionAllocator().GetNextExtent(_BlockSize(), true,
				                                            filesetAddress, filesetExtent);
				if (!error) {				                                            
					_PrintUpdate(VERBOSITY_HIGH, "udf: (partition: %d, location: %ld, "
					             "length: %ld) => (location: %ld, length: %ld)",
					             filesetAddress.partition(), filesetAddress.block(),
					             filesetAddress.length(), filesetExtent.location(),
					             filesetExtent.length());
				}	             
			}
			if (!error) {
				logical.file_set_address() = filesetAddress;
				logical.set_map_table_length(sizeof(Udf::physical_partition_map));
				logical.set_partition_map_count(1);
				logical.implementation_id() = Udf::kImplementationId;
				memset(logical.implementation_use().data, 0,
				       logical.implementation_use().size());
				// Allocate a couple of blocks for the integrity sequence
				error = _Allocator().GetNextExtent(_BlockSize()*2, true,
				                                   integrityExtent);				                                   
			}
			if (!error) {
				logical.integrity_sequence_extent() = integrityExtent;
				Udf::physical_partition_map map;
				map.set_type(1);
				map.set_length(6);
				map.set_volume_sequence_number(1);
				map.set_partition_number(partitionNumber);
				memcpy(logical.partition_maps(), &map, sizeof(map));
				logical.tag().set_id(Udf::TAGID_LOGICAL_VOLUME_DESCRIPTOR);
				logical.tag().set_version(_UdfDescriptorVersion());
				logical.tag().set_serial_number(0);
					// note that the checksums haven't been set yet, since the
					// location is dependent on which sequence (primary or reserve)
					// the descriptor is currently being written to. Thus we have to
					// recalculate the checksums for each sequence.
				DUMP(logical);
				// write partition descriptor to primary vds
				uint32 logicalSize = Udf::kLogicalVolumeDescriptorBaseSize + sizeof(map);
				logical.tag().set_location(primaryVdsExtent.location()+vdsNumber);
				logical.tag().set_checksums(logical, logicalSize);
				ssize_t bytes = _OutputFile().WriteAt(off_t(logical.tag().location()) << _BlockShift(),
				                              &logical, logicalSize);
				error = check_size_error(bytes, logicalSize);                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((off_t(logical.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
				// write logical descriptor to reserve vds				                             
				if (!error) {
					logical.tag().set_location(reserveVdsExtent.location()+vdsNumber);
					logical.tag().set_checksums(logical, logicalSize);
					ssize_t bytes = _OutputFile().WriteAt(off_t(logical.tag().location()) << _BlockShift(),
		        			                              &logical, sizeof(logical));
					error = check_size_error(bytes, sizeof(logical));                              
					if (!error && bytes < ssize_t(_BlockSize())) {
						ssize_t bytesLeft = _BlockSize() - bytes;
						bytes = _OutputFile().ZeroAt((off_t(logical.tag().location()) << _BlockShift())
						                             + bytes, bytesLeft);
						error = check_size_error(bytes, bytesLeft);			                             
					}
				}
			}		
		}
		
		// write implementation use descriptor
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing implementation use descriptor");
			// build implementationUse descriptor
	 		vdsNumber++;
			implementationUse.set_vds_number(vdsNumber);
			switch (_UdfRevision()) {
				case 0x0150:
					implementationUse.implementation_id() = Udf::kLogicalVolumeInfoId150;
					break;
				case 0x0201:
					implementationUse.implementation_id() = Udf::kLogicalVolumeInfoId201;
					break;
				default:
					_PrintError("Invalid udf revision: 0x04x", _UdfRevision());
					error = B_ERROR;
			}
			ssize_t bytes = 0;
			if (!error) {					
				Udf::logical_volume_info &info = implementationUse.info();
				info.character_set() = Udf::kCs0CharacterSet;
				Udf::DString logicalVolumeId(_UdfVolumeName(),
				                           	 info.logical_volume_id().size());
				memcpy(info.logical_volume_id().data, logicalVolumeId.String(),
				       info.logical_volume_id().size());
				Udf::DString info1("Logical Volume Info #1",
				                   info.logical_volume_info_1().size());
				memcpy(info.logical_volume_info_1().data, info1.String(),
				       info.logical_volume_info_1().size());
				Udf::DString info2("Logical Volume Info #2",
				                   info.logical_volume_info_2().size());
				memcpy(info.logical_volume_info_2().data, info2.String(),
				       info.logical_volume_info_2().size());
				Udf::DString info3("Logical Volume Info #3",
				                   info.logical_volume_info_3().size());
				memcpy(info.logical_volume_info_3().data, info3.String(),
				       info.logical_volume_info_3().size());
				info.implementation_id() = Udf::kImplementationId;
				memset(info.implementation_use().data, 0, info.implementation_use().size());
				implementationUse.tag().set_id(Udf::TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR);
				implementationUse.tag().set_version(_UdfDescriptorVersion());
				implementationUse.tag().set_serial_number(0);
					// note that the checksums haven't been set yet, since the
					// location is dependent on which sequence (primary or reserve)
					// the descriptor is currently being written to. Thus we have to
					// recalculate the checksums for each sequence.
				DUMP(implementationUse);
				// write implementationUse descriptor to primary vds
				implementationUse.tag().set_location(primaryVdsExtent.location()+vdsNumber);
				implementationUse.tag().set_checksums(implementationUse);
				bytes = _OutputFile().WriteAt(off_t(implementationUse.tag().location()) << _BlockShift(),
				                              &implementationUse, sizeof(implementationUse));
				error = check_size_error(bytes, sizeof(implementationUse));
			}
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(implementationUse.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write implementationUse descriptor to reserve vds				                             
			if (!error) {
				implementationUse.tag().set_location(reserveVdsExtent.location()+vdsNumber);
				implementationUse.tag().set_checksums(implementationUse);
				ssize_t bytes = _OutputFile().WriteAt(off_t(implementationUse.tag().location()) << _BlockShift(),
	        			                              &implementationUse, sizeof(implementationUse));
				error = check_size_error(bytes, sizeof(implementationUse));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((off_t(implementationUse.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}

		// write terminating descriptor
		if (!error) {
	 		vdsNumber++;
			Udf::terminating_descriptor terminator;
			terminator.tag().set_id(Udf::TAGID_TERMINATING_DESCRIPTOR);
			terminator.tag().set_version(_UdfDescriptorVersion());
			terminator.tag().set_serial_number(0);
			terminator.tag().set_location(integrityExtent.location()+1);
			terminator.tag().set_checksums(terminator);
			DUMP(terminator);
			// write terminator to primary vds
			terminator.tag().set_location(primaryVdsExtent.location()+vdsNumber);
			terminator.tag().set_checksums(terminator);
			ssize_t bytes = _OutputFile().WriteAt(off_t(terminator.tag().location()) << _BlockShift(),
			                              &terminator, sizeof(terminator));
			error = check_size_error(bytes, sizeof(terminator));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(terminator.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write terminator to reserve vds				                             
			if (!error) {
				terminator.tag().set_location(reserveVdsExtent.location()+vdsNumber);
				terminator.tag().set_checksums(terminator);
				ssize_t bytes = _OutputFile().WriteAt(off_t(terminator.tag().location()) << _BlockShift(),
	        			                              &terminator, sizeof(terminator));
				error = check_size_error(bytes, sizeof(terminator));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((off_t(terminator.tag().location()) << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}

		// Error check
		if (error) {				 
			_PrintError("Error writing udf vds: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}
	
	// Write the file set descriptor
	if (!error) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing file set descriptor");
		fileset.recording_date_and_time() = _BuildTimeStamp();
		fileset.set_interchange_level(3);
		fileset.set_max_interchange_level(3);
		fileset.set_character_set_list(1);
		fileset.set_max_character_set_list(1);
		fileset.set_file_set_number(0);
		fileset.set_file_set_descriptor_number(0);
		fileset.logical_volume_id_character_set() = Udf::kCs0CharacterSet;
		Udf::DString volumeIdField(_UdfVolumeName(),
		                           fileset.logical_volume_id().size());
		memcpy(fileset.logical_volume_id().data, volumeIdField.String(),
		       fileset.logical_volume_id().size());
		fileset.file_set_id_character_set() = Udf::kCs0CharacterSet;
		Udf::DString filesetIdField(_UdfVolumeName(),
		                           fileset.file_set_id().size());
		memcpy(fileset.file_set_id().data, filesetIdField.String(),
		       fileset.file_set_id().size());
		memset(fileset.copyright_file_id().data, 0,
		       fileset.copyright_file_id().size());
		memset(fileset.abstract_file_id().data, 0,
		       fileset.abstract_file_id().size());
		fileset.root_directory_icb() = kNullAddress;
		fileset.domain_id() = _UdfDomainId();
		fileset.next_extent() = kNullAddress;
		fileset.system_stream_directory_icb() = kNullAddress;
		memset(fileset.reserved().data, 0,
		       fileset.reserved().size());
		fileset.tag().set_id(Udf::TAGID_FILE_SET_DESCRIPTOR);
		fileset.tag().set_version(_UdfDescriptorVersion());
		fileset.tag().set_serial_number(0);
		fileset.tag().set_location(filesetAddress.block());
		fileset.tag().set_checksums(fileset);
		DUMP(fileset);
		// write fsd				                             
		ssize_t bytes = _OutputFile().WriteAt(off_t(filesetExtent.location()) << _BlockShift(),
       			                              &fileset, sizeof(fileset));
		error = check_size_error(bytes, sizeof(fileset));                              
		if (!error && bytes < ssize_t(_BlockSize())) {
			ssize_t bytesLeft = _BlockSize() - bytes;
			bytes = _OutputFile().ZeroAt((off_t(filesetExtent.location()) << _BlockShift())
			                             + bytes, bytesLeft);
			error = check_size_error(bytes, bytesLeft);			                             
		}
	}

	// Build the rest of the image
	if (!error) {
		struct stat rootStats;
		error = _RootDirectory().GetStat(&rootStats);
		if (!error)
			error = _ProcessDirectory(_RootDirectory(), "/", rootStats, rootNode,
			                          kNullAddress, true);		
	}

	if (!error)
		_PrintUpdate(VERBOSITY_LOW, "Finalizing volume");
		
	// Rewrite the fsd with the root dir icb		
	if (!error) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Finalizing file set descriptor");
		fileset.root_directory_icb() = rootNode.icbAddress;
		fileset.tag().set_checksums(fileset);
		DUMP(fileset);
		// write fsd				                             
		ssize_t bytes = _OutputFile().WriteAt(off_t(filesetExtent.location()) << _BlockShift(),
       			                              &fileset, sizeof(fileset));
		error = check_size_error(bytes, sizeof(fileset));                              
		if (!error && bytes < ssize_t(_BlockSize())) {
			ssize_t bytesLeft = _BlockSize() - bytes;
			bytes = _OutputFile().ZeroAt((off_t(filesetExtent.location()) << _BlockShift())
			                             + bytes, bytesLeft);
			error = check_size_error(bytes, bytesLeft);			                             
		}
	}

	// Set the final partition length and rewrite the partition descriptor
	if (!error) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Finalizing partition descriptor");
		partition.set_length(_PartitionAllocator().Length());
		DUMP(partition);
		// write partition descriptor to primary vds
		partition.tag().set_location(primaryVdsExtent.location()+partition.vds_number());
		partition.tag().set_checksums(partition);
		ssize_t bytes = _OutputFile().WriteAt(off_t(partition.tag().location()) << _BlockShift(),
		                              &partition, sizeof(partition));
		error = check_size_error(bytes, sizeof(partition));                              
		if (!error && bytes < ssize_t(_BlockSize())) {
			ssize_t bytesLeft = _BlockSize() - bytes;
			bytes = _OutputFile().ZeroAt((off_t(partition.tag().location()) << _BlockShift())
			                             + bytes, bytesLeft);
			error = check_size_error(bytes, bytesLeft);			                             
		}
		// write partition descriptor to reserve vds				                             
		if (!error) {
			partition.tag().set_location(reserveVdsExtent.location()+partition.vds_number());
			partition.tag().set_checksums(partition);
			ssize_t bytes = _OutputFile().WriteAt(off_t(partition.tag().location()) << _BlockShift(),
        			                              &partition, sizeof(partition));
			error = check_size_error(bytes, sizeof(partition));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((off_t(partition.tag().location()) << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
		}
		// Error check
		if (error) {				 
			_PrintError("Error writing udf vds: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}
	
	// Write the integrity sequence
	if (!error && _DoUdf()) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing logical volume integrity sequence");
		Udf::MemoryChunk chunk(_BlockSize());
		error = chunk.InitCheck();
		// write closed integrity descriptor
		if (!error) {
			memset(chunk.Data(), 0, _BlockSize());
			Udf::logical_volume_integrity_descriptor *lvid =
				reinterpret_cast<Udf::logical_volume_integrity_descriptor*>(chunk.Data());
			lvid->recording_time() = Udf::timestamp(real_time_clock());
				// recording time must be later than all file access times
			lvid->set_integrity_type(Udf::INTEGRITY_CLOSED);
			lvid->next_integrity_extent() = kNullExtent;
			memset(lvid->logical_volume_contents_use().data, 0,
			       lvid->logical_volume_contents_use().size());
			lvid->set_next_unique_id(_NextUniqueId());
			lvid->set_partition_count(1);
			lvid->set_implementation_use_length(
				Udf::logical_volume_integrity_descriptor::minimum_implementation_use_length);
			lvid->free_space_table()[0] = 0;
			lvid->size_table()[0] = _PartitionAllocator().Length();
			lvid->implementation_id() = Udf::kImplementationId;
			lvid->set_file_count(_Stats().Files());
			lvid->set_directory_count(_Stats().Directories());
			lvid->set_minimum_udf_read_revision(_UdfRevision());
			lvid->set_minimum_udf_write_revision(_UdfRevision());
			lvid->set_maximum_udf_write_revision(_UdfRevision());
			lvid->tag().set_id(Udf::TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR);
			lvid->tag().set_version(_UdfDescriptorVersion());
			lvid->tag().set_serial_number(0);
			lvid->tag().set_location(integrityExtent.location());
			lvid->tag().set_checksums(*lvid, lvid->descriptor_size());
			PDUMP(lvid);
			// write lvid				                             
			ssize_t bytes = _OutputFile().WriteAt(off_t(integrityExtent.location()) << _BlockShift(),
	       			                              lvid, _BlockSize());
			error = check_size_error(bytes, _BlockSize());
		}
		// write terminating descriptor
		if (!error) {
			memset(chunk.Data(), 0, _BlockSize());
			Udf::terminating_descriptor *terminator	=
				reinterpret_cast<Udf::terminating_descriptor*>(chunk.Data());
			terminator->tag().set_id(Udf::TAGID_TERMINATING_DESCRIPTOR);
			terminator->tag().set_version(_UdfDescriptorVersion());
			terminator->tag().set_serial_number(0);
			terminator->tag().set_location(integrityExtent.location()+1);
			terminator->tag().set_checksums(*terminator);
			PDUMP(terminator);
			// write terminator				                             
			ssize_t bytes = _OutputFile().WriteAt(off_t((integrityExtent.location()+1)) << _BlockShift(),
	       			                              terminator, _BlockSize());
			error = check_size_error(bytes, _BlockSize());
		}
	}
		
	// reserve and write anchorN
	if (!error) {
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing anchorN");
		_PrintUpdate(VERBOSITY_HIGH, "udf: Reserving space for anchorN");
		uint32 blockN = _Allocator().Tail();
		error = _Allocator().GetBlock(blockN);
		if (!error) 
			_PrintUpdate(VERBOSITY_HIGH, "udf: (location: %ld, length: %ld)",
			             blockN, _BlockSize());
		if (!error) {
			anchorN.main_vds() = primaryVdsExtent;
			anchorN.reserve_vds() = reserveVdsExtent;
			Udf::descriptor_tag &tag = anchorN.tag();
			tag.set_id(Udf::TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER);
			tag.set_version(_UdfDescriptorVersion());
			tag.set_serial_number(0);
			tag.set_location(blockN);
			tag.set_checksums(anchorN);
			_OutputFile().Seek(off_t(blockN) << _BlockShift(), SEEK_SET);
			ssize_t bytes = _OutputFile().Write(&anchorN, sizeof(anchorN));
			error = check_size_error(bytes, sizeof(anchorN));
			if (!error && bytes < ssize_t(_BlockSize())) {
				bytes = _OutputFile().Zero(_BlockSize()-sizeof(anchorN));
				error = check_size_error(bytes, _BlockSize()-sizeof(anchorN));
			}
		}
	}
	
	// NOTE: After this point, no more blocks may be allocated without jacking
	// up anchorN's position as the last block in the volume. So don't allocate
	// any, damn it.
	
	// Pad the end of the file to an even multiple of the block
	// size, if necessary
	if (!error) {
		_OutputFile().Seek(0, SEEK_END);
		uint32 tail = _OutputFile().Position() % _BlockSize();
		if (tail > 0) {
			uint32 padding = _BlockSize() - tail;
			ssize_t bytes = _OutputFile().Zero(padding);
			error = check_size_error(bytes, padding);
		}
		if (!error)
			_Stats().SetImageSize(_OutputFile().Position());
	}
	
	if (!error) {
		_PrintUpdate(VERBOSITY_LOW, "Flushing image data");
		_OutputFile().Flush();
	}
	
	fListener.OnCompletion(error, _Stats());
	RETURN(error);
}

/*! \brief Returns the next unique id, then increments the id (the lower
	32-bits of which wrap to 16 instead of 0, per UDF-2.50 3.2.1.1).
*/
uint64
UdfBuilder::_NextUniqueId()
{
	uint64 result = fNextUniqueId++;	
	if ((fNextUniqueId & 0xffffffff) == 0) {
		fNextUniqueId |= 0x10;
		f32BitIdsNoLongerUnique = true;
	}
	return result;
}

/*! \brief Sets the time at which image building began.
*/
void
UdfBuilder::_SetBuildTime(time_t time)
{
	fBuildTime = time;
	Udf::timestamp stamp(time);
	fBuildTimeStamp = stamp;
}

/*! \brief Uses vsprintf() to output the given format string and arguments
	into the given message string.
	
	va_start() must be called prior to calling this function to obtain the
	\a arguments parameter, but va_end() must *not* be called upon return,
	as this function takes the liberty of doing so for you.
*/
status_t
UdfBuilder::_FormatString(char *message, const char *formatString, va_list arguments) const
{
	status_t error = message && formatString ? B_OK : B_BAD_VALUE;
	if (!error) {
		vsprintf(message, formatString, arguments);
		va_end(arguments);	
	}
	return error;
}

/*! \brief Outputs a printf()-style error message to the listener.
*/
void
UdfBuilder::_PrintError(const char *formatString, ...) const
{
	if (!formatString) {
		DEBUG_INIT_ETC("UdfBuilder", ("formatString: `%s'", formatString));
		PRINT(("ERROR: _PrintError() called with NULL format string!\n"));
		return;
	}
	char message[kMaxUpdateStringLength];
	va_list arguments;
	va_start(arguments, formatString);
	status_t error = _FormatString(message, formatString, arguments);
	if (!error)
		fListener.OnError(message);	
}

/*! \brief Outputs a printf()-style warning message to the listener.
*/
void
UdfBuilder::_PrintWarning(const char *formatString, ...) const
{
	if (!formatString) {
		DEBUG_INIT_ETC("UdfBuilder", ("formatString: `%s'", formatString));
		PRINT(("ERROR: _PrintWarning() called with NULL format string!\n"));
		return;
	}
	char message[kMaxUpdateStringLength];
	va_list arguments;
	va_start(arguments, formatString);
	status_t error = _FormatString(message, formatString, arguments);
	if (!error)
		fListener.OnWarning(message);	
}

/*! \brief Outputs a printf()-style update message to the listener
	at the given verbosity level.
*/
void
UdfBuilder::_PrintUpdate(VerbosityLevel level, const char *formatString, ...) const
{
	if (!formatString) {
		DEBUG_INIT_ETC("UdfBuilder", ("level: %d, formatString: `%s'",
		               level, formatString));
		PRINT(("ERROR: _PrintUpdate() called with NULL format string!\n"));
		return;
	}
	char message[kMaxUpdateStringLength];
	va_list arguments;
	va_start(arguments, formatString);
	status_t error = _FormatString(message, formatString, arguments);
	if (!error)
		fListener.OnUpdate(level, message);	
}

/*! \brief Processes the given directory and its children.

	\param entry The directory to process.
	\param path Pathname of the directory with respect to the fileset
	            in construction.
	\param node Output parameter into which the icb address and dataspace
	            information for the processed directory is placed.
	\param isRootDirectory Used to signal that the directory being processed
	                       is the root directory, since said directory has
	                       a special unique id assigned to it.
*/
status_t
UdfBuilder::_ProcessDirectory(BEntry &entry, const char *path, struct stat stats,
                              node_data &node, Udf::long_address parentIcbAddress,
                              bool isRootDirectory)
{
	DEBUG_INIT_ETC("UdfBuilder", ("path: `%s'", path));
	uint32 udfDataLength = 0;
	uint64 udfUniqueId = isRootDirectory ? 0 : _NextUniqueId();
		// file entry id must be numerically lower than unique id's
		// in all fids that reference it, thus we allocate it now.
	status_t error = entry.InitCheck() == B_OK && path ? B_OK : B_BAD_VALUE;	
	if (!error) {
		_PrintUpdate(VERBOSITY_LOW, "Adding `%s'", path);
		BDirectory directory(&entry);
		error = directory.InitCheck();
		if (!error) {

			// Max length of a udf file identifier Cs0 string		
			const uint32 maxUdfIdLength = _BlockSize() - (38);

			_PrintUpdate(VERBOSITY_MEDIUM, "Gathering statistics");

			// Figure out how many file identifier characters we have
			// for each filesystem
			uint32 entries = 0;
			//uint32 isoChars = 0;
			while (error == B_OK) {
				BEntry childEntry;
				error = directory.GetNextEntry(&childEntry);
				if (error == B_ENTRY_NOT_FOUND) {
					error = B_OK;
					break;
				}
				if (!error)
					error = childEntry.InitCheck();
				if (!error) {
					BPath childPath;
					error = childEntry.GetPath(&childPath);
					if (!error)
						error = childPath.InitCheck();
					// Skip symlinks until we add symlink support; this
					// allows graceful skipping of them later on instead
					// of stopping with a fatal error
					struct stat childStats;
					if (!error)
						error = childEntry.GetStat(&childStats);
					if (!error && S_ISLNK(childStats.st_mode))
						continue;					
					if (!error) {
						_PrintUpdate(VERBOSITY_MEDIUM, "found child: `%s'", childPath.Leaf());
						entries++;
						// Determine udf char count
						Udf::String name(childPath.Leaf());
						uint32 udfLength = name.Cs0Length();
						udfLength = maxUdfIdLength >= udfLength
						            ? udfLength : maxUdfIdLength;
						Udf::file_id_descriptor id;
						id.set_id_length(udfLength);
						id.set_implementation_use_length(0);
						udfDataLength += id.total_length();
						// Determine iso char count
						// isoChars += ???
					}
				}
			}
			
//			entries = 0;
//			udfChars = 0;
			
			// Include parent directory entry in data length calculation
			if (!error) {
				Udf::file_id_descriptor id;
				id.set_id_length(0);
				id.set_implementation_use_length(0);
				udfDataLength += id.total_length();
			}				
			
			_PrintUpdate(VERBOSITY_MEDIUM, "children: %ld", entries);
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: data length: %ld", udfDataLength);

			// Reserve iso dir entry space
			
			// Reserve udf icb space
			Udf::long_address icbAddress;
			Udf::extent_address icbExtent;
			if (!error && _DoUdf()) {
				_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for icb");
				error = _PartitionAllocator().GetNextExtent(_BlockSize(), true, icbAddress,
				                                            icbExtent);
				if (!error) {				                                            
					_PrintUpdate(VERBOSITY_HIGH, "udf: (partition: %d, location: %ld, "
					             "length: %ld) => (location: %ld, length: %ld)",
					             icbAddress.partition(), icbAddress.block(),
					             icbAddress.length(), icbExtent.location(),
					             icbExtent.length());
					node.icbAddress = icbAddress;
				}
			}
			
			DataStream *udfData = NULL;
			std::list<Udf::extent_address> udfDataExtents;
			std::list<Udf::long_address> &udfDataAddresses = node.udfData;
			
			// Reserve udf dir data space
			if (!error && _DoUdf()) {
				_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for directory data");
				error = _PartitionAllocator().GetNextExtents(udfDataLength, udfDataAddresses,
				                                            udfDataExtents);
				if (!error) {
					int extents = udfDataAddresses.size();
					if (extents > 1)
						_PrintUpdate(VERBOSITY_HIGH, "udf: Reserved %d extents",
						             extents);
					std::list<Udf::long_address>::iterator a;
					std::list<Udf::extent_address>::iterator e;
					for (a = udfDataAddresses.begin(), e = udfDataExtents.begin();
					       a != udfDataAddresses.end() && e != udfDataExtents.end();
					         a++, e++)
					{
						_PrintUpdate(VERBOSITY_HIGH, "udf: (partition: %d, location: %ld, "
						             "length: %ld) => (location: %ld, length: %ld)",
						             a->partition(), a->block(), a->length(), e->location(),
						             e->length());
					}
				}
				if (!error) {
					udfData = new(nothrow) ExtentStream(_OutputFile(), udfDataExtents, _BlockSize());
					error = udfData ? B_OK : B_NO_MEMORY;
				}
			}
			
			uint32 udfAllocationDescriptorsLength = udfDataAddresses.size()
			                                        * sizeof(Udf::long_address);

			// Process attributes
			uint16 attributeCount = 0; 
			
			// Write iso parent directory
			
			// Write udf parent directory fid
			if (!error && _DoUdf()) {
				Udf::MemoryChunk chunk((38) + 4);
				error = chunk.InitCheck();
				if (!error) {
					memset(chunk.Data(), 0, (38) + 4);                
					Udf::file_id_descriptor *parent =
						reinterpret_cast<Udf::file_id_descriptor*>(chunk.Data());
					parent->set_version_number(1);
					// Clear characteristics to false, then set
					// those that need to be true
					parent->set_characteristics(0);
					parent->set_is_directory(true);
					parent->set_is_parent(true);
					parent->set_id_length(0);
					parent->icb() = isRootDirectory ? icbAddress : parentIcbAddress;
					if (!isRootDirectory)
						parent->icb().set_unique_id(uint32(_NextUniqueId()));
					parent->set_implementation_use_length(0);
					parent->tag().set_id(Udf::TAGID_FILE_ID_DESCRIPTOR);
					parent->tag().set_version(_UdfDescriptorVersion());
					parent->tag().set_serial_number(0);
					uint32 block;
					error = block_for_offset(udfData->Position(), udfDataAddresses,
					                              _BlockSize(), block);
					if (!error) {
						parent->tag().set_location(block);
						parent->tag().set_checksums(*parent, parent->descriptor_size());
						ssize_t bytes = udfData->Write(parent, parent->total_length());
						error = check_size_error(bytes, parent->total_length());
					}
				}
			}
			
			// Process children
			uint16 childDirCount = 0;
			if (!error)
				error = directory.Rewind();
			while (error == B_OK) {
				BEntry childEntry;
				error = directory.GetNextEntry(&childEntry);
				if (error == B_ENTRY_NOT_FOUND) {
					error = B_OK;
					break;
				}
				if (!error)
					error = childEntry.InitCheck();
				if (!error) {
					BPath childPath;
					error = childEntry.GetPath(&childPath);
					if (!error)
						error = childPath.InitCheck();
					struct stat childStats;
					if (!error)
						error = childEntry.GetStat(&childStats);
					if (!error) {
						node_data childNode;
						std::string childImagePath(path);
						childImagePath += (childImagePath[childImagePath.length()-1] == '/'
						                  ? "" : "/");
						childImagePath += childPath.Leaf();
						// Process child
						if (S_ISREG(childStats.st_mode)) {
							// Regular file
							error = _ProcessFile(childEntry, childImagePath.c_str(),
							                          childStats, childNode);
						} else if (S_ISDIR(childStats.st_mode)) {
							// Directory							
							error = _ProcessDirectory(childEntry, childImagePath.c_str(),
							                          childStats, childNode, icbAddress);
							if (!error)
								childDirCount++;
						} else if (S_ISLNK(childStats.st_mode)) {
							// Symlink
							// For now, skip it
							_Stats().AddSymlink();
							_PrintWarning("No symlink support yet; skipping symlink: `%s'",
							              childImagePath.c_str());
							continue;
						}
				
						// Write iso direntry
						
						// Write udf fid
						if (!error) {
							Udf::String udfName(childPath.Leaf());
							uint32 udfNameLength = udfName.Cs0Length();
							uint32 idLength = (38)
							                  + udfNameLength;
							Udf::MemoryChunk chunk(idLength + 4);
							error = chunk.InitCheck();
							if (!error) {
								memset(chunk.Data(), 0, idLength + 4);                
								Udf::file_id_descriptor *id =
									reinterpret_cast<Udf::file_id_descriptor*>(chunk.Data());
								id->set_version_number(1);
								// Clear characteristics to false, then set
								// those that need to be true
								id->set_characteristics(0);
								id->set_is_directory(S_ISDIR(childStats.st_mode));
								id->set_is_parent(false);
								id->set_id_length(udfNameLength);
								id->icb() = childNode.icbAddress;
								id->icb().set_unique_id(uint32(_NextUniqueId()));
								id->set_implementation_use_length(0);
								memcpy(id->id(), udfName.Cs0(), udfNameLength);
								id->tag().set_id(Udf::TAGID_FILE_ID_DESCRIPTOR);
								id->tag().set_version(_UdfDescriptorVersion());
								id->tag().set_serial_number(0);
								uint32 block;
								error = block_for_offset(udfData->Position(), udfDataAddresses,
								                              _BlockSize(), block);
								if (!error) {
									id->tag().set_location(block);
									id->tag().set_checksums(*id, id->descriptor_size());
									PDUMP(id);
									PRINT(("pos: %Ld\n", udfData->Position()));
									ssize_t bytes = udfData->Write(id, id->total_length());
									PRINT(("pos: %Ld\n", udfData->Position()));
									error = check_size_error(bytes, id->total_length());									
								}
							}
						}
					}
				}
			}
				
			// Build and write udf icb
			Udf::MemoryChunk chunk(_BlockSize());
			if (!error)
				error = chunk.InitCheck();
			if (!error) {
				memset(chunk.Data(), 0, _BlockSize());
				uint8 fileType = Udf::ICB_TYPE_DIRECTORY;
				uint16 linkCount = 1 + attributeCount + childDirCount;
				if (_UdfRevision() <= 0x0150) {
					error = _WriteFileEntry(
						reinterpret_cast<Udf::file_icb_entry*>(chunk.Data()),
						fileType, linkCount, udfDataLength, udfDataLength,
						stats, udfUniqueId, udfAllocationDescriptorsLength,
						Udf::TAGID_FILE_ENTRY,
						icbAddress, icbExtent, udfDataAddresses
					);
				} else {
					error = _WriteFileEntry(
						reinterpret_cast<Udf::extended_file_icb_entry*>(chunk.Data()),
						fileType, linkCount, udfDataLength, udfDataLength,
						stats, udfUniqueId, udfAllocationDescriptorsLength,
						Udf::TAGID_EXTENDED_FILE_ENTRY,
						icbAddress, icbExtent, udfDataAddresses
					);
				}
			}				

			delete udfData;
			udfData = NULL;
		}
	}
		
	if (!error) {
		_Stats().AddDirectory();
		uint32 totalLength = udfDataLength + _BlockSize();
		_Stats().AddDirectoryBytes(totalLength);
	}
	RETURN(error);
}

status_t
UdfBuilder::_ProcessFile(BEntry &entry, const char *path, struct stat stats,
                         node_data &node)
{
	DEBUG_INIT_ETC("UdfBuilder", ("path: `%s'", path));
	status_t error = entry.InitCheck() == B_OK && path ? B_OK : B_BAD_VALUE;	
	off_t udfDataLength = stats.st_size;
	if (udfDataLength > ULONG_MAX && _DoIso()) {
		_PrintError("File `%s' too large for iso9660 filesystem (filesize: %Lu bytes, max: %lu bytes)",
		            path, udfDataLength, ULONG_MAX);
		error = B_ERROR;	            
	}
	if (!error) {
		_PrintUpdate(VERBOSITY_LOW, "Adding `%s' (%s)", path,
		             bytes_to_string(stats.st_size).c_str());
		BFile file(&entry, B_READ_ONLY);
		error = file.InitCheck();
		if (!error) {
			// Reserve udf icb space
			Udf::long_address icbAddress;
			Udf::extent_address icbExtent;
			if (!error && _DoUdf()) {
				_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for icb");
				error = _PartitionAllocator().GetNextExtent(_BlockSize(), true, icbAddress,
				                                            icbExtent);
				if (!error) {				                                            
					_PrintUpdate(VERBOSITY_HIGH, "udf: (partition: %d, location: %ld, "
					             "length: %ld) => (location: %ld, length: %ld)",
					             icbAddress.partition(), icbAddress.block(),
					             icbAddress.length(), icbExtent.location(),
					             icbExtent.length());
					node.icbAddress = icbAddress;
				}
			}
			
			DataStream *udfData = NULL;
			std::list<Udf::extent_address> udfDataExtents;
			std::list<Udf::long_address> &udfDataAddresses = node.udfData;
			
			// Reserve iso/udf data space
			if (!error) {
				_PrintUpdate(VERBOSITY_MEDIUM, "Reserving space for file data");
				if (_DoIso()) {
					// Reserve a contiguous extent, as iso requires
					Udf::long_address address;
					Udf::extent_address extent;
					error = _PartitionAllocator().GetNextExtent(udfDataLength, true, address, extent);
					if (!error) {
						udfDataAddresses.empty();	// just in case
						udfDataAddresses.push_back(address);
						udfDataExtents.push_back(extent);
					}
				} else {
					// Udf can handle multiple extents if necessary
					error = _PartitionAllocator().GetNextExtents(udfDataLength, udfDataAddresses,
				                                                 udfDataExtents);
				}				                                            
				if (!error) {
					int extents = udfDataAddresses.size();
					if (extents > 1)
						_PrintUpdate(VERBOSITY_HIGH, "udf: Reserved %d extents",
						             extents);
					std::list<Udf::long_address>::iterator a;
					std::list<Udf::extent_address>::iterator e;
					for (a = udfDataAddresses.begin(), e = udfDataExtents.begin();
					       a != udfDataAddresses.end() && e != udfDataExtents.end();
					         a++, e++)
					{
						_PrintUpdate(VERBOSITY_HIGH, "udf: (partition: %d, location: %ld, "
						             "length: %ld) => (location: %ld, length: %ld)",
						             a->partition(), a->block(), a->length(), e->location(),
						             e->length());
					}
				}
				if (!error) {
					udfData = new(nothrow) ExtentStream(_OutputFile(), udfDataExtents, _BlockSize());
					error = udfData ? B_OK : B_NO_MEMORY;
				}
			}
			
			uint32 udfAllocationDescriptorsLength = udfDataAddresses.size()
			                                        * sizeof(Udf::long_address);

			// Process attributes
			uint16 attributeCount = 0;
			
			// Write file data
			if (!error) {
				_PrintUpdate(VERBOSITY_MEDIUM, "Writing file data");
				ssize_t bytes = udfData->Write(file, udfDataLength);
				error = check_size_error(bytes, udfDataLength);
			}
			
			// Build and write udf icb
			Udf::MemoryChunk chunk(_BlockSize());
			if (!error)
				error = chunk.InitCheck();
			if (!error) {
				memset(chunk.Data(), 0, _BlockSize());
				uint8 fileType = Udf::ICB_TYPE_REGULAR_FILE;
				uint16 linkCount = 1 + attributeCount;
				uint64 uniqueId = _NextUniqueId();
				if (_UdfRevision() <= 0x0150) {
					error = _WriteFileEntry(
						reinterpret_cast<Udf::file_icb_entry*>(chunk.Data()),
						fileType, linkCount, udfDataLength, udfDataLength,
						stats, uniqueId, udfAllocationDescriptorsLength,
						Udf::TAGID_FILE_ENTRY,
						icbAddress, icbExtent, udfDataAddresses
					);
				} else {
					error = _WriteFileEntry(
						reinterpret_cast<Udf::extended_file_icb_entry*>(chunk.Data()),
						fileType, linkCount, udfDataLength, udfDataLength,
						stats, uniqueId, udfAllocationDescriptorsLength,
						Udf::TAGID_EXTENDED_FILE_ENTRY,
						icbAddress, icbExtent, udfDataAddresses
					);
				}
			}				

			delete udfData;
			udfData = NULL;
		}
	}
		
	if (!error) {
		_Stats().AddFile();
		off_t totalLength = udfDataLength + _BlockSize();
		_Stats().AddFileBytes(totalLength);
	}
	RETURN(error);
}                         

