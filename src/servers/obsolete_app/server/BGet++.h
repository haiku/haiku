#ifndef HGET_H_
#define HGET_H_

#include <SupportDefs.h>

class MemPool
{
public:
	MemPool(void);
	virtual ~MemPool(void);
	
	void AddToPool(void *buffer, ssize_t len);
	void *GetBuffer(ssize_t size, bool zero=false);
	void *ReallocateBuffer(void *buffer, ssize_t size);
	void ReleaseBuffer(void *buffer);
	
	virtual int *CompactMem(ssize_t sizereq, int sequence);
	virtual void *AcquireMem(ssize_t size);
	virtual void ReleaseMem(void *buffer);
	void SetPoolIncrement(ssize_t increment);
	ssize_t PoolIncrement(void);
	
	void Stats(ssize_t *curalloc, ssize_t *totfree, ssize_t *maxfree,
			long *nget, long *nrel);
	
	void ExtendedStats(ssize_t *pool_incr, long *npool, long *npget,
			long *nprel, long *ndget, long *ndrel);
	
	void BufferDump(void *buf);
	void PoolDump(void *pool, bool dumpalloc, bool dumpfree);
	int	Validate(void *buffer);

private:
	ssize_t totalloc;
	long numget, numrel;
	long numpblk;
	long numpget, numprel;
	long numdget, numdrel;
	ssize_t exp_incr;
	ssize_t pool_len;
	
};

class AreaPool : public MemPool
{
public:
	AreaPool(void);
	virtual ~AreaPool(void);
	virtual void *AcquireMem(ssize_t size);
	virtual void ReleaseMem(void *buffer);
};

#endif
