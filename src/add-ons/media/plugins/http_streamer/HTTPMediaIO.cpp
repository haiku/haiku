/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include "HTTPMediaIO.h"


HTTPMediaIO::HTTPMediaIO(BUrl* url)
	:
	fContext(),
	fBuffer(),
	fInitErr(B_ERROR)
{
	fContext->AcquireReference();

	fReq = new BHttpRequest(*url);
	fReq->SetContext(fContext);
	fReq->Run();
	fReq->AdoptInputData(fBuffer);

	if (!fReq->IsRunning())
		return;

	fInitErr = _IntegrityCheck();
}


HTTPMediaIO::~HTTPMediaIO()
{
	fContext->ReleaseReference();
	delete fContext;
	delete fReq;
}


status_t
HTTPMediaIO::InitCheck() const
{
	return fInitErr;
}


ssize_t
HTTPMediaIO::ReadAt(off_t position, void* buffer, size_t size)
{
	return fBuffer->ReadAt(position, buffer, size);
}


ssize_t
HTTPMediaIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


off_t
HTTPMediaIO::Seek(off_t position, uint32 seekMode)
{
	return fBuffer->Seek(position, seekMode);
}

off_t
HTTPMediaIO::Position() const
{
	return fBuffer->Position();
}


status_t
HTTPMediaIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
HTTPMediaIO::GetSize(off_t* size) const
{
	return B_NOT_SUPPORTED;
}


bool
HTTPMediaIO::IsSeekable() const
{
	return false;
}


bool
HTTPMediaIO::IsEndless() const
{
	return true;
}


status_t
HTTPMediaIO::_IntegrityCheck()
{
	const BHttpResult& r = dynamic_cast<const BHttpResult&>(fReq->Result());
	if (r.StatusCode() != 200)
		return B_ERROR;

	if (BString("OK")!=  r.StatusText())
		return B_ERROR;

	return B_OK;
}
