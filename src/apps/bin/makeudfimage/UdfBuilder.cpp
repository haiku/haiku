//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file UdfBuilder.cpp

	Main UDF image building class implementation.
*/

#include "UdfBuilder.h"

#include <OS.h>
#include <stdio.h>
#include <string>

#include "UdfDebug.h"
#include "Utils.h"

using Udf::bool_to_string;
using Udf::check_size_error;

//! Application identifier entity_id
static const Udf::entity_id kApplicationId(0, "*OpenBeOS makeudfimage");

/*! \brief Creates a new UdfBuilder object.
*/
UdfBuilder::UdfBuilder(const char *outputFile, uint32 blockSize, bool doUdf,
                       bool doIso, const char *udfVolumeName,
                       const char *isoVolumeName,
                       const ProgressListener &listener)
	: fInitStatus(B_NO_INIT)
	, fOutputFile(outputFile, B_READ_WRITE | B_CREATE_FILE)
	, fOutputFilename(outputFile)
	, fBlockSize(blockSize)
	, fBlockShift(0)
	, fDoUdf(doUdf)
	, fDoIso(doIso)
	, fUdfVolumeName(udfVolumeName)
	, fIsoVolumeName(isoVolumeName)
	, fListener(listener)
	, fAllocator(blockSize)
	, fBuildTime(0)	// set at start of Build()
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
	time_t timer = time(&timer);
//	_SetBuildTime(real_time_clock());
	_SetBuildTime(timer);

	_OutputFile().Seek(0, SEEK_SET);		
	_PrintUpdate(VERBOSITY_LOW, "Output file: `%s'", fOutputFilename.c_str());		


	_PrintUpdate(VERBOSITY_LOW, "Initializing volume");

	// Reserve the first 32KB and zero them out.
	if (!error) {
		const int reservedAreaSize = 32 * 1024;
		Udf::extent_address extent(0, reservedAreaSize);
		error = _Allocator().GetExtent(extent);
		if (!error) {
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
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing beginning extended area descriptor");
		Udf::volume_structure_descriptor_header bea(0, Udf::kVSDID_BEA, 1);
		error = _Allocator().GetNextExtent(vrsBlockSize, true, extent);
		if (!error) {
			ssize_t bytes = _OutputFile().Write(&bea, sizeof(bea));
			error = check_size_error(bytes, sizeof(bea));
			if (!error) {
				bytes = _OutputFile().Zero(vrsBlockSize-sizeof(bea));
				error = check_size_error(bytes, vrsBlockSize-sizeof(bea));
			}
		}				
		// Nsr
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing nsr descriptor");
		Udf::volume_structure_descriptor_header nsr(0, Udf::kVSDID_ECMA167_3, 1);
		if (!error)
			error = _Allocator().GetNextExtent(vrsBlockSize, true, extent);
		if (!error) {
			ssize_t bytes = _OutputFile().Write(&nsr, sizeof(nsr));
			error = check_size_error(bytes, sizeof(nsr));
			if (!error) {
				bytes = _OutputFile().Zero(vrsBlockSize-sizeof(nsr));
				error = check_size_error(bytes, vrsBlockSize-sizeof(nsr));
			}
		}				
		// Tea
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing terminating extended area descriptor");
		Udf::volume_structure_descriptor_header tea(0, Udf::kVSDID_TEA, 1);
		if (!error)
			error = _Allocator().GetNextExtent(vrsBlockSize, true, extent);
		if (!error) {
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
	
	// Write the udf anchor_256 and volume descriptor sequences
	if (!error && _DoUdf()) {
		// reserve anchor_256
		_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for anchor_256 at block 256");
		error = _Allocator().GetBlock(256);
		// reserve primary vds (min length = 16 blocks, which is plenty for us)
		Udf::extent_address primaryExtent;
		Udf::extent_address reserveExtent;
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for primary vds");
			error = _Allocator().GetNextExtent(16 << _BlockShift(), true, primaryExtent);
			if (!error) 
				_PrintUpdate(VERBOSITY_HIGH, "udf: <location: %ld, length: %ld>",
				             primaryExtent.location(), primaryExtent.length());
		}
		// reserve reserve vds. try to grab the 16 blocks preceding block 256. if
		// that fails, just grab any 16. most commercial discs just put the reserve
		// vds immediately following the primary vds, which seems a bit stupid to me,
		// now that I think about it... 
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Reserving space for reserve vds");
			reserveExtent.set_location(256-16);
			reserveExtent.set_length(16 << _BlockShift());
			error = _Allocator().GetExtent(reserveExtent);
			if (error)
				error = _Allocator().GetNextExtent(16 << _BlockShift(), true, reserveExtent);
			if (!error) 
				_PrintUpdate(VERBOSITY_HIGH, "udf: <location: %ld, length: %ld>",
				             reserveExtent.location(), reserveExtent.length());
		}
		// write anchor_256
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing anchor_256");
			Udf::anchor_volume_descriptor anchor;
			anchor.main_vds() = primaryExtent;
			anchor.reserve_vds() = reserveExtent;
			Udf::descriptor_tag &tag = anchor.tag();
			tag.set_id(Udf::TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER);
			tag.set_version(3);
			tag.set_serial_number(0);
			tag.set_location(256);
			tag.set_checksums(anchor);
			_OutputFile().Seek(256 << _BlockShift(), SEEK_SET);
			ssize_t bytes = _OutputFile().Write(&anchor, sizeof(anchor));
			error = check_size_error(bytes, sizeof(anchor));
			if (!error && bytes < ssize_t(_BlockSize())) {
				bytes = _OutputFile().Zero(_BlockSize()-sizeof(anchor));
				error = check_size_error(bytes, _BlockSize()-sizeof(anchor));
			}
		}
		uint32 vdsNumber = 0;
		// write primary_vd
		if (!error) {
			_PrintUpdate(VERBOSITY_MEDIUM, "udf: Writing primary volume descriptor");
			// build primary_vd
			Udf::primary_volume_descriptor primary;
			primary.set_vds_number(vdsNumber);
			primary.set_primary_volume_descriptor_number(0);
			uint32 nameLength = _UdfVolumeName().Cs0Length();
			if (nameLength > 32) {
				_PrintWarning("udf: Truncating volume name as stored in primary "
				              "volume descriptor to 32 byte limit. This shouldn't matter, "
				              "as the complete name is %d bytes long, which is short enough "
				              "to fit completely in the logical volume descriptor.",
				              nameLength);
				nameLength = 32;
			}
			memcpy(primary.volume_identifier().data, _UdfVolumeName().Cs0(),
			       nameLength);
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
			Udf::String Cs0VolumeSetId(volumeSetId.c_str());
			memcpy(primary.volume_set_identifier().data, Cs0VolumeSetId.Cs0(),
			       Cs0VolumeSetId.Cs0Length());
			primary.descriptor_character_set() = Udf::kCs0CharacterSet;
			primary.explanatory_character_set() = Udf::kCs0CharacterSet;
			Udf::extent_address nullAddress(0, 0);
			primary.volume_abstract() = nullAddress;
			primary.volume_copyright_notice() = nullAddress;
			primary.application_id() = kApplicationId;
			primary.recording_date_and_time() = _BuildTimeStamp();
			primary.implementation_id() = Udf::kImplementationId;
			memset(primary.implementation_use().data, 0,
			       primary.implementation_use().size());
			primary.set_predecessor_volume_descriptor_sequence_location(0);
			primary.set_flags(0);	// ToDo: maybe 1 is more appropriate?	       
			memset(primary.reserved().data, 0, primary.reserved().size());
			primary.tag().set_id(Udf::TAGID_PRIMARY_VOLUME_DESCRIPTOR);
			primary.tag().set_version(3);
			primary.tag().set_serial_number(0);
				// note that the checksums haven't been set yet, since the
				// location is dependent on which sequence (primary or reserve)
				// the descriptor is currently being written to. Thus we have to
				// recalculate the checksums for each sequence.
			DUMP(primary);
			// write primary_vd to primary vds
			primary.tag().set_location(primaryExtent.location()+vdsNumber);
			primary.tag().set_checksums(primary);
			ssize_t bytes = _OutputFile().WriteAt(primary.tag().location() << _BlockShift(),
			                              &primary, sizeof(primary));
			error = check_size_error(bytes, sizeof(primary));                              
			if (!error && bytes < ssize_t(_BlockSize())) {
				ssize_t bytesLeft = _BlockSize() - bytes;
				bytes = _OutputFile().ZeroAt((primary.tag().location() << _BlockShift())
				                             + bytes, bytesLeft);
				error = check_size_error(bytes, bytesLeft);			                             
			}
			// write primary_vd to reserve vds				                             
			if (!error) {
				primary.tag().set_location(reserveExtent.location()+vdsNumber);
				primary.tag().set_checksums(primary);
				ssize_t bytes = _OutputFile().WriteAt(primary.tag().location() << _BlockShift(),
	        			                              &primary, sizeof(primary));
				error = check_size_error(bytes, sizeof(primary));                              
				if (!error && bytes < ssize_t(_BlockSize())) {
					ssize_t bytesLeft = _BlockSize() - bytes;
					bytes = _OutputFile().ZeroAt((primary.tag().location() << _BlockShift())
					                             + bytes, bytesLeft);
					error = check_size_error(bytes, bytesLeft);			                             
				}
			}
		}
		vdsNumber++;			

		// write logical vds
//		Udf::logical_volume_descriptor logical;

		// write partition descriptor
//		Udf::partition_descriptor partition;
		
		// Error check
		if (error) {				 
			_PrintError("Error writing udf vds: 0x%lx, `%s'",
			            error, strerror(error));
		}
	}

	if (!error)
		_PrintUpdate(VERBOSITY_LOW, "Finished");
	RETURN(error);
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

