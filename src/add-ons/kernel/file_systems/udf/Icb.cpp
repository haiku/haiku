#include "Icb.h"

#include "Volume.h"

using namespace Udf;

Icb::Icb(Volume *volume, udf_long_address address)
	: fVolume(NULL)
	, fIcbData(volume ? volume->BlockSize() : 0)
	, fInitStatus(B_NO_INIT)
	, fId((address.block() << 16) & (address.partition()))
{
	DEBUG_INIT_ETC(CF_PUBLIC, "Icb", ("Volume*(%p), long_address(block: %ld, partition: %d, length: %ld)", volume, address.block(), address.partition(), address.length()));  
	status_t err = volume ? B_OK : B_BAD_VALUE;
	if (!err) {
		err = fIcbData.InitCheck();
		if (!err) {
			err = volume->Read(address, fIcbData.Size(), fIcbData.Data());
			if (!err) {
				udf_icb_header *header = reinterpret_cast<udf_icb_header*>(fIcbData.Data());
				PDUMP(header);
				err = header->tag().init_check(address.block());
			}
		}
	}		
	fInitStatus = err;
}
	
status_t
Icb::InitCheck()
{
	return fInitStatus;
}
