/*
 * Copyright 2008, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SATA_REQUEST_H
#define _SATA_REQUEST_H


#include "ahci_defs.h"
#include "scsi_cmds.h"


class sata_request {
public:
								sata_request();
								sata_request(scsi_ccb* ccb);
								~sata_request();

			void				SetData(void* data, size_t dataSize);

			void				SetATACommand(uint8 command);
			void				SetATA28Command(uint8 command, uint32 lba,
									uint8 sectorCount);
			void				SetATA48Command(uint8 command, uint64 lba,
									uint16 sectorCount);
			void				SetFeature(uint16 feature);

			void				SetATAPICommand(size_t transferLength);
			bool				IsATAPI();
			bool				IsTestUnitReady();

			scsi_ccb*			CCB();
			const void*			FIS();
			void*				Data();
			int					Size();
			void				Finish(int tfd, size_t bytesTransfered);
			void				Abort();

			void				WaitForCompletion();
			int					CompletionStatus();

private:
			scsi_ccb*			fCcb;
			uint8				fFis[20];
			bool				fIsATAPI;
			sem_id				fCompletionSem;
			int					fCompletionStatus;
			void*				fData;
			size_t				fDataSize;
};


inline scsi_ccb*
sata_request::CCB()
{
	return fCcb;
}


inline const void*
sata_request::FIS()
{
	return fFis;
}


inline bool
sata_request::IsATAPI()
{
	return fIsATAPI;
}


inline bool
sata_request::IsTestUnitReady()
{
	return fIsATAPI && fCcb != NULL && fCcb->cdb[0] == SCSI_OP_TEST_UNIT_READY;
}


inline void*
sata_request::Data()
{
	return fData;
}


inline int
sata_request::Size()
{
	return fDataSize;
}


#endif	/* _SATA_REQUEST_H */
