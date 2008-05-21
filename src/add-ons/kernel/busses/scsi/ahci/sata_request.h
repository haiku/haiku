/*
 * Copyright 2008, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SATA_REQUEST_H
#define _SATA_REQUEST_H

#include "ahci_defs.h"

class sata_request
{
public:
					sata_request();
					sata_request(scsi_ccb *ccb);
					~sata_request();

	void			set_data(void *data, size_t dataSize);

	void			set_ata_cmd(uint8 command);
	void			set_ata28_cmd(uint8 command, uint32 lba, uint8 sectorCount);
	void			set_ata48_cmd(uint8 command, uint64 lba, uint16 sectorCount);

	void			set_atapi_cmd();
	bool 			is_atapi();

	scsi_ccb *		ccb();
	const void *	fis();
	void *			data();
	int				size();
	void			finish(int tfd, size_t bytesTransfered);
	void			abort();

	void			wait_for_completition();
	int				completition_status();

private:
	scsi_ccb *		fCcb;
	uint8			fFis[20];
	bool			fIsATAPI;
	sem_id			fCompletionSem;
	int				fCompletionStatus;
	void *			fData;
	size_t			fDataSize;
};


inline scsi_ccb *
sata_request::ccb()
{
	return fCcb;
}


inline const void *
sata_request::fis()
{
	return fFis;
}


inline bool
sata_request::is_atapi()
{
	return fIsATAPI;
}


inline void *
sata_request::data()
{
	return fData;
}


inline int
sata_request::size()
{
	return fDataSize;
}


#endif
